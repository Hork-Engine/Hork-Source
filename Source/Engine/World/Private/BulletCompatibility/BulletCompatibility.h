#pragma once

#include <Engine/Core/Public/Math.h>

#include <LinearMath/btVector3.h>
#include <LinearMath/btQuaternion.h>

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


