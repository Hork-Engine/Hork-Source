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

#include <Hork/RHI/Common/ImmediateContext.h>

HK_NAMESPACE_BEGIN

namespace RenderUtils
{

class SphereMesh : public RefCounted
{
public:
                        SphereMesh(RHI::IDevice* device, int HDiv = 128, int VDiv = 128);

    void                Draw(RHI::IImmediateContext* immediateCtx, RHI::IPipeline* pipeline, unsigned int instanceCount = 1);

private:
    Ref<RHI::IBuffer>   m_VertexBuffer;
    Ref<RHI::IBuffer>   m_IndexBuffer;
    unsigned int        m_IndexCount;
};

}

HK_NAMESPACE_END
