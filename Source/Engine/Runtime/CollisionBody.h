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

#pragma once

#include "BaseObject.h"
#include <Geometry/BV/BvAxisAlignedBox.h>
#include <Geometry/Plane.h>

class AIndexedMeshSubpart;

class ACollisionBody : public ABaseObject {
    AN_CLASS( ACollisionBody, ABaseObject )

public:
    Float3 Position;
    Quat Rotation;
    float Margin;

    enum { AXIAL_X, AXIAL_Y, AXIAL_Z, AXIAL_DEFAULT = AXIAL_Y };

    virtual bool IsConvex() const { return false; }

    virtual void GatherGeometry( TPodVectorHeap< Float3 > & _Vertices, TPodVectorHeap< unsigned int > & _Indices ) const {}

protected:
    ACollisionBody()
        : Position( 0.0f )
        , Rotation( Quat::Identity() )
        , Margin( 0.01f )
    {
    }

public:
    virtual class btCollisionShape * Create() { AN_ASSERT( 0 ); return nullptr; }
};

class ACollisionSphere : public ACollisionBody {
    AN_CLASS( ACollisionSphere, ACollisionBody )

public:
    float Radius = 0.5f;
    bool bProportionalScale = true;

    bool IsConvex() const override { return true; }

    void GatherGeometry( TPodVectorHeap< Float3 > & _Vertices, TPodVectorHeap< unsigned int > & _Indices ) const override;

protected:
    ACollisionSphere() {}

public:
    btCollisionShape * Create() override;
};

class ACollisionSphereRadii : public ACollisionBody {
    AN_CLASS( ACollisionSphereRadii, ACollisionBody )

public:
    Float3 Radius = Float3(0.5f);

    bool IsConvex() const override { return true; }

    void GatherGeometry( TPodVectorHeap< Float3 > & _Vertices, TPodVectorHeap< unsigned int > & _Indices ) const override;

protected:
    ACollisionSphereRadii() {}

public:
    btCollisionShape * Create() override;
};

class ACollisionBox : public ACollisionBody {
    AN_CLASS( ACollisionBox, ACollisionBody )

public:
    Float3 HalfExtents = Float3(0.5f);

    bool IsConvex() const override { return true; }

    void GatherGeometry( TPodVectorHeap< Float3 > & _Vertices, TPodVectorHeap< unsigned int > & _Indices ) const override;

protected:
    ACollisionBox() {}

public:
    btCollisionShape * Create() override;
};

class ACollisionCylinder : public ACollisionBody {
    AN_CLASS( ACollisionCylinder, ACollisionBody )

public:
    Float3 HalfExtents = Float3(1.0f);
    int Axial = AXIAL_DEFAULT;

    bool IsConvex() const override { return true; }

    void GatherGeometry( TPodVectorHeap< Float3 > & _Vertices, TPodVectorHeap< unsigned int > & _Indices ) const override;

protected:
    ACollisionCylinder() {}

public:
    btCollisionShape * Create() override;
};

class ACollisionCone : public ACollisionBody {
    AN_CLASS( ACollisionCone, ACollisionBody )

public:
    float Radius = 1;
    float Height = 1;
    int Axial = AXIAL_DEFAULT;

    bool IsConvex() const override { return true; }

    void GatherGeometry( TPodVectorHeap< Float3 > & _Vertices, TPodVectorHeap< unsigned int > & _Indices ) const override;

protected:
    ACollisionCone() {}

public:
    btCollisionShape * Create() override;
};

class ACollisionCapsule : public ACollisionBody {
    AN_CLASS( ACollisionCapsule, ACollisionBody )

public:

    /** Radius of the capsule. The total height is Height + 2 * Radius */
    float Radius = 1;

    /** Height between the center of each sphere of the capsule caps */
    float Height = 1;

    int Axial = AXIAL_DEFAULT;

    float GetTotalHeight() const { return Height + 2 * Radius; }

    bool IsConvex() const override { return true; }

    void GatherGeometry( TPodVectorHeap< Float3 > & _Vertices, TPodVectorHeap< unsigned int > & _Indices ) const override;

protected:
    ACollisionCapsule() {}

public:
    btCollisionShape * Create() override;
};

