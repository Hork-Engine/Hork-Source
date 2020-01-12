/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <World/Public/Render/RenderWorld.h>
#include <World/Public/World.h>
#include <World/Public/Components/SkinnedComponent.h>
#include <World/Public/Components/DirectionalLightComponent.h>
#include <World/Public/Components/PointLightComponent.h>
#include <World/Public/Components/SpotLightComponent.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>
#include <Runtime/Public/Runtime.h>
#include "LightVoxelizer.h"

ARuntimeVariable RVDrawFrustumClusters( _CTS( "DrawFrustumClusters" ), _CTS( "0" ), VAR_CHEAT );

ARenderWorld::ARenderWorld( AWorld * InOwnerWorld )
    : pOwnerWorld( InOwnerWorld )
{
}

void ARenderWorld::AddDrawable( ADrawable * _Drawable ) {
    INTRUSIVE_ADD_UNIQUE( _Drawable, Next, Prev, DrawableList, DrawableListTail );
}

void ARenderWorld::RemoveDrawable( ADrawable * _Drawable ) {
    INTRUSIVE_REMOVE( _Drawable, Next, Prev, DrawableList, DrawableListTail );
}

void ARenderWorld::AddMesh( AMeshComponent * _Mesh ) {
    INTRUSIVE_ADD_UNIQUE( _Mesh, Next, Prev, MeshList, MeshListTail );
}

void ARenderWorld::RemoveMesh( AMeshComponent * _Mesh ) {
    INTRUSIVE_REMOVE( _Mesh, Next, Prev, MeshList, MeshListTail );
}

void ARenderWorld::AddSkinnedMesh( ASkinnedComponent * _Skeleton ) {
    INTRUSIVE_ADD_UNIQUE( _Skeleton, Next, Prev, SkinnedMeshList, SkinnedMeshListTail );
}

void ARenderWorld::RemoveSkinnedMesh( ASkinnedComponent * _Skeleton ) {
    INTRUSIVE_REMOVE( _Skeleton, Next, Prev, SkinnedMeshList, SkinnedMeshListTail );
}

void ARenderWorld::AddShadowCaster( AMeshComponent * _Mesh ) {
    INTRUSIVE_ADD_UNIQUE( _Mesh, NextShadowCaster, PrevShadowCaster, ShadowCasters, ShadowCastersTail );
}

void ARenderWorld::RemoveShadowCaster( AMeshComponent * _Mesh ) {
    INTRUSIVE_REMOVE( _Mesh, NextShadowCaster, PrevShadowCaster, ShadowCasters, ShadowCastersTail );
}

void ARenderWorld::AddDirectionalLight( ADirectionalLightComponent * _Light ) {
    INTRUSIVE_ADD_UNIQUE( _Light, Next, Prev, DirectionalLightList, DirectionalLightListTail );
}

void ARenderWorld::RemoveDirectionalLight( ADirectionalLightComponent * _Light ) {
    INTRUSIVE_REMOVE( _Light, Next, Prev, DirectionalLightList, DirectionalLightListTail );
}

void ARenderWorld::AddPointLight( APointLightComponent * _Light ) {
    INTRUSIVE_ADD_UNIQUE( _Light, Next, Prev, PointLightList, PointLightListTail );
}

void ARenderWorld::RemovePointLight( APointLightComponent * _Light ) {
    INTRUSIVE_REMOVE( _Light, Next, Prev, PointLightList, PointLightListTail );
}

void ARenderWorld::AddSpotLight( ASpotLightComponent * _Light ) {
    INTRUSIVE_ADD_UNIQUE( _Light, Next, Prev, SpotLightList, SpotLightListTail );
}

void ARenderWorld::RemoveSpotLight( ASpotLightComponent * _Light ) {
    INTRUSIVE_REMOVE( _Light, Next, Prev, SpotLightList, SpotLightListTail );
}

void ARenderWorld::DrawDebug( ADebugRenderer * InRenderer )
{
    if ( RVDrawFrustumClusters ) {
        GLightVoxelizer.DrawVoxels( InRenderer );
    }
}