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

#include <Engine/World/Public/ResourceManager.h>
#include <Engine/Core/Public/Logger.h>

FResourceManager & GResourceManager = FResourceManager::Inst();

FResourceManager::FResourceManager() {}

void FResourceManager::Initialize() {

}

void FResourceManager::Deinitialize() {
    ResourceHash.Free();

    for ( CacheEntry & entry : ResourceCache ) {
        entry.Object->RemoveRef();
    }

    ResourceCache.clear();
}

FBaseObject * FResourceManager::FindCachedResource( FClassMeta const & _ClassMeta, const char * _Path, bool & _bMetadataMismatch, int & _Hash ) {
    _Hash = FCore::HashCase( _Path, FString::Length( _Path ) );

    _bMetadataMismatch = false;

    for ( int i = ResourceHash.First( _Hash ) ; i != -1 ; i = ResourceHash.Next( i ) ) {
        if ( !ResourceCache[i].Path.Icmp( _Path ) ) {
            if ( &ResourceCache[i].Object->FinalClassMeta() != &_ClassMeta ) {
                GLogger.Printf( "FindCachedResource: %s class doesn't match meta data (%s vs %s)\n", _Path, ResourceCache[i].Object->FinalClassName(), _ClassMeta.GetName() );
                _bMetadataMismatch = true;
                return nullptr;
            }
            return ResourceCache[i].Object;
        }
    }
    return nullptr;
}

//enum ELoadFlags {
//    LF_LOAD_NOW             = 0,
//    LF_LOAD_ASYNC           = 1,
//    LF_LOAD_ON_LEVEL_LOAD   = 2
//};

FBaseObject * FResourceManager::LoadResource( FClassMeta const & _ClassMeta, const char * _Path/*, ELoadFlags _Flags*/ ) {
    int hash;
    bool bMetadataMismatch;

    FBaseObject * resource = FindCachedResource( _ClassMeta, _Path, bMetadataMismatch, hash );

    if ( bMetadataMismatch ) {
        // Create empty object
        return static_cast< FBaseObject * >( _ClassMeta.CreateInstance() );
    }

    if ( !resource ) {
        GLogger.Printf( "Loading \"%s\"\n", _Path );
        resource = static_cast< FBaseObject * >( _ClassMeta.CreateInstance() );
        resource->AddRef();
        resource->LoadObject( _Path );
        CacheEntry entry;
        entry.Object = resource;
        entry.Path = _Path;
        ResourceHash.Insert( hash, ResourceCache.size() );
        ResourceCache.push_back( entry );
    }

    return resource;
}
