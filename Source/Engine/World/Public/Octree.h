/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#if 0
#include <World/Public/Base/BaseObject.h>
#include <World/Public/Resource/IndexedMesh.h>

#include <Core/Public/BV/BvFrustum.h>

class ALevelArea;
class ADrawable;
class AMaterialInstance;




class ASpatialTree : public ABaseObject {
    AN_CLASS( ASpatialTree, ABaseObject )

public:

    void AddObject( ADrawable * _Object );
    void RemoveObject( ADrawable * _Object );
    void UpdateObject( ADrawable * _Object );

    virtual void Build() {

    }

    virtual bool Trace( struct SWorldRaycastClosestResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd ) {
        return false;
    }

    virtual void Update();

    ALevelArea * Owner;

protected:
    ASpatialTree() {}
    ~ASpatialTree();

    int FindPendingObject( ADrawable * _Object );

    void ClearPendingList();

    struct SPendingObjectInfo {
        enum { PENDING_ADD, PENDING_REMOVE, PENDING_UPDATE };

        ADrawable * Object;
        int PendingOp;
    };

    TPodArray< SPendingObjectInfo > PendingObjects;
};

struct SOctreeNode {
    BvAxisAlignedBox BoundingBox;

    SOctreeNode * Parent;
    SOctreeNode * Childs[8];
};

class AOctree : public ASpatialTree {
    AN_CLASS( AOctree, ASpatialTree )

public:

    void Build() override;

    void Purge() {

    }

    bool Trace( SWorldRaycastClosestResult & _Result, Float3 const & _RayStart, Float3 const & _RayEnd ) override {
        return false;
    }

    void Update() override;

protected:

    AOctree() {}

    ~AOctree() {
        Purge();
    }

private:
    void TreeAddObject( ADrawable * _Object );
    void TreeRemoveObject( int _Index );
    void TreeUpdateObject( int _Index );

//    SOctreeNode * Root;
//    SOctreeNode * Nodes;
    int NumLevels;

    TPodArray< ADrawable * > ObjectsInTree;
};
#endif
