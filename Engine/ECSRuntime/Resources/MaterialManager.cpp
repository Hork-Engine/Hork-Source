#include "MaterialManager.h"
#include "ResourceManager.h"

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
    auto i = m_Libraries.IndexOf(TRef<MaterialLibrary>(library));
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
