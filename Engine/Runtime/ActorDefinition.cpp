/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "World/SceneComponent.h"
#include "World/Actor.h"

#include <Engine/Core/Parse.h>

HK_NAMESPACE_BEGIN

HK_CLASS_META(ActorDefinition)

ActorDefinition::ActorDefinition()
{
}

void ActorDefinition::InitializeFromDocument(Document const& Document)
{
    THashMap<uint64_t, int> componentIdMap;
    THashSet<String> publicPropertyNames;

    auto* mActorClassName = Document.FindMember("classname");
    if (mActorClassName)
    {
        auto className = mActorClassName->GetStringView();
        if (!className.IsEmpty())
        {
            m_ActorClass = ActorComponent::Factory().LookupClass(className);
            if (!m_ActorClass)
                LOG("WARNING: Unknown C++ actor class '{}'\n", className);
        }
    }

    if (!m_ActorClass)
        m_ActorClass = &Actor::GetClassMeta();

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

            auto className = mClassName->GetStringView();
            if (className.IsEmpty())
                continue;

            ClassMeta const* classMeta = ActorComponent::Factory().LookupClass(className);
            if (!classMeta)
                continue;

            ComponentDef componentDef;
            componentDef.ComponentClass = classMeta;

            componentDef.Name = mComponent->GetString("name", "Unnamed");
            componentDef.Id   = mComponent->GetUInt64("id");

            if (classMeta->IsSubclassOf<SceneComponent>())
            {
                componentDef.Attach = mComponent->GetUInt64("attach");
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
                            componentDef.PropertyHash[mProperty->GetName()] = mPropertyValue->GetStringView();
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
                componentIdMap[componentDef.Id] = m_Components.Size();
            }

            m_Components.Add(std::move(componentDef));
        }
    }

    uint64_t rootId = Document.GetUInt64("root");
    if (rootId)
    {
        auto it = componentIdMap.Find(rootId);
        if (it != componentIdMap.End())
        {
            int            index        = it->second;
            ComponentDef& componentDef = m_Components[index];
            if (componentDef.ComponentClass->IsSubclassOf<SceneComponent>())
            {
                m_RootIndex = index;
            }
            else
            {
                LOG("WARNING: Root component must be derived from SceneComponent\n");
            }
        }
        else
        {
            LOG("WARNING: Specified root with unexisted id\n");
        }
    }

    for (ComponentDef& componentDef : m_Components)
    {
        if (componentDef.Attach)
        {
            auto it = componentIdMap.Find(componentDef.Attach);
            if (it != componentIdMap.End())
            {
                int index = it->second;

                ComponentDef& parentComponentDecl = m_Components[index];
                if (parentComponentDecl.ComponentClass->IsSubclassOf<SceneComponent>() && componentDef.Id != componentDef.Attach)
                {
                    // TODO: Check cyclical relationship
                    componentDef.ParentIndex = index;
                }
                else
                {
                    LOG("WARNING: Component can be attached only to other component derived from SceneComponent\n");
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
                    m_ActorPropertyHash[mProperty->GetName()] = mPropertyValue->GetStringView();
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

            auto propertyName = mProperty->GetStringView();
            if (propertyName.IsEmpty())
                continue;

            auto* mPublicName = mPublicProperty->FindMember("public_name");
            if (!mPublicName)
                continue;

            auto publicName = mPublicName->GetStringView();
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
                uint64_t componentId = Core::ParseUInt64(mComponentId->GetStringView());
                if (!componentId)
                    continue;

                auto it = componentIdMap.Find(componentId);
                if (it == componentIdMap.End())
                    continue;

                componentIndex = it->second;
            }            

            publicPropertyNames.Insert(publicName);

            PublicProperty publicProperty;
            publicProperty.ComponentIndex = componentIndex;
            publicProperty.PropertyName   = std::move(propertyName);
            publicProperty.PublicName     = std::move(publicName);
            m_PublicProperties.Add(std::move(publicProperty));
        }
    }

    auto* mScript = Document.FindMember("script");
    if (mScript && mScript->IsObject())
    {
        auto mScriptObj = mScript->GetArrayValues();
        if (mScriptObj)
        {
            auto* mModule = mScriptObj->FindMember("module");

            m_ScriptModule = mModule ? mModule->GetStringView() : "";

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
                            m_ScriptPropertyHash[mProperty->GetName()] = mPropertyValue->GetStringView();
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

                    auto propertyName = mProperty->GetStringView();
                    if (propertyName.IsEmpty())
                        continue;

                    auto* mPublicName = mPublicProperty->FindMember("public_name");
                    if (!mPublicName)
                        continue;

                    auto publicName = mPublicName->GetStringView();
                    if (publicName.IsEmpty())
                        continue;

                    if (publicPropertyNames.Count(publicName))
                    {
                        LOG("WARNING: Unique public names expected\n");
                        continue;
                    }

                    publicPropertyNames.Insert(publicName);

                    ScriptPublicProperty publicProperty;
                    publicProperty.PropertyName = std::move(propertyName);
                    publicProperty.PublicName   = std::move(publicName);
                    m_ScriptPublicProperties.Add(std::move(publicProperty));
                }
            }
        }
    }
}

bool ActorDefinition::LoadResource(IBinaryStreamReadInterface& Stream)
{
    String actorDefScript = Stream.AsString();

    DocumentDeserializeInfo deserializeInfo;
    deserializeInfo.bInsitu       = true;
    deserializeInfo.pDocumentData = actorDefScript.CStr();

    Document document;
    document.DeserializeFromString(deserializeInfo);

    InitializeFromDocument(document);

    return true;
}

void ActorDefinition::LoadInternalResource(StringView _Path)
{
    // Empty resource
}

HK_NAMESPACE_END
