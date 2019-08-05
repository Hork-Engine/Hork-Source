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


#include <Engine/World/Public/BSP.h>

static int DrawSurfMarker = 0;

FBinarySpaceData::FBinarySpaceData() {
    Visdata = nullptr;

    NumVisSurfs = 0;
    ViewLeafCluster = -1;
}

FBinarySpaceData::~FBinarySpaceData() {
    DeallocateBufferData( Visdata );
}

int FBinarySpaceData::FindLeaf( const Float3 & _Position ) {
    FBinarySpaceNode * node;
    float d;
    FBinarySpacePlane * plane;
    int nodeIndex;

    if ( !Nodes.Size() ) {
        GLogger.Printf( "FBinarySpaceData::FindLeaf: no nodes\n" );
        return -1;
    }

    node = Nodes.ToPtr();
    while ( 1 ) {
        plane = node->Plane;
        if ( plane->Type < 3 ) {
            d = _Position[ plane->Type ] * plane->Normal[ plane->Type ] + plane->D;
        } else {
            d = FMath::Dot( _Position, plane->Normal ) + plane->D;
        }

        nodeIndex = node->ChildrenIdx[ ( d <= 0 ) ];
        if ( nodeIndex == 0 ) {
            // solid
            return -1;
        }

        if ( nodeIndex < 0 ) {
            return -1 - nodeIndex;
        }

        node = Nodes.ToPtr() + nodeIndex;
    }
    return -1;
}

#define MAX_MAP_LEAFS 0x20000

class FEmptyVisData {
public:
    FEmptyVisData();

    byte Data[ MAX_MAP_LEAFS / 8 ];
};

FEmptyVisData::FEmptyVisData() {
    memset( Data, 0xff, sizeof( Data ) );
}

static FEmptyVisData EmptyVis;

byte const * FBinarySpaceData::DecompressVisdata( byte const * _Data ) {
    static byte Decompressed[ MAX_MAP_LEAFS / 8 ];
    int c;
    byte *out;
    int row;

    row = ( Leafs.Size() + 7 ) >> 3;
    out = Decompressed;

    if ( !_Data ) { // no vis info, so make all visible
        while ( row ) {
            *out++ = 0xff;
            row--;
        }
        return Decompressed;
    }

    do {
        if ( *_Data ) {
            *out++ = *_Data++;
            continue;
        }

        c = _Data[ 1 ];
        _Data += 2;
        while ( c ) {
            *out++ = 0;
            c--;
        }
    } while ( out - Decompressed < row );

    return Decompressed;
}

byte const * FBinarySpaceData::LeafPVS( FBinarySpaceLeaf const * _Leaf ) {
    if ( bCompressedVisData ) {
        if (_Leaf == Leafs.ToPtr() ) {
            return EmptyVis.Data;
        }
        return DecompressVisdata( _Leaf->Visdata );
    } else {
        if ( !_Leaf->Visdata ) {
            return EmptyVis.Data;
        }
        return _Leaf->Visdata;
    }
}

int FBinarySpaceData::MarkLeafs( int _ViewLeaf ) {
    //ViewLeaf = _ViewLeaf;

    if ( _ViewLeaf < 0 ) {
        return VisFrameCount;
    }

    FBinarySpaceLeaf * viewLeaf = &Leafs[ _ViewLeaf ];

    if ( ViewLeafCluster == viewLeaf->Cluster ) {
        return VisFrameCount;
    }

    VisFrameCount++;
    ViewLeafCluster = viewLeaf->Cluster;

    byte const * vis = LeafPVS( viewLeaf );

    int numLeafs = Leafs.Size();

    int cluster;
    for ( int i = 0 ; i < numLeafs ; i++ ) {
        FBinarySpaceLeaf * Leaf = &Leafs[ i ];

        cluster = Leaf->Cluster;
        if ( cluster < 0 || cluster >= NumVisClusters ) {
            continue;
        }

        if ( !( vis[ cluster >> 3 ] & ( 1 << ( cluster & 7 ) ) ) ) {
            continue;
        }

        // TODO: check for door connection here

        FBinarySpaceNode * parent = ( FBinarySpaceNode* )Leaf;
        do {
            if ( parent->VisFrame == VisFrameCount ) {
                break;
            }
            parent->VisFrame = VisFrameCount;
            parent = parent->Parent;
        } while ( parent );
    }

    return VisFrameCount;
}

