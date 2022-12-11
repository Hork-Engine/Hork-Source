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

#pragma once

#include "ImmediateContextGLImpl.h"

namespace RenderCore
{

class VertexArrayObjectGL
{
public:
    uint32_t HandleGL{};

    uint32_t VertexBufferUIDs[MAX_VERTEX_BUFFER_SLOTS]    = {};
    uint32_t VertexBufferOffsets[MAX_VERTEX_BUFFER_SLOTS] = {};
    uint32_t IndexBufferUID{};

    VertexArrayObjectGL(uint32_t HandleGL) :
        HandleGL(HandleGL)
    {}
};

class VertexLayoutGL : public RefCounted
{
public:
    VertexLayoutGL(VertexLayoutDescGL const& Desc) :
        Desc(Desc)
    {
        for (VertexBindingInfo const* binding = Desc.VertexBindings; binding < &Desc.VertexBindings[Desc.NumVertexBindings]; binding++)
        {
            VertexBindingsStrides[binding->InputSlot] = binding->Stride;
        }
    }

    VertexLayoutDescGL const& GetDesc() const { return Desc; }

    uint32_t const* GetVertexBindingsStrides() const {return VertexBindingsStrides; }

    VertexArrayObjectGL* GetVAO(ImmediateContextGLImpl* pContext)
    {
        // Fast path for apps with single context
        if (pContext->IsMainContext())
        {
            if (!VaoHandleMainContext)
                VaoHandleMainContext = CreateVAO();
            return VaoHandleMainContext.get();
        }

        auto it = VaoHandles.find(pContext->GetUID());
        if (it != VaoHandles.end())
            return it->second.get();

        auto vaoHandle = CreateVAO();

        VertexArrayObjectGL* ptr      = vaoHandle.get();
        VaoHandles[pContext->GetUID()] = std::move(vaoHandle);
        return ptr;
    }

    void DestroyVAO(ImmediateContextGLImpl* pContext);

private:
    HK_NODISCARD std::unique_ptr<VertexArrayObjectGL> CreateVAO();

    THashMap<uint32_t /* context id */, std::unique_ptr<VertexArrayObjectGL>> VaoHandles;
    std::unique_ptr<VertexArrayObjectGL>                                      VaoHandleMainContext;
    VertexLayoutDescGL                                                        Desc;
    uint32_t                                                                  VertexBindingsStrides[MAX_VERTEX_BUFFER_SLOTS] = {};
};

} // namespace RenderCore
