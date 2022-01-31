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

#include <Platform/BaseTypes.h>

class AWorld;
class ADrawable;
class ASkinnedComponent;
class ADirectionalLightComponent;
class ADebugRenderer;

class ARenderWorld
{
    AN_FORBID_COPY( ARenderWorld )

public:
    ARenderWorld();
    virtual ~ARenderWorld() {}

    /** Get skinned meshes in the world */
    ASkinnedComponent * GetSkinnedMeshes() { return SkinnedMeshList; }

    /** Get all shadow casters in the world */
    ADrawable * GetShadowCasters() { return ShadowCasters; }

    /** Get directional lights in the world */
    ADirectionalLightComponent * GetDirectionalLights() { return DirectionalLightList; }

    void DrawDebug( ADebugRenderer * InRenderer );

private:
    friend class ADrawable;
    void AddShadowCaster( ADrawable * InMesh );
    void RemoveShadowCaster( ADrawable * InMesh );

private:
    friend class ASkinnedComponent;
    void AddSkinnedMesh( ASkinnedComponent * InSkeleton );
    void RemoveSkinnedMesh( ASkinnedComponent * InSkeleton );

private:
    friend class ADirectionalLightComponent;
    void AddDirectionalLight( ADirectionalLightComponent * InLight );
    void RemoveDirectionalLight( ADirectionalLightComponent * InLight );

private:
    ASkinnedComponent * SkinnedMeshList = nullptr;
    ASkinnedComponent * SkinnedMeshListTail = nullptr;
    ADrawable * ShadowCasters = nullptr;
    ADrawable * ShadowCastersTail = nullptr;
    ADirectionalLightComponent * DirectionalLightList = nullptr;
    ADirectionalLightComponent * DirectionalLightListTail = nullptr;
};
