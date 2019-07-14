/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2018 Alexander Samusev.

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
#include <Engine/Core/Public/BV/BvAxisAlignedBox.h>

class btCollisionShape;
class btCompoundShape;
class btBvhTriangleMeshShape;
struct btTriangleInfoMap;
class FCollisionBodyComposition;
struct FSubpart;

class FCollisionBody : public FBaseObject {
    AN_CLASS( FCollisionBody, FBaseObject )

    friend class FWorld;
    friend void CreateCollisionShape( FCollisionBodyComposition const & BodyComposition, Float3 const & _Scale, btCompoundShape ** _CompoundShape, Float3 * _CenterOfMass );

public:
    Float3 Position;
    Quat Rotation;
    float Margin;

    enum { AXIAL_X, AXIAL_Y, AXIAL_Z, AXIAL_DEFAULT = AXIAL_Y };

    virtual bool IsConvex() const { return false; }

    virtual void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const {}

protected:
    FCollisionBody() {
        Rotation = Quat::Identity();
        Margin = 0.01f;
    }

    // Only FWorld and CreateCollisionShape can call Create()
    virtual btCollisionShape * Create() { AN_Assert( 0 ); return nullptr; }
};

class FCollisionSphere : public FCollisionBody {
    AN_CLASS( FCollisionSphere, FCollisionBody )

public:
    float Radius = 0.5f;
    bool bProportionalScale = true;

    bool IsConvex() const override { return true; }

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    FCollisionSphere() {}

private:
    btCollisionShape * Create() override;
};

class FCollisionSphereRadii : public FCollisionBody {
    AN_CLASS( FCollisionSphereRadii, FCollisionBody )

public:
    Float3 Radius = Float3(0.5f);

    bool IsConvex() const override { return true; }

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    FCollisionSphereRadii() {}

private:
    btCollisionShape * Create() override;
};

class FCollisionBox : public FCollisionBody {
    AN_CLASS( FCollisionBox, FCollisionBody )

public:
    Float3 HalfExtents = Float3(0.5f);

    bool IsConvex() const override { return true; }

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    FCollisionBox() {}

private:
    btCollisionShape * Create() override;
};

class FCollisionCylinder : public FCollisionBody {
    AN_CLASS( FCollisionCylinder, FCollisionBody )

public:
    Float3 HalfExtents = Float3(1.0f);
    int Axial = AXIAL_DEFAULT;

    bool IsConvex() const override { return true; }

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    FCollisionCylinder() {}

private:
    btCollisionShape * Create() override;
};

class FCollisionCone : public FCollisionBody {
    AN_CLASS( FCollisionCone, FCollisionBody )

public:
    float Radius = 1;
    float Height = 1;
    int Axial = AXIAL_DEFAULT;

    bool IsConvex() const override { return true; }

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    FCollisionCone() {}

private:
    btCollisionShape * Create() override;
};

class FCollisionCapsule : public FCollisionBody {
    AN_CLASS( FCollisionCapsule, FCollisionBody )

public:

    // Radius of the capsule. The total height is Height + 2 * Radius
    float Radius = 1;

    // Height between the center of each sphere of the capsule caps
    float Height = 1;

    int Axial = AXIAL_DEFAULT;

    float GetTotalHeight() const { return Height + 2 * Radius; }

    bool IsConvex() const override { return true; }

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    FCollisionCapsule() {}

private:
    btCollisionShape * Create() override;
};

void ConvexHullVerticesFromPlanes( PlaneF const * _Planes, int _NumPlanes, TPodArray< Float3 > & _Vertices );

//class FCollisionConvexHull : public FCollisionBody {
//    AN_CLASS( FCollisionConvexHull, FCollisionBody )

//public:
//    TPodArray< Float3 > Vertices;

//    void InitializeFromPlanes( PlaneF const * _Planes, int _NumPlanes ) {
//        Vertices.Clear();
//        ConvexHullVerticesFromPlanes( _Planes, _NumPlanes, Vertices );
//    }

//    bool IsConvex() const override { return true; }

//    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

//protected:
//    FCollisionConvexHull() {}

//private:
//    btCollisionShape * Create() override;
//};

class FCollisionConvexHullData : public FBaseObject {
    AN_CLASS( FCollisionConvexHullData, FBaseObject )

    friend class FCollisionConvexHull;

public:

    void Initialize( Float3 const * _Vertices, int _VertexCount, unsigned int * _Indices, int _IndexCount );

    Float3 const * GetVertices() const { return Vertices.ToPtr(); }
    int GetVertexCount() const { return Vertices.Length(); }

    unsigned int const * GetIndices() const { return Indices.ToPtr(); }
    int GetIndexCount() const { return Indices.Length(); }

//    void InitializeFromPlanes( PlaneF const * _Planes, int _NumPlanes ) {
//        Vertices.Clear();
//        ConvexHullVerticesFromPlanes( _Planes, _NumPlanes, Vertices );
//    }

protected:
    FCollisionConvexHullData();
    ~FCollisionConvexHullData();

    TPodArray< Float3 > Vertices;
    TPodArray< unsigned int > Indices;
    class btVector3 * Data;
};

class FCollisionConvexHull : public FCollisionBody {
    AN_CLASS( FCollisionConvexHull, FCollisionBody )

public:
    TRefHolder< FCollisionConvexHullData > HullData;

    bool IsConvex() const override { return true; }

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    FCollisionConvexHull() {}

private:
    btCollisionShape * Create() override;
};

class FCollisionTriangleSoupData : public FBaseObject {
    AN_CLASS( FCollisionTriangleSoupData, FBaseObject )

public:
    struct FSubpart {
        int BaseVertex;
        int VertexCount;
        int FirstIndex;
        int IndexCount;
    };

