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


#include <Engine/World/Public/Octree.h>
#include <Engine/World/Public/Level.h>
#include <Engine/Core/Public/Logger.h>

AN_CLASS_META( FSpatialTree )
AN_CLASS_META( FOctree )

void FOctree::Build() {
    Purge();

    BvAxisAlignedBox const & boundingBox = Owner->GetBoundingBox();

    if ( boundingBox.IsEmpty() ) {
        GLogger.Printf( "Octree: invalid bounding box\n" );
        return;
    }

    float width = boundingBox.Maxs[0] - boundingBox.Mins[0];
    float height = boundingBox.Maxs[1] - boundingBox.Mins[1];
    float depth = boundingBox.Maxs[2] - boundingBox.Mins[2];
    float maxDim = FMath::Max( width, height, depth );

    NumLevels = FMath::Log2( FMath::ToIntFast( maxDim + 0.5f ) ) + 1;

    GLogger.Printf( "Max levels %d\n", NumLevels );

//        GLogger.Printf( "Octree levels %d\n", NumLevels );

//        int numNodes = 0;
//        for ( int i = 0 ; i < NumLevels ; i++ ) {
//            int n = 1 << i;
//            numNodes += n*n*n;
//        }

//        Nodes = GZoneMemory.Alloc( NumLevels, 1 );

//        Root = Nodes[0];
//        Root->BoundingBox = boundingBox;
}

void FOctree::TreeAddObject( FSpatialObject * _Object ) {
    ObjectsInTree.Append( _Object );
    _Object->AddRef();

    //BvAxisAlignedBox const & bounds = _Object->GetWorldBounds();
}

void FOctree::TreeRemoveObject( int _Index ) {
    ObjectsInTree[ _Index ]->RemoveRef();
    ObjectsInTree.RemoveSwap( _Index );
}

void FOctree::TreeUpdateObject( int _Index ) {

}

void FOctree::Update() {
    for ( FPendingObjectInfo & info : PendingObjects ) {

        int i = ObjectsInTree.IndexOf( info.Object );

        switch ( info.PendingOp ) {
        case FPendingObjectInfo::PENDING_ADD:
            if ( i != -1 ) {
                // object already in tree, just update it

                TreeUpdateObject( i );
            } else {
                // add object to tree nodes

                TreeAddObject( info.Object );
            }
            break;
        case FPendingObjectInfo::PENDING_UPDATE:
            if ( i != -1 ) {
                // just update it

                TreeUpdateObject( i );
            } else {
                // nothing to update
            }
            break;
        case FPendingObjectInfo::PENDING_REMOVE:
            if ( i != -1 ) {
                // remove it

                TreeRemoveObject( i );

            } else {
                // nothing to remove
            }
            break;
        }
    }

    ClearPendingList();
}




FSpatialTree::~FSpatialTree() {
    ClearPendingList();
}

int FSpatialTree::FindPendingObject( FSpatialObject * _Object ) {
    for ( int i = 0 ; i < PendingObjects.Size() ; i++ ) {
        if ( PendingObjects[i].Object == _Object ) {
            return i;
        }
    }
    return -1;
}

void FSpatialTree::AddObject( FSpatialObject * _Object ) {
    int i = FindPendingObject( _Object );
    if ( i != -1 ) {
        FPendingObjectInfo & info = PendingObjects[i];
        info.PendingOp = FPendingObjectInfo::PENDING_ADD;
    } else {
        FPendingObjectInfo & info = PendingObjects.Append();

        info.PendingOp = FPendingObjectInfo::PENDING_ADD;
        info.Object = _Object;

        _Object->AddRef();
    }
}

void FSpatialTree::RemoveObject( FSpatialObject * _Object ) {
    int i = FindPendingObject( _Object );
    if ( i != -1 ) {
        FPendingObjectInfo & info = PendingObjects[i];
        info.PendingOp = FPendingObjectInfo::PENDING_REMOVE;
    } else {
        FPendingObjectInfo & info = PendingObjects.Append();

        info.PendingOp = FPendingObjectInfo::PENDING_REMOVE;
        info.Object = _Object;

        _Object->AddRef();
    }
}

void FSpatialTree::UpdateObject( FSpatialObject * _Object ) {
    int i = FindPendingObject( _Object );
    if ( i != -1 ) {
        FPendingObjectInfo & info = PendingObjects[i];
        info.PendingOp = FPendingObjectInfo::PENDING_UPDATE;
    } else {
        FPendingObjectInfo & info = PendingObjects.Append();

        info.PendingOp = FPendingObjectInfo::PENDING_UPDATE;
        info.Object = _Object;

        _Object->AddRef();
    }
}

void FSpatialTree::Update() {
}

void FSpatialTree::ClearPendingList() {
    for ( FPendingObjectInfo & info : PendingObjects ) {
        info.Object->RemoveRef();
    }
    PendingObjects.Clear();
}
