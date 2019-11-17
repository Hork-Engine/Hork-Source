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

#include <Engine/Resource/Public/ResourceManager.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Base/Public/GameModuleInterface.h>

AResourceManager & GResourceManager = AResourceManager::Inst();

AResourceManager::AResourceManager() {}

void AResourceManager::Initialize() {
    RootPath = IGameModule::RootPath;
    if ( RootPath.IsEmpty() ) {
        RootPath = "Data/";
    } else {
        RootPath.FixSeparator();
        if ( RootPath[RootPath.Length()-1] != '/' ) {
            RootPath += '/';
        }
    }
    LoadResourceGUID();
}

void AResourceManager::Deinitialize() {
    SaveResourceGUID();

    for ( int i = ResourceCache.Size() - 1; i >= 0; i-- ) {
        ResourceCache[ i ]->RemoveRef();
    }
    ResourceCache.Free();
    ResourceHash.Free();

    ResourceGUID.clear();
    ResourceGUIDHash.Free();

    RootPath.Free();
}

void AResourceManager::LoadResourceGUID() {
    AFileStream f;

    if ( f.OpenRead( RootPath + "ResourceGUID.bin" ) )
    {
        char buf[8192];
        char * path;
        int pathLength;

        while ( f.Gets( buf, sizeof( buf ) ) )
        {
            if ( AString::Length( buf ) < 36 )
            {
                // Invalid GUID
                continue;
            }

            buf[36] = 0;
            path = &buf[37];

            pathLength = AString::Length( path );
            if ( !pathLength )
            {
                // Path is empty
                continue;
            }

            if ( path[pathLength-1] == '\n' )
            {
                // Skip trailing 'newline'
                path[--pathLength] = 0;
            }

            if ( !*path )
            {
                // Path is empty
                continue;
            }

            SetResourceGUID( buf, path );
        }

#if 0
        uint32_t count = f.ReadUInt32();

        ResourceGUID.reserve( count );

        AString guid, path;
        for ( uint32_t i = 0 ; i < count ; i++ ) {
            f.ReadString( guid );
            f.ReadString( path );

            SetResourceGUID( guid, path.CStr() );
        }
#endif
    }
}

void AResourceManager::SaveResourceGUID() {
    AFileStream f;
    if ( f.OpenWrite( RootPath + "ResourceGUID.bin" ) ) {
        int count = ResourceGUID.size();

        for ( int i = 0 ; i < count ; i++ ) {
            f.Printf( "%s:%s\n", ResourceGUID[i].first.c_str(), ResourceGUID[i].second.c_str() );
        }
    }
}

AResourceBase * AResourceManager::FindResource( AClassMeta const & _ClassMeta, const char * _Alias, bool & _bMetadataMismatch, int & _Hash ) {
    _Hash = Core::HashCase( _Alias, AString::Length( _Alias ) );

    _bMetadataMismatch = false;

    for ( int i = ResourceHash.First( _Hash ) ; i != -1 ; i = ResourceHash.Next( i ) ) {
        if ( !ResourceCache[i]->GetResourceAlias().Icmp( _Alias ) ) {
            if ( &ResourceCache[i]->FinalClassMeta() != &_ClassMeta ) {
                GLogger.Printf( "FindResource: %s class doesn't match meta data (%s vs %s)\n", _Alias, ResourceCache[i]->FinalClassName(), _ClassMeta.GetName() );
                _bMetadataMismatch = true;
                return nullptr;
            }
            return ResourceCache[i];
        }
    }
    return nullptr;
}

AResourceBase * AResourceManager::FindResourceByAlias( const char * _Alias ) {
    int hash = Core::HashCase( _Alias, AString::Length( _Alias ) );

    for ( int i = ResourceHash.First( hash ) ; i != -1 ; i = ResourceHash.Next( i ) ) {
        if ( !ResourceCache[i]->GetResourceAlias().Icmp( _Alias ) ) {
            return ResourceCache[i];
        }
    }

    return nullptr;
}

