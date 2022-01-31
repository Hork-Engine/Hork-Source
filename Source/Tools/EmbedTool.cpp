/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

/*

Resource embedding tool program

*/

#include <string>
#include <functional>
#include <algorithm>
#include <miniz/miniz.h>

using byte = uint8_t;

using SReadDirCallback = std::function< void( std::string const & FileName, bool bIsDirectory ) >;

#ifdef __linux__

#include <dirent.h>
#include <sys/stat.h>
#include <functional>

void ReadDir( std::string const & Path, bool bSubDirs, SReadDirCallback Callback )
{
    std::string fn;
    DIR * dir = opendir( Path.c_str() );
    if ( dir ) {
        while ( 1 ) {
            dirent * entry = readdir( dir );
            if ( !entry ) {
                break;
            }

            fn = Path;
            int len = Path.length();
            if ( len > 1 && Path[len-1] != '/' ) {
                fn += "/";
            }
            fn += entry->d_name;

            struct stat statInfo;
            lstat( fn.c_str(), &statInfo );

            if ( S_ISDIR( statInfo.st_mode ) ) {
                if ( bSubDirs ) {
                    if ( !strcmp( ".", entry->d_name )
                         || !strcmp( "..", entry->d_name ) ) {
                        continue;
                    }
                    ReadDir( fn.c_str(), bSubDirs, Callback );
                }
                Callback( fn, true );
            } else {
                Callback( Path + "/" + entry->d_name, false );
            }
        }
        closedir( dir );
    }
}

#endif

#ifdef _WIN32

#include <Windows.h>

#pragma warning( disable : 4996 )

void ReadDir( std::string const & Path, bool bSubDirs, SReadDirCallback Callback )
{
    std::string fn;

    HANDLE fh;
    WIN32_FIND_DATAA fd;

    if ( (fh = FindFirstFileA( (Path +  "/*.*").c_str(), &fd )) != INVALID_HANDLE_VALUE ) {
        do {
            fn = Path;
            int len = (int)Path.length();
            if ( len > 1 && Path[len-1] != '/' ) {
                fn += "/";
            }
            fn += fd.cFileName;

            if ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
                if ( bSubDirs ) {
                    if ( !strcmp( ".", fd.cFileName )
                         || !strcmp( "..", fd.cFileName ) ) {
                        continue;
                    }
                    ReadDir( fn.c_str(), bSubDirs, Callback );
                }
                Callback( fn, true );
            } else {
                Callback( Path + "/" + fd.cFileName, false );
            }
        } while ( FindNextFileA( fh, &fd ) );

        FindClose( fh );
    }
}

#endif

void FixSeparator( std::string & s ) {
    std::replace( s.begin(), s.end(), '\\', '/' );
}

bool IsPathSeparator( char _Char ) {
    return _Char == '/' || _Char == '\\';
}

void ClipFilename( std::string & s ) {
    int len = (int)s.length();
    while ( --len > 0 && !IsPathSeparator( s[len] ) ) {
        ;
    }
    s.resize( len );
}

constexpr int32_t INT64_HIGH_INT( uint64_t i64 ) {
    return i64 >> 32;
}

constexpr int32_t INT64_LOW_INT( uint64_t i64 ) {
    return i64 & 0xFFFFFFFF;
}

constexpr size_t Align( size_t N, size_t Alignment ) {
    return ( N + ( Alignment - 1 ) ) & ~( Alignment - 1 );
}

