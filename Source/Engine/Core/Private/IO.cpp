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

#include <Core/Public/IO.h>
#include <Core/Public/Logger.h>
#include <Core/Public/WindowsDefs.h>
#include <Core/Public/BaseMath.h>

#ifdef AN_OS_WIN32
#include <direct.h>     // _mkdir
#include <io.h>         // access
#endif
#ifdef AN_OS_LINUX
#include <sys/stat.h>   // _mkdir
#include <unistd.h>     // access
#endif

#include "miniz/miniz.h"

AFileStream::AFileStream()
    : Handle( nullptr )
    , Mode( M_Closed )
{
}

AFileStream::~AFileStream() {
    Close();
}

bool AFileStream::OpenRead( const char * _FileName ) {
    return Open( _FileName, M_Read );
}

bool AFileStream::OpenWrite( const char * _FileName ) {
    return Open( _FileName, M_Write );
}

bool AFileStream::OpenAppend( const char * _FileName ) {
    return Open( _FileName, M_Append );
}

static FILE * OpenFile( const char * _FileName, const char * _Mode ) {
    FILE *f;
#if defined(_MSC_VER)
    wchar_t wMode[64];

    if ( 0 == MultiByteToWideChar( CP_UTF8, 0, _Mode, -1, wMode, AN_ARRAY_SIZE( wMode ) ) )
        return NULL;

    int n = MultiByteToWideChar( CP_UTF8, 0, _FileName, -1, NULL, 0 );
    if ( 0 == n ) {
        return NULL;
    }

    wchar_t * wFilename = (wchar_t *)StackAlloc( n * sizeof( wchar_t ) );

    MultiByteToWideChar( CP_UTF8, 0, _FileName, -1, wFilename, n );

#if _MSC_VER >= 1400
    if ( 0 != _wfopen_s( &f, wFilename, wMode ) )
        f = NULL;
#else
    f = _wfopen( wFilename, wMode );
#endif
#else
    f = fopen( _FileName, _Mode );
#endif
    return f;
}

bool AFileStream::Open( const char * _FileName, int _Mode ) {
    Close();

    AString fn( _FileName );
    fn.FixPath();
    if ( fn.Length() && fn[ fn.Length() - 1 ] == '/' ) {
        GLogger.Printf( "Invalid file name %s\n", _FileName );
        return false;
    }

    AN_ASSERT( _Mode >= 0 && _Mode < 3 );

    if ( _Mode == M_Write || _Mode == M_Append ) {
        Core::MakeDir( fn.CStr(), true );
    }

    constexpr const char * fopen_mode[3] = { "rb", "wb", "ab" };
    FILE * f = OpenFile( fn.CStr(), fopen_mode[ _Mode ] );
    if ( !f ) {
        GLogger.Printf( "Couldn't open %s\n", fn.CStr() );
        return false;
    }

    Name = fn;
    Handle = f;
    Mode = _Mode;

    return true;
}

void AFileStream::Close() {
    if ( Mode == M_Closed ) {
        return;
    }

    Name.Clear();
    Mode = M_Closed;

    fclose( ( FILE * )Handle );
    Handle = nullptr;
}

const char * AFileStream::Impl_GetFileName() const {
    return Name.CStr();
}

int AFileStream::Impl_Read( void * _Buffer, int _SizeInBytes ) {
    if ( Mode != M_Read ) {
        GLogger.Printf( "Failed to read from %s (wrong mode)\n", Name.CStr() );
        return 0;
    }
    return fread( _Buffer, 1, _SizeInBytes, ( FILE * )Handle );
}

int AFileStream::Impl_Write( const void * _Buffer, int _SizeInBytes ) {
    if ( Mode != M_Write && Mode != M_Append ) {
        GLogger.Printf( "Failed to write %s (wrong mode)\n", Name.CStr() );
        return 0;
    }

    return fwrite( _Buffer, 1, _SizeInBytes, ( FILE * )Handle );
}

