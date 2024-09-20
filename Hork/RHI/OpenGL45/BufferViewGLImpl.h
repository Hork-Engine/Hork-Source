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

#include <Hork/RHI/Common/BufferView.h>

#include "BufferGLImpl.h"

HK_NAMESPACE_BEGIN

namespace RHI
{

class DeviceGLImpl;

class BufferViewGLImpl final : public IBufferView
{
public:
    BufferViewGLImpl(BufferViewDesc const& Desc, BufferGLImpl* pBuffer);
    ~BufferViewGLImpl();

    void SetRange(size_t Offset, size_t SizeInBytes) override;

    size_t GetBufferOffset(uint16_t MipLevel) const override;
    size_t GetBufferSizeInBytes(uint16_t MipLevel) const override;

private:
    BufferGLImpl* pSrcBuffer;
    int           InternalFormat;
};

} // namespace RHI

HK_NAMESPACE_END