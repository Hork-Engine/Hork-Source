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

#include <Engine/World/Public/WorldRaycastQuery.h>
#include <Engine/World/Public/World.h>
#include <Engine/World/Public/Components/MeshComponent.h>

SWorldRaycastFilter AWorldRaycastQuery::DefaultRaycastFilter;

bool AWorldRaycastQuery::Raycast( AWorld const * _World, SWorldRaycastResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SWorldRaycastFilter const * _Filter ) {
    Float3 rayVec = _RayEnd - _RayStart;
    Float3 rayDir;
    Float3 invRayDir;
    Float3 rayStartLocal;
    Float3 rayEndLocal;
    Float3 rayDirLocal;
    float boxMin, boxMax;

    if ( !_Filter ) {
        _Filter = &DefaultRaycastFilter;
    }

    _Result.Clear();

    float rayLength = rayVec.Length();

    if ( rayLength < 0.0001f ) {
        return false;
    }

    rayDir = rayVec / rayLength;

    invRayDir.X = 1.0f / rayDir.X;
    invRayDir.Y = 1.0f / rayDir.Y;
    invRayDir.Z = 1.0f / rayDir.Z;

    for ( AMeshComponent * mesh = _World->GetMeshes() ; mesh ; mesh = mesh->GetNextMesh() ) {

        if ( mesh->bUseDynamicRange ) {
            continue;
        }

        if ( !( mesh->RenderingGroup & _Filter->RenderingMask ) ) {
            continue;
        }

        if ( mesh->IsSkinnedMesh() ) {
            continue;
        }

        AIndexedMesh * resource = mesh->GetMesh();

        if ( !BvRayIntersectBox( _RayStart, invRayDir, mesh->GetWorldBounds(), boxMin, boxMax ) ) {
            continue;
        }

        if ( boxMin > rayLength ) {
            // Ray intersects the box, but box is too far
            continue;
        }

        Float3x4 transformInverse = mesh->ComputeWorldTransformInverse();

        // transform ray to object space
        rayStartLocal = transformInverse * _RayStart;
        rayEndLocal = transformInverse * _RayEnd;
        rayDirLocal = rayEndLocal - rayStartLocal;

        float hitDistanceLocal = rayDirLocal.Length();
        if ( hitDistanceLocal < 0.0001f ) {
            continue;
        }

        rayDirLocal /= hitDistanceLocal;

        int firstHit = _Result.Hits.Size();

        if ( resource->Raycast( rayStartLocal, rayDirLocal, hitDistanceLocal, _Result.Hits ) ) {

            SWorldRaycastEntity & raycastEntity = _Result.Entities.Append();

            raycastEntity.Object = mesh;
            raycastEntity.FirstHit = firstHit;
            raycastEntity.NumHits = _Result.Hits.Size() - firstHit;
            raycastEntity.ClosestHit = raycastEntity.FirstHit;

            // Convert hits to worldspace and find closest hit

            Float3x4 const & transform = mesh->GetWorldTransformMatrix();
            Float3x3 normalMatrix;

            transform.DecomposeNormalMatrix( normalMatrix );

            for ( int i = 0 ; i < raycastEntity.NumHits ; i++ ) {
                int hitNum = raycastEntity.FirstHit + i;
                STriangleHitResult & hitResult = _Result.Hits[hitNum];

                hitResult.Location = transform * hitResult.Location;
                hitResult.Normal = ( normalMatrix * hitResult.Normal ).Normalized();
                hitResult.Distance = (hitResult.Location - _RayStart).Length();

                if ( hitResult.Distance < _Result.Hits[ raycastEntity.ClosestHit ].Distance ) {
                    raycastEntity.ClosestHit = hitNum;
                }
            }
        }
    }

    if ( _Result.Entities.IsEmpty() ) {
        return false;
    }

    if ( _Filter->bSortByDistance ) {
        _Result.Sort();
    }

    return true;
}