char * AFileStream::Impl_Gets( char * _StrBuf, int _StrSz ) {
    if ( Mode != M_Read ) {
        GLogger.Printf( "Failed to read from %s (wrong mode)\n", Name.CStr() );
        return nullptr;
    }

    return fgets( _StrBuf, _StrSz, ( FILE * )Handle );
}

void AFileStream::Impl_Flush() {
    fflush( ( FILE * )Handle );
}

long AFileStream::Impl_Tell() {
    return ftell( ( FILE * )Handle );
}

bool AFileStream::Impl_SeekSet( long _Offset ) {
    return fseek( ( FILE * )Handle, _Offset, SEEK_SET ) == 0;
}

bool AFileStream::Impl_SeekCur( long _Offset ) {
    return fseek( ( FILE * )Handle, _Offset, SEEK_CUR ) == 0;
}

bool AFileStream::Impl_SeekEnd( long _Offset ) {
    return fseek( ( FILE * )Handle, _Offset, SEEK_END ) == 0;
}

size_t AFileStream::Impl_SizeInBytes() {
    long offset = Tell();
    SeekEnd( 0 );
    long sizeInBytes = Tell();
    SeekSet( offset );
    return (size_t)sizeInBytes;
}

bool AFileStream::Impl_Eof() {
    return feof( ( FILE * )Handle ) != 0;
}

AMemoryStream::AMemoryStream()
    : Mode( M_Closed )
    , MemoryBuffer( nullptr )
    , MemoryBufferSize( 0 )
    , bMemoryBufferOwner( false )
    , MemoryBufferOffset( 0 )
    , Granularity( 1024 )
{
}

AMemoryStream::~AMemoryStream() {
    Close();
}

void * AMemoryStream::Alloc( size_t _SizeInBytes ) {
    return GHeapMemory.Alloc( _SizeInBytes );
}

void * AMemoryStream::Realloc( void * _Data, size_t _SizeInBytes ) {
    return GHeapMemory.Realloc( _Data, _SizeInBytes, true );
}

void AMemoryStream::Free( void * _Data ) {
    GHeapMemory.Free( _Data );
}

bool AMemoryStream::OpenRead( AString const & _FileName, const byte * _MemoryBuffer, size_t _SizeInBytes ) {
    return OpenRead( _FileName.CStr(), _MemoryBuffer, _SizeInBytes );
}

bool AMemoryStream::OpenRead( AString const & _FileName, AArchive & _Archive ) {
    return OpenRead( _FileName.CStr(), _Archive );
}

bool AMemoryStream::OpenWrite( AString const & _FileName, byte * _MemoryBuffer, size_t _SizeInBytes ) {
    return OpenWrite( _FileName.CStr(), _MemoryBuffer, _SizeInBytes );
}

bool AMemoryStream::OpenWrite( AString const & _FileName, size_t _ReservedSize ) {
    return OpenWrite( _FileName.CStr(), _ReservedSize );
}

bool AMemoryStream::OpenRead( const char * _FileName, const byte * _MemoryBuffer, size_t _SizeInBytes ) {
    Close();
    Name = _FileName;
    MemoryBuffer = const_cast< byte * >( _MemoryBuffer );
    MemoryBufferSize = _SizeInBytes;
    bMemoryBufferOwner = false;
    MemoryBufferOffset = 0;
    Mode = M_Read;
    return true;
}

bool AMemoryStream::OpenRead( const char * _FileName, AArchive & _Archive ) {
    Close();

    if ( !_Archive.ExtractFileToHeapMemory( _FileName, &MemoryBuffer, &MemoryBufferSize ) ) {
        return false;
    }

    Name = _FileName;
    bMemoryBufferOwner = true;
    MemoryBufferOffset = 0;
    Mode = M_Read;
    return true;
}

bool AMemoryStream::OpenWrite( const char * _FileName, byte * _MemoryBuffer, size_t _SizeInBytes ) {
    Close();
    Name = _FileName;
    MemoryBuffer = _MemoryBuffer;
    MemoryBufferSize = _SizeInBytes;
    bMemoryBufferOwner = false;
    MemoryBufferOffset = 0;
    Mode = M_Write;
    return true;
}

