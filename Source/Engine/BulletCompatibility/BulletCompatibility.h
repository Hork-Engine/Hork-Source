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

#include <Engine/Core/Public/CoreMath.h>

#include <LinearMath/btVector3.h>
#include <LinearMath/btMatrix3x3.h>
#include <LinearMath/btQuaternion.h>

#include <Bullet3Common/b3AlignedAllocator.h>

AN_FORCEINLINE Float3 btVectorToFloat3( const btVector3 & _In ) {
    return Float3( _In.x(), _In.y(), _In.z() );
}

AN_FORCEINLINE btVector3 btVectorToFloat3( const Float3 & _In ) {
    return btVector3( _In.X, _In.Y, _In.Z );
}

AN_FORCEINLINE Float4 btVectorToFloat4( const btVector4 & _In ) {
    return Float4( _In.x(), _In.y(), _In.z(), _In.w() );
}

AN_FORCEINLINE btVector4 btVectorToFloat4( const Float4 & _In ) {
    return btVector4( _In.X, _In.Y, _In.Z, _In.W );
}

AN_FORCEINLINE Quat btQuaternionToQuat( const btQuaternion & _In ) {
    return Quat( _In.w(), _In.x(), _In.y(), _In.z() );
}

AN_FORCEINLINE btQuaternion btQuaternionToQuat( const Quat & _In ) {
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

#define b3New( _ClassName, ... ) new (b3AlignedAlloc(sizeof(_ClassName),16)) _ClassName( __VA_ARGS__ )
#define b3Destroy( _Object ) { dtor( _Object ); b3AlignedFree( _Object ); }

template< typename T > AN_FORCEINLINE void dtor( T * object ) { object->~T(); }