static constexpr int CullIndices[ 8 ][ 6 ] = {
        { 0, 4, 5, 3, 1, 2 },
        { 3, 4, 5, 0, 1, 2 },
        { 0, 1, 5, 3, 4, 2 },
        { 3, 1, 5, 0, 4, 2 },
        { 0, 4, 2, 3, 1, 5 },
        { 3, 4, 2, 0, 1, 5 },
        { 0, 1, 2, 3, 4, 5 },
        { 3, 1, 2, 0, 4, 5 }
};

static bool CullNode( BvFrustum const & Frustum, BvAxisAlignedBox const & _Bounds, int & _CullBits ) {
    Float3 p;

    Float const * pBounds = _Bounds.ToPtr();
    int const * pIndices;
    if ( _CullBits & 1 ) {
        pIndices = CullIndices[ Frustum[0].CachedSignBits ];

        p[ 0 ] = pBounds[ pIndices[ 0 ] ];
        p[ 1 ] = pBounds[ pIndices[ 1 ] ];
        p[ 2 ] = pBounds[ pIndices[ 2 ] ];

        if ( FMath::Dot( p, Frustum[ 0 ].Normal ) <= -Frustum[ 0 ].D ) {
            return true;
        }

        p[ 0 ] = pBounds[ pIndices[ 3 ] ];
        p[ 1 ] = pBounds[ pIndices[ 4 ] ];
        p[ 2 ] = pBounds[ pIndices[ 5 ] ];

        if ( FMath::Dot( p, Frustum[ 0 ].Normal ) >= -Frustum[ 0 ].D ) {
            _CullBits &= ~1;
        }
    }

    if ( _CullBits & 2 ) {
        pIndices = CullIndices[ Frustum[1].CachedSignBits ];

        p[ 0 ] = pBounds[ pIndices[ 0 ] ];
        p[ 1 ] = pBounds[ pIndices[ 1 ] ];
        p[ 2 ] = pBounds[ pIndices[ 2 ] ];

        if ( FMath::Dot( p, Frustum[ 1 ].Normal ) <= -Frustum[ 1 ].D ) {
            return true;
        }

        p[ 0 ] = pBounds[ pIndices[ 3 ] ];
        p[ 1 ] = pBounds[ pIndices[ 4 ] ];
        p[ 2 ] = pBounds[ pIndices[ 5 ] ];

        if ( FMath::Dot( p, Frustum[ 1 ].Normal ) >= -Frustum[ 1 ].D ) {
            _CullBits &= ~2;
        }
    }

    if ( _CullBits & 4 ) {
        pIndices = CullIndices[ Frustum[2].CachedSignBits ];

        p[ 0 ] = pBounds[ pIndices[ 0 ] ];
        p[ 1 ] = pBounds[ pIndices[ 1 ] ];
        p[ 2 ] = pBounds[ pIndices[ 2 ] ];

        if ( FMath::Dot( p, Frustum[ 2 ].Normal ) <= -Frustum[ 2 ].D ) {
            return true;
        }

        p[ 0 ] = pBounds[ pIndices[ 3 ] ];
        p[ 1 ] = pBounds[ pIndices[ 4 ] ];
        p[ 2 ] = pBounds[ pIndices[ 5 ] ];

        if ( FMath::Dot( p, Frustum[ 2 ].Normal ) >= -Frustum[ 2 ].D ) {
            _CullBits &= ~4;
        }
    }

    if ( _CullBits & 8 ) {
        pIndices = CullIndices[ Frustum[3].CachedSignBits ];

        p[ 0 ] = pBounds[ pIndices[ 0 ] ];
        p[ 1 ] = pBounds[ pIndices[ 1 ] ];
        p[ 2 ] = pBounds[ pIndices[ 2 ] ];

        if ( FMath::Dot( p, Frustum[ 3 ].Normal ) <= -Frustum[ 3 ].D ) {
            return true;
        }

        p[ 0 ] = pBounds[ pIndices[ 3 ] ];
        p[ 1 ] = pBounds[ pIndices[ 4 ] ];
        p[ 2 ] = pBounds[ pIndices[ 5 ] ];

        if ( FMath::Dot( p, Frustum[ 3 ].Normal ) >= -Frustum[ 3 ].D ) {
            _CullBits &= ~8;
        }
    }

    return false;
}