bool AMemoryStream::OpenWrite( const char * _FileName, size_t _ReservedSize ) {
    Close();
    Name = _FileName;
    MemoryBuffer = ( byte * )Alloc( _ReservedSize );
    MemoryBufferSize = _ReservedSize;
    bMemoryBufferOwner = true;
    MemoryBufferOffset = 0;
    Mode = M_Write;
    return true;
}

void AMemoryStream::Close() {
    if ( Mode == M_Closed ) {
        return;
    }

    Name.Clear();
    Mode = M_Closed;

    if ( bMemoryBufferOwner ) {
        Free( MemoryBuffer );
    }
}

const char * AMemoryStream::Impl_GetFileName() const {
    return Name.CStr();
}

int AMemoryStream::Impl_Read( void * _Buffer, int _SizeInBytes ) {
    if ( Mode != M_Read ) {
        GLogger.Printf( "Failed to read from %s (wrong mode)\n", Name.CStr() );
        return 0;
    }

    int bytesRemaining = MemoryBufferSize - MemoryBufferOffset;
    if ( _SizeInBytes > bytesRemaining ) {
        _SizeInBytes = bytesRemaining;
    }

    if ( _SizeInBytes > 0 ) {
        Core::Memcpy( _Buffer, MemoryBuffer + MemoryBufferOffset, _SizeInBytes );
        MemoryBufferOffset += _SizeInBytes;
    }

    return _SizeInBytes;
}

int AMemoryStream::Impl_Write( const void * _Buffer, int _SizeInBytes ) {
    if ( Mode != M_Write ) {
        GLogger.Printf( "Failed to write %s (wrong mode)\n", Name.CStr() );
        return 0;
    }

    int requiredSize = MemoryBufferOffset + _SizeInBytes;
    if ( requiredSize > MemoryBufferSize ) {
        if ( !bMemoryBufferOwner ) {
            GLogger.Printf( "Failed to write %s (buffer overflowed)\n", Name.CStr() );
            return 0;
        }
        const int mod = requiredSize % Granularity;
        requiredSize = mod ? requiredSize + Granularity - mod : requiredSize;
        MemoryBuffer = ( byte * )Realloc( MemoryBuffer, requiredSize );
        MemoryBufferSize = requiredSize;
    }
    Core::Memcpy( MemoryBuffer + MemoryBufferOffset, _Buffer, _SizeInBytes );
    MemoryBufferOffset += _SizeInBytes;
    return _SizeInBytes;
}

char * AMemoryStream::Impl_Gets( char * _StrBuf, int _StrSz ) {
    if ( Mode != M_Read ) {
        GLogger.Printf( "Failed to read from %s (wrong mode)\n", Name.CStr() );
        return nullptr;
    }

    if ( _StrSz == 0 || MemoryBufferOffset >= MemoryBufferSize ) {
        return nullptr;
    }

    int charsCount = _StrSz-1;
    if ( MemoryBufferOffset + charsCount > MemoryBufferSize ) {
        charsCount = MemoryBufferSize - MemoryBufferOffset;
    }

    char * memoryPointer, * memory = ( char * )&MemoryBuffer[ MemoryBufferOffset ];
    char * stringPointer = _StrBuf;

    for ( memoryPointer = memory ; memoryPointer < &memory[charsCount] ; memoryPointer++ ) {
        if ( *memoryPointer == '\n' ) {
            *stringPointer++ = *memoryPointer++;
            break;
        }
        *stringPointer++ = *memoryPointer;
    }

    *stringPointer = '\0';
    MemoryBufferOffset += memoryPointer - memory;

    return _StrBuf;
}

void AMemoryStream::Impl_Flush() {
}

long AMemoryStream::Impl_Tell() {
    return static_cast< long >( MemoryBufferOffset );
}

