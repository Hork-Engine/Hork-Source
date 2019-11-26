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

#include <World/Public/RenderWorld.h>
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
        level->RenderFrontend_AddInstances( _Def );
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

            _Def->View->ShadowInstanceCount++;

            _Def->ShadowMapPolyCount += instance->IndexCount / 3;

            if ( component->bUseDynamicRange ) {
                // If component uses dynamic range, mesh has actually one subpart
                break;
            }
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
