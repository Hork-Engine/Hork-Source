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

#include "MaterialManager.h"

#include <Engine/Core/DOM.h>
#include <Engine/World/Resources/ResourceManager.h>
#include <Engine/GameApplication/GameApplication.h>

HK_NAMESPACE_BEGIN

Ref<Material> MaterialLibrary::CreateMaterial(StringView name)
{
    if (name.IsEmpty())
    {
        LOG("MaterialLibrary::CreateMaterial: invalid name\n");
        return {};
    }

    if (TryGet(name))
    {
        LOG("MaterialLibrary::CreateMaterial: material {} already exists\n", name);
        return {};
    }

    // TODO: Use pool allocator
    Ref<Material> instance = MakeRef<Material>(name);
    m_Instances[name] = instance;
    return instance;
}

void MaterialLibrary::DestroyMaterial(Material* material)
{
    if (!material)
        return;

    auto it = m_Instances.Find(material->GetName());
    if (it == m_Instances.End())
        return;

    m_Instances.Erase(it);
}

void MaterialLibrary::Read(IBinaryStreamReadInterface& stream)
{
    ResourceManager& resourceMngr = GameApplication::GetResourceManager();

    DOM::Object document = DOM::Parser().Parse(stream.AsString());
    DOM::ObjectView documentView = document;

    for (auto dmember : DOM::MemberConstIterator(documentView))
    {
        auto materialName = dmember->GetName();

        DOM::ObjectView dinstance = dmember->GetObject();
        if (!dinstance.IsStructure())
            continue;

        Material* instance = CreateMaterial(materialName.GetStringView());
        if (!instance)
            continue;

        auto resource = dinstance["Material"].AsString();
        instance->SetResource(resourceMngr.GetResource<MaterialResource>(!resource.IsEmpty() ? resource : "/Default/Materials/Unlit"));

        auto dtextures = dinstance["Textures"];
        uint32_t textureCount = Math::Min<uint32_t>(MAX_MATERIAL_TEXTURES, dtextures.GetArraySize());
        for (uint32_t slot = 0; slot < textureCount; ++slot)
            instance->SetTexture(slot, resourceMngr.GetResource<TextureResource>(dtextures.At(slot).AsString()));

        auto dconstants = dinstance["Constants"];
        uint32_t constantCount = Math::Min<uint32_t>(MAX_MATERIAL_UNIFORMS, dconstants.GetArraySize());
        for (uint32_t index = 0; index < constantCount; ++index)
            instance->SetConstant(index, dconstants.At(index).As<float>());
    }
}

void MaterialLibrary::Write(IBinaryStreamWriteInterface& stream)
{
    // TODO
}

Ref<Material> MaterialLibrary::TryGet(StringView name)
{
    auto it = m_Instances.Find(name);
    if (it == m_Instances.End())
        return {};
    return it->second;
}

Ref<MaterialLibrary> MaterialManager::CreateLibrary()
{
    Ref<MaterialLibrary> library;
    library.Attach(new MaterialLibrary);
    m_Libraries.Add(library);
    return library;
}

Ref<MaterialLibrary> MaterialManager::LoadLibrary(StringView fileName)
{
    auto& resourceMngr = GameApplication::GetResourceManager();
    if (auto file = resourceMngr.OpenFile(fileName))
    {
        auto library = CreateLibrary();
        library->Read(file);
        return library;
    }
    return {};
}

void MaterialManager::RemoveLibrary(MaterialLibrary* library)
{
    auto i = m_Libraries.IndexOf(library, [](auto& a, MaterialLibrary* b) { return a.RawPtr() == b; } );
    if (i != Core::NPOS)
        m_Libraries.Remove(i);
}

Ref<Material> MaterialManager::TryGet(StringView name)
{
    for (auto& library : m_Libraries)
    {
        if (auto instance = library->TryGet(name))
            return instance;
    }
    return {};
}

HK_NAMESPACE_END
