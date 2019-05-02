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

    FBaseObject * FindResource( FClassMeta const & _ClassMeta, const char * _Name, bool & _bMetadataMismatch, int & _Hash );

    FBaseObject * FindResourceByName( const char * _Name );

    FBaseObject * GetResource( FClassMeta const & _ClassMeta, const char * _Name, bool * _bResourceFoundResult = nullptr, bool * _bMetadataMismatch = nullptr );

    FClassMeta const * GetResourceInfo( const char * _Name );

    template< typename T >
    AN_FORCEINLINE T * GetResource( const char * _Name, bool * _bResourceFoundResult = nullptr, bool * _bMetadataMismatch = nullptr ) {
        return static_cast< T * >( GetResource( T::ClassMeta(), _Name, _bResourceFoundResult, _bMetadataMismatch ) );
    }

    bool RegisterResource( FBaseObject * _Resource );

    bool UnregisterResource( FBaseObject * _Resource );

    void UnregisterResources( FClassMeta const & _ClassMeta );

    template< typename T >
    AN_FORCEINLINE void UnregisterResources() {
        UnregisterResources( T::ClassMeta() );
    }

    void UnregisterResources();

    FBaseObject * CreateUniqueResource( FClassMeta const &  _ClassMeta, const char * _FileName, const char * _Alias = nullptr );

    template< typename T >
    AN_FORCEINLINE T * CreateUniqueResource( const char * _FileName, const char * _Alias = nullptr ) {
        return static_cast< T * >( CreateUniqueResource( T::ClassMeta(), _FileName, _Alias ) );
    }

private:
    TPodArray< FBaseObject * > ResourceCache;
    THash<> ResourceHash;
};

extern FResourceManager & GResourceManager;