bool AMemoryStream::Impl_SeekSet( long _Offset ) {
    const int newOffset = _Offset;

#if 0
    if ( newOffset < 0 || newOffset > MemoryBufferSize ) {
        GLogger.Printf( "Bad seek offset for %s\n", Name.CStr() );
        return false;
    }
#endif

    MemoryBufferOffset = Math::Clamp( newOffset, 0, MemoryBufferSize );
    return true;
}

bool AMemoryStream::Impl_SeekCur( long _Offset ) {
    const int newOffset = MemoryBufferOffset + _Offset;

#if 0
    if ( newOffset < 0 || newOffset > MemoryBufferSize ) {
        GLogger.Printf( "Bad seek offset for %s\n", Name.CStr() );
        return false;
    }
#endif

    MemoryBufferOffset = Math::Clamp( newOffset, 0, MemoryBufferSize );
    return true;
}

bool AMemoryStream::Impl_SeekEnd( long _Offset ) {
    const int newOffset = MemoryBufferSize + _Offset;

#if 0
    if ( newOffset < 0 || newOffset > MemoryBufferSize ) {
        GLogger.Printf( "Bad seek offset for %s\n", Name.CStr() );
        return false;
    }
#endif

    MemoryBufferOffset = Math::Clamp( newOffset, 0, MemoryBufferSize );
    return true;
}

size_t AMemoryStream::Impl_SizeInBytes() {
    return static_cast< size_t >(MemoryBufferSize);
}

bool AMemoryStream::Impl_Eof() {
    return MemoryBufferOffset >= MemoryBufferSize;
}

byte * AMemoryStream::GrabMemory() {
    return MemoryBuffer;
}


AArchive::AArchive()
 : Handle( nullptr )
{

}

AArchive::~AArchive() {
    Close();
}

bool AArchive::Open( const char * _ArchiveName ) {
    Close();

    mz_zip_archive arch;

    Core::ZeroMem( &arch, sizeof( arch ) );

    mz_bool status = mz_zip_reader_init_file( &arch, _ArchiveName, 0 );
    if ( !status ) {
        GLogger.Printf( "Couldn't open archive %s\n", _ArchiveName );
        return false;
    }

    Handle = GZoneMemory.Alloc( sizeof( mz_zip_archive ) );
    Core::Memcpy( Handle, &arch, sizeof( arch ) );

    // Keep pointer valid
    ((mz_zip_archive *)Handle)->m_pIO_opaque = Handle;

    return true;
}

void AArchive::Close() {
    if ( !Handle ) {
        return;
    }

    mz_zip_reader_end( (mz_zip_archive *)Handle );

    GZoneMemory.Free( Handle );
    Handle = nullptr;
}

int AArchive::GetNumFiles() const {
    return mz_zip_reader_get_num_files( (mz_zip_archive *)Handle );
}

int AArchive::LocateFile( const char * _FileName ) {
    return mz_zip_reader_locate_file( (mz_zip_archive *)Handle, _FileName, NULL, 0 );
}

#define MZ_ZIP_CDH_COMPRESSED_SIZE_OFS 20
#define MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS 24

extern "C" const mz_uint8 *mz_zip_get_cdh( mz_zip_archive *pZip, mz_uint file_index );

bool AArchive::GetFileSize( int _FileIndex, size_t * _CompressedSize, size_t * _UncompressedSize ) {
    // All checks are processd in mz_zip_get_cdh
    const mz_uint8 *p = mz_zip_get_cdh( (mz_zip_archive *)Handle, _FileIndex );
    if ( !p ) {
        return false;
    }

    if ( _CompressedSize ) {
        *_CompressedSize = MZ_READ_LE32(p + MZ_ZIP_CDH_COMPRESSED_SIZE_OFS);
    }

    if ( _UncompressedSize ) {
        *_UncompressedSize = MZ_READ_LE32(p + MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS);
    }
    return true;
}

bool AArchive::ExtractFileToMemory( int _FileIndex, void * _MemoryBuffer, size_t _SizeInBytes ) {
    // All checks are processd in mz_zip_reader_extract_to_mem
    return !!mz_zip_reader_extract_to_mem( (mz_zip_archive *)Handle, _FileIndex, _MemoryBuffer, _SizeInBytes, 0 );
}

