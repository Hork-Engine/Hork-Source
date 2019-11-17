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

#include <Engine/Base/Public/BaseObject.h>
#include <Engine/Core/Public/BV/BvAxisAlignedBox.h>

class btCollisionShape;
class btCompoundShape;
class btBvhTriangleMeshShape;
struct btTriangleInfoMap;
class ACollisionBodyComposition;
class AIndexedMeshSubpart;

class ACollisionBody : public ABaseObject {
    AN_CLASS( ACollisionBody, ABaseObject )

    friend struct AWorldCollisionQuery;
    friend void CreateCollisionShape( ACollisionBodyComposition const & BodyComposition, Float3 const & _Scale, btCompoundShape ** _CompoundShape, Float3 * _CenterOfMass );

public:
    Float3 Position;
    Quat Rotation;
    float Margin;

    enum { AXIAL_X, AXIAL_Y, AXIAL_Z, AXIAL_DEFAULT = AXIAL_Y };

    virtual bool IsConvex() const { return false; }

    virtual void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const {}

protected:
    ACollisionBody() {
        Rotation = Quat::Identity();
        Margin = 0.01f;
    }

    // Only AWorldCollisionQuery and CreateCollisionShape can call Create()
    virtual btCollisionShape * Create() { AN_Assert( 0 ); return nullptr; }
};

class ACollisionSphere : public ACollisionBody {
    AN_CLASS( ACollisionSphere, ACollisionBody )

public:
    float Radius = 0.5f;
    bool bProportionalScale = true;

    bool IsConvex() const override { return true; }

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    ACollisionSphere() {}

private:
    btCollisionShape * Create() override;
};

class ACollisionSphereRadii : public ACollisionBody {
    AN_CLASS( ACollisionSphereRadii, ACollisionBody )

public:
    Float3 Radius = Float3(0.5f);

    bool IsConvex() const override { return true; }

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    ACollisionSphereRadii() {}

private:
    btCollisionShape * Create() override;
};

class ACollisionBox : public ACollisionBody {
    AN_CLASS( ACollisionBox, ACollisionBody )

public:
    Float3 HalfExtents = Float3(0.5f);

    bool IsConvex() const override { return true; }

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    ACollisionBox() {}

private:
    btCollisionShape * Create() override;
};

class ACollisionCylinder : public ACollisionBody {
    AN_CLASS( ACollisionCylinder, ACollisionBody )

public:
    Float3 HalfExtents = Float3(1.0f);
    int Axial = AXIAL_DEFAULT;

    bool IsConvex() const override { return true; }

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    ACollisionCylinder() {}

private:
    btCollisionShape * Create() override;
};

class ACollisionCone : public ACollisionBody {
    AN_CLASS( ACollisionCone, ACollisionBody )

public:
    float Radius = 1;
    float Height = 1;
    int Axial = AXIAL_DEFAULT;

    bool IsConvex() const override { return true; }

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    ACollisionCone() {}

private:
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

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    ACollisionCapsule() {}

private:
    btCollisionShape * Create() override;
};

//class ACollisionConvexHull : public ACollisionBody {
//    AN_CLASS( ACollisionConvexHull, ACollisionBody )

//public:
//    TPodArray< Float3 > Vertices;

//    void InitializeFromPlanes( PlaneF const * _Planes, int _NumPlanes ) {
//        Vertices.Clear();
//        ConvexHullVerticesFromPlanes( _Planes, _NumPlanes, Vertices );
//    }

//    bool IsConvex() const override { return true; }

//    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

//protected:
//    ACollisionConvexHull() {}

//private:
//    btCollisionShape * Create() override;
//};

class ACollisionConvexHullData : public ABaseObject {
    AN_CLASS( ACollisionConvexHullData, ABaseObject )

    friend class ACollisionConvexHull;

public:

    void Initialize( Float3 const * _Vertices, int _VertexCount, unsigned int * _Indices, int _IndexCount );

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

    TPodArray< Float3 > Vertices;
    TPodArray< unsigned int > Indices;
    class btVector3 * Data;
};

class ACollisionConvexHull : public ACollisionBody {
    AN_CLASS( ACollisionConvexHull, ACollisionBody )

public:
    TRef< ACollisionConvexHullData > HullData;

    bool IsConvex() const override { return true; }

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    ACollisionConvexHull() {}

private:
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

    TPodArray< Float3 > Vertices;
    TPodArray< unsigned int > Indices;
    TPodArray< SSubpart > Subparts;
    BvAxisAlignedBox BoundingBox;

    /** Initialize collision triangle soup from indexed mesh */
    void Initialize( float const * _Vertices, int _VertexStride, int _VertexCount, unsigned int const * _Indices, int _IndexCount, AIndexedMeshSubpart * const * _Subparts, int _SubpartsCount );

    /** Initialize collision triangle soup from subparts */
    void Initialize( float const * _Vertices, int _VertexStride, int _VertexCount, unsigned int const * _Indices, int _IndexCount, SSubpart const * _Subparts, int _SubpartsCount, BvAxisAlignedBox const & _BoundingBox );

    /** Initialize collision triangle soup with single subpart */
    void Initialize( float const * _Vertices, int _VertexStride, int _VertexCount, unsigned int const * _Indices, int _IndexCount, BvAxisAlignedBox const & _BoundingBox );

protected:
    ACollisionTriangleSoupData() {}
};

