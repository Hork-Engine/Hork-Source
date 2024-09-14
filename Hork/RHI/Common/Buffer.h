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

#pragma once

#include "BufferView.h"

HK_NAMESPACE_BEGIN

namespace RHI
{

/// Buffer bindings
enum BUFFER_BINDING : uint8_t
{
    BUFFER_BIND_CONSTANT,
    BUFFER_BIND_STORAGE,
    BUFFER_BIND_FEEDBACK,
    BUFFER_BIND_ATOMIC_COUNTER
};

/// There are three hints that the user can specify the data for mutable storage buffer
enum MUTABLE_STORAGE_CLIENT_ACCESS : uint8_t
{
    MUTABLE_STORAGE_CLIENT_WRITE_ONLY,   /// The user will be writing data to the buffer, but the user will not read it.
    MUTABLE_STORAGE_CLIENT_READ_ONLY,    /// The user will not be writing data, but the user will be reading it back.
    MUTABLE_STORAGE_CLIENT_NO_TRANSFER,  /// The user will be neither writing nor reading the data.
    MUTABLE_STORAGE_CLIENT_DONT_CARE = 0 /// Use this for immutable buffers
};

/// There are three hints for how frequently the user will be changing the mutable buffer's data.
enum MUTABLE_STORAGE_USAGE : uint8_t
{
    MUTABLE_STORAGE_STATIC,        /// The user will set the data once.
    MUTABLE_STORAGE_DYNAMIC,       /// The user will set the data occasionally.
    MUTABLE_STORAGE_STREAM,        /// The user will be changing the data after every use. Or almost every use.
    MUTABLE_STORAGE_DONT_CARE = 0, /// Use this for immutable buffers
};

/// This bits cover how the user may directly read from or write to the immutable buffer.
/// But this only restricts how the user directly modifies the data store; "server-side" operations on buffer contents are always available.
enum IMMUTABLE_STORAGE_FLAGS : uint16_t
{
    IMMUTABLE_MAP_READ           = 0x1,   /// Allows the user to read the buffer via mapping the buffer. Without this flag, attempting to map the buffer for reading will fail.
    IMMUTABLE_MAP_WRITE          = 0x2,   /// Allows the user to map the buffer for writing. Without this flag, attempting to map the buffer for writing will fail.
    IMMUTABLE_MAP_PERSISTENT     = 0x40,  /// Allows the buffer object to be mapped in such a way that it can be used while it is mapped. Without this flag, attempting to perform any operation on the buffer while it is mapped will fail. You must use one of the mapping bits when using this bit.
    IMMUTABLE_MAP_COHERENT       = 0x80,  /// Allows reads from and writes to a persistent buffer to be coherent with OpenGL, without an explicit barrier. Without this flag, you must use an explicit barrier to achieve coherency. You must use IMMUTABLE_MAP_PERSISTENT when using this bit.
    IMMUTABLE_DYNAMIC_STORAGE    = 0x100, /// Allows the user to modify the contents of the storage with client-side CopyBufferRange, CopyBufferData​. Without this flag, attempting to call that function on this buffer will fail.
    IMMUTABLE_MAP_CLIENT_STORAGE = 0x200  /// A hint that suggests to the implementation that the storage for the buffer should come from "client" memory.
};

/// The following operations are always valid on immutable buffers regardless of the ImmutableStorageFlags:
/// - Writing to the buffer with any rendering pipeline process. These include Transform Feedback, Image Load Store, Atomic Counter, and Shader Storage Buffer Object. Basically, anything that is part of the rendering pipeline that can write to a buffer will always work.
/// - Clearing the buffer. Because this only transfers a few bytes of data, it is not considered "client-side" modification.
/// - Copying the buffer. This copies from one buffer to another, so it all happens "server-side".
/// - Invalidating the buffer. This only wipes out the contents of the buffer, so it is considered "server-side".
/// - Asynchronous pixel transfers into the buffer. This sets the data in a buffer, but only through pure-OpenGL mechanisms.
/// - Using ReadRange,Read​ to read a part of the buffer back to the CPU. This is not a "server-side" operation, but it's always available regardless.
enum MAP_TRANSFER : uint8_t
{
    MAP_TRANSFER_READ, /// Allows the user to perform read-only operations with buffer.
                       /// Attempting to map the buffer for writing will fail.

