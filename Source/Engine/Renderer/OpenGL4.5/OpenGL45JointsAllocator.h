/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#pragma once

#include "OpenGL45Common.h"
#include <Engine/Core/Public/Logger.h>

namespace OpenGL45 {

// NOTE: We can use TBO to increase max joints and draw instanced
class FJointAllocator {
public:
    GHI::Buffer Buffer;
    size_t Offset;
    size_t MaxUsage;

    void Initialize() {
        Offset = 0;
        MaxUsage = 0;

        GHI::BufferCreateInfo bufferCI = {};
        bufferCI.ImmutableStorageFlags = GHI::IMMUTABLE_DYNAMIC_STORAGE;
        bufferCI.bImmutableStorage = true;
        bufferCI.SizeInBytes = sizeof( Float3x4 ) * MAX_SKINNED_MESH_JOINTS * MAX_SKINNED_MESH_INSTANCES_PER_FRAME;

        Buffer.Initialize( bufferCI );

        GLogger.Printf( "Allocated %u bytes for joints\n", bufferCI.SizeInBytes );
    }

    void Deinitialize() {
        Buffer.Deinitialize();
    }

    void Reset() {
        Offset = 0;
    }

    size_t AllocJoints( size_t _Count ) {
        size_t SizeInBytes = _Count * sizeof( Float3x4 );
        if ( Offset + SizeInBytes > Buffer.GetSizeInBytes() ) {
            GLogger.Printf( "FJointAllocator::AllocJoints: overflow\n" );
            // TODO: allocate new buffer?
            return 0;
        }
        size_t Ofs = Offset;
        Offset += SizeInBytes;
        Offset = GHI::UBOAligned( Offset );
        MaxUsage = FMath::Max( MaxUsage, Ofs + SizeInBytes );
        GLogger.Printf( "AllocJoints: Allocated %u bytes. Max usage %u bytes\n", SizeInBytes, MaxUsage );
        return Ofs;
    }
};

extern FJointAllocator GJointsAllocator;

}
