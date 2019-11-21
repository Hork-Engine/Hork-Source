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


#include <World/Public/Octree.h>
#include <World/Public/Level.h>
#include <Core/Public/Logger.h>

AN_CLASS_META( ASpatialTree )
AN_CLASS_META( AOctree )

void AOctree::Build() {
    Purge();

    BvAxisAlignedBox const & boundingBox = Owner->GetBoundingBox();

    if ( boundingBox.IsEmpty() ) {
        GLogger.Printf( "Octree: invalid bounding box\n" );
        return;
    }

    float width = boundingBox.Maxs[0] - boundingBox.Mins[0];
    float height = boundingBox.Maxs[1] - boundingBox.Mins[1];
    float depth = boundingBox.Maxs[2] - boundingBox.Mins[2];
    float maxDim = Math::Max( width, height, depth );

    NumLevels = Math::Log2( Math::ToIntFast( maxDim + 0.5f ) ) + 1;

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

void AOctree::TreeAddObject( ADrawable * _Object ) {
    ObjectsInTree.Append( _Object );
    _Object->AddRef();

    //BvAxisAlignedBox const & bounds = _Object->GetWorldBounds();
}

void AOctree::TreeRemoveObject( int _Index ) {
    ObjectsInTree[ _Index ]->RemoveRef();
    ObjectsInTree.RemoveSwap( _Index );
}

void AOctree::TreeUpdateObject( int _Index ) {

}

void AOctree::Update() {
    for ( SPendingObjectInfo & info : PendingObjects ) {

        int i = ObjectsInTree.IndexOf( info.Object );

        switch ( info.PendingOp ) {
        case SPendingObjectInfo::PENDING_ADD:
            if ( i != -1 ) {
                // object already in tree, just update it

                TreeUpdateObject( i );
            } else {
                // add object to tree nodes

                TreeAddObject( info.Object );
            }
            break;
        case SPendingObjectInfo::PENDING_UPDATE:
            if ( i != -1 ) {
                // just update it

                TreeUpdateObject( i );
            } else {
                // nothing to update
            }
            break;
        case SPendingObjectInfo::PENDING_REMOVE:
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




ASpatialTree::~ASpatialTree() {
    ClearPendingList();
}

int ASpatialTree::FindPendingObject( ADrawable * _Object ) {
    for ( int i = 0 ; i < PendingObjects.Size() ; i++ ) {
        if ( PendingObjects[i].Object == _Object ) {
            return i;
        }
    }
    return -1;
}

void ASpatialTree::AddObject( ADrawable * _Object ) {
    int i = FindPendingObject( _Object );
    if ( i != -1 ) {
        SPendingObjectInfo & info = PendingObjects[i];
        info.PendingOp = SPendingObjectInfo::PENDING_ADD;
    } else {
        SPendingObjectInfo & info = PendingObjects.Append();

        info.PendingOp = SPendingObjectInfo::PENDING_ADD;
        info.Object = _Object;

        _Object->AddRef();
    }
}

void ASpatialTree::RemoveObject( ADrawable * _Object ) {
    int i = FindPendingObject( _Object );
    if ( i != -1 ) {
        SPendingObjectInfo & info = PendingObjects[i];
        info.PendingOp = SPendingObjectInfo::PENDING_REMOVE;
    } else {
        SPendingObjectInfo & info = PendingObjects.Append();

        info.PendingOp = SPendingObjectInfo::PENDING_REMOVE;
        info.Object = _Object;

        _Object->AddRef();
    }
}

void ASpatialTree::UpdateObject( ADrawable * _Object ) {
    int i = FindPendingObject( _Object );
    if ( i != -1 ) {
        SPendingObjectInfo & info = PendingObjects[i];
        info.PendingOp = SPendingObjectInfo::PENDING_UPDATE;
    } else {
        SPendingObjectInfo & info = PendingObjects.Append();

        info.PendingOp = SPendingObjectInfo::PENDING_UPDATE;
        info.Object = _Object;

        _Object->AddRef();
    }
}

void ASpatialTree::Update() {
}

void ASpatialTree::ClearPendingList() {
    for ( SPendingObjectInfo & info : PendingObjects ) {
        info.Object->RemoveRef();
    }
    PendingObjects.Clear();
}
