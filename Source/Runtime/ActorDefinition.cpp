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

#include "ActorDefinition.h"
#include "Actor.h"
#include "SceneComponent.h"

HK_CLASS_META(AActorDefinition)

AActorDefinition::AActorDefinition()
{
}

void AActorDefinition::InitializeFromDocument(ADocument const& Document)
{
    THashMap<uint64_t, int> componentIdMap;
    THashSet<AString> publicPropertyNames;

    auto* mActorClassName = Document.FindMember("classname");
    if (mActorClassName)
    {
        AString className = mActorClassName->GetString();
        if (!className.IsEmpty())
        {
            ActorClass = AActorComponent::Factory().LookupClass(className);
            if (!ActorClass)
                LOG("WARNING: Unknown C++ actor class '{}'\n", className);
        }
    }

    if (!ActorClass)
        ActorClass = &AActor::ClassMeta();

    auto* mComponents = Document.FindMember("components");
    if (mComponents)
    {
        for (auto* mComponent = mComponents->GetArrayValues(); mComponent; mComponent = mComponent->GetNext())
        {
            if (!mComponent->IsObject())
                continue;

            auto* mClassName = mComponent->FindMember("classname");
            if (!mClassName)
                continue;

            AString className = mClassName->GetString();
            if (className.IsEmpty())
                continue;

            AClassMeta const* classMeta = AActorComponent::Factory().LookupClass(className);
            if (!classMeta)
                continue;

            SComponentDef componentDef;
            componentDef.ClassMeta = classMeta;

            auto* mComponentName = mComponent->FindMember("name");
            componentDef.Name    = mComponentName ? mComponentName->GetString() : AString("Unnamed");

            auto* mComponentId = mComponent->FindMember("id");
            componentDef.Id    = mComponentId ? Core::ParseUInt64(mComponentId->GetString()) : 0;

            if (classMeta->IsSubclassOf<ASceneComponent>())
            {
                auto* mComponentAttach = mComponent->FindMember("attach");
                componentDef.Attach    = mComponentAttach ? Core::ParseUInt64(mComponentAttach->GetString()) : 0;
            }
            else
            {
                componentDef.Attach = 0;
            }

            auto* mProperties = mComponent->FindMember("properties");
            if (mProperties)
            {
                auto* mPropertyContainer = mProperties->GetArrayValues();
                if (mPropertyContainer)
                {
                    for (auto* mProperty = mPropertyContainer->GetListOfMembers(); mProperty; mProperty = mProperty->GetNext())
                    {
                        auto* mPropertyValue = mProperty->GetArrayValues();
                        if (mPropertyValue)
                        {
                            componentDef.PropertyHash[mProperty->GetName()] = mPropertyValue->GetString();
                        }
                    }
                }
            }

            if (componentDef.Id)
            {
                if (componentIdMap.Count(componentDef.Id) != 0)
                {
                    LOG("WARNING: Found components with same id\n");
                }
                componentIdMap[componentDef.Id] = Components.Size();
            }

            Components.Add(std::move(componentDef));
        }
    }

    auto* mRoot = Document.FindMember("root");
    if (mRoot)
    {
        uint64_t rootId = Core::ParseUInt64(mRoot->GetString());
        if (rootId)
        {
            auto it = componentIdMap.Find(rootId);
            if (it != componentIdMap.End())
            {
                int            index        = it->second;
                SComponentDef& componentDef = Components[index];
                if (componentDef.ClassMeta->IsSubclassOf<ASceneComponent>())
                {
                    RootIndex = index;
                }
                else
                {
                    LOG("WARNING: Root component must be derived from ASceneComponent\n");
                }
            }
            else
            {
                LOG("WARNING: Specified root with unexisted id\n");
            }
        }
    }

    for (SComponentDef& componentDef : Components)
    {
        if (componentDef.Attach)
        {
            auto it = componentIdMap.Find(componentDef.Attach);
            if (it != componentIdMap.End())
            {
                int index = it->second;

                SComponentDef& parentComponentDecl = Components[index];
                if (parentComponentDecl.ClassMeta->IsSubclassOf<ASceneComponent>() && componentDef.Id != componentDef.Attach)
                {
                    // TODO: Check cyclical relationship
                    componentDef.ParentIndex = index;
                }
                else
                {
                    LOG("WARNING: Component can be attached only to other component derived from ASceneComponent\n");
                }
            }
        }
    }

    auto* mProperties = Document.FindMember("properties");
    if (mProperties)
    {
        auto* mPropertyContainer = mProperties->GetArrayValues();
        if (mPropertyContainer)
        {
            for (auto* mProperty = mPropertyContainer->GetListOfMembers(); mProperty; mProperty = mProperty->GetNext())
            {
                auto* mPropertyValue = mProperty->GetArrayValues();
                if (mPropertyValue)
                {
                    ActorPropertyHash[mProperty->GetName()] = mPropertyValue->GetString();
                }
            }
        }
    }

    auto* mPublicProperties = Document.FindMember("public_properties");
    if (mPublicProperties)
    {
        for (auto* mPublicProperty = mPublicProperties->GetArrayValues(); mPublicProperty; mPublicProperty = mPublicProperty->GetNext())
        {
            if (!mPublicProperty->IsObject())
                continue;

            auto* mProperty = mPublicProperty->FindMember("property");
            if (!mProperty)
                continue;

            AString propertyName = mProperty->GetString();
            if (propertyName.IsEmpty())
                continue;

            auto* mPublicName = mPublicProperty->FindMember("public_name");
            if (!mPublicName)
                continue;

            AString publicName = mPublicName->GetString();
            if (publicName.IsEmpty())
                continue;

            if (publicPropertyNames.Count(publicName))
            {
                LOG("WARNING: Unique public names expected\n");
                continue;
            }

            int componentIndex = -1;

            auto* mComponentId = mPublicProperty->FindMember("component_id");
            if (mComponentId)
            {
                uint64_t componentId = Core::ParseUInt64(mComponentId->GetString());
                if (!componentId)
                    continue;

                auto it = componentIdMap.Find(componentId);
                if (it == componentIdMap.End())
                    continue;

                componentIndex = it->second;
            }            

            publicPropertyNames.Insert(publicName);

            SPublicProperty publicProperty;
            publicProperty.ComponentIndex = componentIndex;
            publicProperty.PropertyName   = std::move(propertyName);
            publicProperty.PublicName     = std::move(publicName);
            PublicProperties.Add(std::move(publicProperty));
        }
    }

    auto* mScript = Document.FindMember("script");
    if (mScript && mScript->IsObject())
    {
        auto mScriptObj = mScript->GetArrayValues();
        if (mScriptObj)
        {
            auto* mModule = mScriptObj->FindMember("module");

            ScriptModule = mModule ? mModule->GetString() : "";

            mProperties = mScriptObj->FindMember("properties");
            if (mProperties)
            {
                auto* mPropertyContainer = mProperties->GetArrayValues();
                if (mPropertyContainer)
                {
                    for (auto* mProperty = mPropertyContainer->GetListOfMembers(); mProperty; mProperty = mProperty->GetNext())
                    {
                        auto* mPropertyValue = mProperty->GetArrayValues();
                        if (mPropertyValue)
                        {
                            ScriptPropertyHash[mProperty->GetName()] = mPropertyValue->GetString();
                        }
                    }
                }
            }

            auto* mScriptPublicProperties = mScriptObj->FindMember("public_properties");
            if (mScriptPublicProperties)
            {
                for (auto* mPublicProperty = mScriptPublicProperties->GetArrayValues(); mPublicProperty; mPublicProperty = mPublicProperty->GetNext())
                {
                    if (!mPublicProperty->IsObject())
                        continue;

                    auto* mProperty = mPublicProperty->FindMember("property");
                    if (!mProperty)
                        continue;

                    AString propertyName = mProperty->GetString();
                    if (propertyName.IsEmpty())
                        continue;

                    auto* mPublicName = mPublicProperty->FindMember("public_name");
                    if (!mPublicName)
                        continue;

                    AString publicName = mPublicName->GetString();
                    if (publicName.IsEmpty())
                        continue;

                    if (publicPropertyNames.Count(publicName))
                    {
                        LOG("WARNING: Unique public names expected\n");
                        continue;
                    }

                    publicPropertyNames.Insert(publicName);

                    SScriptPublicProperty publicProperty;
                    publicProperty.PropertyName = std::move(propertyName);
                    publicProperty.PublicName   = std::move(publicName);
                    ScriptPublicProperties.Add(std::move(publicProperty));
                }
            }
        }
    }
}

bool AActorDefinition::LoadResource(IBinaryStreamReadInterface& Stream)
{
    AString actorDefScript = Stream.AsString();

    SDocumentDeserializeInfo deserializeInfo;
    deserializeInfo.bInsitu       = true;
    deserializeInfo.pDocumentData = actorDefScript.CStr();

    ADocument document;
    document.DeserializeFromString(deserializeInfo);

    InitializeFromDocument(document);

    return true;
}

void AActorDefinition::LoadInternalResource(AStringView _Path)
{
    // Empty resource
}
