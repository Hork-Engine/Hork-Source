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

class AVertexArrayObjectGL
{
public:
    uint32_t HandleGL{};

    uint32_t VertexBufferUIDs[MAX_VERTEX_BUFFER_SLOTS]    = {};
    uint32_t VertexBufferOffsets[MAX_VERTEX_BUFFER_SLOTS] = {};
    uint32_t IndexBufferUID{};

    AVertexArrayObjectGL(uint32_t HandleGL) :
        HandleGL(HandleGL)
    {}
};

struct SVertexLayoutDescGL
{
    uint32_t           NumVertexBindings{};
    SVertexBindingInfo VertexBindings[MAX_VERTEX_BINDINGS] = {};
    uint32_t           NumVertexAttribs{};
    SVertexAttribInfo  VertexAttribs[MAX_VERTEX_ATTRIBS] = {};

    bool operator==(SVertexLayoutDescGL const& Rhs) const
    {
        return std::memcmp(this, &Rhs, sizeof(*this)) == 0;
    }

    bool operator!=(SVertexLayoutDescGL const& Rhs) const
    {
        return !(operator==(Rhs));
    }
};

class AVertexLayoutGL : public ARefCounted
{
public:
    AVertexLayoutGL(SVertexLayoutDescGL const& Desc) :
        Desc(Desc)
    {
        for (SVertexBindingInfo const* binding = Desc.VertexBindings; binding < &Desc.VertexBindings[Desc.NumVertexBindings]; binding++)
        {
            VertexBindingsStrides[binding->InputSlot] = binding->Stride;
        }
    }

    SVertexLayoutDescGL const& GetDesc() const { return Desc; }

    uint32_t const* GetVertexBindingsStrides() const {return VertexBindingsStrides; }

    AVertexArrayObjectGL* GetVAO(AImmediateContextGLImpl* pContext)
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

        AVertexArrayObjectGL* ptr      = vaoHandle.get();
        VaoHandles[pContext->GetUID()] = std::move(vaoHandle);
        return ptr;
    }

    void DestroyVAO(AImmediateContextGLImpl* pContext);

private:
    [[nodiscard]] std::unique_ptr<AVertexArrayObjectGL> CreateVAO();

    std::unordered_map<uint32_t /* context id */, std::unique_ptr<AVertexArrayObjectGL>> VaoHandles;
    std::unique_ptr<AVertexArrayObjectGL>                                                VaoHandleMainContext;
    SVertexLayoutDescGL                                                                  Desc;
    uint32_t                                                                             VertexBindingsStrides[MAX_VERTEX_BUFFER_SLOTS] = {};
};

} // namespace RenderCore