    TPodArray< Float3 > Vertices;
    TPodArray< unsigned int > Indices;
    TPodArray< FSubpart > Subparts;
    BvAxisAlignedBox BoundingBox;

    void Initialize( float const * _Vertices, int _VertexStride, int _VertexCount, unsigned int const * _Indices, int _IndexCount, ::FSubpart const * _Subparts, int _SubpartsCount );
    void Initialize( float const * _Vertices, int _VertexStride, int _VertexCount, unsigned int const * _Indices, int _IndexCount, BvAxisAlignedBox const & _BoundingBox );

protected:
    FCollisionTriangleSoupData() {}
};

class FCollisionTriangleSoupBVHData : public FBaseObject {
    AN_CLASS( FCollisionTriangleSoupBVHData, FBaseObject )

public:
    TRefHolder< FCollisionTriangleSoupData > TrisData;

    void BuildBVH( bool bForceQuantizedAabbCompression = false );
    //void BuildQuantizedBVH();

    bool UsedQuantizedAabbCompression() const;

    btBvhTriangleMeshShape * GetData() { return Data; }

protected:
    FCollisionTriangleSoupBVHData();
    ~FCollisionTriangleSoupBVHData();

private:
    btBvhTriangleMeshShape * Data; // TODO: Try btMultimaterialTriangleMeshShape
    btTriangleInfoMap * TriangleInfoMap;
 
    class FStridingMeshInterface * Interface;

    bool bUsedQuantizedAabbCompression;
};

// FCollisionTriangleSoupBVH can be used only for static or kinematic objects
class FCollisionTriangleSoupBVH : public FCollisionBody {
    AN_CLASS( FCollisionTriangleSoupBVH, FCollisionBody )

public:
    // BVH data for static or kinematic objects
    TRefHolder< FCollisionTriangleSoupBVHData > BvhData;

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    FCollisionTriangleSoupBVH() {}

private:
    btCollisionShape * Create() override;
};

class FCollisionTriangleSoupGimpact : public FCollisionBody {
    AN_CLASS( FCollisionTriangleSoupGimpact, FCollisionBody )

public:
    TRefHolder< FCollisionTriangleSoupData > TrisData;

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    FCollisionTriangleSoupGimpact();
    ~FCollisionTriangleSoupGimpact();

private:
    btCollisionShape * Create() override;

    class FStridingMeshInterface * Interface;
};

class FCollisionBodyComposition {
    AN_FORBID_COPY( FCollisionBodyComposition )

public:

    FCollisionBodyComposition() {
        CenterOfMass.Clear();
    }

    ~FCollisionBodyComposition() {
        Clear();
    }

    void Clear() {
        for ( FCollisionBody * body : CollisionBodies ) {
            body->RemoveRef();
        }
        CollisionBodies.Clear();
        CenterOfMass.Clear();
    }

    template< typename T >
    T * NewCollisionBody() {
        T * body = static_cast< T * >( T::ClassMeta().CreateInstance() );
        AddCollisionBody( body );
        return body;
    }

    void AddCollisionBody( FCollisionBody * _Body ) {
        AN_Assert( CollisionBodies.Find( _Body ) == CollisionBodies.End() );
        CollisionBodies.Append( _Body );
        _Body->AddRef();
    }

    void RemoveCollisionBody( FCollisionBody * _Body ) {
        auto it = CollisionBodies.Find( _Body );
        if ( it == CollisionBodies.End() ) {
            return;
        }
        _Body->RemoveRef();
        CollisionBodies.Erase( it );
    }

    void Duplicate( FCollisionBodyComposition & _Composition ) const {
        _Composition.Clear();
        _Composition.CollisionBodies = CollisionBodies;
        for ( FCollisionBody * body : CollisionBodies ) {
            body->AddRef();
        }
        _Composition.CenterOfMass = CenterOfMass;
    }

    void ComputeCenterOfMassAvg() {
        CenterOfMass.Clear();
        if ( !CollisionBodies.IsEmpty() ) {
            for ( FCollisionBody * body : CollisionBodies ) {
                CenterOfMass += body->Position;
            }
            CenterOfMass /= CollisionBodies.Length();
        }
    }

    int NumCollisionBodies() const { return CollisionBodies.Length(); }

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const;

    TPodArray< FCollisionBody *, 2 > CollisionBodies;
    Float3 CenterOfMass;
};

struct FConvexHullDesc {
    int FirstVertex;
    int VertexCount;
    int FirstIndex;
    int IndexCount;
    Float3 Centroid;
};

void BakeCollisionMarginConvexHull( Float3 const * _InVertices, int _VertexCount, TPodArray< Float3 > & _OutVertices, float _Margin = 0.01f );

void PerformConvexDecomposition( Float3 const * _Vertices,
                                 int _VerticesCount,
                                 int _VertexStride,
                                 unsigned int const * _Indices,
                                 int _IndicesCount,
                                 TPodArray< Float3 > & _OutVertices,
                                 TPodArray< unsigned int > & _OutIndices,
                                 TPodArray< FConvexHullDesc > & _OutHulls );

void PerformConvexDecomposition( Float3 const * _Vertices,
                                 int _VerticesCount,
                                 int _VertexStride,
                                 unsigned int const * _Indices,
                                 int _IndicesCount,
                                 FCollisionBodyComposition & _BodyComposition );

void PerformConvexDecompositionVHACD( Float3 const * _Vertices,
                                      int _VerticesCount,
                                      int _VertexStride,
                                      unsigned int const * _Indices,
                                      int _IndicesCount,
                                      TPodArray< Float3 > & _OutVertices,
                                      TPodArray< unsigned int > & _OutIndices,
                                      TPodArray< FConvexHullDesc > & _OutHulls,
                                      Float3 & _CenterOfMass );
