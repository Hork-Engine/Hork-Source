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
#include <Engine/World/Resources/ResourceManager.h>

#include <Engine/Core/DOM.h>

HK_NAMESPACE_BEGIN

MaterialLibrary::~MaterialLibrary()
{
    for (auto& pair : m_Instances)
    {
        delete pair.second;
    }
}

MaterialInstance* MaterialLibrary::CreateMaterial(StringView name)
{
    if (name.IsEmpty())
    {
        LOG("MaterialLibrary::CreateMaterial: invalid name\n");
        return nullptr;
    }

    if (Get(name))
    {
        LOG("MaterialLibrary::CreateMaterial: material {} already exists\n", name);
        return nullptr;
    }

    // TODO: Use pool allocator
    MaterialInstance* instance = new MaterialInstance(name);

    m_Instances[name] = instance;
    return instance;
}

void MaterialLibrary::DestroyMaterial(MaterialInstance* material)
{
    auto it = m_Instances.Find(material->m_Name);
    if (it == m_Instances.End())
        return;

    m_Instances.Erase(it);
    delete material;
}

void MaterialLibrary::Read(IBinaryStreamReadInterface& stream, ResourceManager* resManager)
{
    DOM::Object document = DOM::Parser().Parse(stream.AsString());
    DOM::ObjectView documentView = document;

    for (auto dmember : DOM::MemberConstIterator(documentView))
    {
        auto materialName = dmember->GetName();

        DOM::ObjectView dinstance = dmember->GetObject();
        if (!dinstance.IsStructure())
            continue;

        MaterialInstance* instance = CreateMaterial(materialName.GetStringView());
        if (!instance)
            continue;

        auto material = dinstance["Material"].AsString();
        instance->m_Material = resManager->GetResource<MaterialResource>(!material.IsEmpty() ? material : "/Default/Materials/Unlit");

        auto dtextures = dinstance["Textures"];
        int textureCount = Math::Min<int>(MAX_MATERIAL_TEXTURES, dtextures.GetArraySize());
        for (int i = 0; i < textureCount; i++)
        {
            instance->m_Textures[i] = resManager->GetResource<TextureResource>(dtextures.At(i).AsString());
        }

        auto dconstants = dinstance["Constants"];
        int constantCount = Math::Min<int>(MAX_MATERIAL_UNIFORMS, dconstants.GetArraySize());
        for (int i = 0; i < constantCount; i++)
        {
            instance->m_Constants[i] = dconstants.At(i).As<float>();
        }
    }
}

void MaterialLibrary::Write(IBinaryStreamWriteInterface& stream)
{
    // TODO
}

MaterialInstance* MaterialLibrary::Get(StringView name)
{
    auto it = m_Instances.Find(name);
    if (it == m_Instances.End())
        return nullptr;
    return it->second;
}

MaterialManager::~MaterialManager()
{
}

void MaterialManager::AddMaterialLibrary(MaterialLibrary* library)
{
    m_Libraries.Add() = library;
}

void MaterialManager::RemoveMaterialLibrary(MaterialLibrary* library)
{
    auto i = m_Libraries.IndexOf(Ref<MaterialLibrary>(library));
    if (i != Core::NPOS)
        m_Libraries.Remove(i);
}

MaterialInstance* MaterialManager::Get(StringView name)
{
    for (MaterialLibrary* library : m_Libraries)
    {
        MaterialInstance* instance = library->Get(name);
        if (instance)
            return instance;
    }
    return nullptr;
}

HK_NAMESPACE_END