//class ACollisionConvexHull : public ACollisionBody {
//    AN_CLASS( ACollisionConvexHull, ACollisionBody )

//public:
//    TPodVector< Float3 > Vertices;

//    void InitializeFromPlanes( PlaneF const * _Planes, int _NumPlanes ) {
//        Vertices.Clear();
//        ConvexHullVerticesFromPlanes( _Planes, _NumPlanes, Vertices );
//    }

//    bool IsConvex() const override { return true; }

//    void GatherGeometry( TPodVector< Float3 > & _Vertices, TPodVector< unsigned int > & _Indices ) const override;

//protected:
//    ACollisionConvexHull() {}

//private:
//    btCollisionShape * Create() override;
//};

class ACollisionConvexHullData : public ABaseObject {
    AN_CLASS( ACollisionConvexHullData, ABaseObject )

    friend class ACollisionConvexHull;

public:
    void Initialize( Float3 const * _Vertices, int _VertexCount, unsigned int const * _Indices, int _IndexCount );

    Float3 const * GetVertices() const { return Vertices.ToPtr(); }
    int GetVertexCount() const { return Vertices.Size(); }

    unsigned int const * GetIndices() const { return Indices.ToPtr(); }
    int GetIndexCount() const { return Indices.Size(); }

//    void InitializeFromPlanes( PlaneF const * _Planes, int _NumPlanes ) {
//        Vertices.Clear();
//        ConvexHullVerticesFromPlanes( _Planes, _NumPlanes, Vertices );
//    }

protected:
    ACollisionConvexHullData();
    ~ACollisionConvexHullData();

    TPodVectorHeap< Float3 > Vertices;
    TPodVectorHeap< unsigned int > Indices;
    //class btVector3 * Data = nullptr;
};

class ACollisionConvexHull : public ACollisionBody {
    AN_CLASS( ACollisionConvexHull, ACollisionBody )

public:
    TRef< ACollisionConvexHullData > HullData;

    bool IsConvex() const override { return true; }

    void GatherGeometry( TPodVectorHeap< Float3 > & _Vertices, TPodVectorHeap< unsigned int > & _Indices ) const override;

protected:
    ACollisionConvexHull() {}

public:
    btCollisionShape * Create() override;
};

class ACollisionTriangleSoupData : public ABaseObject {
    AN_CLASS( ACollisionTriangleSoupData, ABaseObject )

public:
    struct SSubpart {
        int BaseVertex;
        int VertexCount;
        int FirstIndex;
        int IndexCount;
    };

    TPodVectorHeap< Float3 > Vertices;
    TPodVectorHeap< unsigned int > Indices;
    TPodVectorHeap< SSubpart > Subparts;
    BvAxisAlignedBox BoundingBox;

    /** Initialize collision triangle soup from indexed mesh */
    void Initialize( float const * _Vertices, int _VertexStride, int _VertexCount, unsigned int const * _Indices, int _IndexCount, AIndexedMeshSubpart * const * _Subparts, int _SubpartsCount );

    /** Initialize collision triangle soup from subparts */
    void Initialize( float const * _Vertices, int _VertexStride, int _VertexCount, unsigned int const * _Indices, int _IndexCount, SSubpart const * _Subparts, int _SubpartsCount, BvAxisAlignedBox const & _BoundingBox );

    /** Initialize collision triangle soup with single subpart */
    void Initialize( float const * _Vertices, int _VertexStride, int _VertexCount, unsigned int const * _Indices, int _IndexCount, BvAxisAlignedBox const & _BoundingBox );

protected:
    ACollisionTriangleSoupData() {
        BoundingBox.Clear();
    }
};

class ACollisionTriangleSoupBVHData : public ABaseObject {
    AN_CLASS( ACollisionTriangleSoupBVHData, ABaseObject )

public:
    TRef< ACollisionTriangleSoupData > TrisData;

    void BuildBVH( bool bForceQuantizedAabbCompression = false );
    //void BuildQuantizedBVH();

    //void Read( IBinaryStream & _Stream );
    //void Write( IBinaryStream & _Stream ) const;

    bool UsedQuantizedAabbCompression() const;

