/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include "SamplerGLImpl.h"
#include "DeviceGLImpl.h"

#include <unordered_map>

#include "GL/glew.h"

namespace RenderCore
{

static std::unordered_map<uint64_t, int> BindlessHandleRefCount;

ABindlessSamplerGLImpl::ABindlessSamplerGLImpl(ADeviceGLImpl* pDevice, ITexture* pTexture, SSamplerDesc const& Desc) :
    IBindlessSampler(pDevice)
{
    Texture = pTexture;
    Texture->AddRef();

    if (!pDevice->IsFeatureSupported(FEATURE_BINDLESS_TEXTURE))
    {
        GLogger.Printf("ABindlessSamplerGLImpl::ctor: bindless textures not supported by current hardware\n");
        return;
    }

    unsigned int samplerId = pDevice->CachedSampler(Desc);

    uint64_t id = glGetTextureSamplerHandleARB(pTexture->GetHandleNativeGL(), samplerId);

    SetHandleNativeGL(id);

    if (id)
    {
        ++BindlessHandleRefCount[id];
    }
}

ABindlessSamplerGLImpl::~ABindlessSamplerGLImpl()
{
    Texture->RemoveRef();

    uint64_t id = GetHandleNativeGL();

    if (id)
    {
        if (0 == --BindlessHandleRefCount[id])
        {
            glMakeTextureHandleNonResidentARB(id);
        }
    }
}

void ABindlessSamplerGLImpl::MakeResident()
{
    uint64_t id = GetHandleNativeGL();
    if (id)
    {
        glMakeTextureHandleResidentARB(id);
    }
}

void ABindlessSamplerGLImpl::MakeNonResident()
{
    uint64_t id = GetHandleNativeGL();
    if (id)
    {
        glMakeTextureHandleNonResidentARB(id);
    }
}

bool ABindlessSamplerGLImpl::IsResident() const
{
    uint64_t id = GetHandleNativeGL();
    if (id)
    {
        return !!glIsTextureHandleResidentARB(id);
    }
    return false;
}

} // namespace RenderCore
