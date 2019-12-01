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

#include <World/Public/Render/RenderWorld.h>
#include <World/Public/World.h>
#include <World/Public/Components/SkinnedComponent.h>
#include <World/Public/Components/DirectionalLightComponent.h>
#include <World/Public/Components/PointLightComponent.h>
#include <World/Public/Components/SpotLightComponent.h>
#include <Core/Public/IntrusiveLinkedListMacro.h>
#include <Runtime/Public/Runtime.h>
#include "ShadowCascade.h"
#include "LightVoxelizer.h"

ARuntimeVariable RVDrawFrustumClusters( _CTS( "DrawFrustumClusters" ), _CTS( "1" ), VAR_CHEAT );
ARuntimeVariable RVFreezeFrustumClusters( _CTS( "FreezeFrustumClusters" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVFixFrustumClusters( _CTS( "FixFrustumClusters" ), _CTS( "0" ), VAR_CHEAT );

static ALightVoxelizer LightVoxelizer;

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

void ARenderWorld::RenderFrontend_AddInstances( SRenderFrontendDef * _Def ) {
    SRenderFrame * frameData = GRuntime.GetFrameData();
    SRenderView * view = _Def->View;

    for ( ALevel * level : pOwnerWorld->GetArrayOfLevels() ) {
        RenderFrontend_AddLevelInstances( level, _Def );
    }

    // Add directional lights
    for ( ADirectionalLightComponent * light = DirectionalLightList ; light ; light = light->Next ) {

        if ( !light->IsEnabled() ) {
            continue;
        }

        if ( view->NumDirectionalLights > MAX_DIRECTIONAL_LIGHTS ) {
            GLogger.Printf( "MAX_DIRECTIONAL_LIGHTS hit\n" );
            break;
        }

        SDirectionalLightDef * lightDef = (SDirectionalLightDef *)GRuntime.AllocFrameMem( sizeof( SDirectionalLightDef ) );
        if ( !lightDef ) {
            break;
        }

        frameData->DirectionalLights.Append( lightDef );

        lightDef->ColorAndAmbientIntensity = light->GetEffectiveColor();
        lightDef->Matrix = light->GetWorldRotation().ToMatrix();
        lightDef->MaxShadowCascades = light->GetMaxShadowCascades();
        lightDef->RenderMask = light->RenderingGroup;
        lightDef->NumCascades = 0;  // this will be calculated later
        lightDef->FirstCascade = 0; // this will be calculated later
        lightDef->bCastShadow = light->bCastShadow;

        view->NumDirectionalLights++;
    }


    // Add point lights
    for ( APointLightComponent * light = PointLightList ; light ; light = light->Next ) {

        if ( !light->IsEnabled() ) {
            continue;
        }

        // TODO: cull light

        SLightDef * lightDef = (SLightDef *)GRuntime.AllocFrameMem( sizeof( SLightDef ) );
        if ( !lightDef ) {
            break;
        }

        frameData->Lights.Append( lightDef );

        lightDef->bSpot = false;
        lightDef->BoundingBox = light->GetWorldBounds();
        lightDef->ColorAndAmbientIntensity = light->GetEffectiveColor();
        lightDef->Position = light->GetWorldPosition();
        lightDef->RenderMask = light->RenderingGroup;
        lightDef->InnerRadius = light->GetInnerRadius();
        lightDef->OuterRadius = light->GetOuterRadius();
        lightDef->OBBTransformInverse = light->GetOBBTransformInverse();

        view->NumLights++;
    }

    // Add spot lights
    for ( ASpotLightComponent * light = SpotLightList ; light ; light = light->Next ) {

        if ( !light->IsEnabled() ) {
            continue;
        }

        // TODO: cull light

        SLightDef * lightDef = (SLightDef *)GRuntime.AllocFrameMem( sizeof( SLightDef ) );
        if ( !lightDef ) {
            break;
        }

        frameData->Lights.Append( lightDef );

        lightDef->bSpot = true;
        lightDef->BoundingBox = light->GetWorldBounds();
        lightDef->ColorAndAmbientIntensity = light->GetEffectiveColor();
        lightDef->Position = light->GetWorldPosition();
        lightDef->RenderMask = light->RenderingGroup;
        lightDef->InnerRadius = light->GetInnerRadius();
        lightDef->OuterRadius = light->GetOuterRadius();
        lightDef->InnerConeAngle = light->GetInnerConeAngle();
        lightDef->OuterConeAngle = light->GetOuterConeAngle();
        lightDef->SpotDirection = light->GetWorldDirection();
        lightDef->SpotExponent = light->GetSpotExponent();
        lightDef->OBBTransformInverse = light->GetOBBTransformInverse();

        view->NumLights++;
    }


    //GLogger.Printf( "FrameLightData %f KB\n", sizeof( SFrameLightData ) / 1024.0f );
    if ( !RVFixFrustumClusters ) {
        LightVoxelizer.Voxelize( frameData, view );
    }
}

void ARenderWorld::RenderFrontend_AddDirectionalShadowmapInstances( SRenderFrontendDef * _Def ) {
    SRenderFrame * frameData = GRuntime.GetFrameData();

    CreateDirectionalLightCascades( frameData, _Def->View );

    if ( !_Def->View->NumShadowMapCascades ) {
        return;
    }

    // Create shadow instances

    for ( AMeshComponent * component = ShadowCasters ; component ; component = component->GetNextShadowCaster() ) {

        // TODO: Perform culling for each shadow cascade, set CascadeMask

        //if ( component->RenderMark == _Def->VisMarker ) {
        //    return;
        //}

        if ( (component->RenderingGroup & _Def->RenderingMask) == 0 ) {
        //    component->RenderMark = _Def->VisMarker;
            continue;
        }

//        if ( component->VSDPasses & VSD_PASS_FACE_CULL ) {
//            // TODO: bTwoSided and bFrontSided must came from component
//            const bool bTwoSided = false;
//            const bool bFrontSided = true;
//            const float EPS = 0.25f;
//
//            if ( !bTwoSided ) {
//                PlaneF const & plane = component->FacePlane;
//                float d = _Def->View->ViewPosition.Dot( plane.Normal );
//
//                bool bFaceCull = false;
//
//                if ( bFrontSided ) {
//                    if ( d < -plane.D - EPS ) {
//                        bFaceCull = true;
//                    }
//                } else {
//                    if ( d > -plane.D + EPS ) {
//                        bFaceCull = true;
//                    }
//                }
//
//                if ( bFaceCull ) {
//                    component->RenderMark = _Def->VisMarker;
//#ifdef DEBUG_TRAVERSING_COUNTERS
//                    Dbg_CulledByDotProduct++;
//#endif
//                    return;
//                }
//            }
//        }

//        if ( component->VSDPasses & VSD_PASS_BOUNDS ) {
//
//            // TODO: use SSE cull
//            BvAxisAlignedBox const & bounds = component->GetWorldBounds();
//
//            if ( Cull( _CullPlanes, _CullPlanesCount, bounds ) ) {
//#ifdef DEBUG_TRAVERSING_COUNTERS
//                Dbg_CulledBySurfaceBounds++;
//#endif
//                return;
//            }
//        }

//        component->RenderMark = _Def->VisMarker;

        //if ( component->VSDPasses & VSD_PASS_CUSTOM_VISIBLE_STEP ) {

        //    bool bVisible;
        //    component->RenderFrontend_CustomVisibleStep( _Def, bVisible );

        //    if ( !bVisible ) {
        //        return;
        //    }
        //}

        //if ( component->VSDPasses & VSD_PASS_VIS_MARKER ) {
        //    bool bVisible = component->VisMarker == _Def->VisMarker;
        //    if ( !bVisible ) {
        //        return;
        //    }
        //}

        Float3x4 const * instanceMatrix;

        AIndexedMesh * mesh = component->GetMesh();

        size_t skeletonOffset = 0;
        size_t skeletonSize = 0;
        if ( mesh->IsSkinned() && component->IsSkinnedMesh() ) {
            ASkinnedComponent * skeleton = static_cast< ASkinnedComponent * >(component);
            skeleton->UpdateJointTransforms( skeletonOffset, skeletonSize, frameData->FrameNumber );
        }

        if ( component->bNoTransform ) {
            instanceMatrix = &Float3x4::Identity();
        } else {
            instanceMatrix = &component->GetWorldTransformMatrix();
        }

        AIndexedMeshSubpartArray const & subparts = mesh->GetSubparts();

        for ( int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++ ) {

            // FIXME: check subpart bounding box here

            AIndexedMeshSubpart * subpart = subparts[subpartIndex];

            AMaterialInstance * materialInstance = component->GetMaterialInstance( subpartIndex );
            AN_ASSERT( materialInstance );

            AMaterial * material = materialInstance->GetMaterial();

            // Prevent rendering of instances with disabled shadow casting
            if ( material->GetGPUResource()->bNoCastShadow ) {
                continue;
            }

            SMaterialFrameData * materialInstanceFrameData = materialInstance->RenderFrontend_Update( _Def->VisMarker );

            // Add render instance
            SShadowRenderInstance * instance = (SShadowRenderInstance *)GRuntime.AllocFrameMem( sizeof( SShadowRenderInstance ) );
            if ( !instance ) {
                break;
            }

            frameData->ShadowInstances.Append( instance );

            instance->Material = material->GetGPUResource();
            instance->MaterialInstance = materialInstanceFrameData;
            instance->VertexBuffer = mesh->GetVertexBufferGPU();
            instance->IndexBuffer = mesh->GetIndexBufferGPU();
            instance->WeightsBuffer = mesh->GetWeightsBufferGPU();

            if ( component->bUseDynamicRange ) {
                instance->IndexCount = component->DynamicRangeIndexCount;
                instance->StartIndexLocation = component->DynamicRangeStartIndexLocation;
                instance->BaseVertexLocation = component->DynamicRangeBaseVertexLocation;
            } else {
                instance->IndexCount = subpart->GetIndexCount();
                instance->StartIndexLocation = subpart->GetFirstIndex();
                instance->BaseVertexLocation = subpart->GetBaseVertex() + component->SubpartBaseVertexOffset;
            }

            instance->SkeletonOffset = skeletonOffset;
            instance->SkeletonSize = skeletonSize;
            instance->WorldTransformMatrix = *instanceMatrix;
            instance->CascadeMask = 0xffff; // TODO: Calculate!!!

            // Generate sort key.
            // NOTE: 8 bits are still unused. We can use it in future.
            instance->SortKey =   ((uint64_t)(component->RenderingOrder & 0xffu) << 56u)
                                | ((uint64_t)(Core::PHHash64( (uint64_t)instance->Material ) & 0xffffu) << 40u)
                                | ((uint64_t)(Core::PHHash64( (uint64_t)instance->MaterialInstance ) & 0xffffu) << 24u)
                                | ((uint64_t)(Core::PHHash64( (uint64_t)instance->VertexBuffer ) & 0xffffu) << 8u);

            _Def->View->ShadowInstanceCount++;

            _Def->ShadowMapPolyCount += instance->IndexCount / 3;

            if ( component->bUseDynamicRange ) {
                // If component uses dynamic range, mesh has actually one subpart
                break;
            }
        }
    }
}

void ARenderWorld::RenderFrontend_AddLevelInstances( ALevel * InLevel, SRenderFrontendDef * _Def ) {
    // Update view area
    int areaNum = InLevel->FindArea( _Def->View->ViewPosition );

    // Cull invisible objects
    CullInstances( InLevel, areaNum, _Def );
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Portals traversing variables
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// Portal stack
//

#define MAX_PORTAL_STACK 64

struct SPortalScissor {
    float MinX;
    float MinY;
    float MaxX;
    float MaxY;
};

struct SPortalStack {
    PlaneF AreaFrustum[ 4 ];
    int PlanesCount;
    SAreaPortal const * Portal;
    SPortalScissor Scissor;
};

static SPortalStack PortalStack[ MAX_PORTAL_STACK ];
static int PortalStackPos;

//
// Viewer variables
//

static Float3 RightVec;
static Float3 UpVec;
static PlaneF ViewPlane;
static float  ViewZNear;
static Float3 ViewCenter;

//
// Hull clipping variables
//

#define MAX_HULL_POINTS 128
static float ClipDistances[ MAX_HULL_POINTS ];
static EPlaneSide ClipSides[ MAX_HULL_POINTS ];

struct SPortalHull {
    int NumPoints;
    Float3 Points[ MAX_HULL_POINTS ];
};
static SPortalHull PortalHull[ 2 ];


//
// Portal scissors debug
//

//#define DEBUG_PORTAL_SCISSORS
#ifdef DEBUG_PORTAL_SCISSORS
static TPodArray< SPortalScissor > DebugScissors;
#endif

//
// Debugging counters
//

//#define DEBUG_TRAVERSING_COUNTERS

#ifdef DEBUG_TRAVERSING_COUNTERS
static int Dbg_SkippedByVisFrame;
static int Dbg_SkippedByPlaneOffset;
static int Dbg_CulledByDrawableBounds;
static int Dbg_CulledSubpartsCount;
static int Dbg_CulledByDotProduct;
static int Dbg_CulledByLightBounds;
static int Dbg_CulledByEnvCaptureBounds;
static int Dbg_ClippedPortals;
static int Dbg_PassedPortals;
static int Dbg_StackDeep;
#endif

//
// AABB culling
//
AN_FORCEINLINE bool Cull( PlaneF const * _Planes, int _Count, Float3 const & _Mins, Float3 const & _Maxs ) {
    bool inside = true;
    for ( PlaneF const * p = _Planes; p < _Planes + _Count; p++ ) {
        inside &= ( Math::Max( _Mins.X * p->Normal.X, _Maxs.X * p->Normal.X )
            + Math::Max( _Mins.Y * p->Normal.Y, _Maxs.Y * p->Normal.Y )
            + Math::Max( _Mins.Z * p->Normal.Z, _Maxs.Z * p->Normal.Z )
            + p->D ) > 0;
    }
    return !inside;
}

AN_FORCEINLINE bool Cull( PlaneF const * _Planes, int _Count, BvAxisAlignedBox const & _AABB ) {
    return Cull( _Planes, _Count, _AABB.Mins, _AABB.Maxs );
}

//
// Sphere culling
//
AN_FORCEINLINE bool Cull( PlaneF const * _Planes, int _Count, BvSphereSSE const & _Sphere ) {
    bool cull = false;
    for ( const PlaneF * p = _Planes; p < _Planes + _Count; p++ ) {
        if ( Math::Dot( p->Normal, _Sphere.Center ) + p->D <= -_Sphere.Radius ) {
            cull = true;
        }
    }
    return cull;
}

//
// Fast polygon clipping. Without memory allocations.
//
static bool ClipPolygonFast( Float3 const * _InPoints, int _InNumPoints, SPortalHull * _Out, PlaneF const & _Plane, const float _Epsilon ) {
    int Front = 0;
    int Back = 0;
    int i;
    float Dist;

    AN_ASSERT( _InNumPoints + 4 <= MAX_HULL_POINTS );

    // Определить с какой стороны находится каждая точка исходного полигона
    for ( i = 0; i < _InNumPoints; i++ ) {
        Dist = _InPoints[ i ].Dot( _Plane.Normal ) + _Plane.D;

        ClipDistances[ i ] = Dist;

        if ( Dist > _Epsilon ) {
            ClipSides[ i ] = EPlaneSide::Front;
            Front++;
        } else if ( Dist < -_Epsilon ) {
            ClipSides[ i ] = EPlaneSide::Back;
            Back++;
        } else {
            ClipSides[ i ] = EPlaneSide::On;
        }
    }

    if ( !Front ) {
        // Все точки находятся по заднюю сторону плоскости
        _Out->NumPoints = 0;
        return true;
    }

    if ( !Back ) {
        // Все точки находятся по фронтальную сторону плоскости
        return false;
    }

    _Out->NumPoints = 0;

    ClipSides[ i ] = ClipSides[ 0 ];
    ClipDistances[ i ] = ClipDistances[ 0 ];

    for ( i = 0; i < _InNumPoints; i++ ) {
        Float3 const & v = _InPoints[ i ];

        if ( ClipSides[ i ] == EPlaneSide::On ) {
            _Out->Points[ _Out->NumPoints++ ] = v;
            continue;
        }

        if ( ClipSides[ i ] == EPlaneSide::Front ) {
            _Out->Points[ _Out->NumPoints++ ] = v;
        }

        EPlaneSide NextSide = ClipSides[ i + 1 ];

        if ( NextSide == EPlaneSide::On || NextSide == ClipSides[ i ] ) {
            continue;
        }

        Float3 & NewVertex = _Out->Points[ _Out->NumPoints++ ];

        NewVertex = _InPoints[ ( i + 1 ) % _InNumPoints ];

        Dist = ClipDistances[ i ] / ( ClipDistances[ i ] - ClipDistances[ i + 1 ] );
        //for ( int j = 0 ; j < 3 ; j++ ) {
        //	if ( _Plane.Normal[ j ] == 1 ) {
        //		NewVertex[ j ] = -_Plane.D;
        //	} else if ( _Plane.Normal[ j ] == -1 ) {
        //		NewVertex[ j ] = _Plane.D;
        //	} else {
        //		NewVertex[ j ] = v[ j ] + Dist * ( NewVertex[j] - v[j] );
        //	}
        //}
        NewVertex = v + Dist * ( NewVertex - v );
    }

    return true;
}


void ARenderWorld::CullInstances( ALevel * InLevel, int _AreaNum, SRenderFrontendDef * _Def ) {
    AN_ASSERT( _AreaNum < InLevel->GetAreas().Size() );

    #ifdef DEBUG_TRAVERSING_COUNTERS
    Dbg_SkippedByVisFrame = 0;
    Dbg_SkippedByPlaneOffset = 0;
    Dbg_CulledByDrawableBounds = 0;
    Dbg_CulledSubpartsCount = 0;
    Dbg_CulledByDotProduct = 0;
    Dbg_CulledByLightBounds = 0;
    Dbg_CulledByEnvCaptureBounds = 0;
    Dbg_ClippedPortals = 0;
    Dbg_PassedPortals = 0;
    Dbg_StackDeep = 0;
    #endif

    #ifdef DEBUG_PORTAL_SCISSORS
    DebugScissors.Clear();
    #endif

    BvFrustum const & frustum = *_Def->Frustum;

    RightVec = _Def->View->ViewRightVec;
    UpVec = _Def->View->ViewUpVec;
    ViewPlane = frustum[ FPL_NEAR ];
    ViewZNear = ViewPlane.Dist( _Def->View->ViewPosition );//Camera->GetZNear();
    ViewCenter = ViewPlane.Normal * ViewZNear;

    // Get corner at left-bottom of frustum
    Float3 corner = Math::Cross( frustum[ FPL_BOTTOM ].Normal, frustum[ FPL_LEFT ].Normal );

    // Project left-bottom corner to near plane
    corner = corner * ( ViewZNear / Math::Dot( ViewPlane.Normal, corner ) );

    float x = Math::Dot( RightVec, corner );
    float y = Math::Dot( UpVec, corner );

    // w = tan( half_fov_x_rad ) * znear * 2;
    // h = tan( half_fov_y_rad ) * znear * 2;

    PortalStackPos = 0;
    PortalStack[ 0 ].AreaFrustum[ 0 ] = frustum[ 0 ];
    PortalStack[ 0 ].AreaFrustum[ 1 ] = frustum[ 1 ];
    PortalStack[ 0 ].AreaFrustum[ 2 ] = frustum[ 2 ];
    PortalStack[ 0 ].AreaFrustum[ 3 ] = frustum[ 3 ];
    PortalStack[ 0 ].PlanesCount = 4;
    PortalStack[ 0 ].Portal = NULL;
    PortalStack[ 0 ].Scissor.MinX = x;
    PortalStack[ 0 ].Scissor.MinY = y;
    PortalStack[ 0 ].Scissor.MaxX = -x;
    PortalStack[ 0 ].Scissor.MaxY = -y;

    FlowThroughPortals_r( _Def, _AreaNum >= 0 ? InLevel->GetAreas()[ _AreaNum ] : InLevel->GetOutdoorArea() );

    #ifdef DEBUG_TRAVERSING_COUNTERS
    GLogger.Printf( "VSD: VisFrame %d\n", Dbg_SkippedByVisFrame );
    GLogger.Printf( "VSD: PlaneOfs %d\n", Dbg_SkippedByPlaneOffset );
    GLogger.Printf( "VSD: FaceCull %d\n", Dbg_CulledByDotProduct );
    GLogger.Printf( "VSD: AABBCull %d\n", Dbg_CulledByDrawableBounds );
    GLogger.Printf( "VSD: AABBCull (subparts) %d\n", Dbg_CulledSubpartsCount );
    GLogger.Printf( "VSD: LightCull %d\n", Dbg_CulledByLightBounds );
    GLogger.Printf( "VSD: EnvCaptureCull %d\n", Dbg_CulledByEnvCaptureBounds );
    GLogger.Printf( "VSD: Clipped %d\n", Dbg_ClippedPortals );
    GLogger.Printf( "VSD: PassedPortals %d\n", Dbg_PassedPortals );
    GLogger.Printf( "VSD: StackDeep %d\n", Dbg_StackDeep );
    #endif
}

void ARenderWorld::RenderDrawables( SRenderFrontendDef * _Def, TPodArray< ADrawable * > const & _Drawables, PlaneF const * _CullPlanes, int _CullPlanesCount ) {
    for ( ADrawable * drawable : _Drawables ) {
        if ( !drawable->bLightPass ) {
            continue;
        }

        if ( drawable->RenderMark == _Def->VisMarker ) {
            continue;
        }

        if ( ( drawable->RenderingGroup & _Def->RenderingMask ) == 0 ) {
            drawable->RenderMark = _Def->VisMarker;
            continue;
        }

        if ( drawable->VSDPasses & VSD_PASS_FACE_CULL ) {
            // TODO: bTwoSided and bFrontSided must came from the drawable
            const bool bTwoSided = false;
            const bool bFrontSided = true;
            const float EPS = 0.25f;

            if ( !bTwoSided ) {
                PlaneF const & plane = drawable->FacePlane;
                float d = _Def->View->ViewPosition.Dot( plane.Normal );

                bool bFaceCull = false;

                if ( bFrontSided ) {
                    if ( d < -plane.D - EPS ) {
                        bFaceCull = true;
                    }
                } else {
                    if ( d > -plane.D + EPS ) {
                        bFaceCull = true;
                    }
                }

                if ( bFaceCull ) {
                    drawable->RenderMark = _Def->VisMarker;
                    #ifdef DEBUG_TRAVERSING_COUNTERS
                    Dbg_CulledByDotProduct++;
                    #endif
                    continue;
                }
            }
        }

        if ( drawable->VSDPasses & VSD_PASS_BOUNDS ) {

            // TODO: use SSE cull
            BvAxisAlignedBox const & bounds = drawable->GetWorldBounds();

            if ( Cull( _CullPlanes, _CullPlanesCount, bounds ) ) {
                #ifdef DEBUG_TRAVERSING_COUNTERS
                Dbg_CulledByDrawableBounds++;
                #endif
                continue;
            }
        }

        drawable->RenderMark = _Def->VisMarker;

        if ( drawable->VSDPasses & VSD_PASS_CUSTOM_VISIBLE_STEP ) {

            bool bVisible = false;
            drawable->RenderFrontend_CustomVisibleStep( _Def, bVisible );

            if ( !bVisible ) {
                continue;
            }
        }

        if ( drawable->VSDPasses & VSD_PASS_VIS_MARKER ) {
            bool bVisible = drawable->VisMarker == _Def->VisMarker;
            if ( !bVisible ) {
                continue;
            }
        }

        AMeshComponent * mesh = Upcast< AMeshComponent >( drawable );

        if ( mesh ) {
            RenderMesh( _Def, mesh, _CullPlanes, _CullPlanesCount );
        } else {
            GLogger.Printf( "Not a mesh\n" );

            // TODO: Add other drawables (Sprite, ...)
        }
    }
}

void ARenderWorld::RenderArea( SRenderFrontendDef * _Def, ALevelArea * _Area, PlaneF const * _CullPlanes, int _CullPlanesCount ) {
    RenderDrawables( _Def, _Area->GetDrawables(), _CullPlanes, _CullPlanesCount );
#ifdef FUTURE
    RenderPointLights( _Def, _Area->GetPointLights(), _CullPlanes, _CullPlanesCount );
    RenderSpotLights( _Def, _Area->GetSpotLights(), _CullPlanes, _CullPlanesCount );
    RenderEnvProbes( _Def, _Area->GetEnvProbes(), _CullPlanes, _CullPlanesCount );
#endif
}

void ARenderWorld::FlowThroughPortals_r( SRenderFrontendDef * _Def, ALevelArea * _Area ) {
    SPortalStack * prevStack = &PortalStack[ PortalStackPos ];
    SPortalStack * stack = prevStack + 1;

    RenderArea( _Def, _Area, prevStack->AreaFrustum, prevStack->PlanesCount );

    if ( PortalStackPos == ( MAX_PORTAL_STACK - 1 ) ) {
        GLogger.Printf( "MAX_PORTAL_STACK hit\n" );
        return;
    }

    ++PortalStackPos;

    #ifdef DEBUG_TRAVERSING_COUNTERS
    Dbg_StackDeep = Math::Max( Dbg_StackDeep, PortalStackPos );
    #endif

    static float x, y, d;
    static Float3 vec;
    static Float3 p;
    static Float3 rightMin;
    static Float3 rightMax;
    static Float3 upMin;
    static Float3 upMax;
    static Float3 corners[ 4 ];
    static int flip = 0;

    for ( SAreaPortal const * portal = _Area->GetPortals(); portal; portal = portal->Next ) {

        //if ( portal->DoublePortal->VisFrame == _Def->VisMarker ) {
        //    #ifdef DEBUG_TRAVERSING_COUNTERS
        //    Dbg_SkippedByVisFrame++;
        //    #endif
        //    continue;
        //}

        d = portal->Plane.Dist( _Def->View->ViewPosition );
        if ( d <= 0.0f ) {
            #ifdef DEBUG_TRAVERSING_COUNTERS
            Dbg_SkippedByPlaneOffset++;
            #endif
            continue;
        }

        if ( d > 0.0f && d <= ViewZNear ) {
            // View intersecting the portal

            for ( int i = 0; i < prevStack->PlanesCount; i++ ) {
                stack->AreaFrustum[ i ] = prevStack->AreaFrustum[ i ];
            }
            stack->PlanesCount = prevStack->PlanesCount;
            stack->Scissor = prevStack->Scissor;

        } else {

            //for ( int i = 0 ; i < PortalStackPos ; i++ ) {
            //    if ( PortalStack[ i ].Portal == portal ) {
            //        GLogger.Printf( "Recursive!\n" );
            //    }
            //}

            // Clip portal winding by view plane
            //static PolygonF Winding;
            //PolygonF * portalWinding = &portal->Hull;
            //if ( ClipPolygonFast( portal->Hull, Winding, ViewPlane, 0.0f ) ) {
            //    if ( Winding.IsEmpty() ) {
            //        #ifdef DEBUG_TRAVERSING_COUNTERS
            //        Dbg_ClippedPortals++;
            //        #endif
            //        continue; // Culled
            //    }
            //    portalWinding = &Winding;
            //}

            if ( !ClipPolygonFast( portal->Hull->Points, portal->Hull->NumPoints, &PortalHull[ flip ], ViewPlane, 0.0f ) ) {

                AN_ASSERT( portal->Hull->NumPoints <= MAX_HULL_POINTS );

                memcpy( PortalHull[ flip ].Points, portal->Hull->Points, portal->Hull->NumPoints * sizeof( Float3 ) );
                PortalHull[ flip ].NumPoints = portal->Hull->NumPoints;
            }

            if ( PortalHull[ flip ].NumPoints >= 3 ) {
                for ( int i = 0; i < prevStack->PlanesCount; i++ ) {
                    if ( ClipPolygonFast( PortalHull[ flip ].Points, PortalHull[ flip ].NumPoints, &PortalHull[ ( flip + 1 ) & 1 ], prevStack->AreaFrustum[ i ], 0.0f ) ) {
                        flip = ( flip + 1 ) & 1;

                        if ( PortalHull[ flip ].NumPoints < 3 ) {
                            break;
                        }
                    }
                }
            }

            SPortalHull * portalWinding = &PortalHull[ flip ];

            if ( portalWinding->NumPoints < 3 ) {
                // Invisible
                #ifdef DEBUG_TRAVERSING_COUNTERS
                Dbg_ClippedPortals++;
                #endif
                continue;
            }

            float & minX = stack->Scissor.MinX;
            float & minY = stack->Scissor.MinY;
            float & maxX = stack->Scissor.MaxX;
            float & maxY = stack->Scissor.MaxY;

            minX = 99999999.0f;
            minY = 99999999.0f;
            maxX = -99999999.0f;
            maxY = -99999999.0f;

            for ( int i = 0; i < portalWinding->NumPoints; i++ ) {

                // Project portal vertex to view plane
                vec = portalWinding->Points[ i ] - _Def->View->ViewPosition;

                d = Math::Dot( ViewPlane.Normal, vec );

                //if ( d < ViewZNear ) {
                //    AN_ASSERT(0);
                //}

                p = d < ViewZNear ? vec : vec * ( ViewZNear / d );

                // Compute relative coordinates
                x = Math::Dot( RightVec, p );
                y = Math::Dot( UpVec, p );

                // Compute bounds
                minX = Math::Min( x, minX );
                minY = Math::Min( y, minY );

                maxX = Math::Max( x, maxX );
                maxY = Math::Max( y, maxY );
            }

            // Clip bounds by current scissor bounds
            minX = Math::Max( prevStack->Scissor.MinX, minX );
            minY = Math::Max( prevStack->Scissor.MinY, minY );
            maxX = Math::Min( prevStack->Scissor.MaxX, maxX );
            maxY = Math::Min( prevStack->Scissor.MaxY, maxY );

            if ( minX >= maxX || minY >= maxY ) {
                // invisible
                #ifdef DEBUG_TRAVERSING_COUNTERS
                Dbg_ClippedPortals++;
                #endif
                continue; // go to next portal
            }

            // Compute 3D frustum to cull objects inside vis area
            if ( portalWinding->NumPoints <= 4 ) {
                stack->PlanesCount = portalWinding->NumPoints;

                // Compute based on portal winding
                for ( int i = 0; i < stack->PlanesCount; i++ ) {
                    stack->AreaFrustum[ i ].FromPoints( _Def->View->ViewPosition, portalWinding->Points[ ( i + 1 ) % portalWinding->NumPoints ], portalWinding->Points[ i ] );
                }
            } else {
                // Compute based on portal scissor
                rightMin = RightVec * minX + ViewCenter;
                rightMax = RightVec * maxX + ViewCenter;
                upMin = UpVec * minY;
                upMax = UpVec * maxY;
                corners[ 0 ] = rightMin + upMin;
                corners[ 1 ] = rightMax + upMin;
                corners[ 2 ] = rightMax + upMax;
                corners[ 3 ] = rightMin + upMax;

                // bottom
                p = Math::Cross( corners[ 1 ], corners[ 0 ] );
                stack->AreaFrustum[ 0 ].Normal = p * Math::RSqrt( Math::Dot( p, p ) );
                stack->AreaFrustum[ 0 ].D = -Math::Dot( stack->AreaFrustum[ 0 ].Normal, _Def->View->ViewPosition );

                // right
                p = Math::Cross( corners[ 2 ], corners[ 1 ] );
                stack->AreaFrustum[ 1 ].Normal = p * Math::RSqrt( Math::Dot( p, p ) );
                stack->AreaFrustum[ 1 ].D = -Math::Dot( stack->AreaFrustum[ 1 ].Normal, _Def->View->ViewPosition );

                // top
                p = Math::Cross( corners[ 3 ], corners[ 2 ] );
                stack->AreaFrustum[ 2 ].Normal = p * Math::RSqrt( Math::Dot( p, p ) );
                stack->AreaFrustum[ 2 ].D = -Math::Dot( stack->AreaFrustum[ 2 ].Normal, _Def->View->ViewPosition );

                // left
                p = Math::Cross( corners[ 0 ], corners[ 3 ] );
                stack->AreaFrustum[ 3 ].Normal = p * Math::RSqrt( Math::Dot( p, p ) );
                stack->AreaFrustum[ 3 ].D = -Math::Dot( stack->AreaFrustum[ 3 ].Normal, _Def->View->ViewPosition );

                stack->PlanesCount = 4;
            }
        }

        #ifdef DEBUG_PORTAL_SCISSORS
        DebugScissors.Append( stack->Scissor );
        #endif

        #ifdef DEBUG_TRAVERSING_COUNTERS
        Dbg_PassedPortals++;
        #endif

        stack->Portal = portal;

        portal->Owner->VisMark = _Def->VisMarker;
        FlowThroughPortals_r( _Def, portal->ToArea );
    }

    --PortalStackPos;
}

void ARenderWorld::RenderMesh( SRenderFrontendDef * _Def, AMeshComponent * component, PlaneF const * _CullPlanes, int _CullPlanesCount ) {
    Float4x4 tmpMatrix;
    Float4x4 * instanceMatrix;

    AIndexedMesh * mesh = component->GetMesh();

    size_t skeletonOffset = 0;
    size_t skeletonSize = 0;
    if ( mesh->IsSkinned() && component->IsSkinnedMesh() ) {
        ASkinnedComponent * skeleton = static_cast< ASkinnedComponent * >( component );
        skeleton->UpdateJointTransforms( skeletonOffset, skeletonSize, GRuntime.GetFrameData()->FrameNumber );
    }

    Float3x4 const & componentWorldTransform = component->GetWorldTransformMatrix();

    if ( component->bNoTransform ) {
        instanceMatrix = &_Def->View->ModelviewProjection;
    } else {
        tmpMatrix = _Def->View->ModelviewProjection * componentWorldTransform; // TODO: optimize: parallel, sse, check if transformable
        instanceMatrix = &tmpMatrix;
    }

    AActor * actor = component->GetParentActor();
    ALevel * level = actor->GetLevel();

    AIndexedMeshSubpartArray const & subparts = mesh->GetSubparts();

    for ( int subpartIndex = 0; subpartIndex < subparts.Size(); subpartIndex++ ) {

        AIndexedMeshSubpart * subpart = subparts[ subpartIndex ];

        if ( !mesh->IsSkinned() && subparts.Size() > 1 && ( component->VSDPasses & VSD_PASS_BOUNDS ) ) {

            // TODO: use SSE cull

            if ( Cull( _CullPlanes, _CullPlanesCount, subpart->GetBoundingBox().Transform( componentWorldTransform ) ) ) {
                #ifdef DEBUG_TRAVERSING_COUNTERS
                Dbg_CulledSubpartsCount++;
                #endif
                continue;
            }
        }

        AMaterialInstance * materialInstance = component->GetMaterialInstance( subpartIndex );
        AN_ASSERT( materialInstance );

        AMaterial * material = materialInstance->GetMaterial();

        SMaterialFrameData * materialInstanceFrameData = materialInstance->RenderFrontend_Update( _Def->VisMarker );

        // Add render instance
        SRenderInstance * instance = ( SRenderInstance * )GRuntime.AllocFrameMem( sizeof( SRenderInstance ) );
        if ( !instance ) {
            return;
        }

        GRuntime.GetFrameData()->Instances.Append( instance );

        instance->Material = material->GetGPUResource();
        instance->MaterialInstance = materialInstanceFrameData;
        instance->VertexBuffer = mesh->GetVertexBufferGPU();
        instance->IndexBuffer = mesh->GetIndexBufferGPU();
        instance->WeightsBuffer = mesh->GetWeightsBufferGPU();

        if ( component->LightmapUVChannel && component->LightmapBlock >= 0 && component->LightmapBlock < level->Lightmaps.Size() ) {
            instance->LightmapUVChannel = component->LightmapUVChannel->GetGPUResource();
            instance->LightmapOffset = component->LightmapOffset;
            instance->Lightmap = level->Lightmaps[ component->LightmapBlock ]->GetGPUResource();
        } else {
            instance->LightmapUVChannel = nullptr;
            instance->Lightmap = nullptr;
        }

        if ( component->VertexLightChannel ) {
            instance->VertexLightChannel = component->VertexLightChannel->GetGPUResource();
        } else {
            instance->VertexLightChannel = nullptr;
        }

        if ( component->bUseDynamicRange ) {
            instance->IndexCount = component->DynamicRangeIndexCount;
            instance->StartIndexLocation = component->DynamicRangeStartIndexLocation;
            instance->BaseVertexLocation = component->DynamicRangeBaseVertexLocation;
        } else {
            instance->IndexCount = subpart->GetIndexCount();
            instance->StartIndexLocation = subpart->GetFirstIndex();
            instance->BaseVertexLocation = subpart->GetBaseVertex() + component->SubpartBaseVertexOffset;
        }

        instance->SkeletonOffset = skeletonOffset;
        instance->SkeletonSize = skeletonSize;
        instance->Matrix = *instanceMatrix;

        if ( material->GetType() == MATERIAL_TYPE_PBR || material->GetType() == MATERIAL_TYPE_BASELIGHT ) {
            instance->ModelNormalToViewSpace = _Def->View->NormalToViewMatrix * component->GetWorldRotation().ToMatrix();
        }

        // Generate sort key.
        // NOTE: 8 bits are still unused. We can use it in future.
        instance->SortKey =
                ((uint64_t)(component->RenderingOrder & 0xffu) << 56u)
              | ((uint64_t)(Core::PHHash64( (uint64_t)instance->Material ) & 0xffffu) << 40u)
              | ((uint64_t)(Core::PHHash64( (uint64_t)instance->MaterialInstance ) & 0xffffu) << 24u)
              | ((uint64_t)(Core::PHHash64( (uint64_t)instance->VertexBuffer ) & 0xffffu) << 8u);

        _Def->View->InstanceCount++;

        _Def->PolyCount += instance->IndexCount / 3;

        if ( component->bUseDynamicRange ) {
            // If component uses dynamic range, mesh has actually one subpart
            break;
        }
    }
}

void ARenderWorld::DrawDebug( ADebugRenderer * InRenderer )
{
    if ( RVDrawFrustumClusters ) {
        static std::vector< Float3 > LinePoints;

        if ( !RVFreezeFrustumClusters )
        {
            Float3 clusterMins;
            Float3 clusterMaxs;
            Float4 p[ 8 ];
            Float3 * lineP;
            Float4x4 projMat;

            SRenderView const * view = InRenderer->GetRenderView();

            projMat = view->ClusterProjectionMatrix;

            Float4x4 ViewProj = projMat * view->ViewMatrix;
            Float4x4 ViewProjInv = ViewProj.Inversed();

            LinePoints.clear();

            for ( int sliceIndex = 0 ; sliceIndex < MAX_FRUSTUM_CLUSTERS_Z ; sliceIndex++ ) {

                clusterMins.Z = FRUSTUM_SLICE_ZCLIP[ sliceIndex + 1 ];
                clusterMaxs.Z = FRUSTUM_SLICE_ZCLIP[ sliceIndex ];

                for ( int clusterY = 0 ; clusterY < MAX_FRUSTUM_CLUSTERS_Y ; clusterY++ ) {

                    clusterMins.Y = clusterY * FRUSTUM_CLUSTER_HEIGHT - 1.0f;
                    clusterMaxs.Y = clusterMins.Y + FRUSTUM_CLUSTER_HEIGHT;

                    for ( int clusterX = 0 ; clusterX < MAX_FRUSTUM_CLUSTERS_X ; clusterX++ ) {

                        clusterMins.X = clusterX * FRUSTUM_CLUSTER_WIDTH - 1.0f;
                        clusterMaxs.X = clusterMins.X + FRUSTUM_CLUSTER_WIDTH;

                        if (   LightVoxelizer.ClusterData[ sliceIndex ][ clusterY ][ clusterX ].LightsCount > 0
                            || LightVoxelizer.ClusterData[ sliceIndex ][ clusterY ][ clusterX ].DecalsCount > 0
                            || LightVoxelizer.ClusterData[ sliceIndex ][ clusterY ][ clusterX ].ProbesCount > 0 ) {
                            p[ 0 ] = Float4( clusterMins.X, clusterMins.Y, clusterMins.Z, 1.0f );
                            p[ 1 ] = Float4( clusterMaxs.X, clusterMins.Y, clusterMins.Z, 1.0f );
                            p[ 2 ] = Float4( clusterMaxs.X, clusterMaxs.Y, clusterMins.Z, 1.0f );
                            p[ 3 ] = Float4( clusterMins.X, clusterMaxs.Y, clusterMins.Z, 1.0f );
                            p[ 4 ] = Float4( clusterMaxs.X, clusterMins.Y, clusterMaxs.Z, 1.0f );
                            p[ 5 ] = Float4( clusterMins.X, clusterMins.Y, clusterMaxs.Z, 1.0f );
                            p[ 6 ] = Float4( clusterMins.X, clusterMaxs.Y, clusterMaxs.Z, 1.0f );
                            p[ 7 ] = Float4( clusterMaxs.X, clusterMaxs.Y, clusterMaxs.Z, 1.0f );
                            LinePoints.resize( LinePoints.size() + 8 );
                            lineP = LinePoints.data() + LinePoints.size() - 8;
                            for ( int i = 0 ; i < 8 ; i++ ) {
                                p[ i ] = ViewProjInv * p[ i ];
                                const float Denom = 1.0f / p[ i ].W;
                                lineP[ i ].X = p[ i ].X * Denom;
                                lineP[ i ].Y = p[ i ].Y * Denom;
                                lineP[ i ].Z = p[ i ].Z * Denom;
                            }
                        }
                    }
                }
            }
        }

        if ( LightVoxelizer.bUseSSE )//if ( RVReverseNegativeZ )
            InRenderer->SetColor( AColor4( 0, 0, 1 ) );
        else
            InRenderer->SetColor( AColor4( 1, 0, 0 ) );

        int n = 0;
        for ( Float3 * lineP = LinePoints.data() ; n < LinePoints.size() ; lineP += 8, n += 8 )
        {
            InRenderer->DrawLine( lineP, 4, true );
            InRenderer->DrawLine( lineP + 4, 4, true );
            InRenderer->DrawLine( lineP[ 0 ], lineP[ 5 ] );
            InRenderer->DrawLine( lineP[ 1 ], lineP[ 4 ] );
            InRenderer->DrawLine( lineP[ 2 ], lineP[ 7 ] );
            InRenderer->DrawLine( lineP[ 3 ], lineP[ 6 ] );
        }
    }
}
