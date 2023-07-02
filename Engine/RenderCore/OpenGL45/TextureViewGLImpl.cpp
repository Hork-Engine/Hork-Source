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

#include "TextureViewGLImpl.h"
#include "LUT.h"
#include "GL/glew.h"

#include <Engine/Core/Logger.h>

HK_NAMESPACE_BEGIN

namespace RenderCore
{

TextureViewGLImpl::TextureViewGLImpl(TextureViewDesc const& TextureViewDesc, ITexture* pTexture) :
    ITextureView(TextureViewDesc, pTexture)
{
    TextureDesc const& textureDesc = pTexture->GetDesc();

    if (TextureViewDesc.ViewType == TEXTURE_VIEW_RENDER_TARGET)
    {
        HK_ASSERT(!IsDepthStencilFormat(textureDesc.Format));

        SetHandleNativeGL(pTexture->GetHandleNativeGL());
    }
    else if (TextureViewDesc.ViewType == TEXTURE_VIEW_DEPTH_STENCIL)
    {
        HK_ASSERT(IsDepthStencilFormat(textureDesc.Format));

        SetHandleNativeGL(pTexture->GetHandleNativeGL());
    }
    else if (TextureViewDesc.ViewType == TEXTURE_VIEW_UNORDERED_ACCESS)
    {
        SetHandleNativeGL(pTexture->GetHandleNativeGL());
    }
    else if (TextureViewDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE)
    {
        // clang-format off
        bool bFullView = TextureViewDesc.Type          == textureDesc.Type &&
                         TextureViewDesc.Format        == textureDesc.Format &&
                         TextureViewDesc.FirstMipLevel == 0 &&
                         TextureViewDesc.NumMipLevels  == textureDesc.NumMipLevels &&
                         TextureViewDesc.FirstSlice    == 0 &&
                         TextureViewDesc.NumSlices     == pTexture->GetSliceCount();
        // clang-format on

        if (bFullView)
        {
            SetHandleNativeGL(pTexture->GetHandleNativeGL());
        }
        else
        {
            GLuint id;
            GLint  internalFormat = InternalFormatLUT[TextureViewDesc.Format].InternalFormat;

            GLenum target = TextureTargetLUT[TextureViewDesc.Type].Target;
            if (pTexture->IsMultisample())
            {
                if (target == GL_TEXTURE_2D)
                    target = GL_TEXTURE_2D_MULTISAMPLE;
                if (target == GL_TEXTURE_2D_ARRAY)
                    target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
            }

            // Avoid previous errors if any
            (void)glGetError();

            glGenTextures(1, &id);

            // 4.3
            glTextureView(id, target, pTexture->GetHandleNativeGL(),
                          internalFormat, TextureViewDesc.FirstMipLevel, TextureViewDesc.NumMipLevels, TextureViewDesc.FirstSlice, TextureViewDesc.NumSlices);

            if (glGetError() != GL_NO_ERROR)
            {
                // Incompatible texture formats (See OpenGL specification for details)
                if (glIsTexture(id))
                {
                    glDeleteTextures(1, &id);
                }
                LOG("TextureViewGLImpl::ctor: failed to initialize texture view, incompatible texture formats\n");
                return;
            }

            SetHandleNativeGL(id);
        }
    }
}

TextureViewGLImpl::~TextureViewGLImpl()
{
    if (GetDesc().ViewType == TEXTURE_VIEW_SHADER_RESOURCE)
    {
        GLuint id = GetHandleNativeGL();

        if (id && id != GetTexture()->GetHandleNativeGL())
        {
            glDeleteTextures(1, &id);
        }
    }
}


} // namespace RenderCore

HK_NAMESPACE_END
