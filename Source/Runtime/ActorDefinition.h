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

#pragma once

#include "Resource.h"

#include <Core/Document.h>

class AActorDefinition : public AResource
{
    AN_CLASS(AActorDefinition, AResource)

public:
    struct SComponentDef
    {
        AClassMeta const*                ClassMeta;
        AString                          Name;
        uint64_t                         Id;
        uint64_t                         Attach;
        int                              ParentIndex{-1};
        THashContainer<AString, AString> AttributeHash;
    };

    struct SPublicAttriubte
    {
        int     ComponentIndex;
        AString AttributeName;
        AString PublicName;
    };

    struct SScriptPublicAttriubte
    {
        AString AttributeName;
        AString PublicName;
    };

    AClassMeta const*         ActorClass{nullptr};
    TStdVector<SComponentDef> Components;
    int                       RootIndex{-1};

    THashContainer<AString, AString> ActorAttributeHash;
    TStdVector<SPublicAttriubte> PublicAttributes;

    AString                          ScriptModule;
    THashContainer<AString, AString> ScriptAttributeHash;

    TStdVector<SScriptPublicAttriubte> ScriptPublicAttributes;

protected:
    AActorDefinition();

    /** Load resource from file */
    bool LoadResource(IBinaryStream& Stream) override;

    /** Create internal resource */
    void LoadInternalResource(const char* _Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/ActorDefinition/Default"; }

private:
    void InitializeFromDocument(ADocument const& Document);
};