void FBinarySpaceData::PerformVSD( Float3 const & _ViewOrigin, BvFrustum const & _Frustum, bool _SortLightmapGroup ) {

    ++DrawSurfMarker;

    ViewOrigin = _ViewOrigin;
    Frustum = &_Frustum;

    VisSurfs.ResizeInvalidate( Surfaces.Size() );

    NumVisSurfs = 0;

    int Leaf = FindLeaf( ViewOrigin );

    VisFrame = MarkLeafs( Leaf );

    Traverse_r( 0, 0xf );

    struct FSortFunction {
        bool operator() ( FSurfaceDef const * _A, FSurfaceDef const * _B ) {
            return ( _A->LightmapGroup < _B->LightmapGroup );
        }
    } SortFunction;

    if ( _SortLightmapGroup && NumVisSurfs > 0 ) {
        StdSort( &VisSurfs[0], &VisSurfs[NumVisSurfs], SortFunction );
    }
}

void FBinarySpaceData::Traverse_r( int _NodeIndex, int _CullBits ) {
    int * mark;
    FBinarySpaceLeaf const * pleaf;
    int Count;
    FSurfaceDef * Surf;
    FNodeBase const * Node;

    while ( 1 ) {
        if ( _NodeIndex < 0 ) {
            Node = &Leafs[ -1 - _NodeIndex ]; //SpatialTree->Nodes.ToPtr() + ( -1 - _NodeIndex );
        } else {
            Node = Nodes.ToPtr() + _NodeIndex;
        }

        if ( Node->VisFrame != VisFrame )
            return;

        if ( CullNode( *Frustum, Node->Bounds, _CullBits ) ) {
            //TotalCulled++;
            return;
        }

        //if ( !Frustum.CheckAABB2( _Node->Bounds ) ) {
        //    CullMiss++;
        //}

        //if ( _Node->Contents < 0 ) {
        //	break; // leaf
        //}

        if ( _NodeIndex < 0 ) {
            // leaf
            break;
        }

        if ( _NodeIndex == 0 ) {
            // solid
            // FIXME: what to do?
            //return;
        }

        Traverse_r( ((FBinarySpaceNode *)Node)->ChildrenIdx[0], _CullBits );

        _NodeIndex = ((FBinarySpaceNode *)Node)->ChildrenIdx[1];
    }

    pleaf = ( FBinarySpaceLeaf const * )Node;

    mark = &Marksurfaces[ pleaf->FirstSurface ];

    Count = pleaf->NumSurfaces;
    while ( Count-- ) {

        Surf = &Surfaces[ *mark ];

//        Renderable = Surf;

        if ( Surf->Marker != DrawSurfMarker ) {
            Surf->Marker = DrawSurfMarker;

//        if ( Renderable->VisFrame[ RenderTargetStack ] != Proxy->DrawSerialIndex ) {
//            Renderable->VisFrame[ RenderTargetStack ] = Proxy->DrawSerialIndex;

            bool FaceCull = false;
            //if ( r_faceCull.GetBool() ) {
                const bool TwoSided = false;
                const bool FrontSided = true;
                const float EPS = 0.25f;
                switch ( Surf->Type ) {
                    case SURF_PLANAR:
                    {
                        if ( !TwoSided ) {
                            const PlaneF & Plane = Surf->Plane;
                            float d = FMath::Dot( ViewOrigin, Plane.Normal );

                            if ( FrontSided ) {
                                if ( d < -Plane.D - EPS ) {
                                    FaceCull = true;
                                }
                            } else {
                                if ( d > -Plane.D + EPS ) {
                                    FaceCull = true;
                                }
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
            //}

            if ( !FaceCull ) {
                VisSurfs[NumVisSurfs++] = Surf;
            }
        } else {
            //GLogger.Printf( "Skip\n" );
        }
        mark++;
    }

    // TODO: Mark objects in leaf to render
}
