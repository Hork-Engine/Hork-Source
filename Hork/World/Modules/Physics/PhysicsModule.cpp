/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

This file is part of the Hork Engine Source Code.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "PhysicsModule.h"

#include <Hork/Core/Logger.h>
#include <Hork/Core/Thread.h>
#include <Hork/Core/BaseMath.h>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>

HK_NAMESPACE_BEGIN

namespace
{
    void JPH_Trace(const char* inFMT, ...)
    {
        // Format the message
        va_list list;
        va_start(list, inFMT);
        char buffer[1024];
        Core::Sprintf(buffer, sizeof(buffer), inFMT, list);
        va_end(list);

        // Print to the TTY
        LOG("{}\n", buffer);
    }

    bool JPH_AssertFailed(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine)
    {
        // Print to the TTY
        LOG("{}:{}: ({}) {}\n", inFile, inLine, inExpression, (inMessage != nullptr ? inMessage : ""));

        // Breakpoint
        return true;
    }

    class TempAllocatorImpl final : public JPH::TempAllocator
    {
    public:
        JPH_OVERRIDE_NEW_DELETE

        /// Constructs the allocator with a maximum allocatable size of inSize
        explicit                        TempAllocatorImpl(JPH::uint inSize) :
            mBase(static_cast<JPH::uint8 *>(JPH::AlignedAllocate(inSize, JPH_RVECTOR_ALIGNMENT))),
            mSize(inSize)
        {
        }

        /// Destructor, frees the block
        virtual							~TempAllocatorImpl() override
        {
            JPH_ASSERT(mTop == 0);
            JPH::AlignedFree(mBase);
        }

        // See: TempAllocator
        virtual void *					Allocate(JPH::uint inSize) override
        {
            if (inSize == 0)
            {
                return nullptr;
            }
            else
            {
                inSize = JPH::AlignUp(inSize, JPH_RVECTOR_ALIGNMENT);
                JPH::uint new_top = mTop + inSize;
                void *address;
                if (new_top <= mSize)
                {
                    address = mBase + mTop;
                }
                else
                {
                    static bool warn = true;
                    if (warn)
                    {
                        LOG("TempAllocator: The temporary buffer exceeded {:.1f} megabytes. Fallback to general-purpose allocator.\n", static_cast<float>(mSize) / 1024 / 1024);
                        warn = false;
                    }
                    address = JPH::AlignedAllocate(inSize, JPH_RVECTOR_ALIGNMENT);
                }
                mTop = new_top;
                return address;
            }
        }

        // See: TempAllocator
        virtual void					Free(void *inAddress, JPH::uint inSize) override
        {
            if (inAddress == nullptr)
            {
                JPH_ASSERT(inSize == 0);
            }
            else
            {
                inSize = JPH::AlignUp(inSize, JPH_RVECTOR_ALIGNMENT);
                JPH::uint new_top = mTop - inSize;
                if (mTop <= mSize)
                {
                    if (mBase + new_top != inAddress)
                    {
                        JPH::Trace("TempAllocator: Freeing in the wrong order");
                        std::abort();
                    }
                }
                else
                {
                    JPH::AlignedFree(inAddress);
                }
                mTop = new_top;
            }
        }

    private:
        JPH::uint8 *						mBase;
        JPH::uint							mSize;
        JPH::uint							mTop = 0;
    };
}

PhysicsModule::PhysicsModule()
{
    // Register allocation hook
    //JPH::RegisterDefaultAllocator();

    JPH::Allocate = [](size_t inSize)
    {
        return Core::GetHeapAllocator<HEAP_PHYSICS>().Alloc(inSize, 0);
    };
    JPH::Reallocate = [](void *inBlock, size_t inOldSize, size_t inNewSize)
    {
        return Core::GetHeapAllocator<HEAP_PHYSICS>().Realloc(inBlock, inNewSize, 0);
    };
    JPH::Free = [](void* inBlock)
    {
        return Core::GetHeapAllocator<HEAP_PHYSICS>().Free(inBlock);
    };
    JPH::AlignedAllocate = [](size_t inSize, size_t inAlignment)
    {
        return Core::GetHeapAllocator<HEAP_PHYSICS>().Alloc(inSize, inAlignment);
    };
    JPH::AlignedFree = [](void* inBlock)
    {
        return Core::GetHeapAllocator<HEAP_PHYSICS>().Free(inBlock);
    };

    // Install callbacks
    JPH::Trace = JPH_Trace;

    #ifdef JPH_ENABLE_ASSERTS
    JPH::AssertFailed = JPH_AssertFailed;
    #endif // JPH_ENABLE_ASSERTS

    // Create a factory
    JPH::Factory::sInstance = new JPH::Factory();

    // Register all Jolt physics types
    JPH::RegisterTypes();

    // We need a temp allocator for temporary allocations during the physics update. We're
    // pre-allocating 10 MB to avoid having to do allocations during the physics update.
    // B.t.w. 10 MB is a typical value you can use.
    // If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
    // malloc / free.
    m_PhysicsTempAllocator = MakeUnique<TempAllocatorImpl>(10 * 1024 * 1024);

    // We need a job system that will execute physics jobs on multiple threads. Typically
    // you would implement the JobSystem interface yourself and let Jolt Physics run on top
    // of your own job scheduler. JobSystemThreadPool is an example implementation.
    int numThreads = Math::Max(1, Thread::NumHardwareThreads - 1);
    LOG("Job system thread count {}\n", numThreads);
    m_JobSystemThreadPool = MakeUnique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, numThreads);
}

PhysicsModule::~PhysicsModule()
{
    m_PhysicsTempAllocator.Reset();
    m_JobSystemThreadPool.Reset();

    // Destroy the factory
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

HK_NAMESPACE_END
