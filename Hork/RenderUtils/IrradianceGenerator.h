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

#include <Hork/RHI/Common/Device.h>
#include <Hork/Math/VectorMath.h>
#include "SphereMesh.h"

HK_NAMESPACE_BEGIN

class IrradianceGenerator
{
public:
                            IrradianceGenerator(RHI::IDevice* device, RenderUtils::SphereMesh* sphereMesh);

    void                    GenerateArray(int cubemapsCount, RHI::ITexture** cubemaps, Ref<RHI::ITexture>* ppTextureArray);
    void                    Generate(RHI::ITexture* sourceCubemap, Ref<RHI::ITexture>* ppTexture);

private:
    struct ConstantData
    {
        Float4x4    Transform[6];
        Float4      Index;
    };
    Ref<RHI::IDevice>       m_Device;
    Ref<RenderUtils::SphereMesh> m_SphereMesh;
    Ref<RHI::IBuffer>       m_ConstantBuffer;
    ConstantData            m_ConstantBufferData;
    Ref<RHI::IPipeline>     m_Pipeline;
};

HK_NAMESPACE_END
