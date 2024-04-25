#pragma once

#include <Engine/World/Resources/Resource_MaterialInstance.h>

HK_NAMESPACE_BEGIN

class MaterialLibrary final : public RefCounted
{
public:
    ~MaterialLibrary();

    MaterialInstance* CreateMaterial(StringView name);
    void DestroyMaterial(MaterialInstance* material);

    void Read(IBinaryStreamReadInterface& stream, ResourceManager* resManager);
    void Write(IBinaryStreamWriteInterface& stream);

    MaterialInstance* Get(StringView name);

    TStringHashMap<MaterialInstance*> m_Instances;
};

class MaterialManager final
{
public:
    ~MaterialManager();

    void AddMaterialLibrary(MaterialLibrary* library);
    void RemoveMaterialLibrary(MaterialLibrary* library);

    MaterialInstance* Get(StringView name);

private:
    TVector<TRef<MaterialLibrary>> m_Libraries;
};

HK_NAMESPACE_END
