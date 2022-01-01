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

#include <World/Public/Base/Resource.h>
#include <Core/Public/CompileTimeString.h>
#include <Core/Public/Guid.h>

class AResourceManager {
public:
    AResourceManager();
    virtual ~AResourceManager();

    void AddResourcePack( AStringView FileName );

    /** Find file in resource packs */
    bool FindFile( AStringView FileName, AArchive ** ppResourcePack, int * pFileIndex );

    /** Get or create resource. Return default object if fails. */
    template< typename T >
    AN_FORCEINLINE T * GetOrCreateResource( const char * _Path ) {
        return static_cast< T * >( GetOrCreateResource( T::ClassMeta(), _Path ) );
    }

    /** Get resource. Return default object if fails. */
    template< typename T >
    AN_FORCEINLINE T * GetResource( const char * _Alias, bool * _bResourceFoundResult = nullptr, bool * _bMetadataMismatch = nullptr ) {
        return static_cast< T * >( GetResource( T::ClassMeta(), _Alias, _bResourceFoundResult, _bMetadataMismatch ) );
    }

    /** Get or create resource. Return default object if fails. */
    AResource * GetOrCreateResource( AClassMeta const &  _ClassMeta, const char * _Path );

    /** Get resource. Return default object if fails. */
    AResource * GetResource( AClassMeta const & _ClassMeta, const char * _Alias, bool * _bResourceFoundResult = nullptr, bool * _bMetadataMismatch = nullptr );

    /** Get resource meta. Return null if fails. */
    AClassMeta const * GetResourceInfo( const char * _Alias );

    /** Find resource in cache. Return null if fails. */
    template< typename T >
    T * FindResource( const char * _Alias, bool & _bMetadataMismatch, int & _Hash ) {
        return static_cast< T * >( FindResource( T::ClassMeta(), _Alias, _bMetadataMismatch, _Hash ) );
    }

    /** Find resource in cache. Return null if fails. */
    AResource * FindResource( AClassMeta const & _ClassMeta, const char * _Alias, bool & _bMetadataMismatch, int & _Hash );

    /** Find resource in cache. Return null if fails. */
    AResource * FindResourceByAlias( const char * _Alias );

    /** Register object as resource. */
    bool RegisterResource( AResource * _Resource, const char * _Alias );

    /** Unregister object as resource. */
    bool UnregisterResource( AResource * _Resource );

    /** Unregister all resources with same meta. */
    void UnregisterResources( AClassMeta const & _ClassMeta );

    /** Unregister all resources by type. */
    template< typename T >
    AN_FORCEINLINE void UnregisterResources() {
        UnregisterResources( T::ClassMeta() );
    }

    /** Unregister all resources. */
    void UnregisterResources();

    AArchive const * GetCommonResources() const { return CommonResources.GetObject(); }

private:
    TPodVector< AResource * > ResourceCache;
    THash<> ResourceHash;
    TStdVector< TUniqueRef< AArchive > > ResourcePacks;
    TUniqueRef< AArchive > CommonResources;
};

extern AResourceManager * GResourceManager;

/*

Helpers

*/

/** Get or create resource. Return default object if fails. */
template< typename T >
AN_FORCEINLINE T * GetOrCreateResource( const char * _Path ) {
    return GResourceManager->GetOrCreateResource< T >( _Path );
}

/** Get resource. Return default object if fails. */
template< typename T >
AN_FORCEINLINE T * GetResource( const char * _Alias, bool * _bResourceFoundResult = nullptr, bool * _bMetadataMismatch = nullptr ) {
    return GResourceManager->GetResource< T >( _Alias, _bResourceFoundResult, _bMetadataMismatch );
}

/** Get resource meta. Return null if fails. */
AN_FORCEINLINE AClassMeta const * GetResourceInfo( const char * _Alias ) {
    return GResourceManager->GetResourceInfo( _Alias );
}

/** Find resource in cache. Return null if fails. */
AN_FORCEINLINE AResource * FindResource( AClassMeta const & _ClassMeta, const char * _Alias, bool & _bMetadataMismatch, int & _Hash ) {
    return GResourceManager->FindResource( _ClassMeta, _Alias, _bMetadataMismatch, _Hash );
}

/** Find resource in cache. Return null if fails. */
template< typename T >
AN_FORCEINLINE T * FindResource( const char * _Alias, bool & _bMetadataMismatch, int & _Hash ) {
    return static_cast< T * >( FindResource( T::ClassMeta(), _Alias, _bMetadataMismatch, _Hash ) );
}

/** Find resource in cache. Return null if fails. */
AN_FORCEINLINE AResource * FindResourceByAlias( const char * _Alias ) {
    return GResourceManager->FindResourceByAlias( _Alias );
}

/** Register object as resource. */
AN_FORCEINLINE bool RegisterResource( AResource * _Resource, const char * _Alias ) {
    return GResourceManager->RegisterResource( _Resource, _Alias );
}

/** Unregister object as resource. */
AN_FORCEINLINE bool UnregisterResource( AResource * _Resource ) {
    return GResourceManager->UnregisterResource( _Resource );
}

/** Unregister all resources by type. */
template< typename T >
AN_FORCEINLINE void UnregisterResources() {
    GResourceManager->UnregisterResources< T >();
}

/** Unregister all resources. */
AN_FORCEINLINE void UnregisterResources() {
    GResourceManager->UnregisterResources();
}

/**

Static resource finder

Usage:
static TStaticResourceFinder< AIndexedMesh > Resource( _CTS( "/Root/Meshes/MyMesh.asset" ) );
AIndexedMesh * mesh = Resource.GetObject();

*/
template< typename T >
struct TStaticResourceFinder {

    template< char... Chars >
    TStaticResourceFinder( TCompileTimeString<Chars...> const & _Path )
        : ResourcePath( _Path.CStr() )
    {
        Object = GetOrCreateResource< T >( ResourcePath );
    }

    TStaticResourceFinder( const char * _Alias )
        : ResourcePath( _Alias )
    {
        Object = GetOrCreateResource< T >( ResourcePath );
    }

    T * GetObject() {
        if ( Object.IsExpired() ) {
            Object = GetOrCreateResource< T >( ResourcePath );
        }
        return Object;
    }

private:
    const char * ResourcePath;
    TWeakRef< T > Object;
};
