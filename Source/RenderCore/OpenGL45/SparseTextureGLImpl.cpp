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

#include "SparseTextureGLImpl.h"
#include "DeviceGLImpl.h"
#include "LUT.h"
#include "GL/glew.h"

namespace RenderCore
{

static void SetSwizzleParams(GLuint _Id, TextureSwizzle const& _Swizzle)
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

SparseTextureGLImpl::SparseTextureGLImpl(DeviceGLImpl* pDevice, SparseTextureDesc const& Desc) :
    ISparseTexture(pDevice, Desc)
{
    GLuint id;
    GLenum target         = SparseTextureTargetLUT[Desc.Type].Target;
    GLenum internalFormat = InternalFormatLUT[Desc.Format].InternalFormat;

    HK_ASSERT(Desc.NumMipLevels > 0);

    bCompressed = IsCompressedFormat(Desc.Format);

    int  pageSizeIndex = 0;

    bool r = pDevice->ChooseAppropriateSparseTexturePageSize(Desc.Type,
                                                             Desc.Format,
                                                             Desc.Resolution.Width,
                                                             Desc.Resolution.Height,
                                                             (Desc.Type == SPARSE_TEXTURE_3D) ? Desc.Resolution.SliceCount : 1,
                                                             &pageSizeIndex,
                                                             &PageSizeX,
                                                             &PageSizeY,
                                                             &PageSizeZ);
    if (!r)
    {
        LOG("SparseTextureGLImpl::ctor: failed to find appropriate sparse texture page size\n");
        return;
    }

    glCreateTextures(target, 1, &id);
    glTextureParameteri(id, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
    glTextureParameteri(id, GL_VIRTUAL_PAGE_SIZE_INDEX_ARB, pageSizeIndex);

    SetSwizzleParams(id, Desc.Swizzle);

    switch (Desc.Type)
    {
        case SPARSE_TEXTURE_2D:
        case SPARSE_TEXTURE_CUBE_MAP:
            glTextureStorage2D(id, Desc.NumMipLevels, internalFormat, Desc.Resolution.Width, Desc.Resolution.Height);
            break;
        case SPARSE_TEXTURE_2D_ARRAY:
        case SPARSE_TEXTURE_3D:
        case SPARSE_TEXTURE_CUBE_MAP_ARRAY:
            glTextureStorage3D(id, Desc.NumMipLevels, internalFormat, Desc.Resolution.Width, Desc.Resolution.Height, Desc.Resolution.SliceCount);
            break;        
    }

    SetHandleNativeGL(id);
}

SparseTextureGLImpl::~SparseTextureGLImpl()
{
    GLuint id = GetHandleNativeGL();

    if (!id)
    {
        return;
    }

    glDeleteTextures(1, &id);
}

} // namespace RenderCore