void WriteBinaryToC( FILE * Stream, const char * SymName, const void * pData, size_t SizeInBytes, bool bEncodeBase85 )
{
    const byte * bytes = (const byte *)pData;

    fprintf( Stream, "#include <stdio.h>\n" );
    fprintf( Stream, "#include <stdint.h>\n" );

    if ( bEncodeBase85 ) {
        fprintf( Stream, "const char %s_Data_Base85[%d+1] =\n    \"", SymName, (int)((SizeInBytes+3)/4)*5 );
        char prev_c = 0;
        for ( size_t i = 0; i < SizeInBytes; i += 4 ) {
            uint32_t d = *(uint32_t *)(bytes + i);
            for ( int j = 0; j < 5; j++, d /= 85 ) {
                unsigned int x = (d % 85) + 35;
                char c = (x >= '\\') ? x + 1 : x;
                fprintf( Stream, (c == '?' && prev_c == '?') ? "\\%c" : "%c", c );
                prev_c = c;
            }
            if ( (i % 112) == 112-4 )
                fprintf( Stream, "\"\n    \"" );
        }
        fprintf( Stream, "\";\n\n" );
    } else {
        fprintf( Stream, "const size_t %s_Size = %d;\n", SymName, (int)SizeInBytes );
        fprintf( Stream, "const uint64_t %s_Data[%d] =\n{", SymName, (int)Align( SizeInBytes, 8 ) );
        int column = 0;
        for ( size_t i = 0; i < SizeInBytes; i += 8 ) {
            uint64_t d = *(uint64_t *)(bytes + i);
            if ( (column++ % 6) == 0 ) {
                fprintf( Stream, "\n    0x%08x%08x", INT64_HIGH_INT( d ), INT64_LOW_INT( d ) );
            } else {
                fprintf( Stream, "0x%08x%08x", INT64_HIGH_INT( d ), INT64_LOW_INT( d ) );
            }
            if ( i + 8 < SizeInBytes ) {
                fprintf( Stream, ", " );
            }
        }
        fprintf( Stream, "\n};\n\n" );
    }
}

int GenerateEmbeddedResources( const char * SourcePath, const char * ResultFile )
{
    std::string path = SourcePath;
    FixSeparator( path );

    void * pBuf = nullptr;
    size_t size = 0;

    mz_zip_archive zip;
    memset( &zip, 0, sizeof( zip ) );

    printf( "Source '%s'\n"
            "Destination: '%s'\n", SourcePath, ResultFile );

    if ( mz_zip_writer_init_heap_v2( &zip, 0, 0, 0 ) ) {
        ReadDir( path, true,
                 [&zip, &path]( std::string const & FileName, bool bIsDirectory )
        {
            if ( !bIsDirectory ) {
                printf( "Embedding '%s'\n", &FileName[path.length()] );

                mz_bool status = mz_zip_writer_add_file( &zip, &FileName[path.length()], FileName.c_str(), nullptr, 0, MZ_UBER_COMPRESSION );
                if ( !status ) {
                    printf( "Failed to zip %s\n", FileName.c_str() );
                }
            }
        } );

        mz_zip_writer_finalize_heap_archive( &zip, &pBuf, &size );
        mz_zip_writer_end( &zip );
    }

    if ( pBuf ) {
        path = ResultFile;
        FixSeparator( path );

        FILE * f;
        f = fopen( path.c_str(), "w" );
        if ( f ) {
            WriteBinaryToC( f, "EmbeddedResources", pBuf, size, false );
            fclose( f );
        }

        mz_free( pBuf );
    }

    return 0;
}

int GenerateEmbeddedResourcesZip( const char * SourcePath, const char * ResultFile )
{
    std::string path = SourcePath;
    FixSeparator( path );

    mz_zip_archive zip;
    memset( &zip, 0, sizeof( zip ) );

    printf( "Source '%s'\n"
            "Destination: '%s'\n", SourcePath, ResultFile );

    if ( mz_zip_writer_init_file( &zip, ResultFile, 0 ) ) {
        ReadDir( path, true,
                 [&zip, &path]( std::string const & FileName, bool bIsDirectory )
        {
            if ( !bIsDirectory ) {
                printf( "Embedding '%s'\n", &FileName[path.length()] );

                mz_bool status = mz_zip_writer_add_file( &zip, &FileName[path.length()], FileName.c_str(), nullptr, 0, MZ_UBER_COMPRESSION );
                if ( !status ) {
                    printf( "Failed to zip %s\n", FileName.c_str() );
                }
            }
        } );

        mz_zip_writer_finalize_archive( &zip );
        mz_zip_writer_end( &zip );
    }

    return 0;
}

int main( int argc, char *argv[] ) {
    printf( "Start embedding\n" );
    if ( argc < 3 ) {
        printf( "Not enough command line parameters\n" );
        return -1;
    }
    //GenerateEmbeddedResourcesZip( argv[1], (std::string(argv[2])+".zip").c_str() );
    return GenerateEmbeddedResources( argv[1], argv[2] );
}