class ACollisionTriangleSoupBVHData : public ABaseObject {
    AN_CLASS( ACollisionTriangleSoupBVHData, ABaseObject )

public:
    TRef< ACollisionTriangleSoupData > TrisData;

    void BuildBVH( bool bForceQuantizedAabbCompression = false );
    //void BuildQuantizedBVH();

    //void Read( IStreamBase & _Stream );
    //void Write( IStreamBase & _Stream ) const;

    bool UsedQuantizedAabbCompression() const;

    btBvhTriangleMeshShape * GetData() { return Data; }

protected:
    ACollisionTriangleSoupBVHData();
    ~ACollisionTriangleSoupBVHData();

private:
    btBvhTriangleMeshShape * Data; // TODO: Try btMultimaterialTriangleMeshShape
    btTriangleInfoMap * TriangleInfoMap;
 
    class AStridingMeshInterface * Interface;

    bool bUsedQuantizedAabbCompression;
};

// ACollisionTriangleSoupBVH can be used only for static or kinematic objects
class ACollisionTriangleSoupBVH : public ACollisionBody {
    AN_CLASS( ACollisionTriangleSoupBVH, ACollisionBody )

public:
    /** BVH data for static or kinematic objects */
    TRef< ACollisionTriangleSoupBVHData > BvhData;

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    ACollisionTriangleSoupBVH() {}

private:
    btCollisionShape * Create() override;
};

class ACollisionTriangleSoupGimpact : public ACollisionBody {
    AN_CLASS( ACollisionTriangleSoupGimpact, ACollisionBody )

public:
    TRef< ACollisionTriangleSoupData > TrisData;

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const override;

protected:
    ACollisionTriangleSoupGimpact();
    ~ACollisionTriangleSoupGimpact();

private:
    btCollisionShape * Create() override;

    class AStridingMeshInterface * Interface;
};

class ACollisionBodyComposition {
    AN_FORBID_COPY( ACollisionBodyComposition )

public:

    ACollisionBodyComposition() {
        CenterOfMass.Clear();
    }

    ~ACollisionBodyComposition() {
        Clear();
    }

    void Clear() {
        for ( ACollisionBody * body : CollisionBodies ) {
            body->RemoveRef();
        }
        CollisionBodies.Clear();
        CenterOfMass.Clear();
    }

    template< typename T >
    T * AddCollisionBody() {
        T * body = CreateInstanceOf< T >();
        AddCollisionBody( body );
        return body;
    }

    void AddCollisionBody( ACollisionBody * _Body ) {
        AN_Assert( CollisionBodies.Find( _Body ) == CollisionBodies.End() );
        CollisionBodies.Append( _Body );
        _Body->AddRef();
    }

    void RemoveCollisionBody( ACollisionBody * _Body ) {
        auto it = CollisionBodies.Find( _Body );
        if ( it == CollisionBodies.End() ) {
            return;
        }
        _Body->RemoveRef();
        CollisionBodies.Erase( it );
    }

    void Duplicate( ACollisionBodyComposition & _Composition ) const {
        _Composition.Clear();
        _Composition.CollisionBodies = CollisionBodies;
        for ( ACollisionBody * body : CollisionBodies ) {
            body->AddRef();
        }
        _Composition.CenterOfMass = CenterOfMass;
    }

    void ComputeCenterOfMassAvg() {
        CenterOfMass.Clear();
        if ( !CollisionBodies.IsEmpty() ) {
            for ( ACollisionBody * body : CollisionBodies ) {
                CenterOfMass += body->Position;
            }
            CenterOfMass /= CollisionBodies.Size();
        }
    }

    int NumCollisionBodies() const { return CollisionBodies.Size(); }

    void CreateGeometry( TPodArray< Float3 > & _Vertices, TPodArray< unsigned int > & _Indices ) const;

    TPodArray< ACollisionBody *, 2 > CollisionBodies;
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

void BakeCollisionMarginConvexHull( Float3 const * _InVertices, int _VertexCount, TPodArray< Float3 > & _OutVertices, float _Margin = 0.01f );

void PerformConvexDecomposition( Float3 const * _Vertices,
                                 int _VerticesCount,
                                 int _VertexStride,
                                 unsigned int const * _Indices,
                                 int _IndicesCount,
                                 TPodArray< Float3 > & _OutVertices,
                                 TPodArray< unsigned int > & _OutIndices,
                                 TPodArray< SConvexHullDesc > & _OutHulls );

void PerformConvexDecomposition( Float3 const * _Vertices,
                                 int _VerticesCount,
                                 int _VertexStride,
                                 unsigned int const * _Indices,
                                 int _IndicesCount,
                                 ACollisionBodyComposition & _BodyComposition );

void PerformConvexDecompositionVHACD( Float3 const * _Vertices,
                                      int _VerticesCount,
                                      int _VertexStride,
                                      unsigned int const * _Indices,
                                      int _IndicesCount,
                                      TPodArray< Float3 > & _OutVertices,
                                      TPodArray< unsigned int > & _OutIndices,
                                      TPodArray< SConvexHullDesc > & _OutHulls,
                                      Float3 & _CenterOfMass );

void ConvexHullVerticesFromPlanes( PlaneF const * _Planes, int _NumPlanes, TPodArray< Float3 > & _Vertices );

void CreateCollisionShape( ACollisionBodyComposition const & BodyComposition, Float3 const & _Scale, btCompoundShape ** _CompoundShape, Float3 * _CenterOfMass );

void DestroyCollisionShape( btCompoundShape * _CompoundShape );
