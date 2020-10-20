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
#include <Runtime/Public/RuntimeVariable.h>
#include "LightVoxelizer.h"

ARuntimeVariable com_DrawFrustumClusters( _CTS( "com_DrawFrustumClusters" ), _CTS( "0" ), VAR_CHEAT );

ARenderWorld::ARenderWorld( AWorld * InOwnerWorld )
    : pOwnerWorld( InOwnerWorld )
{
}

void ARenderWorld::AddSkinnedMesh( ASkinnedComponent * _Skeleton ) {
    INTRUSIVE_ADD_UNIQUE( _Skeleton, Next, Prev, SkinnedMeshList, SkinnedMeshListTail );
}

void ARenderWorld::RemoveSkinnedMesh( ASkinnedComponent * _Skeleton ) {
    INTRUSIVE_REMOVE( _Skeleton, Next, Prev, SkinnedMeshList, SkinnedMeshListTail );
}

void ARenderWorld::AddShadowCaster( ADrawable * _Mesh ) {
    INTRUSIVE_ADD_UNIQUE( _Mesh, NextShadowCaster, PrevShadowCaster, ShadowCasters, ShadowCastersTail );
}

void ARenderWorld::RemoveShadowCaster( ADrawable * _Mesh ) {
    INTRUSIVE_REMOVE( _Mesh, NextShadowCaster, PrevShadowCaster, ShadowCasters, ShadowCastersTail );
}

void ARenderWorld::AddDirectionalLight( ADirectionalLightComponent * _Light ) {
    INTRUSIVE_ADD_UNIQUE( _Light, Next, Prev, DirectionalLightList, DirectionalLightListTail );
}

void ARenderWorld::RemoveDirectionalLight( ADirectionalLightComponent * _Light ) {
    INTRUSIVE_REMOVE( _Light, Next, Prev, DirectionalLightList, DirectionalLightListTail );
}

void ARenderWorld::DrawDebug( ADebugRenderer * InRenderer )
{
    if ( com_DrawFrustumClusters ) {
        GLightVoxelizer.DrawVoxels( InRenderer );
    }
}
