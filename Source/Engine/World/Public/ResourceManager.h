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

#include "BaseObject.h"

class FTexture;
class FIndexedMesh;
class FSkeleton;

class FResourceManager {
    AN_SINGLETON( FResourceManager )

public:
    void Initialize();
    void Deinitialize();

    // Get or create resource. Return default object if fails.
    template< typename T >
    AN_FORCEINLINE T * CreateResource( const char * _FileName, const char * _Alias = nullptr ) {
        return static_cast< T * >( CreateResource( T::ClassMeta(), _FileName, _Alias ) );
    }

    // Get resource. Return default object if fails.
    template< typename T >
    AN_FORCEINLINE T * GetResource( const char * _Name, bool * _bResourceFoundResult = nullptr, bool * _bMetadataMismatch = nullptr ) {
        return static_cast< T * >( GetResource( T::ClassMeta(), _Name, _bResourceFoundResult, _bMetadataMismatch ) );
    }

    // Get or create resource. Return default object if fails.
    FBaseObject * CreateResource( FClassMeta const &  _ClassMeta, const char * _FileName, const char * _Alias = nullptr );

    // Get resource. Return default object if fails.
    FBaseObject * GetResource( FClassMeta const & _ClassMeta, const char * _Name, bool * _bResourceFoundResult = nullptr, bool * _bMetadataMismatch = nullptr );

    // Get resource meta.
    FClassMeta const * GetResourceInfo( const char * _Name );

    // Find resource in cache. Return null if fails.
    FBaseObject * FindResource( FClassMeta const & _ClassMeta, const char * _Name, bool & _bMetadataMismatch, int & _Hash );

    // Find resource in cache. Return null if fails.
    FBaseObject * FindResourceByName( const char * _Name );

    // Register object as resource.
    bool RegisterResource( FBaseObject * _Resource );

    // Unregister object as resource.
    bool UnregisterResource( FBaseObject * _Resource );

    // Unregister all resources with same meta.
    void UnregisterResources( FClassMeta const & _ClassMeta );

    // Unregister all resources by type.
    template< typename T >
    AN_FORCEINLINE void UnregisterResources() {
        UnregisterResources( T::ClassMeta() );
    }

    // Unregister all resources.
    void UnregisterResources();

private:
    TPodArray< FBaseObject * > ResourceCache;
    THash<> ResourceHash;
};

extern FResourceManager & GResourceManager;

/*

Helpers

*/

// Get or create resource. Return default object if fails.
template< typename T >
AN_FORCEINLINE T * CreateResource( const char * _FileName, const char * _Alias = nullptr ) {
    return GResourceManager.CreateResource< T >( _FileName, _Alias );
}

// Get resource. Return default object if fails.
template< typename T >
AN_FORCEINLINE T * GetResource( const char * _Name, bool * _bResourceFoundResult = nullptr, bool * _bMetadataMismatch = nullptr ) {
    return GResourceManager.GetResource< T >( _Name, _bResourceFoundResult, _bMetadataMismatch );
}

// Get resource meta.
AN_FORCEINLINE FClassMeta const * GetResourceInfo( const char * _Name ) {
    return GResourceManager.GetResourceInfo( _Name );
}

// Find resource in cache. Return null if fails.
AN_FORCEINLINE FBaseObject * FindResource( FClassMeta const & _ClassMeta, const char * _Name, bool & _bMetadataMismatch, int & _Hash ) {
    return GResourceManager.FindResource( _ClassMeta, _Name, _bMetadataMismatch, _Hash );
}

// Find resource in cache. Return null if fails.
AN_FORCEINLINE FBaseObject * FindResourceByName( const char * _Name ) {
    return GResourceManager.FindResourceByName( _Name );
}

// Register object as resource.
AN_FORCEINLINE bool RegisterResource( FBaseObject * _Resource ) {
    return GResourceManager.RegisterResource( _Resource );
}

// Unregister object as resource.
AN_FORCEINLINE bool UnregisterResource( FBaseObject * _Resource ) {
    return GResourceManager.UnregisterResource( _Resource );
}

// Unregister all resources by type.
template< typename T >
AN_FORCEINLINE void UnregisterResources() {
    GResourceManager.UnregisterResources< T >();
}

// Unregister all resources.
AN_FORCEINLINE void UnregisterResources() {
    GResourceManager.UnregisterResources();
}