    class btBvhTriangleMeshShape * GetData() { return Data.GetObject(); }

protected:
    ACollisionTriangleSoupBVHData();
    ~ACollisionTriangleSoupBVHData();

private:
    TUniqueRef< btBvhTriangleMeshShape > Data; // TODO: Try btMultimaterialTriangleMeshShape
    TUniqueRef< struct btTriangleInfoMap > TriangleInfoMap;
    TUniqueRef< class AStridingMeshInterface > Interface;

    bool bUsedQuantizedAabbCompression = false;
};

// ACollisionTriangleSoupBVH can be used only for static or kinematic objects
class ACollisionTriangleSoupBVH : public ACollisionBody {
    AN_CLASS( ACollisionTriangleSoupBVH, ACollisionBody )

public:
    /** BVH data for static or kinematic objects */
    TRef< ACollisionTriangleSoupBVHData > BvhData;

    void GatherGeometry( TPodVectorHeap< Float3 > & _Vertices, TPodVectorHeap< unsigned int > & _Indices ) const override;

protected:
    ACollisionTriangleSoupBVH() {}

public:
    btCollisionShape * Create() override;
};

class ACollisionTriangleSoupGimpact : public ACollisionBody {
    AN_CLASS( ACollisionTriangleSoupGimpact, ACollisionBody )

public:
    TRef< ACollisionTriangleSoupData > TrisData;

    void GatherGeometry( TPodVectorHeap< Float3 > & _Vertices, TPodVectorHeap< unsigned int > & _Indices ) const override;

protected:
    ACollisionTriangleSoupGimpact();
    ~ACollisionTriangleSoupGimpact();

public:
    btCollisionShape * Create() override;

private:
    TUniqueRef< class AStridingMeshInterface > Interface;
};

struct SBoneCollision
{
    int JointIndex;

    int CollisionGroup;// = CM_WORLD;
    int CollisionMask;// = CM_ALL;

    TRef< ACollisionBody > CollisionBody;
};

class ACollisionModel : public ABaseObject // TODO: AResource
{
    AN_CLASS( ACollisionModel, ABaseObject )

public:
    ACollisionModel()
    {
        CenterOfMass.Clear();
    }

    ~ACollisionModel()
    {
        Clear();
    }

    void Clear()
    {
        for ( ACollisionBody * body : CollisionBodies ) {
            body->RemoveRef();
        }
        CollisionBodies.Clear();
        CenterOfMass.Clear();
        BoneCollisions.Clear();
    }

    template< typename T >
    T * CreateBody()
    {
        T * body = CreateInstanceOf< T >();
        CollisionBodies.Append( body );
        body->AddRef();
        return body;
    }

//    void AddBody( ACollisionBody * _Body )
//    {
//        AN_ASSERT( CollisionBodies.Find( _Body ) == CollisionBodies.End() );
//        CollisionBodies.Append( _Body );
//        _Body->AddRef();
//    }

//    void RemoveCollisionBody( ACollisionBody * _Body )
//    {
//        auto it = CollisionBodies.Find( _Body );
//        if ( it == CollisionBodies.End() ) {
//            return;
//        }
//        _Body->RemoveRef();
//        CollisionBodies.Erase( it );
//    }

//    void Duplicate( ACollisionModel * _CollisionModel ) const
//    {
//        _CollisionModel->Clear();
//        _CollisionModel->CollisionBodies = CollisionBodies;
//        for ( ACollisionBody * body : CollisionBodies ) {
//            body->AddRef();
//        }
//        _CollisionModel->CenterOfMass = CenterOfMass;
//    }

    void ComputeCenterOfMassAvg()
    {
        CenterOfMass.Clear();
        if ( !CollisionBodies.IsEmpty() ) {
            for ( ACollisionBody * body : CollisionBodies ) {
                CenterOfMass += body->Position;
            }
            CenterOfMass /= CollisionBodies.Size();
        }
    }

    void SetCenterOfMass( Float3 const & _CenterOfMass )
    {
        CenterOfMass = _CenterOfMass;
    }

    Float3 const & GetCenterOfMass() const
    {
        return CenterOfMass;
    }

    int NumCollisionBodies() const
    {
        return CollisionBodies.Size();
    }

