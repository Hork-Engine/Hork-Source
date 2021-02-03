/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include <World/Public/Base/Resource.h>
#include <World/Public/Base/ResourceManager.h>
#include <Runtime/Public/Runtime.h>
#include <Core/Public/Logger.h>

AN_CLASS_META( AResource )

void AResource::InitializeDefaultObject()
{
    InitializeFromFile( GetDefaultResourcePath() );
}

void AResource::InitializeFromFile( const char * _Path )
{
    if ( !LoadFromPath( _Path ) ) {
        InitializeDefaultObject();
    }
}

bool AResource::LoadFromPath( const char * _Path )
{
    if ( !Core::StricmpN( _Path, "/Default/", 9 ) ) {
        LoadInternalResource( _Path );
        return true;
    }

    if ( !Core::StricmpN( _Path, "/Root/", 6 ) ) {
        _Path += 6;

        // try to load from resource pack
        AMemoryStream mem;
        if ( mem.OpenRead( _Path, *GResourceManager.GetGameResources() ) ) {
            return LoadResource( mem );
        }

        // try to load from file system
        AString fileSystemPath = GRuntime.GetRootPath() + _Path;

        AFileStream f;
        if ( f.OpenRead( fileSystemPath ) ) {
            return LoadResource( f );
        }

        return false;
    }

    if ( !Core::StricmpN( _Path, "/Common/", 8 ) ) {
        _Path += 1;

        // try to load from resource pack
        AMemoryStream mem;
        if ( mem.OpenRead( _Path + 7, *GResourceManager.GetCommonResources() ) ) {
            return LoadResource( mem );
        }

        // try to load from file system
        AFileStream f;
        if ( f.OpenRead( _Path ) ) {
            return LoadResource( f );
        }

        return false;
    }

    if ( !Core::StricmpN( _Path, "/FS/", 4 ) ) {
        _Path += 4;

        AFileStream f;
        if ( !f.OpenRead( _Path ) ) {
            return false;
        }

        return LoadResource( f );
    }

    if ( !Core::StricmpN( _Path, "/Embedded/", 10 ) ) {
        _Path += 10;

        AMemoryStream f;
        if ( !f.OpenRead( _Path, GetEmbeddedResources() ) ) {
            GLogger.Printf( "Failed to open /Embedded/%s\n", _Path );
            return false;
        }

        return LoadResource( f );
    }

    // Invalid path
    GLogger.Printf( "Invalid path \"%s\"\n", _Path );
    return false;
}
