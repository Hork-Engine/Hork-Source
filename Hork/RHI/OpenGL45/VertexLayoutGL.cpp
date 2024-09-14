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

#include "VertexLayoutGL.h"
#include "ImmediateContextGLImpl.h"
#include "LUT.h"
#include "GL/glew.h"

HK_NAMESPACE_BEGIN

namespace RHI
{

std::unique_ptr<VertexArrayObjectGL> VertexLayoutGL::CreateVAO()
{
    GLuint vaoHandle = 0;

    glCreateVertexArrays(1, &vaoHandle);
    if (!vaoHandle)
    {
        LOG("VertexLayoutGL::CreateVAO: couldn't create vertex array object\n");

        // Create a dummy vao
        return std::make_unique<VertexArrayObjectGL>(0);
    }

    for (VertexAttribInfo const* attrib = Desc.VertexAttribs; attrib < &Desc.VertexAttribs[Desc.NumVertexAttribs]; attrib++)
    {
        // glVertexAttribFormat, glVertexAttribBinding, glVertexBindingDivisor - v4.3 or GL_ARB_vertex_attrib_binding

        switch (attrib->Mode)
        {
            case VAM_FLOAT:
                glVertexArrayAttribFormat(vaoHandle,
                                          attrib->Location,
                                          attrib->NumComponents(),
                                          VertexAttribTypeLUT[attrib->TypeOfComponent()],
                                          attrib->IsNormalized(),
                                          attrib->Offset);
                break;
            case VAM_DOUBLE:
                glVertexArrayAttribLFormat(vaoHandle,
                                           attrib->Location,
                                           attrib->NumComponents(),
                                           VertexAttribTypeLUT[attrib->TypeOfComponent()],
                                           attrib->Offset);
                break;
            case VAM_INTEGER:
                glVertexArrayAttribIFormat(vaoHandle,
                                           attrib->Location,
                                           attrib->NumComponents(),
                                           VertexAttribTypeLUT[attrib->TypeOfComponent()],
                                           attrib->Offset);
                break;
        }

        glVertexArrayAttribBinding(vaoHandle, attrib->Location, attrib->InputSlot);

        for (VertexBindingInfo const* binding = Desc.VertexBindings; binding < &Desc.VertexBindings[Desc.NumVertexBindings]; binding++)
        {
            if (binding->InputSlot == attrib->InputSlot)
            {
                if (binding->InputRate == INPUT_RATE_PER_INSTANCE)
                {
                    //glVertexAttribDivisor( ) // тоже самое, что и glVertexBindingDivisor если attrib->Location==InputSlot
                    glVertexArrayBindingDivisor(vaoHandle, attrib->InputSlot, attrib->InstanceDataStepRate); // Since GL v4.3
                }
                else
                {
                    glVertexArrayBindingDivisor(vaoHandle, attrib->InputSlot, 0); // Since GL v4.3
                }
                break;
            }
        }

        glEnableVertexArrayAttrib(vaoHandle, attrib->Location);
    }

    return std::make_unique<VertexArrayObjectGL>(vaoHandle);
}

void VertexLayoutGL::DestroyVAO(ImmediateContextGLImpl* pContext)
{
    if (pContext->IsMainContext())
    {
        if (VaoHandleMainContext)
        {
            if (VaoHandleMainContext->HandleGL)
                glDeleteVertexArrays(1, &VaoHandleMainContext->HandleGL);

            VaoHandleMainContext.reset();
        }
    }
    else
    {
        auto it = VaoHandles.Find(pContext->GetUID());
        if (it != VaoHandles.End())
        {
            auto& vaoHandle = it->second;
            HK_ASSERT(vaoHandle);

            if (vaoHandle->HandleGL)
                glDeleteVertexArrays(1, &vaoHandle->HandleGL);

            VaoHandles.Erase(it);
        }
    }
}

} // namespace RHI

HK_NAMESPACE_END
