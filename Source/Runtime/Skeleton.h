/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

#include "Resource.h"
#include <Core/Guid.h>
#include <Geometry/BV/BvAxisAlignedBox.h>
#include <Geometry/Skinning.h>

HK_NAMESPACE_BEGIN

/**

Skeleton

Skeleton structure

*/
class Skeleton : public Resource
{
    HK_CLASS(Skeleton, Resource)

public:
    Skeleton();
    ~Skeleton();

    static Skeleton* Create(SkeletonJoint* Joints, int JointsCount, BvAxisAlignedBox const& BindposeBounds)
    {
        Skeleton* skeleton = NewObj<Skeleton>();
        skeleton->Initialize(Joints, JointsCount, BindposeBounds);
        return skeleton;
    }

    void Purge();

    int FindJoint(const char* _Name) const;

    TPodVector<SkeletonJoint> const& GetJoints() const { return m_Joints; }

    BvAxisAlignedBox const& GetBindposeBounds() const { return m_BindposeBounds; }

protected:
    void Initialize(SkeletonJoint* _Joints, int _JointsCount, BvAxisAlignedBox const& _BindposeBounds);

    /** Load resource from file */
    bool LoadResource(IBinaryStreamReadInterface& Stream) override;

    /** Create internal resource */
    void LoadInternalResource(StringView _Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/Skeleton/Default"; }

private:
    TPodVector<SkeletonJoint> m_Joints;
    BvAxisAlignedBox   m_BindposeBounds;
};

HK_NAMESPACE_END
