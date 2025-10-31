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

#include "MaterialManager.h"

#include <Hork/Core/DOM.h>
#include <Hork/Runtime/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

void MaterialLibrary::Load(IBinaryStreamReadInterface& stream)
{
    Clear();

    ResourceManager& resourceMngr = GameApplication::sGetResourceManager();

    DOM::Object document = DOM::Parser().Parse(stream.AsString());
    DOM::ObjectView documentView = document;

    for (auto dmember : DOM::MemberConstIterator(documentView))
    {
        auto materialName = dmember->GetName();

        DOM::ObjectView dinstance = dmember->GetObject();
        if (!dinstance.IsStructure())
            continue;

        MatInstanceRef matInstance(new MatInstance);

        auto resource = dinstance["Material"].AsString();
        if (!resource.IsEmpty())
        {
            if (resource[0] == '/') // path to file
                matInstance->SetResource(resourceMngr.Load<Material>(resource));
            else // procedural
                matInstance->SetResource(resourceMngr.Acquire<Material>(resource));
        }

        auto dtextures = dinstance["Textures"];
        uint32_t textureCount = Math::Min<uint32_t>(MAX_MATERIAL_TEXTURES, dtextures.GetArraySize());
        for (uint32_t slot = 0; slot < textureCount; ++slot)
        {
            resource = dtextures.At(slot).AsString();
            if (!resource.IsEmpty())
            {
                if (resource[0] == '/') // path to file
                    matInstance->SetTexture(slot, resourceMngr.Load<Texture>(resource));
                else
                    matInstance->SetTexture(slot, resourceMngr.Acquire<Texture>(resource));
            }
        }

        auto dconstants = dinstance["Constants"];
        uint32_t constantCount = Math::Min<uint32_t>(MAX_MATERIAL_UNIFORMS, dconstants.GetArraySize());
        for (uint32_t index = 0; index < constantCount; ++index)
            matInstance->SetConstant(index, dconstants.At(index).As<float>());

        AddMaterial(materialName.GetStringView(), std::move(matInstance));
    }
}

void MaterialLibrary::Clear()
{
    m_Instances.Clear();
}

void MaterialLibrary::AddMaterial(StringView name, MatInstanceRef matInstance)
{
    m_Instances[name] = std::move(matInstance);
}

void MaterialLibrary::RemoveMaterial(StringView name)
{
    m_Instances.Erase(name);
}

bool MaterialLibrary::HasMaterial(StringView name) const
{
    return m_Instances.Find(name) != m_Instances.End();
}

MatInstanceRef MaterialLibrary::FindMaterial(StringView name)
{
    auto it = m_Instances.Find(name);
    return (it != m_Instances.End()) ? it->second : nullptr;
}

Vector<String> MaterialLibrary::GetMaterialNames() const
{
    Vector<String> names;
    names.Reserve(m_Instances.Size());
    for (auto& it : m_Instances)
        names.EmplaceBack(it.first);
    return names;
}

size_t MaterialLibrary::GetMaterialCount() const
{
    return m_Instances.Size();
}

void MaterialManager::Clear()
{
    m_Libraries.Clear();
}

void MaterialManager::AddLibrary(StringView name, IntrusiveRef<MaterialLibrary> library)
{
    m_Libraries[name] = std::move(library);
}

IntrusiveRef<MaterialLibrary> MaterialManager::LoadLibrary(StringView name)
{
    auto& resourceMngr = GameApplication::sGetResourceManager();
    if (auto file = resourceMngr.OpenFile(name))
    {
        auto& library = m_Libraries[name];
        if (!library)
            library.Reset(new MaterialLibrary);
        library->Load(file);
        return library;
    }
    return nullptr;
}

IntrusiveRef<MaterialLibrary> MaterialManager::GetLibrary(StringView name) const
{
    auto it = m_Libraries.Find(name);
    return it != m_Libraries.End() ? it->second : nullptr;
}

void MaterialManager::RemoveLibrary(StringView name)
{
    m_Libraries.Erase(name);
}

MatInstanceRef MaterialManager::FindMaterial(StringView name) const
{
    for (auto& pair : m_Libraries)
    {
        if (auto instance = pair.second->FindMaterial(name))
            return instance;
    }
    return nullptr;
}

HK_NAMESPACE_END
