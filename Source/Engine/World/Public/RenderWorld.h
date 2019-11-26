/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#pragma once

#include <Core/Public/BaseTypes.h>

class AWorld;
class ADrawable;
class AMeshComponent;
class ASkinnedComponent;
class ADirectionalLightComponent;
class APointLightComponent;
class ASpotLightComponent;
class ADebugRenderer;
struct SRenderFrontendDef;

class ARenderWorld {
    AN_FORBID_COPY( ARenderWorld )

public:
    explicit ARenderWorld( AWorld * InOwnerWorld );
    ~ARenderWorld() {}

    void AddDrawable( ADrawable * _Drawable );
    void RemoveDrawable( ADrawable * _Drawable );

    void AddMesh( AMeshComponent * _Mesh );
    void RemoveMesh( AMeshComponent * _Mesh );

    void AddSkinnedMesh( ASkinnedComponent * _Skeleton );
    void RemoveSkinnedMesh( ASkinnedComponent * _Skeleton );

    void AddShadowCaster( AMeshComponent * _Mesh );
    void RemoveShadowCaster( AMeshComponent * _Mesh );

    void AddDirectionalLight( ADirectionalLightComponent * _Light );
    void RemoveDirectionalLight( ADirectionalLightComponent * _Light );

    void AddPointLight( APointLightComponent * _Light );
    void RemovePointLight( APointLightComponent * _Light );

    void AddSpotLight( ASpotLightComponent * _Light );
    void RemoveSpotLight( ASpotLightComponent * _Light );

    /** Get all drawables in the world */
    ADrawable * GetDrawables() { return DrawableList; }

    /** Get all drawables in the world */
    ADrawable * GetDrawables() const { return DrawableList; }

    /** Get static and skinned meshes in the world */
    AMeshComponent * GetMeshes() { return MeshList; }

    /** Get static and skinned meshes in the world */
    AMeshComponent * GetMeshes() const { return MeshList; }

    /** Get skinned meshes in the world */
    ASkinnedComponent * GetSkinnedMeshes() { return SkinnedMeshList; }

    /** Get all shadow casters in the world */
    AMeshComponent * GetShadowCasters() { return ShadowCasters; }

    /** Get directional lights in the world */
    ADirectionalLightComponent * GetDirectionalLights() { return DirectionalLightList; }

    /** Get point lights in the world */
    APointLightComponent * GetPointLights() { return PointLightList; }

    /** Get spot lights in the world */
    ASpotLightComponent * GetSpotLights() { return SpotLightList; }

    void RenderFrontend_AddInstances( SRenderFrontendDef * _Def );
    void RenderFrontend_AddDirectionalShadowmapInstances( SRenderFrontendDef * _Def );

    void DrawDebug( ADebugRenderer * InRenderer );

private:
    AWorld * pOwnerWorld;

    ADrawable * DrawableList;
    ADrawable * DrawableListTail;
    AMeshComponent * MeshList;
    AMeshComponent * MeshListTail;
    ASkinnedComponent * SkinnedMeshList;
    ASkinnedComponent * SkinnedMeshListTail;
    AMeshComponent * ShadowCasters;
    AMeshComponent * ShadowCastersTail;
    ADirectionalLightComponent * DirectionalLightList;
    ADirectionalLightComponent * DirectionalLightListTail;
    APointLightComponent * PointLightList;
    APointLightComponent * PointLightListTail;
    ASpotLightComponent * SpotLightList;
    ASpotLightComponent * SpotLightListTail;
};