bool AWorldRaycastQuery::RaycastAABB( AWorld const * _World, TPodArray< SBoxHitResult > & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SWorldRaycastFilter const * _Filter ) {
    Float3 rayVec = _RayEnd - _RayStart;
    Float3 rayDir;
    Float3 invRayDir;

    if ( !_Filter ) {
        _Filter = &DefaultRaycastFilter;
    }

    _Result.Clear();

    float rayLength = rayVec.Length();

    if ( rayLength < 0.0001f ) {
        return false;
    }

    rayDir = rayVec / rayLength;

    invRayDir.X = 1.0f / rayDir.X;
    invRayDir.Y = 1.0f / rayDir.Y;
    invRayDir.Z = 1.0f / rayDir.Z;

    float boxMin, boxMax;

    for ( AMeshComponent * mesh = _World->GetMeshes() ; mesh ; mesh = mesh->GetNextMesh() ) {

        if ( mesh->bUseDynamicRange ) {
            continue;
        }

        if ( !( mesh->RenderingGroup & _Filter->RenderingMask ) ) {
            continue;
        }

        if ( !BvRayIntersectBox( _RayStart, invRayDir, mesh->GetWorldBounds(), boxMin, boxMax ) ) {
            continue;
        }

        if ( boxMin > rayLength ) {
            // Ray intersects the box, but box is too far
            continue;
        }

        SBoxHitResult & hitResult = _Result.Append();

        hitResult.Object = mesh;
        hitResult.LocationMin = _RayStart + rayDir * boxMin;
        hitResult.LocationMax = _RayStart + rayDir * boxMax;
        hitResult.DistanceMin = boxMin;
        hitResult.DistanceMax = boxMax;
    }

    if ( _Result.IsEmpty() ) {
        return false;
    }

    if ( _Filter->bSortByDistance ) {
        struct ASortHit {
            bool operator() ( SBoxHitResult const & _A, SBoxHitResult const & _B ) {
                return ( _A.DistanceMin < _B.DistanceMin );
            }
        } SortHit;

        StdSort( _Result.ToPtr(), _Result.ToPtr() + _Result.Size(), SortHit );
    }

    return true;
}

bool AWorldRaycastQuery::RaycastClosest( AWorld const * _World, SWorldRaycastClosestResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SWorldRaycastFilter const * _Filter ) {
    AMeshComponent * hitObject = nullptr;
    Float3 rayVec = _RayEnd - _RayStart;
    Float3 rayDir;
    Float3 invRayDir;
    Float3 rayStartLocal;
    Float3 rayEndLocal;
    Float3 rayDirLocal;
    Float3 hitLocation;
    Float2 hitUV;
    float hitDistance;
    unsigned int indices[3];
    TRef< AMaterialInstance > material;

    if ( !_Filter ) {
        _Filter = &DefaultRaycastFilter;
    }

    _Result.Clear();

    float rayLength = rayVec.Length();

    if ( rayLength < 0.0001f ) {
        return false;
    }

    rayDir = rayVec / rayLength;

    invRayDir.X = 1.0f / rayDir.X;
    invRayDir.Y = 1.0f / rayDir.Y;
    invRayDir.Z = 1.0f / rayDir.Z;

    float boxMin, boxMax;

    hitDistance = rayLength;
    hitLocation = _RayEnd;

    for ( AMeshComponent * mesh = _World->GetMeshes() ; mesh ; mesh = mesh->GetNextMesh() ) {

        if ( mesh->bUseDynamicRange ) {
            continue;
        }

        if ( !( mesh->RenderingGroup & _Filter->RenderingMask ) ) {
            continue;
        }

        if ( mesh->IsSkinnedMesh() ) {
            continue;
        }

        AIndexedMesh * resource = mesh->GetMesh();

        if ( !BvRayIntersectBox( _RayStart, invRayDir, mesh->GetWorldBounds(), boxMin, boxMax ) ) {
            continue;
        }

        if ( boxMin > hitDistance ) {
            // Ray intersects the box, but box is too far
            continue;
        }

        Float3x4 transformInverse = mesh->ComputeWorldTransformInverse();

        // transform ray to object space
        rayStartLocal = transformInverse * _RayStart;
        rayEndLocal = transformInverse * hitLocation;
        rayDirLocal = rayEndLocal - rayStartLocal;

        float hitDistanceLocal = rayDirLocal.Length();
        if ( hitDistanceLocal < 0.0001f ) {
            continue;
        }

        rayDirLocal /= hitDistanceLocal;

        if ( resource->RaycastClosest( rayStartLocal, rayDirLocal, hitDistanceLocal, hitLocation, hitUV, hitDistance, indices, material ) ) {
            hitObject = mesh;

            // transform hit location to world space
            hitLocation = hitObject->GetWorldTransformMatrix() * hitLocation;

            // recalc hit distance in world space
            hitDistance = (hitLocation - _RayStart).Length();

            // hit is close enough to stop ray casting?
            if ( hitDistance < 0.0001f ) {
                break;
            }
        }
    }

    if ( !hitObject ) {
        return false;
    }

    AIndexedMesh * resource = hitObject->GetMesh();
    SMeshVertex * vertices = resource->GetVertices();
    Float3x4 const & transform = hitObject->GetWorldTransformMatrix();

    Float3 const & v0 = vertices[indices[0]].Position;
    Float3 const & v1 = vertices[indices[1]].Position;
    Float3 const & v2 = vertices[indices[2]].Position;

    // calc triangle vertices
    _Result.Vertices[0] = transform * v0;
    _Result.Vertices[1] = transform * v1;
    _Result.Vertices[2] = transform * v2;

    STriangleHitResult & triangleHit = _Result.TriangleHit;
#if 1
    triangleHit.Normal = ( _Result.Vertices[1]-_Result.Vertices[0] ).Cross( _Result.Vertices[2]-_Result.Vertices[0] ).Normalized();
#else
    Float3x3 normalMat;
    transform.DecomposeNormalMatrix( normalMat );
    triangleHit.Normal = (normalMat * (v1-v0).Cross( v2-v0 )).Normalized();
#endif
    triangleHit.Location = hitLocation;
    triangleHit.Distance = hitDistance;
    triangleHit.Indices[0] = indices[0];
    triangleHit.Indices[1] = indices[1];
    triangleHit.Indices[2] = indices[2];
    triangleHit.Material = material;
    triangleHit.UV = hitUV;

    _Result.Object = hitObject;
    _Result.Fraction = hitDistance / rayLength;

    // calc texcoord
    Float2 const & uv0 = vertices[indices[0]].TexCoord;
    Float2 const & uv1 = vertices[indices[1]].TexCoord;
    Float2 const & uv2 = vertices[indices[2]].TexCoord;
    _Result.Texcoord = uv0 * hitUV[0] + uv1 * hitUV[1] + uv2 * ( 1.0f - hitUV[0] - hitUV[1] );

    return true;
}

