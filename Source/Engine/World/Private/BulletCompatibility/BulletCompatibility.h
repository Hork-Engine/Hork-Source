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

#include <World/Public/Base/DebugRenderer.h>

#include <LinearMath/btVector3.h>
#include <LinearMath/btMatrix3x3.h>
#include <LinearMath/btQuaternion.h>

#include <Bullet3Common/b3AlignedAllocator.h>

#ifdef AN_COMPILER_MSVC
#pragma warning( push )
#pragma warning( disable : 4456 )
#pragma warning( disable : 4305 )
#endif
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#ifdef AN_COMPILER_MSVC
#pragma warning( pop )
#endif

AN_FORCEINLINE Float3 btVectorToFloat3( btVector3 const & _In ) {
    return Float3( _In.x(), _In.y(), _In.z() );
}

AN_FORCEINLINE btVector3 btVectorToFloat3( Float3 const & _In ) {
    return btVector3( _In.X, _In.Y, _In.Z );
}

AN_FORCEINLINE Float4 btVectorToFloat4( btVector4 const & _In ) {
    return Float4( _In.x(), _In.y(), _In.z(), _In.w() );
}

AN_FORCEINLINE btVector4 btVectorToFloat4( Float4 const & _In ) {
    return btVector4( _In.X, _In.Y, _In.Z, _In.W );
}

AN_FORCEINLINE Quat btQuaternionToQuat( btQuaternion const & _In ) {
    return Quat( _In.w(), _In.x(), _In.y(), _In.z() );
}

AN_FORCEINLINE btQuaternion btQuaternionToQuat( Quat const & _In ) {
    return btQuaternion( _In.X, _In.Y, _In.Z, _In.W );
}

AN_FORCEINLINE Float3x3 btMatrixToFloat3x3( btMatrix3x3 const & _In ) {
    return Float3x3( _In[0][0], _In[0][1], _In[0][2],
                     _In[1][0], _In[1][1], _In[1][2],
                     _In[2][0], _In[2][1], _In[2][2] );
}

AN_FORCEINLINE btMatrix3x3 btMatrixToFloat3x3( Float3x3 const & _In ) {
    return btMatrix3x3( _In[0][0], _In[0][1], _In[0][2],
                        _In[1][0], _In[1][1], _In[1][2],
                        _In[2][0], _In[2][1], _In[2][2] );
}

void btDrawCollisionShape( ADebugRenderer * InRenderer, btTransform const & Transform, btCollisionShape const * Shape );

void btDrawCollisionObject( ADebugRenderer * InRenderer, btCollisionObject * CollisionObject );
