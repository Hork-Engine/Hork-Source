/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "TextureGLImpl.h"
#include "DeviceGLImpl.h"
#include "ImmediateContextGLImpl.h"
#include "LUT.h"
#include "GL/glew.h"

namespace RenderCore
{

static size_t CalcTextureRequiredMemory()
{
    // TODO: calculate!
    return 0;
}

static void SetSwizzleParams(GLuint _Id, STextureSwizzle const& _Swizzle)
{
    if (_Swizzle.R != TEXTURE_SWIZZLE_IDENTITY)
    {
        glTextureParameteri(_Id, GL_TEXTURE_SWIZZLE_R, SwizzleLUT[_Swizzle.R]);
    }
    if (_Swizzle.G != TEXTURE_SWIZZLE_IDENTITY)
    {
        glTextureParameteri(_Id, GL_TEXTURE_SWIZZLE_G, SwizzleLUT[_Swizzle.G]);
    }
    if (_Swizzle.B != TEXTURE_SWIZZLE_IDENTITY)
    {
        glTextureParameteri(_Id, GL_TEXTURE_SWIZZLE_B, SwizzleLUT[_Swizzle.B]);
    }
    if (_Swizzle.A != TEXTURE_SWIZZLE_IDENTITY)
    {
        glTextureParameteri(_Id, GL_TEXTURE_SWIZZLE_A, SwizzleLUT[_Swizzle.A]);
    }
}

ATextureGLImpl::ATextureGLImpl(ADeviceGLImpl* pDevice, STextureDesc const& TextureDesc, bool bDummyTexture) :
    ITexture(pDevice, TextureDesc), bDummyTexture(bDummyTexture)
{
    GLuint id = 0;

    if (!bDummyTexture)
    {
        GLenum target         = TextureTargetLUT[TextureDesc.Type].Target;
        GLenum internalFormat = InternalFormatLUT[TextureDesc.Format].InternalFormat;

        if (TextureDesc.Multisample.NumSamples > 1)
        {
            switch (target)
            {
                case GL_TEXTURE_2D:
                    target = GL_TEXTURE_2D_MULTISAMPLE;
                    break;
                case GL_TEXTURE_2D_ARRAY:
                    target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
                    break;
            }
        }

        glCreateTextures(target, 1, &id);

        SetSwizzleParams(id, TextureDesc.Swizzle);

        //glTextureParameterf( id, GL_TEXTURE_BASE_LEVEL, 0 );
        //glTextureParameterf( id, GL_TEXTURE_MAX_LEVEL, StorageNumMipLevels - 1 );

        switch (TextureDesc.Type)
        {
            case TEXTURE_1D:
                glTextureStorage1D(id, TextureDesc.NumMipLevels, internalFormat, TextureDesc.Resolution.Width);
                break;
            case TEXTURE_1D_ARRAY:
                glTextureStorage2D(id, TextureDesc.NumMipLevels, internalFormat, TextureDesc.Resolution.Width, TextureDesc.Resolution.SliceCount);
                break;
            case TEXTURE_2D:
                if (TextureDesc.Multisample.NumSamples > 1)
                {
                    glTextureStorage2DMultisample(id, TextureDesc.Multisample.NumSamples, internalFormat, TextureDesc.Resolution.Width, TextureDesc.Resolution.Height, TextureDesc.Multisample.bFixedSampleLocations);
                }
                else
                {
                    glTextureStorage2D(id, TextureDesc.NumMipLevels, internalFormat, TextureDesc.Resolution.Width, TextureDesc.Resolution.Height);
                }
                break;
            case TEXTURE_2D_ARRAY:
                if (TextureDesc.Multisample.NumSamples > 1)
                {
                    glTextureStorage3DMultisample(id, TextureDesc.Multisample.NumSamples, internalFormat, TextureDesc.Resolution.Width, TextureDesc.Resolution.Height, TextureDesc.Resolution.SliceCount, TextureDesc.Multisample.bFixedSampleLocations);
                }
                else
                {
                    glTextureStorage3D(id, TextureDesc.NumMipLevels, internalFormat, TextureDesc.Resolution.Width, TextureDesc.Resolution.Height, TextureDesc.Resolution.SliceCount);
                }
                break;
            case TEXTURE_3D:
                glTextureStorage3D(id, TextureDesc.NumMipLevels, internalFormat, TextureDesc.Resolution.Width, TextureDesc.Resolution.Height, TextureDesc.Resolution.SliceCount);
                break;
            case TEXTURE_CUBE_MAP:
                glTextureStorage2D(id, TextureDesc.NumMipLevels, internalFormat, TextureDesc.Resolution.Width, TextureDesc.Resolution.Height);
                break;
            case TEXTURE_CUBE_MAP_ARRAY:
                glTextureStorage3D(id, TextureDesc.NumMipLevels, internalFormat, TextureDesc.Resolution.Width, TextureDesc.Resolution.Height, TextureDesc.Resolution.SliceCount);
                break;
            case TEXTURE_RECT_GL:
                glTextureStorage2D(id, TextureDesc.NumMipLevels, internalFormat, TextureDesc.Resolution.Width, TextureDesc.Resolution.Height);
                break;
        }

        //glTextureParameteri( id, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        //glTextureParameteri( id, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        //glTextureParameteri( id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        //glTextureParameteri( id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        //glTextureParameteri( id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
        //glTextureParameterf( id, GL_TEXTURE_LOD_BIAS, 0.0f );
        //glTextureParameterf( id, GL_TEXTURE_MIN_LOD, 0 );
        //glTextureParameterf( id, GL_TEXTURE_MAX_LOD, StorageMipLevels - 1 );
        //glTextureParameteri( id, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.0f );
        //glTextureParameteri( id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
        //glTextureParameteri( id, GL_TEXTURE_COMPARE_FUNC, ComparisonFunc );
        //glTextureParameterfv( id, GL_TEXTURE_BORDER_COLOR, BorderColor );
        //glTextureParameteri( id, GL_TEXTURE_CUBE_MAP_SEAMLESS, bCubemapSeamless );
    }

    pDevice->TextureMemoryAllocated += CalcTextureRequiredMemory();

    bCompressed = IsCompressedFormat(TextureDesc.Format);

    SetHandleNativeGL(id);

    CreateDefaultViews();
}

void ATextureGLImpl::CreateDefaultViews()
{
    STextureViewDesc viewDesc;
    viewDesc.Type          = GetDesc().Type;
    viewDesc.Format        = GetDesc().Format;
    viewDesc.FirstMipLevel = 0;
    viewDesc.FirstSlice    = 0;
    viewDesc.NumSlices     = GetSliceCount();

    if (IsDepthStencilFormat(GetDesc().Format))
    {
        if (GetDesc().BindFlags & BIND_DEPTH_STENCIL)
        {
            viewDesc.ViewType     = TEXTURE_VIEW_DEPTH_STENCIL;
            viewDesc.NumMipLevels = 1;
            pDepthStencilView     = GetTextureView(viewDesc);
        }
    }
    else
    {
        if (GetDesc().BindFlags & BIND_RENDER_TARGET)
        {
            viewDesc.ViewType     = TEXTURE_VIEW_RENDER_TARGET;
            viewDesc.NumMipLevels = 1;
            pRenderTargetView     = GetTextureView(viewDesc);
        }
    }

    if (GetDesc().BindFlags & BIND_SHADER_RESOURCE)
    {
        viewDesc.ViewType     = TEXTURE_VIEW_SHADER_RESOURCE;
        viewDesc.NumMipLevels = Desc.NumMipLevels;
        pShaderResourceView   = GetTextureView(viewDesc);
    }

    if (GetDesc().BindFlags & BIND_UNORDERED_ACCESS)
    {
        viewDesc.ViewType     = TEXTURE_VIEW_UNORDERED_ACCESS;
        viewDesc.NumMipLevels = Desc.NumMipLevels;
        pUnorderedAccesView   = GetTextureView(viewDesc);
    }
}

ITextureView* ATextureGLImpl::GetTextureView(STextureViewDesc const& TextureViewDesc)
{   
    auto it = Views.find(TextureViewDesc);
    if (it == Views.end())
    {
        TRef<ATextureViewGLImpl> textureView;

        textureView = MakeRef<ATextureViewGLImpl>(TextureViewDesc, this);
        Views[TextureViewDesc] = textureView;

        return textureView;
    }

    return it->second;
}

ATextureGLImpl::~ATextureGLImpl()
{
    // It is important to destroy views before a texture
#ifdef HK_ALLOW_ASSERTS
    for (auto& it : Views)
    {
        HK_ASSERT_(it.second->GetRefCount() == 1, "WARNING: Someone keep a strong reference to the texture view");
    }
#endif
    Views.clear();

    for (BindlessHandle it : BindlessSamplers)
    {
        glMakeTextureHandleNonResidentARB(it);
    }

    GLuint id = GetHandleNativeGL();

    if (id)
    {
        glDeleteTextures(1, &id);
    }
    
    static_cast<ADeviceGLImpl*>(GetDevice())->TextureMemoryAllocated -= CalcTextureRequiredMemory();
}

void ATextureGLImpl::MakeBindlessSamplerResident(BindlessHandle BindlessHandle, bool bResident)
{
    HK_ASSERT(BindlessSamplers.count(BindlessHandle) == 1);

    if (bResident)
        glMakeTextureHandleResidentARB(BindlessHandle);
    else
        glMakeTextureHandleNonResidentARB(BindlessHandle);
}

bool ATextureGLImpl::IsBindlessSamplerResident(BindlessHandle BindlessHandle)
{
    HK_ASSERT(BindlessSamplers.count(BindlessHandle) == 1);

    return !!glIsTextureHandleResidentARB(BindlessHandle);
}

BindlessHandle ATextureGLImpl::GetBindlessSampler(SSamplerDesc const& SamplerDesc)
{
    if (!GetDevice()->IsFeatureSupported(FEATURE_BINDLESS_TEXTURE))
    {
        LOG("ATextureGLImpl::GetBindlessSampler: bindless textures are not supported by current hardware\n");
        return 0;
    }

    uint64_t bindlessHandle = glGetTextureSamplerHandleARB(GetHandleNativeGL(), static_cast<ADeviceGLImpl*>(GetDevice())->CachedSampler(SamplerDesc));
    if (!bindlessHandle)
    {
        LOG("ATextureGLImpl::GetBindlessSampler: couldn't get texture sampler handle\n");
        return 0;
    }

    BindlessSamplers.insert(bindlessHandle);

    return bindlessHandle;
}

void ATextureGLImpl::GetMipLevelInfo(uint16_t MipLevel, STextureMipLevelInfo* pInfo) const
{
    *pInfo = STextureMipLevelInfo{};

    switch (Desc.Type)
    {
        case TEXTURE_1D:
            pInfo->Resoultion.Width      = Math::Max(1u, Desc.Resolution.Width >> MipLevel);
            pInfo->Resoultion.Height     = 1;
            pInfo->Resoultion.SliceCount = 1;
            break;
        case TEXTURE_1D_ARRAY:
            pInfo->Resoultion.Width      = Math::Max(1u, Desc.Resolution.Width >> MipLevel);
            pInfo->Resoultion.Height     = 1;
            pInfo->Resoultion.SliceCount = Desc.Resolution.SliceCount;
            break;
        case TEXTURE_2D:
            pInfo->Resoultion.Width      = Math::Max(1u, Desc.Resolution.Width >> MipLevel);
            pInfo->Resoultion.Height     = Math::Max(1u, Desc.Resolution.Height >> MipLevel);
            pInfo->Resoultion.SliceCount = 1;
            break;
        case TEXTURE_2D_ARRAY:
            pInfo->Resoultion.Width      = Math::Max(1u, Desc.Resolution.Width >> MipLevel);
            pInfo->Resoultion.Height     = Math::Max(1u, Desc.Resolution.Height >> MipLevel);
            pInfo->Resoultion.SliceCount = Desc.Resolution.SliceCount;
            break;
        case TEXTURE_3D:
            pInfo->Resoultion.Width      = Math::Max(1u, Desc.Resolution.Width >> MipLevel);
            pInfo->Resoultion.Height     = Math::Max(1u, Desc.Resolution.Height >> MipLevel);
            pInfo->Resoultion.SliceCount = Math::Max(1u, Desc.Resolution.SliceCount >> MipLevel);
            break;
        case TEXTURE_CUBE_MAP:
            pInfo->Resoultion.Width      = Math::Max(1u, Desc.Resolution.Width >> MipLevel);
            pInfo->Resoultion.Height     = pInfo->Resoultion.Width;
            pInfo->Resoultion.SliceCount = 6;
            break;
        case TEXTURE_CUBE_MAP_ARRAY:
            pInfo->Resoultion.Width      = Math::Max(1u, Desc.Resolution.Width >> MipLevel);
            pInfo->Resoultion.Height     = pInfo->Resoultion.Width;
            pInfo->Resoultion.SliceCount = Desc.Resolution.SliceCount;
            break;
        case TEXTURE_RECT_GL:
            pInfo->Resoultion.Width      = Math::Max(1u, Desc.Resolution.Width >> MipLevel);
            pInfo->Resoultion.Height     = Math::Max(1u, Desc.Resolution.Height >> MipLevel);
            pInfo->Resoultion.SliceCount = 1;
            break;
    }

    pInfo->bCompressed = bCompressed;

    if (bCompressed)
    {
        int tmp;
        glGetTextureLevelParameteriv(GetHandleNativeGL(), MipLevel, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &tmp);
        pInfo->CompressedDataSizeInBytes = tmp;
    }

    //GL_TEXTURE_INTERNAL_FORMAT
    //params returns a single value, the internal format of the texture image.

    //GL_TEXTURE_RED_TYPE, GL_TEXTURE_GREEN_TYPE, GL_TEXTURE_BLUE_TYPE, GL_TEXTURE_ALPHA_TYPE, GL_TEXTURE_DEPTH_TYPE
    //The data type used to store the component. The types GL_NONE, GL_SIGNED_NORMALIZED, GL_UNSIGNED_NORMALIZED, GL_FLOAT, GL_INT, and GL_UNSIGNED_INT may be returned to indicate signed normalized fixed-point, unsigned normalized fixed-point, floating-point, integer unnormalized, and unsigned integer unnormalized components, respectively.

    //GL_TEXTURE_RED_SIZE, GL_TEXTURE_GREEN_SIZE, GL_TEXTURE_BLUE_SIZE, GL_TEXTURE_ALPHA_SIZE, GL_TEXTURE_DEPTH_SIZE
    //The internal storage resolution of an individual component. The resolution chosen by the GL will be a close match for the resolution requested by the user with the component argument of glTexImage1D, glTexImage2D, glTexImage3D, glCopyTexImage1D, and glCopyTexImage2D. The initial value is 0.
}

void ATextureGLImpl::Invalidate(uint16_t MipLevel)
{
    if (IsDummyTexture())
        return;
    glInvalidateTexImage(GetHandleNativeGL(), MipLevel);
}

void ATextureGLImpl::InvalidateRect(uint32_t _NumRectangles, STextureRect const* _Rectangles)
{
    if (IsDummyTexture())
        return;

    GLuint id = GetHandleNativeGL();

    for (STextureRect const* rect = _Rectangles; rect < &_Rectangles[_NumRectangles]; rect++)
    {
        glInvalidateTexSubImage(id,
                                rect->Offset.MipLevel,
                                rect->Offset.X,
                                rect->Offset.Y,
                                rect->Offset.Z,
                                rect->Dimension.X,
                                rect->Dimension.Y,
                                rect->Dimension.Z);
    }
}

void ATextureGLImpl::Read(uint16_t     MipLevel,
                          DATA_FORMAT  Format,
                          size_t       SizeInBytes,
                          unsigned int Alignment,
                          void*        pSysMem)
{
    HK_ASSERT(MipLevel < GetDesc().NumMipLevels);

    STextureRect rect;
    rect.Offset.MipLevel = MipLevel;
    rect.Dimension.X     = Math::Max(1u, GetWidth() >> MipLevel);
    rect.Dimension.Y     = Math::Max(1u, GetHeight() >> MipLevel);
    rect.Dimension.Z     = GetSliceCount(MipLevel);

    ReadRect(rect, Format, SizeInBytes, Alignment, pSysMem);
}

void ATextureGLImpl::ReadRect(STextureRect const& Rectangle,
                              DATA_FORMAT         Format,
                              size_t              SizeInBytes,
                              unsigned int        Alignment,
                              void*               pSysMem)
{
    if (IsDummyTexture())
    {
        HK_ASSERT(pContext);
        
        SScopedContextGL scopedContext(pContext);
        pContext->ReadTextureRect(this, Rectangle, Format, SizeInBytes, Alignment, pSysMem);
    }
    else
    {
        AImmediateContextGLImpl* current = AImmediateContextGLImpl::GetCurrent();
        current->ReadTextureRect(this, Rectangle, Format, SizeInBytes, Alignment, pSysMem);
    }
}

bool ATextureGLImpl::Write(uint16_t     MipLevel,
                           DATA_FORMAT  Type, // Specifies a pixel format for the input data
                           size_t       SizeInBytes,
                           unsigned int Alignment, // Specifies alignment of source data
                           const void*  pSysMem)
{
    HK_ASSERT(MipLevel < GetDesc().NumMipLevels);

    STextureRect rect;
    rect.Offset.MipLevel = MipLevel;
    rect.Dimension.X     = Math::Max(1u, GetWidth() >> MipLevel);
    rect.Dimension.Y     = Math::Max(1u, GetHeight() >> MipLevel);
    rect.Dimension.Z     = GetSliceCount(MipLevel);

    return WriteRect(rect,
                     Type,
                     SizeInBytes,
                     Alignment,
                     pSysMem);
}

bool ATextureGLImpl::WriteRect(STextureRect const& Rectangle,
                               DATA_FORMAT         Format, // Specifies a pixel format for the input data
                               size_t              SizeInBytes,
                               unsigned int        Alignment, // Specifies alignment of source data
                               const void*         pSysMem)
{
    AImmediateContextGLImpl* current = AImmediateContextGLImpl::GetCurrent();
    return current->WriteTextureRect(this, Rectangle, Format, SizeInBytes, Alignment, pSysMem);
}

} // namespace RenderCore
