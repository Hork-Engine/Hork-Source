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

#pragma once

#include "Resource.h"

#include <Hork/Core/IntrusiveRef.h>
#include <Hork/Core/UniqueRef.h>
#include <Hork/MaterialGraph/MaterialBinary.h>

HK_NAMESPACE_BEGIN

class Material : public Resource
{
public:
    using DataType = MaterialBinary;

    static const uint8_t        Type = 5;
    static const uint8_t        Version = 2;

                                Material() = default;

    static UniqueRef<MaterialBinary> BeginAsyncLoad(IBinaryStreamReadInterface& stream);

    void                        InitFromData(UniqueRef<MaterialBinary> data);

    void                        Load(IBinaryStreamReadInterface& stream) override;

    void                        Write(IBinaryStreamWriteInterface& stream);

    void                        Purge();

    bool                        IsCastShadow() const;
    bool                        IsTranslucent() const;

    RENDERING_PRIORITY          GetRenderingPriority() const;

    uint32_t                    GetTextureCount() const;
    uint32_t                    GetUniformVectorCount() const;

    MaterialGPU*                GetGpuMaterial() { return m_GpuMaterial.RawPtr(); }
   

private:
    IntrusiveRef<MaterialGPU>   m_GpuMaterial;
    UniqueRef<MaterialBinary>   m_Binary;
};

using MaterialRef = IntrusiveRef<Material>;

HK_NAMESPACE_END