    MAP_TRANSFER_WRITE, /// Allows the user to perform write-only operations with buffer.
                        /// Attempting to map the buffer for reading will fail.

    MAP_TRANSFER_RW /// Allows the user to perform reading and writing operations with buffer.
};

enum MAP_INVALIDATE : uint8_t
{
    MAP_NO_INVALIDATE, /// Indicates that the previous contents of the specified range may not be discarded.

    MAP_INVALIDATE_RANGE, /// Indicates that the previous contents of the specified range may be discarded.
                          /// Data within this range are undefined with the exception of subsequently written data.
                          /// No error is generated if subsequent GL operations access unwritten data,
                          /// but the result is undefined and system errors (possibly including program termination) may occur.
                          /// This flag may not be used in combination with MAP_TRANSFER_READ or MAP_TRANSFER_RW.

    MAP_INVALIDATE_ENTIRE_BUFFER /// Indicates that the previous contents of the entire buffer may be discarded.
                                 /// Data within the entire buffer are undefined with the exception of subsequently written data.
                                 /// No error is generated if subsequent GL operations access unwritten data,
                                 /// but the result is undefined and system errors (possibly including program termination) may occur.
                                 /// This flag may not be used in combination with MAP_TRANSFER_READ or MAP_TRANSFER_RW.
};

enum MAP_PERSISTENCE : uint8_t
{
    MAP_NON_PERSISTENT, /// With this flag, attempting to perform any operation on the buffer while it is mapped will fail.

    MAP_PERSISTENT_COHERENT, /// This flag allows the buffer object to be mapped in such a way that it can be used while it is mapped.
                             /// Allows reads from and writes to a persistent buffer to be coherent with hardware,
                             /// without an explicit barrier.
                             /// If you use this flag for immutable buffer IMMUTABLE_MAP_PERSISTENT must be set on creation stage.

    MAP_PERSISTENT_NO_COHERENT /// With this flag, persistent mappings are not coherent and modified ranges of the buffer
                               /// store must be explicitly communicated to the hardware, either by unmapping the buffer, or through a call
                               /// to FlushMappedRange or MemoryBarrier.
                               /// If you use this flag for immutable buffer IMMUTABLE_MAP_PERSISTENT must be set on creation stage.
};

struct BufferDesc
{
    bool bImmutableStorage = false;

    /// make sense only with bImmutableStorage = true
    IMMUTABLE_STORAGE_FLAGS ImmutableStorageFlags = IMMUTABLE_MAP_WRITE;

    /// only for mutable buffers
    MUTABLE_STORAGE_CLIENT_ACCESS MutableClientAccess = MUTABLE_STORAGE_CLIENT_DONT_CARE;

    /// only for mutable buffers
    MUTABLE_STORAGE_USAGE MutableUsage = MUTABLE_STORAGE_DONT_CARE;

    /// size of buffer in bytes
    size_t SizeInBytes = 0;
};

class IBuffer : public IDeviceObject
{
public:
    static constexpr DEVICE_OBJECT_PROXY_TYPE PROXY_TYPE = DEVICE_OBJECT_TYPE_BUFFER;

    IBuffer(IDevice* pDevice, BufferDesc const& Desc) :
        IDeviceObject(pDevice, PROXY_TYPE), Desc(Desc)
    {}

    virtual bool CreateView(BufferViewDesc const& BufferViewDesc, Ref<IBufferView>* ppBufferView) = 0;

    BufferDesc const& GetDesc() const { return Desc; }

    /// Allocate new storage for the buffer
    virtual bool Orphan() = 0;

    virtual void Invalidate() = 0;

    virtual void InvalidateRange(size_t _RangeOffset, size_t _RangeSize) = 0;

    virtual void FlushMappedRange(size_t _RangeOffset, size_t _RangeSize) = 0;

    virtual void Read(void* pSysMem) = 0;

    virtual void ReadRange(size_t ByteOffset, size_t SizeInBytes, void* pSysMem) = 0;

    virtual void Write(const void* pSysMem) = 0;

    virtual void WriteRange(size_t ByteOffset, size_t SizeInBytes, const void* pSysMem) = 0;

protected:
    BufferDesc Desc;
};

} // namespace RHI

HK_NAMESPACE_END
