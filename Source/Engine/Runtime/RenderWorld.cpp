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

#include "RenderWorld.h"
#include "SkinnedComponent.h"
#include "DirectionalLightComponent.h"
#include "RuntimeVariable.h"
#include <Core/IntrusiveLinkedListMacro.h>

ARenderWorld::ARenderWorld()
{
}

void ARenderWorld::AddSkinnedMesh( ASkinnedComponent * InSkeleton )
{
    INTRUSIVE_ADD_UNIQUE( InSkeleton, Next, Prev, SkinnedMeshList, SkinnedMeshListTail );
}

void ARenderWorld::RemoveSkinnedMesh( ASkinnedComponent * InSkeleton )
{
    INTRUSIVE_REMOVE( InSkeleton, Next, Prev, SkinnedMeshList, SkinnedMeshListTail );
}

void ARenderWorld::AddShadowCaster( ADrawable * InMesh )
{
    INTRUSIVE_ADD_UNIQUE( InMesh, NextShadowCaster, PrevShadowCaster, ShadowCasters, ShadowCastersTail );
}

void ARenderWorld::RemoveShadowCaster( ADrawable * InMesh )
{
    INTRUSIVE_REMOVE( InMesh, NextShadowCaster, PrevShadowCaster, ShadowCasters, ShadowCastersTail );
}

void ARenderWorld::AddDirectionalLight( ADirectionalLightComponent * InLight )
{
    INTRUSIVE_ADD_UNIQUE( InLight, Next, Prev, DirectionalLightList, DirectionalLightListTail );
}

void ARenderWorld::RemoveDirectionalLight( ADirectionalLightComponent * InLight )
{
    INTRUSIVE_REMOVE( InLight, Next, Prev, DirectionalLightList, DirectionalLightListTail );
}

void ARenderWorld::DrawDebug( ADebugRenderer * InRenderer )
{
}
