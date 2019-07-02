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

#include <Engine/GameEngine/Public/ResourceManager.h>
#include <Engine/Core/Public/Logger.h>

FResourceManager & GResourceManager = FResourceManager::Inst();

FResourceManager::FResourceManager() {}

void FResourceManager::Initialize() {

}

void FResourceManager::Deinitialize() {
    for ( int i = ResourceCache.Length() - 1; i >= 0; i-- ) {
        ResourceCache[ i ]->RemoveRef();
    }
    ResourceCache.Free();
    ResourceHash.Free();
}

FBaseObject * FResourceManager::FindResource( FClassMeta const & _ClassMeta, const char * _Name, bool & _bMetadataMismatch, int & _Hash ) {
    _Hash = FCore::HashCase( _Name, FString::Length( _Name ) );

    _bMetadataMismatch = false;

    for ( int i = ResourceHash.First( _Hash ) ; i != -1 ; i = ResourceHash.Next( i ) ) {
        if ( !ResourceCache[i]->GetName().Icmp( _Name ) ) {
            if ( &ResourceCache[i]->FinalClassMeta() != &_ClassMeta ) {
                GLogger.Printf( "FindResource: %s class doesn't match meta data (%s vs %s)\n", _Name, ResourceCache[i]->FinalClassName(), _ClassMeta.GetName() );
                _bMetadataMismatch = true;
                return nullptr;
            }
            return ResourceCache[i];
        }
    }
    return nullptr;
}

FBaseObject * FResourceManager::FindResourceByName( const char * _Name ) {
    int hash = FCore::HashCase( _Name, FString::Length( _Name ) );

    for ( int i = ResourceHash.First( hash ) ; i != -1 ; i = ResourceHash.Next( i ) ) {
        if ( !ResourceCache[i]->GetName().Icmp( _Name ) ) {
            return ResourceCache[i];
        }
    }

    return nullptr;
}

FBaseObject * FResourceManager::GetResource( FClassMeta const & _ClassMeta, const char * _Name, bool * _bResourceFoundResult, bool * _bMetadataMismatch ) {
    int hash = FCore::HashCase( _Name, FString::Length( _Name ) );

    if ( _bResourceFoundResult ) {
        *_bResourceFoundResult = false;
    }

    if ( _bMetadataMismatch ) {
        *_bMetadataMismatch = false;
    }

    for ( int i = ResourceHash.First( hash ) ; i != -1 ; i = ResourceHash.Next( i ) ) {
        if ( !ResourceCache[i]->GetName().Icmp( _Name ) ) {
            if ( &ResourceCache[i]->FinalClassMeta() != &_ClassMeta ) {
                GLogger.Printf( "GetResource: %s class doesn't match meta data (%s vs %s)\n", _Name, ResourceCache[i]->FinalClassName(), _ClassMeta.GetName() );

                if ( _bMetadataMismatch ) {
                    *_bMetadataMismatch = true;
                }
                break;
            }

            if ( _bResourceFoundResult ) {
                *_bResourceFoundResult = true;
            }
            return ResourceCache[i];
        }
    }

    // Never return nullptr, always create default object

    FBaseObject * resource = static_cast< FBaseObject * >( _ClassMeta.CreateInstance() );

    resource->InitializeDefaultObject();

    return resource;
}

FClassMeta const * FResourceManager::GetResourceInfo( const char * _Name ) {
    int hash = FCore::HashCase( _Name, FString::Length( _Name ) );

    for ( int i = ResourceHash.First( hash ) ; i != -1 ; i = ResourceHash.Next( i ) ) {
        if ( !ResourceCache[i]->GetName().Icmp( _Name ) ) {
            return &ResourceCache[i]->FinalClassMeta();
        }
    }

    return nullptr;
}

bool FResourceManager::RegisterResource( FBaseObject * _Resource ) {
    int hash;
    bool bMetadataMismatch;

    FBaseObject * resource = FindResource( _Resource->FinalClassMeta(), _Resource->GetName().ToConstChar(), bMetadataMismatch, hash );
    if ( resource || bMetadataMismatch ) {
        GLogger.Printf( "RegisterResource: Resource with same name already exists\n" );
        return false;
    }

    _Resource->AddRef();
    ResourceHash.Insert( hash, ResourceCache.Length() );
    ResourceCache.Append( _Resource );

    return true;
}

FBaseObject * FResourceManager::GetOrCreateResource( FClassMeta const &  _ClassMeta, const char * _FileName, const char * _Alias ) {
    int hash;
    bool bMetadataMismatch;
    const char * resourceName = _Alias ? _Alias : _FileName;

    FBaseObject * resource = FindResource( _ClassMeta, resourceName, bMetadataMismatch, hash );
    if ( bMetadataMismatch ) {

        // Never return null

        resource = static_cast< FBaseObject * >( _ClassMeta.CreateInstance() );
        resource->InitializeDefaultObject();

        return resource;
    }

    if ( resource ) {
        GLogger.Printf( "Caching resource %s...\n", _FileName );
        return resource;
    }

    resource = static_cast< FBaseObject * >( _ClassMeta.CreateInstance() );

    resource->InitializeFromFile( _FileName );
    resource->SetName( resourceName );
    resource->AddRef();

    ResourceHash.Insert( hash, ResourceCache.Length() );
    ResourceCache.Append( resource );

    return resource;
}

bool FResourceManager::UnregisterResource( FBaseObject * _Resource ) {
    int hash = _Resource->GetName().HashCase();
    int i;

    for ( i = ResourceHash.First( hash ) ; i != -1 ; i = ResourceHash.Next( i ) ) {
        if ( !ResourceCache[i]->GetName().Icmp( _Resource->GetName() ) ) {
            if ( &ResourceCache[i]->FinalClassMeta() != &_Resource->FinalClassMeta() ) {
                GLogger.Printf( "UnregisterResource: %s class doesn't match meta data (%s vs %s)\n", _Resource->GetName().ToConstChar(), ResourceCache[i]->FinalClassName(), _Resource->FinalClassMeta().GetName() );
                return false;
            }
            break;
        }
    }

    if ( i == -1 ) {
        GLogger.Printf( "UnregisterResource: resource %s is not found\n", _Resource->GetName().ToConstChar() );
        return false;
    }

    _Resource->RemoveRef();
    ResourceHash.Remove( hash, i );
    ResourceCache.Remove( i );
    return true;
}

void FResourceManager::UnregisterResources( FClassMeta const & _ClassMeta ) {
    for ( int i = ResourceCache.Length() - 1 ; i >= 0 ; i-- ) {
        if ( ResourceCache[i]->FinalClassId() == _ClassMeta.GetId() ) {
            ResourceCache[i]->RemoveRef();

            ResourceHash.Remove( ResourceCache[i]->GetName().HashCase(), i );
            ResourceCache.Remove( i );
        }
    }
}

void FResourceManager::UnregisterResources() {
    for ( int i = ResourceCache.Length() - 1 ; i >= 0 ; i-- ) {
        ResourceCache[i]->RemoveRef();
    }
    ResourceHash.Clear();
    ResourceCache.Clear();
}