bool AArchive::ExtractFileToHeapMemory( const char * _FileName, byte ** _HeapMemoryPtr, int * _SizeInBytes ) {
    size_t uncompSize;

    *_HeapMemoryPtr = nullptr;
    *_SizeInBytes = 0;

    int fileIndex = LocateFile( _FileName );
    if ( fileIndex < 0 ) {
        return false;
    }

    if ( !GetFileSize( fileIndex, NULL, &uncompSize ) ) {
        return false;
    }

    void * pBuf = GHeapMemory.Alloc( uncompSize );

    if ( !ExtractFileToMemory( fileIndex, pBuf, uncompSize ) ) {
        GHeapMemory.Free( pBuf );
        return false;
    }

    *_HeapMemoryPtr = (byte *)pBuf;
    *_SizeInBytes = uncompSize;

    return true;
}

bool AArchive::ExtractFileToHunkMemory( const char * _FileName, byte ** _HunkMemoryPtr, int * _SizeInBytes, int * _HunkMark ) {
    size_t uncompSize;

    *_HunkMark = GHunkMemory.SetHunkMark();

    *_HunkMemoryPtr = nullptr;
    *_SizeInBytes = 0;

    int fileIndex = LocateFile( _FileName );
    if ( fileIndex < 0 ) {
        return false;
    }

    if ( !GetFileSize( fileIndex, NULL, &uncompSize ) ) {
        return false;
    }

    void * pBuf = GHunkMemory.Alloc( uncompSize );

    if ( !ExtractFileToMemory( fileIndex, pBuf, uncompSize ) ) {
        GHunkMemory.ClearToMark( *_HunkMark );
        return false;
    }

    *_HunkMemoryPtr = (byte *)pBuf;
    *_SizeInBytes = uncompSize;

    return true;
}


namespace Core {

void MakeDir( const char * _Directory, bool _FileName ) {
    size_t len = Core::Strlen( _Directory );
    if ( !len ) {
        return;
    }
    char * tmpStr = ( char * )GZoneMemory.Alloc( len + 1 );
    Core::Memcpy( tmpStr, _Directory, len + 1 );
    char * p = tmpStr;
    #ifdef AN_OS_WIN32
    if ( len >= 3 && _Directory[1] == ':' ) {
        p += 3;
        _Directory += 3;
    }
    #endif
    for ( ; *_Directory ; p++, _Directory++ ) {
        if ( Core::IsPathSeparator( *p ) ) {
            *p = 0;
            #ifdef AN_COMPILER_MSVC
            mkdir( tmpStr );
            #else
            mkdir( tmpStr, S_IRWXU | S_IRWXG | S_IRWXO );
            #endif
            *p = *_Directory;
        }
    }
    if ( !_FileName ) {
        #ifdef AN_COMPILER_MSVC
        mkdir( tmpStr );
        #else
        mkdir( tmpStr, S_IRWXU | S_IRWXG | S_IRWXO );
        #endif
    }
    GZoneMemory.Free( tmpStr );
}

bool IsFileExists( const char * _FileName ) {
    AString s = _FileName;
    s.FixSeparator();
    return access( s.CStr(), 0 ) == 0;
}

void RemoveFile( const char * _FileName ) {
    AString s = _FileName;
    s.FixPath();
#if defined AN_OS_LINUX
    ::remove( s.CStr() );
#elif defined AN_OS_WIN32
    int n = MultiByteToWideChar( CP_UTF8, 0, s.CStr(), -1, NULL, 0 );
    if ( 0 != n ) {
        wchar_t * wFilename = (wchar_t *)StackAlloc( n * sizeof( wchar_t ) );

        MultiByteToWideChar( CP_UTF8, 0, s.CStr(), -1, wFilename, n );

        ::DeleteFile( wFilename );
    }
#else
    static_assert( 0, "TODO: Implement RemoveFile for current build settings" );
#endif
}

}
