/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include "SphereMesh.h"

#include <Hork/Core/Containers/Vector.h>
#include <Hork/RHI/Common/Device.h>

HK_NAMESPACE_BEGIN

namespace RenderUtils
{

SphereMesh::SphereMesh(RHI::IDevice* device, int HDiv, int VDiv)
{
    const int numVerts = VDiv * (HDiv - 1) + 2;
    const int numIndices = (HDiv - 1) * (VDiv - 1) * 6;
    int i, j;
    float a1, a2;

    HK_ASSERT_(numVerts < 65536, "Too many vertices");

    Vector<Float3> vertices;
    Vector<unsigned short> indices;

    vertices.Resize(numVerts);
    indices.Resize(numIndices);

    for (i = 0, a1 = Math::_PI / HDiv; i < (HDiv - 1); i++)
    {
        float y, r;
        Math::SinCos(a1, r, y);
        for (j = 0, a2 = 0; j < VDiv; j++)
        {
            float s, c;
            Math::SinCos(a2, s, c);
            vertices[i * VDiv + j] = Float3(r * c, -y, r * s);
            a2 += Math::_2PI / (VDiv - 1);
        }
        a1 += Math::_PI / HDiv;
    }
    vertices[(HDiv - 1) * VDiv + 0] = Float3(0, -1, 0);
    vertices[(HDiv - 1) * VDiv + 1] = Float3(0, 1, 0);

    // generate indices
    unsigned short* pIndices = indices.ToPtr();
    for (i = 0; i < HDiv; i++)
    {
        for (j = 0; j < VDiv - 1; j++)
        {
            unsigned short i2 = i + 1;
            unsigned short j2 = (j == VDiv - 1) ? 0 : j + 1;
            if (i == (HDiv - 2))
            {
                *pIndices++ = (i * VDiv + j2);
                *pIndices++ = (i * VDiv + j);
                *pIndices++ = ((HDiv - 1) * VDiv + 1);
            }
            else if (i == (HDiv - 1))
            {
                *pIndices++ = (0 * VDiv + j);
                *pIndices++ = (0 * VDiv + j2);
                *pIndices++ = ((HDiv - 1) * VDiv + 0);
            }
            else
            {
                int quad[4] = {i * VDiv + j, i * VDiv + j2, i2 * VDiv + j2, i2 * VDiv + j};
                *pIndices++ = quad[3];
                *pIndices++ = quad[2];
                *pIndices++ = quad[1];
                *pIndices++ = quad[1];
                *pIndices++ = quad[0];
                *pIndices++ = quad[3];
            }
        }
    }

    RHI::BufferDesc bufferCI = {};
    bufferCI.bImmutableStorage = true;

    bufferCI.SizeInBytes = sizeof(Float3) * vertices.Size();
    device->CreateBuffer(bufferCI, vertices.ToPtr(), &m_VertexBuffer);

    m_VertexBuffer->SetDebugName("Sphere mesh vertex buffer");

    bufferCI.SizeInBytes = sizeof(unsigned short) * indices.Size();
    device->CreateBuffer(bufferCI, indices.ToPtr(), &m_IndexBuffer);

    m_VertexBuffer->SetDebugName("Sphere mesh index buffer");

    m_IndexCount = indices.Size();
}

void SphereMesh::Draw(RHI::IImmediateContext* immediateCtx, RHI::IPipeline* pipeline, unsigned int instanceCount)
{
    RHI::DrawIndexedCmd drawCmd = {};
    drawCmd.IndexCountPerInstance = m_IndexCount;
    drawCmd.InstanceCount = instanceCount;

    immediateCtx->BindPipeline(pipeline);
    immediateCtx->BindVertexBuffer(0, m_VertexBuffer);
    immediateCtx->BindIndexBuffer(m_IndexBuffer, RHI::INDEX_TYPE_UINT16);
    immediateCtx->Draw(&drawCmd);
}

}

HK_NAMESPACE_END
