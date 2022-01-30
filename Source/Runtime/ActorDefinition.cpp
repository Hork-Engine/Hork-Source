/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include <Containers/StdHash.h>

AN_CLASS_META(AActorDefinition)

AActorDefinition::AActorDefinition()
{
}

void AActorDefinition::InitializeFromDocument(ADocument const& Document)
{
    TStdHashMap<uint64_t, int> componentIdMap;
    TStdHashSet<AString> publicAttribNames;

    auto* mActorClassName = Document.FindMember("classname");
    if (mActorClassName)
    {
        AString className = mActorClassName->GetString();
        if (!className.IsEmpty())
        {
            ActorClass = AActorComponent::Factory().LookupClass(className.CStr());
            if (!ActorClass)
                GLogger.Printf("WARNING: Unknown C++ actor class '%s'\n", className.CStr());
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

            AClassMeta const* classMeta = AActorComponent::Factory().LookupClass(className.CStr());
            if (!classMeta)
                continue;

            SComponentDef componentDef;
            componentDef.ClassMeta = classMeta;

            auto* mComponentName = mComponent->FindMember("name");
            componentDef.Name    = mComponentName ? mComponentName->GetString() : AString("Unnamed");

            auto* mComponentId = mComponent->FindMember("id");
            componentDef.Id    = mComponentId ? Math::ToInt<uint64_t>(mComponentId->GetString()) : 0;

            if (classMeta->IsSubclassOf<ASceneComponent>())
            {
                auto* mComponentAttach = mComponent->FindMember("attach");
                componentDef.Attach    = mComponentAttach ? Math::ToInt<uint64_t>(mComponentAttach->GetString()) : 0;
            }
            else
            {
                componentDef.Attach = 0;
            }

            auto* mAttributes = mComponent->FindMember("attributes");
            if (mAttributes)
            {
                auto* mAttribContainer = mAttributes->GetArrayValues();
                if (mAttribContainer)
                {
                    for (auto* mAttribute = mAttribContainer->GetListOfMembers(); mAttribute; mAttribute = mAttribute->GetNext())
                    {
                        auto* mAttribValue = mAttribute->GetArrayValues();
                        if (mAttribValue)
                        {
                            componentDef.AttributeHash.Insert(mAttribute->GetName(), mAttribValue->GetString());
                        }
                    }
                }
            }

            if (componentDef.Id)
            {
                if (componentIdMap.count(componentDef.Id) != 0)
                {
                    GLogger.Printf("WARNING: Found components with same id\n");
                }
                componentIdMap[componentDef.Id] = Components.Size();
            }

            Components.push_back(std::move(componentDef));
        }
    }

    auto* mRoot = Document.FindMember("root");
    if (mRoot)
    {
        uint64_t rootId = Math::ToInt<uint64_t>(mRoot->GetString());
        if (rootId)
        {
            auto it = componentIdMap.find(rootId);
            if (it != componentIdMap.end())
            {
                int            index        = it->second;
                SComponentDef& componentDef = Components[index];
                if (componentDef.ClassMeta->IsSubclassOf<ASceneComponent>())
                {
                    RootIndex = index;
                }
                else
                {
                    GLogger.Printf("WARNING: Root component must be derived from ASceneComponent\n");
                }
            }
            else
            {
                GLogger.Printf("WARNING: Specified root with unexisted id\n");
            }
        }
    }

    for (SComponentDef& componentDef : Components)
    {
        if (componentDef.Attach)
        {
            auto it = componentIdMap.find(componentDef.Attach);
            if (it != componentIdMap.end())
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
                    GLogger.Printf("WARNING: Component can be attached only to other component derived from ASceneComponent\n");
                }
            }
        }
    }

    auto* mAttributes = Document.FindMember("attributes");
    if (mAttributes)
    {
        auto* mAttribContainer = mAttributes->GetArrayValues();
        if (mAttribContainer)
        {
            for (auto* mAttribute = mAttribContainer->GetListOfMembers(); mAttribute; mAttribute = mAttribute->GetNext())
            {
                auto* mAttribValue = mAttribute->GetArrayValues();
                if (mAttribValue)
                {
                    ActorAttributeHash.Insert(mAttribute->GetName(), mAttribValue->GetString());
                }
            }
        }
    }

    auto* mPublicAttribs = Document.FindMember("public_attributes");
    if (mPublicAttribs)
    {
        for (auto* mPublicAttrib = mPublicAttribs->GetArrayValues(); mPublicAttrib; mPublicAttrib = mPublicAttrib->GetNext())
        {
            if (!mPublicAttrib->IsObject())
                continue;

            auto* mAttribute = mPublicAttrib->FindMember("attribute");
            if (!mAttribute)
                continue;

            AString attributeName = mAttribute->GetString();
            if (attributeName.IsEmpty())
                continue;

            auto* mPublicName = mPublicAttrib->FindMember("public_name");
            if (!mPublicName)
                continue;

            AString publicName = mPublicName->GetString();
            if (publicName.IsEmpty())
                continue;

            if (publicAttribNames.count(publicName))
            {
                GLogger.Printf("WARNING: Unique public names expected\n");
                continue;
            }

            int componentIndex = -1;

            auto* mComponentId = mPublicAttrib->FindMember("component_id");
            if (mComponentId)
            {
                uint64_t componentId = Math::ToInt<uint64_t>(mComponentId->GetString());
                if (!componentId)
                    continue;

                auto it = componentIdMap.find(componentId);
                if (it == componentIdMap.end())
                    continue;

                componentIndex = it->second;
            }            

            publicAttribNames.insert(publicName);

            SPublicAttriubte publicAttrib;
            publicAttrib.ComponentIndex = componentIndex;
            publicAttrib.AttributeName  = std::move(attributeName);
            publicAttrib.PublicName     = std::move(publicName);
            PublicAttributes.push_back(std::move(publicAttrib));
        }
    }

    auto* mScript = Document.FindMember("script");
    if (mScript && mScript->IsObject())
    {
        auto mScriptObj = mScript->GetArrayValues();
        if (mScriptObj)
        {
            auto* mModule = mScriptObj->FindMember("module");

            ScriptModule = mModule ? mModule->GetString() : AString::NullString();

            mAttributes = mScriptObj->FindMember("attributes");
            if (mAttributes)
            {
                auto* mAttribContainer = mAttributes->GetArrayValues();
                if (mAttribContainer)
                {
                    for (auto* mAttribute = mAttribContainer->GetListOfMembers(); mAttribute; mAttribute = mAttribute->GetNext())
                    {
                        auto* mAttribValue = mAttribute->GetArrayValues();
                        if (mAttribValue)
                        {
                            ScriptAttributeHash.Insert(mAttribute->GetName(), mAttribValue->GetString());
                        }
                    }
                }
            }

            auto* mScriptPublicAttribs = mScriptObj->FindMember("public_attributes");
            if (mScriptPublicAttribs)
            {
                for (auto* mPublicAttrib = mScriptPublicAttribs->GetArrayValues(); mPublicAttrib; mPublicAttrib = mPublicAttrib->GetNext())
                {
                    if (!mPublicAttrib->IsObject())
                        continue;

                    auto* mAttribute = mPublicAttrib->FindMember("attribute");
                    if (!mAttribute)
                        continue;

                    AString attributeName = mAttribute->GetString();
                    if (attributeName.IsEmpty())
                        continue;

                    auto* mPublicName = mPublicAttrib->FindMember("public_name");
                    if (!mPublicName)
                        continue;

                    AString publicName = mPublicName->GetString();
                    if (publicName.IsEmpty())
                        continue;

                    if (publicAttribNames.count(publicName))
                    {
                        GLogger.Printf("WARNING: Unique public names expected\n");
                        continue;
                    }

                    publicAttribNames.insert(publicName);

                    SScriptPublicAttriubte publicAttrib;
                    publicAttrib.AttributeName = std::move(attributeName);
                    publicAttrib.PublicName    = std::move(publicName);
                    ScriptPublicAttributes.push_back(std::move(publicAttrib));
                }
            }
        }
    }
}

bool AActorDefinition::LoadResource(IBinaryStream& Stream)
{
    AString actorDefScript;
    actorDefScript.FromFile(Stream);

    SDocumentDeserializeInfo deserializeInfo;
    deserializeInfo.bInsitu       = true;
    deserializeInfo.pDocumentData = actorDefScript.CStr();

    ADocument document;
    document.DeserializeFromString(deserializeInfo);

    InitializeFromDocument(document);

    return true;
}

void AActorDefinition::LoadInternalResource(const char* _Path)
{
    // Empty resource
}
