/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include <Engine/RenderCore/Buffer.h>

HK_NAMESPACE_BEGIN

namespace RenderCore
{

class DeviceGLImpl;

class BufferGLImpl final : public IBuffer
{
public:
    BufferGLImpl(DeviceGLImpl* pDevice, BufferDesc const& Desc, const void* SysMem = nullptr);
    ~BufferGLImpl();

    bool CreateView(BufferViewDesc const& BufferViewDesc, TRef<IBufferView>* ppBufferView) override;

    bool Orphan() override;

    void Invalidate() override;

    void InvalidateRange(size_t RangeOffset, size_t RangeSize) override;

    void FlushMappedRange(size_t RangeOffset, size_t RangeSize) override;

    void Read(void* pSysMem) override;

    void ReadRange(size_t ByteOffset, size_t SizeInBytes, void* pSysMem) override;

    void Write(const void* pSysMem) override;

    void WriteRange(size_t ByteOffset, size_t SizeInBytes, const void* pSysMem) override;
};

} // namespace RenderCore

HK_NAMESPACE_END
