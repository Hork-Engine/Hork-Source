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

#include "MatInstance.h"

HK_NAMESPACE_BEGIN

class MaterialLibrary final : public IntrusiveRefCounter<MaterialLibrary>
{
public:
    void                    Load(IBinaryStreamReadInterface& stream);

    void                    Clear();

    void                    AddMaterial(StringView name, MatInstanceHandle matInstance);

    void                    RemoveMaterial(StringView name);

    bool                    HasMaterial(StringView name) const;

    MatInstanceHandle       FindMaterial(StringView name);

    Vector<String>          GetMaterialNames() const;

    size_t                  GetMaterialCount() const;

private:
    StringHashMap<MatInstanceHandle> m_Instances;
};

class MaterialManager final : public Noncopyable
{
public:
    void                    Clear();

    void                    AddLibrary(StringView name, IntrusiveRef<MaterialLibrary> library);

    IntrusiveRef<MaterialLibrary>   LoadLibrary(StringView name);
    
    void                    RemoveLibrary(StringView name);

    IntrusiveRef<MaterialLibrary>    GetLibrary(StringView name) const;

    MatInstanceHandle       FindMaterial(StringView name) const;

private:
    StringHashMap<IntrusiveRef<MaterialLibrary>> m_Libraries;
};

HK_NAMESPACE_END
