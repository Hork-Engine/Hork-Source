/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "DebugRenderer.h"

#include <LinearMath/btVector3.h>
#include <LinearMath/btMatrix3x3.h>
#include <LinearMath/btQuaternion.h>

#include <Bullet3Common/b3AlignedAllocator.h>

#ifdef HK_COMPILER_MSVC
#    pragma warning(push)
#    pragma warning(disable : 4456)
#    pragma warning(disable : 4305)
#endif
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#ifdef HK_COMPILER_MSVC
#    pragma warning(pop)
#endif

HK_NAMESPACE_BEGIN

HK_FORCEINLINE Float3 btVectorToFloat3(btVector3 const& _In)
{
    return Float3(_In.x(), _In.y(), _In.z());
}

HK_FORCEINLINE btVector3 btVectorToFloat3(Float3 const& _In)
{
    return btVector3(_In.X, _In.Y, _In.Z);
}

HK_FORCEINLINE Float4 btVectorToFloat4(btVector4 const& _In)
{
    return Float4(_In.x(), _In.y(), _In.z(), _In.w());
}

HK_FORCEINLINE btVector4 btVectorToFloat4(Float4 const& _In)
{
    return btVector4(_In.X, _In.Y, _In.Z, _In.W);
}

HK_FORCEINLINE Quat btQuaternionToQuat(btQuaternion const& _In)
{
    return Quat(_In.w(), _In.x(), _In.y(), _In.z());
}

HK_FORCEINLINE btQuaternion btQuaternionToQuat(Quat const& _In)
{
    return btQuaternion(_In.X, _In.Y, _In.Z, _In.W);
}

HK_FORCEINLINE Float3x3 btMatrixToFloat3x3(btMatrix3x3 const& _In)
{
    return Float3x3(_In[0][0], _In[0][1], _In[0][2],
                    _In[1][0], _In[1][1], _In[1][2],
                    _In[2][0], _In[2][1], _In[2][2]);
}

HK_FORCEINLINE btMatrix3x3 btMatrixToFloat3x3(Float3x3 const& _In)
{
    return btMatrix3x3(_In[0][0], _In[0][1], _In[0][2],
                       _In[1][0], _In[1][1], _In[1][2],
                       _In[2][0], _In[2][1], _In[2][2]);
}

void btDrawCollisionShape(DebugRenderer* InRenderer, btTransform const& Transform, btCollisionShape const* Shape);

void btDrawCollisionObject(DebugRenderer* InRenderer, btCollisionObject* CollisionObject);

HK_NAMESPACE_END
