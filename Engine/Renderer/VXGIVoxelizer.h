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

#include <Engine/RenderCore/ImmediateContext.h>
#include <Engine/Geometry/VectorMath.h>

HK_NAMESPACE_BEGIN

class VXGIVoxelizer {
public:
    VXGIVoxelizer();

    void Render();

private:
    struct DrawElementsIndirectCommand {
        uint32_t vertexCount;
        uint32_t instanceCount;
        uint32_t firstVertex;
        uint32_t baseVertex;
        uint32_t baseInstance;
    };
    struct ComputeIndirectCommand {
        uint32_t workGroupSizeX;
        uint32_t workGroupSizeY;
        uint32_t workGroupSizeZ;
    };

    void CreatePipeline();
    //struct ConstantData
    //{
    //    Float4x4 Transform[6];
    //    Float4 LightDir;
    //};
    //TRef< RenderCore::IBuffer > ConstantBuffer;
    //ConstantData ConstantBufferData;
    TRef< RenderCore::IPipeline > Pipeline;

//    TRef< RenderCore::IFramebuffer > voxelFBO;
    TRef< RenderCore::ITexture > voxel2DTex;
    TRef< RenderCore::ITexture > voxelTex;
    TRef< RenderCore::IBuffer > drawIndBuffer;
    TRef< RenderCore::IBuffer > compIndBuffer;
};

HK_NAMESPACE_END