bool AWorldRaycastQuery::RaycastClosestAABB( AWorld const * _World, SBoxHitResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd, SWorldRaycastFilter const * _Filter ) {
    AMeshComponent * hitObject = nullptr;
    Float3 rayVec = _RayEnd - _RayStart;
    Float3 rayDir;
    Float3 invRayDir;
    float hitDistanceMin;
    float hitDistanceMax;

    if ( !_Filter ) {
        _Filter = &DefaultRaycastFilter;
    }

    _Result.Clear();

    float rayLength = rayVec.Length();

    if ( rayLength < 0.0001f ) {
        return false;
    }

    rayDir = rayVec / rayLength;

    invRayDir.X = 1.0f / rayDir.X;
    invRayDir.Y = 1.0f / rayDir.Y;
    invRayDir.Z = 1.0f / rayDir.Z;

    float boxMin, boxMax;

    hitDistanceMin = rayLength;
    hitDistanceMax = rayLength;

    for ( AMeshComponent * mesh = _World->GetMeshes() ; mesh ; mesh = mesh->GetNextMesh() ) {

        if ( mesh->bUseDynamicRange ) {
            continue;
        }

        if ( !( mesh->RenderingGroup & _Filter->RenderingMask ) ) {
            continue;
        }

        if ( !BvRayIntersectBox( _RayStart, invRayDir, mesh->GetWorldBounds(), boxMin, boxMax ) ) {
            continue;
        }

        if ( boxMin > hitDistanceMin ) {
            // Ray intersects the box, but box is too far
            continue;
        }

        hitObject = mesh;
        hitDistanceMin = boxMin;
        hitDistanceMax = boxMax;

        // hit is close enough to stop ray casting?
        if ( hitDistanceMin < 0.0001f ) {
            break;
        }
    }

    if ( !hitObject ) {
        return false;
    }

    _Result.Object = hitObject;
    _Result.LocationMin = _RayStart + rayDir * hitDistanceMin;
    _Result.LocationMax = _RayStart + rayDir * hitDistanceMax;
    _Result.DistanceMin = hitDistanceMin;
    _Result.DistanceMax = hitDistanceMax;
    //_Result.HitFractionMin = hitDistanceMin / rayLength;
    //_Result.HitFractionMax = hitDistanceMax / rayLength;

    return true;
}