AResourceBase * AResourceManager::GetResource( AClassMeta const & _ClassMeta, const char * _Alias, bool * _bResourceFoundResult, bool * _bMetadataMismatch ) {
    int hash = Core::HashCase( _Alias, AString::Length( _Alias ) );

    if ( _bResourceFoundResult ) {
        *_bResourceFoundResult = false;
    }

    if ( _bMetadataMismatch ) {
        *_bMetadataMismatch = false;
    }

    for ( int i = ResourceHash.First( hash ) ; i != -1 ; i = ResourceHash.Next( i ) ) {
        if ( !ResourceCache[i]->GetResourceAlias().Icmp( _Alias ) ) {
            if ( &ResourceCache[i]->FinalClassMeta() != &_ClassMeta ) {
                GLogger.Printf( "GetResource: %s class doesn't match meta data (%s vs %s)\n", _Alias, ResourceCache[i]->FinalClassName(), _ClassMeta.GetName() );

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

    AResourceBase * resource = static_cast< AResourceBase * >( _ClassMeta.CreateInstance() );

    resource->InitializeDefaultObject();

    return resource;
}

AClassMeta const * AResourceManager::GetResourceInfo( const char * _Alias ) {
    int hash = Core::HashCase( _Alias, AString::Length( _Alias ) );

    for ( int i = ResourceHash.First( hash ) ; i != -1 ; i = ResourceHash.Next( i ) ) {
        if ( !ResourceCache[i]->GetResourceAlias().Icmp( _Alias ) ) {
            return &ResourceCache[i]->FinalClassMeta();
        }
    }

    return nullptr;
}

AResourceBase * AResourceManager::GetOrCreateResource( AClassMeta const &  _ClassMeta, const char * _Alias, const char * _PhysicalPath ) {
    int hash;
    bool bMetadataMismatch;
    AString alias = _Alias;

    if ( *_Alias == '/' && AString::IcmpN( _Alias, "/Default/", 9 ) ) {
        // Used physical path, try to find alias. TODO: Use hash?
        for ( std::pair< std::string, std::string > const & p : ResourceGUID ) {
            if ( !AString::Icmp( _Alias, p.second.c_str() ) ) {
                alias = p.first.c_str();
                break;
            }
        }
    }

    AResourceBase * resource = FindResource( _ClassMeta, alias.CStr(), bMetadataMismatch, hash );
    if ( bMetadataMismatch ) {

        // Never return null

        resource = static_cast< AResourceBase * >( _ClassMeta.CreateInstance() );
        resource->InitializeDefaultObject();

        return resource;
    }

    if ( resource ) {
        //GLogger.Printf( "CACHING %s: Name %s, Physical path: \"%s\", Alias: \"%s\"\n", resource->GetObjectNameConstChar(), resource->FinalClassName(), resource->GetResourcePath().CStr(), resource->GetResourceAlias().CStr() );

        return resource;
    }

    AString path;

    if ( !_PhysicalPath || !*_PhysicalPath ) {
        RestorePhysicalPathFromAlias( alias.CStr(), path );
    } else {
        path = _PhysicalPath;
    }

    resource = static_cast< AResourceBase * >( _ClassMeta.CreateInstance() );
    resource->AddRef();
    resource->SetResourcePath( path );
    resource->SetResourceAlias( alias );
    resource->SetObjectName( alias );
    resource->InitializeFromFile( path.CStr() );

    ResourceHash.Insert( hash, ResourceCache.Size() );
    ResourceCache.Append( resource );

    return resource;
}

bool AResourceManager::RegisterResource( AResourceBase * _Resource, const char * _Alias ) {
    int hash;
    bool bMetadataMismatch;

    AResourceBase * resource = FindResource( _Resource->FinalClassMeta(), _Alias, bMetadataMismatch, hash );
    if ( resource || bMetadataMismatch ) {
        GLogger.Printf( "RegisterResource: Resource with same alias already exists (%s)\n", _Alias );
        return false;
    }

    _Resource->AddRef();
    _Resource->SetResourceAlias( _Alias );
    ResourceHash.Insert( hash, ResourceCache.Size() );
    ResourceCache.Append( _Resource );

    return true;
}

bool AResourceManager::UnregisterResource( AResourceBase * _Resource ) {
    int hash = _Resource->GetResourceAlias().HashCase();
    int i;

    for ( i = ResourceHash.First( hash ) ; i != -1 ; i = ResourceHash.Next( i ) ) {
        if ( !ResourceCache[i]->GetResourceAlias().Icmp( _Resource->GetResourceAlias() ) ) {
            if ( &ResourceCache[i]->FinalClassMeta() != &_Resource->FinalClassMeta() ) {
                GLogger.Printf( "UnregisterResource: %s class doesn't match meta data (%s vs %s)\n", _Resource->GetResourceAlias().CStr(), ResourceCache[i]->FinalClassName(), _Resource->FinalClassMeta().GetName() );
                return false;
            }
            break;
        }
    }

    if ( i == -1 ) {
        GLogger.Printf( "UnregisterResource: resource %s is not found\n", _Resource->GetResourceAlias().CStr() );
        return false;
    }

    _Resource->RemoveRef();
    ResourceHash.RemoveIndex( hash, i );
    ResourceCache.Remove( i );
    return true;
}

void AResourceManager::UnregisterResources( AClassMeta const & _ClassMeta ) {
    for ( int i = ResourceCache.Size() - 1 ; i >= 0 ; i-- ) {
        if ( ResourceCache[i]->FinalClassId() == _ClassMeta.GetId() ) {
            ResourceCache[i]->RemoveRef();

            ResourceHash.RemoveIndex( ResourceCache[i]->GetResourceAlias().HashCase(), i );
            ResourceCache.Remove( i );
        }
    }
}

void AResourceManager::UnregisterResources() {
    for ( int i = ResourceCache.Size() - 1 ; i >= 0 ; i-- ) {
        ResourceCache[i]->RemoveRef();
    }
    ResourceHash.Clear();
    ResourceCache.Clear();
}

void AResourceManager::SetResourceGUID( AGUID const & _GUID, const char * _PhysicalPath ) {
    SetResourceGUID( _GUID.ToString(), _PhysicalPath );
}

void AResourceManager::SetResourceGUID( AString const & _GUID, const char * _PhysicalPath ) {
    int hash = _GUID.HashCase();
    int i;

    for ( i = ResourceGUIDHash.First( hash ) ; i != -1 ; i = ResourceGUIDHash.Next( i ) ) {
        if ( !AString::Icmp( ResourceGUID[i].first.c_str(), _GUID.CStr() ) ) {
            break;
        }
    }

    if ( i != -1 ) {
        ResourceGUID[i].second = _PhysicalPath;
        return;
    }

    ResourceGUIDHash.Insert( hash, ResourceGUID.size() );
    ResourceGUID.push_back( std::make_pair( _GUID.CStr(), _PhysicalPath ) );
}

void AResourceManager::RestorePhysicalPathFromAlias( const char * _Alias, AString & _PhysicalPath ) const {
    if ( *_Alias == '/' ) {
        _PhysicalPath = _Alias;
        return;
    }

    int hash = Core::HashCase( _Alias, AString::Length( _Alias ) );
    int i;

    for ( i = ResourceGUIDHash.First( hash ) ; i != -1 ; i = ResourceGUIDHash.Next( i ) ) {
        if ( !AString::Icmp( ResourceGUID[i].first.c_str(), _Alias ) ) {
            _PhysicalPath = ResourceGUID[i].second.c_str();
            return;
        }
    }

    _PhysicalPath = _Alias;
}