    TPodVector< ACollisionBody *, 2 > const & GetCollisionBodies() const { return CollisionBodies; }

    template< typename T >
    T * CreateBoneCollision( int JointIndex, int CollisionGroup, int CollisionMask )
    {
        SBoneCollision boneCol;
        boneCol.JointIndex = JointIndex;
        boneCol.CollisionGroup = CollisionGroup;
        boneCol.CollisionMask = CollisionMask;
        boneCol.CollisionBody = CreateInstanceOf< T >();
        BoneCollisions.Append( boneCol );
        return static_cast< T * >( boneCol.CollisionBody.GetObject() );
    }

    TStdVector< SBoneCollision > const & GetBoneCollisions() const { return BoneCollisions; }

    void GatherGeometry( TPodVectorHeap< Float3 > & _Vertices, TPodVectorHeap< unsigned int > & _Indices ) const;

    void PerformConvexDecomposition( Float3 const * _Vertices,
                                     int _VerticesCount,
                                     int _VertexStride,
                                     unsigned int const * _Indices,
                                     int _IndicesCount );

    void PerformConvexDecompositionVHACD( Float3 const * _Vertices,
                                          int _VerticesCount,
                                          int _VertexStride,
                                          unsigned int const * _Indices,
                                          int _IndicesCount );

private:
    TPodVector< ACollisionBody *, 2 > CollisionBodies;
    TStdVector< SBoneCollision > BoneCollisions;
    Float3 CenterOfMass;
};

class ACollisionInstance : public ARefCounted
{
public:
    ACollisionInstance( ACollisionModel const * CollisionModel, Float3 const & Scale );
    ~ACollisionInstance();

    Float3 CalculateLocalInertia( float Mass ) const;

    Float3 const & GetCenterOfMass() const { return CenterOfMass; }

    void GetCollisionBodiesWorldBounds( Float3 const & WorldPosition, Quat const & WorldRotation, TPodVector< BvAxisAlignedBox > & _BoundingBoxes ) const;

    void GetCollisionWorldBounds( Float3 const & WorldPosition, Quat const & WorldRotation, BvAxisAlignedBox & _BoundingBox ) const;

    void GetCollisionBodyWorldBounds( int _Index, Float3 const & WorldPosition, Quat const & WorldRotation, BvAxisAlignedBox & _BoundingBox ) const;

    void GetCollisionBodyLocalBounds( int _Index, BvAxisAlignedBox & _BoundingBox ) const;

    float GetCollisionBodyMargin( int _Index ) const;

    int GetCollisionBodiesCount() const;

    btCollisionShape * GetCollisionShape() const { return CollisionShape; }

private:
    TUniqueRef< class btCompoundShape > CompoundShape;
    btCollisionShape * CollisionShape;
    Float3 CenterOfMass;
};


/*

Utilites


*/

struct SConvexHullDesc {
    int FirstVertex;
    int VertexCount;
    int FirstIndex;
    int IndexCount;
    Float3 Centroid;
};

void BakeCollisionMarginConvexHull( Float3 const * _InVertices, int _VertexCount, TPodVector< Float3 > & _OutVertices, float _Margin = 0.01f );

void PerformConvexDecomposition( Float3 const * _Vertices,
                                 int _VerticesCount,
                                 int _VertexStride,
                                 unsigned int const * _Indices,
                                 int _IndicesCount,
                                 TPodVector< Float3 > & _OutVertices,
                                 TPodVector< unsigned int > & _OutIndices,
                                 TPodVector< SConvexHullDesc > & _OutHulls );

void PerformConvexDecompositionVHACD( Float3 const * _Vertices,
                                      int _VerticesCount,
                                      int _VertexStride,
                                      unsigned int const * _Indices,
                                      int _IndicesCount,
                                      TPodVector< Float3 > & _OutVertices,
                                      TPodVector< unsigned int > & _OutIndices,
                                      TPodVector< SConvexHullDesc > & _OutHulls,
                                      Float3 & _CenterOfMass );

void ConvexHullVerticesFromPlanes( PlaneF const * _Planes, int _NumPlanes, TPodVector< Float3 > & _Vertices );
