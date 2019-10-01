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

#include <Engine/Base/Public/BaseObject.h>
#include <Engine/Core/Public/CompileTimeString.h>

class FResourceManager {
    AN_SINGLETON( FResourceManager )

public:
    void Initialize();
    void Deinitialize();

    // Get or create resource. Return default object if fails.
    template< typename T >
    AN_FORCEINLINE T * GetOrCreateResource( const char * _FileName, const char * _Alias = nullptr ) {
        return static_cast< T * >( GetOrCreateResource( T::ClassMeta(), _FileName, _Alias ) );
    }

    // Get or create internal resource. Return default object if fails.
    template< typename T >
    AN_FORCEINLINE T * GetOrCreateInternalResource( const char * _InternalResourceName ) {
        return static_cast< T * >( GetOrCreateInternalResource( T::ClassMeta(), _InternalResourceName ) );
    }

    // Get resource. Return default object if fails.
    template< typename T >
    AN_FORCEINLINE T * GetResource( const char * _Name, bool * _bResourceFoundResult = nullptr, bool * _bMetadataMismatch = nullptr ) {
        return static_cast< T * >( GetResource( T::ClassMeta(), _Name, _bResourceFoundResult, _bMetadataMismatch ) );
    }

    // Get or create resource. Return default object if fails.
    FBaseObject * GetOrCreateResource( FClassMeta const &  _ClassMeta, const char * _FileName, const char * _Alias = nullptr );

    // Get or create internal resource. Return default object if fails.
    FBaseObject * GetOrCreateInternalResource( FClassMeta const &  _ClassMeta, const char * _InternalResourceName );

    // Get resource. Return default object if fails.
    FBaseObject * GetResource( FClassMeta const & _ClassMeta, const char * _Name, bool * _bResourceFoundResult = nullptr, bool * _bMetadataMismatch = nullptr );

    // Get resource meta. Return null if fails.
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
AN_FORCEINLINE T * GetOrCreateResource( const char * _FileName, const char * _Alias = nullptr ) {
    return GResourceManager.GetOrCreateResource< T >( _FileName, _Alias );
}

// Get or create internal resource. Return default object if fails.
template< typename T >
AN_FORCEINLINE T * GetOrCreateInternalResource( const char * _InternalResourceName ) {
    return GResourceManager.GetOrCreateInternalResource< T >( _InternalResourceName );
}

// Get resource. Return default object if fails.
template< typename T >
AN_FORCEINLINE T * GetResource( const char * _Name, bool * _bResourceFoundResult = nullptr, bool * _bMetadataMismatch = nullptr ) {
    return GResourceManager.GetResource< T >( _Name, _bResourceFoundResult, _bMetadataMismatch );
}

// Get resource meta. Return null if fails.
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

//
// Experemental static resource finder
// Usage: static TStaticResourceFinder< FIndexedMesh > Resource( _CTS( "Meshes/MyMesh" ) );
// FIndexedMesh * mesh = Resource.GetObject();
//
template< typename T >
struct TStaticResourceFinder {

    template< char... Chars >
    TStaticResourceFinder( TCompileTimeString<Chars...> const & _Name )
        : ResourceName(_Name.ToConstChar())
    {
        Object = GetOrCreateResource< T >( ResourceName );
    }

    T * GetObject() {
        if ( Object.IsExpired() ) {
            Object = GetOrCreateResource< T >( ResourceName );
        }
        return Object;
    }

private:
    const char * ResourceName;
    TWeakRef< T > Object;
};

template< typename T >
struct TStaticInternalResourceFinder {

    template< char... Chars >
    TStaticInternalResourceFinder( TCompileTimeString<Chars...> const & _Name )
        : ResourceName(_Name.ToConstChar())
    {
        Object = GetOrCreateInternalResource< T >( ResourceName );
    }

    T * GetObject() {
        if ( Object.IsExpired() ) {
            Object = GetOrCreateInternalResource< T >( ResourceName );
        }
        return Object;
    }

private:
    const char * ResourceName;
    TWeakRef< T > Object;
};
