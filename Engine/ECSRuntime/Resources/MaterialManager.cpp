#include "MaterialManager.h"
#include "ResourceManager.h"

#include <Engine/Core/Document.h>
#include <Engine/Core/Parse.h>

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
    String text = stream.AsString();

    DocumentDeserializeInfo deserializeInfo;
    deserializeInfo.pDocumentData = text.CStr();
    deserializeInfo.bInsitu = true;

    Document doc;
    doc.DeserializeFromString(deserializeInfo);

    for (auto* dinstance = doc.GetListOfMembers(); dinstance; dinstance = dinstance->GetNext())
    {
        if (dinstance->IsObject())
        {
            auto* dobject = dinstance->GetArrayValues();

            MaterialInstance* instance = CreateMaterial(dinstance->GetName());
            if (instance)
            {
                auto* dmaterial = dobject->FindMember("Material");

                instance->m_Material = resManager->GetResource<MaterialResource>(dmaterial ? dmaterial->GetStringView() : "/Default/Materials/Unlit");

                auto* dtextures = dobject->FindMember("Textures");
                if (dtextures)
                {
                    int texSlot = 0;
                    for (DocumentValue* v = dtextures->GetArrayValues(); v && texSlot < MAX_MATERIAL_TEXTURES; v = v->GetNext())
                    {
                        instance->m_Textures[texSlot++] = resManager->GetResource<TextureResource>(v->GetStringView());
                    }
                }

                auto* dconstants = dobject->FindMember("Constants");
                if (dconstants)
                {
                    int constantNum = 0;
                    for (DocumentValue* v = dconstants->GetArrayValues(); v && constantNum < MAX_MATERIAL_UNIFORMS; v = v->GetNext())
                    {
                        instance->m_Constants[constantNum++] = Core::ParseFloat(v->GetStringView());
                    }
                }
            }
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
