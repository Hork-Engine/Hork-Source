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



















#include <Engine/World/Public/Texture.h>
#include <Engine/World/Public/IndexedMesh.h>
#include <Engine/World/Public/MeshAsset.h>

TPodArray< FBaseObject * > ResourceCache;
THash<> ResourceHash;

void InitializeResourceManager() {

}

void DeinitializeResourceManager() {
    for ( int i = ResourceCache.Length() - 1 ; i >= 0 ; i-- ) {
        ResourceCache[i]->RemoveRef();
    }
    ResourceCache.Free();
    ResourceHash.Free();
}

FBaseObject * FindResource( FClassMeta const & _ClassMeta, const char * _Name, bool & _bMetadataMismatch, int & _Hash ) {
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

FBaseObject * FindResourceByName( const char * _Name ) {
    int hash = FCore::HashCase( _Name, FString::Length( _Name ) );

    for ( int i = ResourceHash.First( hash ) ; i != -1 ; i = ResourceHash.Next( i ) ) {
        if ( !ResourceCache[i]->GetName().Icmp( _Name ) ) {
            return ResourceCache[i];
        }
    }

    return nullptr;
}

FBaseObject * GetResource( FClassMeta const & _ClassMeta, const char * _Name, bool * _bResourceFoundResult, bool * _bMetadataMismatch ) {
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

FClassMeta const * GetResourceInfo( const char * _Name ) {
    int hash = FCore::HashCase( _Name, FString::Length( _Name ) );

    for ( int i = ResourceHash.First( hash ) ; i != -1 ; i = ResourceHash.Next( i ) ) {
        if ( !ResourceCache[i]->GetName().Icmp( _Name ) ) {
            return &ResourceCache[i]->FinalClassMeta();
        }
    }

    return nullptr;
}

bool RegisterResource( FBaseObject * _Resource ) {
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

FTexture * CreateUniqueTexture( const char * _FileName, const char * _Alias ) {
    int hash;
    bool bMetadataMismatch;
    FClassMeta const & classMeta = FTexture::ClassMeta();
    const char * resourceName = _Alias ? _Alias : _FileName;

    FBaseObject * resource = FindResource( classMeta, resourceName, bMetadataMismatch, hash );
    if ( bMetadataMismatch ) {

        // Never return null

        FTexture * object = static_cast< FTexture * >( classMeta.CreateInstance() );
        object->InitializeDefaultObject();

        return object;
    }

    if ( resource ) {
        GLogger.Printf( "Caching texture...\n" );
        return static_cast< FTexture * >( resource );
    }

    FTexture * object = static_cast< FTexture * >( classMeta.CreateInstance() );

    FImage image;
    if ( !image.LoadRawImage( _FileName, true, true ) ) {
        object->InitializeDefaultObject();
    } else {
        object->FromImage( image );
    }

    resource = object;
    resource->SetName( resourceName );
    resource->AddRef();

    ResourceHash.Insert( hash, ResourceCache.Length() );
    ResourceCache.Append( resource );

    return object;
}

FIndexedMesh * CreateUniqueMesh( const char * _FileName, const char * _Alias ) {
    int hash;
    bool bMetadataMismatch;
    FClassMeta const & classMeta = FIndexedMesh::ClassMeta();
    const char * resourceName = _Alias ? _Alias : _FileName;

    FBaseObject * resource = FindResource( classMeta, resourceName, bMetadataMismatch, hash );
    if ( bMetadataMismatch ) {

        // Never return null

        FIndexedMesh * object = static_cast< FIndexedMesh * >( classMeta.CreateInstance() );
        object->InitializeDefaultObject();

        return object;
    }

    if ( resource ) {
        GLogger.Printf( "Caching mesh...\n" );
        return static_cast< FIndexedMesh * >( resource );
    }

    FIndexedMesh * object = static_cast< FIndexedMesh * >( classMeta.CreateInstance() );

    FFileStream f;
    if ( !f.OpenRead( _FileName ) ) {
        object->InitializeDefaultObject();
    } else {
        FMeshAsset asset;
        asset.Read( f );

        TPodArray< FMaterialInstance * > matInstances;
        matInstances.Resize( asset.Materials.Length() );

        for ( int j = 0 ; j < asset.Materials.Length() ; j++ ) {
            FMeshMaterial const & material = asset.Materials[ j ];

            FMaterialInstance * matInst = static_cast< FMaterialInstance * >( FMaterialInstance::ClassMeta().CreateInstance() );
            //matInst->Material = Material;
            matInstances[j] = matInst;

            for ( int n = 0 ; n < 1/*material.NumTextures*/ ; n++ ) {
                FMaterialTexture const & texture = asset.Textures[ material.Textures[n] ];
                FTexture * texObj = CreateUniqueTexture( texture.FileName.ToConstChar() );
                matInst->SetTexture( n, texObj );
            }
        }

        bool bSkinned = asset.Weights.Length() == asset.Vertices.Length();

        object->Initialize( asset.Vertices.Length(), asset.Indices.Length(), asset.Subparts.size(), bSkinned, false );
        object->WriteVertexData( asset.Vertices.ToPtr(), asset.Vertices.Length(), 0 );
        object->WriteIndexData( asset.Indices.ToPtr(), asset.Indices.Length(), 0 );
        if ( bSkinned ) {
            object->WriteJointWeights( asset.Weights.ToPtr(), asset.Weights.Length(), 0 );
        }
        for ( int j = 0 ; j < object->GetSubparts().Length() ; j++ ) {
            FSubpart const & s = asset.Subparts[j];
            FIndexedMeshSubpart * subpart = object->GetSubpart( j );
            subpart->SetName( s.Name );
            subpart->BaseVertex = s.BaseVertex;
            subpart->FirstIndex = s.FirstIndex;
            subpart->VertexCount = s.VertexCount;
            subpart->IndexCount = s.IndexCount;
            subpart->BoundingBox = s.BoundingBox;
            subpart->MaterialInstance = matInstances[s.Material];
        }

        // TODO: load collision from file. This code is only for test!!!
        FCollisionSharedTriangleSoup * CollisionBody = object->BodyComposition.NewCollisionBody< FCollisionSharedTriangleSoup >();
        CollisionBody->TrisData = static_cast< FCollisionTriangleSoupData * >( FCollisionTriangleSoupData::ClassMeta().CreateInstance() );
        CollisionBody->TrisData->Initialize( (float *)&asset.Vertices.ToPtr()->Position, sizeof( asset.Vertices[0] ), asset.Vertices.Length(),
            asset.Indices.ToPtr(), asset.Indices.Length(), asset.Subparts.data(), asset.Subparts.size() );
    }

    resource = object;
    resource->SetName( resourceName );
    resource->AddRef();

    ResourceHash.Insert( hash, ResourceCache.Length() );
    ResourceCache.Append( resource );

    return object;
}

FSkeleton * CreateUniqueSkeleton( const char * _FileName, const char * _Alias ) {
    int hash;
    bool bMetadataMismatch;
    FClassMeta const & classMeta = FSkeleton::ClassMeta();
    const char * resourceName = _Alias ? _Alias : _FileName;

    FBaseObject * resource = FindResource( classMeta, resourceName, bMetadataMismatch, hash );
    if ( bMetadataMismatch ) {

        // Never return null

        FSkeleton * object = static_cast< FSkeleton * >( classMeta.CreateInstance() );
        object->InitializeDefaultObject();

        return object;
    }

    if ( resource ) {
        GLogger.Printf( "Caching skeleton...\n" );
        return static_cast< FSkeleton * >( resource );
    }

    FSkeleton * object = static_cast< FSkeleton * >( classMeta.CreateInstance() );

    FFileStream f;
    if ( !f.OpenRead( _FileName ) ) {
        object->InitializeDefaultObject();
    } else {
        FSkeletonAsset asset;
        asset.Read( f );

        object->Initialize( asset.Joints.ToPtr(), asset.Joints.Length() );
        for ( int i = 0 ; i < asset.Animations.size() ; i++ ) {
            FSkeletonAnimation * sanim = object->CreateAnimation();
            FSkeletalAnimationAsset const & animAsset = asset.Animations[i];
            sanim->Initialize( animAsset.FrameCount, animAsset.FrameDelta,
                               animAsset.AnimatedJoints.ToPtr(), animAsset.AnimatedJoints.Length(), animAsset.Bounds.ToPtr() );
        }
    }

    resource = object;
    resource->SetName( resourceName );
    resource->AddRef();

    ResourceHash.Insert( hash, ResourceCache.Length() );
    ResourceCache.Append( resource );

    return object;
}

bool UnregisterResource( FBaseObject * _Resource ) {
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

void UnregisterResources( FClassMeta const & _ClassMeta ) {
    for ( int i = ResourceCache.Length() - 1 ; i >= 0 ; i-- ) {
        if ( ResourceCache[i]->FinalClassId() == _ClassMeta.GetId() ) {
            ResourceCache[i]->RemoveRef();

            ResourceHash.Remove( ResourceCache[i]->GetName().HashCase(), i );
            ResourceCache.Remove( i );
        }
    }
}

void UnregisterResources() {
    for ( int i = ResourceCache.Length() - 1 ; i >= 0 ; i-- ) {
        ResourceCache[i]->RemoveRef();
    }
    ResourceHash.Clear();
    ResourceCache.Clear();
}
