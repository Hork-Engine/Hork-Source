/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#ifdef AN_OS_WIN32
#include <direct.h>     // _mkdir
#include <io.h>         // access
#endif
#ifdef AN_OS_LINUX
#include <sys/stat.h>   // _mkdir
#include <unistd.h>     // access
#endif

#include "miniz/miniz.h"

//////////////////////////////////////////////////////////////////////////////////////////
//
// Common IO functions
//
//////////////////////////////////////////////////////////////////////////////////////////

namespace Core {

void MakeDir( const char * _Directory, bool _FileName ) {
    size_t strLen = strlen( _Directory );
    if ( !strLen ) {
        return;
    }
    char * tmpStr = ( char * )GZoneMemory.Alloc( strLen + 1 );
    Core::Memcpy( tmpStr, _Directory, strLen+1 );
    char * p = tmpStr;
    #ifdef AN_OS_WIN32
    if ( strLen >= 3 && _Directory[1] == ':' ) {
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
    s.FixSeparator();
#if defined AN_OS_LINUX
    ::remove( _FileName );
#elif defined AN_OS_WIN32
    ::DeleteFileA( _FileName );
#else
    static_assert( 0, "RemoveFile not implemented" );
#endif
}

}

//////////////////////////////////////////////////////////////////////////////////////////
//
// File stream
//
//////////////////////////////////////////////////////////////////////////////////////////

AFileStream::AFileStream()
    : Mode( M_Closed )
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

static FILE * OpenFile( const char * filename, const char * mode )
{
    FILE *f;
#if defined(_MSC_VER)
    wchar_t wMode[64];
    wchar_t wFilename[1024];
    if ( 0 == MultiByteToWideChar( 65001 /* UTF8 */, 0, filename, -1, wFilename, sizeof( wFilename ) ) )
        return 0;
    if ( 0 == MultiByteToWideChar( 65001 /* UTF8 */, 0, mode, -1, wMode, sizeof( wMode ) ) )
        return 0;
#if _MSC_VER >= 1400
    if ( 0 != _wfopen_s( &f, wFilename, wMode ) )
        f = 0;
#else
    f = _wfopen( wFilename, wMode );
#endif
#else
    f = fopen( filename, mode );
#endif
    return f;
}

bool AFileStream::Open( const char * _FileName, int _Mode ) {
    Close();

    FileName = _FileName;
    FileName.FixSeparator();
    if ( FileName.Length() && FileName[ FileName.Length() - 1 ] == '/' ) {

        if ( bVerbose ) {
            GLogger.Printf( "AFileStream::Open: invalid file name %s\n", _FileName );
        }

        FileName.Free();

        return false;
    }

    constexpr const char * fopen_mode[3] = { "rb", "wb", "ab" };

    AN_ASSERT_( _Mode >= 0 && _Mode < 3, "Invalid mode" );

    if ( _Mode == M_Write || _Mode == M_Append ) {
        Core::MakeDir( FileName.CStr(), true );
    }

#ifdef AN_OS_ANDROID
    AAsset * f = AAssetManager_open( __AssetManager, FileName.CStr(), _Access );
#else
    FILE * f = OpenFile( FileName.CStr(), fopen_mode[ _Mode ] );
#endif

    if ( !f ) {
        if ( bVerbose ) {
            GLogger.Printf( "AFileStream::Open: couldn't open %s\n", FileName.CStr() );
        }
        FileName.Free();
        return false;
    }

    FileHandle = f;
    Mode = _Mode;

    return true;
}

void AFileStream::Close() {
    if ( Mode == M_Closed ) {
        return;
    }

    Mode = M_Closed;

#ifdef AN_OS_ANDROID
    AAsset_close( ( AAsset * )FileHandle );
#else
    fclose( ( FILE * )FileHandle );
#endif
}

const char * AFileStream::Impl_GetFileName() const {
    return FileName.CStr();
}

int AFileStream::Impl_Read( void * _Buffer, int _SizeInBytes ) {
    if ( Mode != M_Read ) {
        GLogger.Printf( "AFileStream::Read: expected read mode for %s\n", FileName.CStr() );
        return 0;
    }

    int count;
    int numTries = 0;
    byte * data = ( byte * )_Buffer;

    for ( int bytesCount = _SizeInBytes ; bytesCount > 0 ; bytesCount -= count, data += count ) {
#ifdef AN_OS_ANDROID
        count = AAsset_read( ( AAsset * )FileHandle, data, bytesCount );
#else
        count = fread( data, 1, bytesCount, ( FILE * )FileHandle );
#endif
        if ( count == 0 ) {
            if ( numTries > 0 ) {
                if ( ferror( ( FILE * )FileHandle ) ) {
                    GLogger.Printf( "AFileStream::Read: read error %s\n", FileName.CStr() );
                } else if ( feof( ( FILE * )FileHandle ) ) {
                    //GLogger.Printf( "AFileStream::Read: unexpected end of file %s\n", FileName.CStr() );
                }
                bytesCount = _SizeInBytes - bytesCount;
                return bytesCount;
            }
            numTries++;
        } else  if ( count < 0 ) {
            GLogger.Printf( "AFileStream::Read: failed to read from %s\n", FileName.CStr() );
            return 0;
        }
    }
    return _SizeInBytes;
}

int AFileStream::Impl_Write( const void * _Buffer, int _SizeInBytes ) {
#ifdef AN_OS_ANDROID
    #error "write mode is not supported on current platform";
#else

    if ( Mode != M_Write && Mode != M_Append ) {
        GLogger.Printf( "AFileStream::Write: expected write or append mode for %s\n", FileName.CStr() );
        return 0;
    }

    int count;
    int numTries = 0;
    byte * data = ( byte * )_Buffer;

    for ( int bytesCount = _SizeInBytes ; bytesCount > 0 ; bytesCount -= count, data += count ) {
        count = fwrite( data, 1, bytesCount, ( FILE * )FileHandle );
        if ( count == 0 ) {
            if ( numTries > 0 ) {
                GLogger.Printf( "AFileStream::Write: write error %s\n", FileName.CStr() );
                bytesCount = _SizeInBytes - bytesCount;
                return bytesCount;
            }
            numTries++;
        } else  if ( count == -1 ) {
            GLogger.Printf( "AFileStream::Write: failed to write to %s\n", FileName.CStr() );
            return 0;
        }
    }

    return _SizeInBytes;
#endif
}

char * AFileStream::Impl_Gets( char * _StrBuf, int _StrSz ) {
#ifdef AN_OS_ANDROID
    #error "Gets() is not supported on current platform";
#else

    if ( Mode != M_Read ) {
        GLogger.Printf( "AFileStream::Gets: expected read mode for %s\n", FileName.CStr() );
        return nullptr;
    }

    return fgets( _StrBuf, _StrSz, ( FILE * )FileHandle );
#endif
}

void AFileStream::Impl_Flush() {
#ifdef AN_OS_ANDROID
    #error "Flush() is not supported on current platform";
#else
    fflush( ( FILE * )FileHandle );
#endif
}

long AFileStream::Impl_Tell() {
#ifdef AN_OS_ANDROID
    return AAsset_getLength( ( AAsset * )FileHandle ) - AAsset_getRemainingLength( ( AAsset * )FileHandle );
#else
    return ftell( ( FILE * )FileHandle );
#endif
}

int AFileStream::Impl_SeekSet( long _Offset ) {
#ifdef AN_OS_ANDROID
    return AAsset_seek( ( AAsset * )FileHandle, _Offset, SEEK_SET );
#else
    return fseek( ( FILE * )FileHandle, _Offset, SEEK_SET );
#endif
}

int AFileStream::Impl_SeekCur( long _Offset ) {
#ifdef AN_OS_ANDROID
    return AAsset_seek( ( AAsset * )FileHandle, _Offset, SEEK_CUR );
#else
    return fseek( ( FILE * )FileHandle, _Offset, SEEK_CUR );
#endif
}

int AFileStream::Impl_SeekEnd( long _Offset ) {
#ifdef AN_OS_ANDROID
    return AAsset_seek( ( AAsset * )FileHandle, _Offset, SEEK_END );
#else
    return fseek( ( FILE * )FileHandle, _Offset, SEEK_END );
#endif
}

long AFileStream::Impl_SizeInBytes() {
    long Offset = Tell();
    SeekEnd( 0 );
    long FileSz = Tell();
    SeekSet( Offset );
    return FileSz;
}

bool AFileStream::Impl_Eof() {
#ifdef AN_OS_ANDROID
    TODO
#else
    return feof( ( FILE * )FileHandle ) != 0;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Memory public
//
//////////////////////////////////////////////////////////////////////////////////////////

AMemoryStream::AMemoryStream()
    : Mode( M_Closed )
    , MemoryBuffer( nullptr )
    , MemoryBufferSize( 0 )
    , bMemoryBufferOwner( false )
    , MemoryBufferOffset( 0 )
{
}

AMemoryStream::~AMemoryStream() {
    Close();
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
    FileName = _FileName;
    MemoryBuffer = const_cast< byte * >( _MemoryBuffer );
    MemoryBufferSize = _SizeInBytes;
    bMemoryBufferOwner = false;
    MemoryBufferOffset = 0;
    Mode = M_Read;
    return true;
}

bool AMemoryStream::OpenRead( const char * _FileName, AArchive & _Archive ) {
    Close();

    if ( !_Archive.ReadFileToZoneMemory( _FileName, &MemoryBuffer, &MemoryBufferSize ) ) {
        return false;
    }

    FileName = _FileName;
    bMemoryBufferOwner = true;
    MemoryBufferOffset = 0;
    Mode = M_Read;
    return true;
}

bool AMemoryStream::OpenWrite( const char * _FileName, byte * _MemoryBuffer, size_t _SizeInBytes ) {
    Close();
    FileName = _FileName;
    MemoryBuffer = _MemoryBuffer;
    MemoryBufferSize = _SizeInBytes;
    bMemoryBufferOwner = false;
    MemoryBufferOffset = 0;
    Mode = M_Write;
    return true;
}

bool AMemoryStream::OpenWrite( const char * _FileName, size_t _ReservedSize ) {
    Close();
    FileName = _FileName;
    MemoryBuffer = ( byte * )GZoneMemory.Alloc( _ReservedSize );
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

    Mode = M_Closed;

    if ( bMemoryBufferOwner ) {
        GZoneMemory.Free( MemoryBuffer );
    }
}

const char * AMemoryStream::Impl_GetFileName() const {
    return FileName.CStr();
}

int AMemoryStream::Impl_Read( void * _Buffer, int _SizeInBytes ) {
    if ( Mode != M_Read ) {
        GLogger.Printf( "AMemoryStream::Read: expected read mode for %s\n", FileName.CStr() );
        return 0;
    }

    int bytesCount = _SizeInBytes;
    if ( MemoryBufferOffset + _SizeInBytes > MemoryBufferSize ) {
        bytesCount = MemoryBufferSize - MemoryBufferOffset;
        //GLogger.Printf( "AFileStream::Read: unexpected end of file %s\n", FileName.CStr() );
    }

    if ( bytesCount > 0 ) {
        Core::Memcpy( _Buffer, MemoryBuffer + MemoryBufferOffset, bytesCount );
        MemoryBufferOffset += bytesCount;
    }

    return bytesCount;
}

int AMemoryStream::Impl_Write( const void * _Buffer, int _SizeInBytes ) {
    if ( Mode != M_Write ) {
        GLogger.Printf( "AMemoryStream::Write: expected write mode for %s\n", FileName.CStr() );
        return 0;
    }

    int diff = MemoryBufferOffset + _SizeInBytes - MemoryBufferSize;
    if ( diff > 0 ) {
        if ( !bMemoryBufferOwner ) {
            GLogger.Printf( "AMemoryStream::Write: buffer overflowed for %s\n", FileName.CStr() );
            return 0;
        }
        const int GRANULARITY = 256;
        int newLength = MemoryBufferSize + diff;
        int mod = newLength % GRANULARITY;
        if ( mod ) {
            newLength = newLength + GRANULARITY - mod;
        }
        MemoryBuffer = ( byte * )GZoneMemory.Realloc( MemoryBuffer, newLength, true );
        MemoryBufferSize = newLength;
    }
    Core::Memcpy( MemoryBuffer + MemoryBufferOffset, _Buffer, _SizeInBytes );
    MemoryBufferOffset += _SizeInBytes;
    return _SizeInBytes;
}

char * AMemoryStream::Impl_Gets( char * _StrBuf, int _StrSz ) {
    if ( Mode != M_Read ) {
        GLogger.Printf( "AMemoryStream::Gets: expected read mode for %s\n", FileName.CStr() );
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

int AMemoryStream::Impl_SeekSet( long _Offset ) {
    const int NewOffset = _Offset;

    if ( NewOffset < 0 || NewOffset > MemoryBufferSize ) {
        GLogger.Printf( "AMemoryStream::Seek: bad offset for %s\n", FileName.CStr() );
        return -1;
    }

    MemoryBufferOffset = NewOffset;
    return 0;
}

int AMemoryStream::Impl_SeekCur( long _Offset ) {
    const int NewOffset = MemoryBufferOffset + _Offset;

    if ( NewOffset < 0 || NewOffset > MemoryBufferSize ) {
        GLogger.Printf( "AMemoryStream::Seek: bad offset for %s\n", FileName.CStr() );
        return -1;
    }

    MemoryBufferOffset = NewOffset;
    return 0;
}

int AMemoryStream::Impl_SeekEnd( long _Offset ) {
    const int NewOffset = MemoryBufferSize + _Offset;

    if ( NewOffset < 0 || NewOffset > MemoryBufferSize ) {
        GLogger.Printf( "AMemoryStream::Seek: bad offset for %s\n", FileName.CStr() );
        return -1;
    }

    MemoryBufferOffset = NewOffset;
    return 0;
}

long AMemoryStream::Impl_SizeInBytes() {
    return static_cast< long >(MemoryBufferSize);
}

bool AMemoryStream::Impl_Eof() {
    return MemoryBufferOffset >= MemoryBufferSize;
}

byte * AMemoryStream::GrabMemory() {
    return MemoryBuffer;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Archive
//
//////////////////////////////////////////////////////////////////////////////////////////

static constexpr int Case_Strcmp = 1;    // comparision is case sensitivity (like strcmp)
static constexpr int Case_Strcmpi = 2;   // comparision is not case sensitivity (like strcmpi or strcasecmp)
static constexpr int Case_OS = 0;        // case sensitivity is defaut of current operating system

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
        GLogger.Printf( "AArchive::Open: couldn't open %s\n", _ArchiveName );
        return false;
    }

    Handle = GZoneMemory.Alloc( sizeof( mz_zip_archive ) );
    Core::Memcpy( Handle, &arch, sizeof( arch ) );

    ((mz_zip_archive *)Handle)->m_pIO_opaque = Handle;

    //Handle = unzOpen( _ArchiveName );
    //if ( !Handle ) {
    //    GLogger.Printf( "AArchive::Open: couldn't open %s\n", _ArchiveName );
    //    return false;
    //}

    return true;
}

void AArchive::Close() {
    if ( !Handle ) {
        return;
    }

    mz_zip_reader_end( (mz_zip_archive *)Handle );

    GZoneMemory.Free( Handle );

    //unzClose( Handle );
    Handle = nullptr;
}

//static const char * GetUnzipErrorStr( int _ErrorCode ) {
//    switch( _ErrorCode ) {
//    case UNZ_OK:
//        return "UNZ_OK";
//    case UNZ_END_OF_LIST_OF_FILE:
//        return "file not found";
//    case UNZ_ERRNO:
//        return "UNZ_ERRNO";
//    case UNZ_PARAMERROR:
//        return "UNZ_PARAMERROR";
//    case UNZ_BADZIPFILE:
//        return "bad Zip file";
//    case UNZ_INTERNALERROR:
//        return "UNZ_INTERNALERROR";
//    case UNZ_CRCERROR:
//        return "CRC error";
//    }
//    return "unknown error";
//}

//bool AArchive::LocateFile( const char * _FileName ) const {
//    if ( !Handle ) {
//        return false;
//    }

//    Index = mz_zip_reader_locate_file( Handle, _FileName, NULL, 0 );

//    return Index != -1;
//    //return Handle ? unzLocateFile( Handle, _FileName, Case_Strcmpi ) == UNZ_OK : false;
//}

//bool AArchive::GoToFirstFile() {
//    if ( !Handle ) {
//        return false;
//    }
//    Index = 0;
//    return true;
//    //return Handle ? unzGoToFirstFile( Handle ) == UNZ_OK : false;
//}

//bool AArchive::GoToNextFile() {
//    if ( !Handle ) {
//        return false;
//    }

//    if ( Index < ((mz_zip_archive *)Handle)->m_total_files-1 ) {
//        Index++;
//        return true;
//    }

//    return false;

//    //return Handle ? unzGoToNextFile( Handle ) == UNZ_OK : false;
//}

//bool AArchive::GetCurrentFileInfo( char * _FileName, size_t _SizeofFileName ) {
//    mz_zip_reader_file_stat()
//    return Handle ? unzGetCurrentFileInfo( Handle, NULL, _FileName, _SizeofFileName, NULL, 0, NULL, 0 ) == UNZ_OK : false;
//}

bool AArchive::ReadFileToZoneMemory( const char * _FileName, byte ** _MemoryBuffer, int * _SizeInBytes ) {
    return ReadFileToMemory( _FileName, _MemoryBuffer, _SizeInBytes, true );
}

bool AArchive::ReadFileToHeapMemory( const char * _FileName, byte ** _MemoryBuffer, int * _SizeInBytes ) {
    return ReadFileToMemory( _FileName, _MemoryBuffer, _SizeInBytes, false );
}

extern "C" {

typedef void * (*fun_alloc)( size_t size );
typedef void (*fun_free)( void * p );

mz_bool mz_zip_set_error( mz_zip_archive *pZip, mz_zip_error err_num );
const mz_uint8 *mz_zip_get_cdh( mz_zip_archive *pZip, mz_uint file_index );

#define MZ_ZIP_CDH_COMPRESSED_SIZE_OFS 20
#define MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS 24

static void *mz_zip_reader_extract_to_heap_custom(mz_zip_archive *pZip, mz_uint file_index, size_t *pSize, mz_uint flags, fun_alloc falloc, fun_free ffree)
{
    mz_uint64 comp_size, uncomp_size, alloc_size;
    const mz_uint8 *p = mz_zip_get_cdh(pZip, file_index);
    void *pBuf;

    if (pSize)
        *pSize = 0;

    if (!p)
    {
        mz_zip_set_error(pZip, MZ_ZIP_INVALID_PARAMETER);
        return NULL;
    }

    comp_size = MZ_READ_LE32(p + MZ_ZIP_CDH_COMPRESSED_SIZE_OFS);
    uncomp_size = MZ_READ_LE32(p + MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS);

    alloc_size = (flags & MZ_ZIP_FLAG_COMPRESSED_DATA) ? comp_size : uncomp_size;
    if (((sizeof(size_t) == sizeof(mz_uint32))) && (alloc_size > 0x7FFFFFFF))
    {
        mz_zip_set_error(pZip, MZ_ZIP_INTERNAL_ERROR);
        return NULL;
    }

    if (NULL == (pBuf = falloc((size_t)alloc_size)))
    {
        mz_zip_set_error(pZip, MZ_ZIP_ALLOC_FAILED);
        return NULL;
    }

    if (!mz_zip_reader_extract_to_mem(pZip, file_index, pBuf, (size_t)alloc_size, flags))
    {
        ffree(pBuf);
        return NULL;
    }

    if (pSize)
        *pSize = (size_t)alloc_size;
    return pBuf;
}

static void *mz_zip_reader_extract_file_to_heap_custom(mz_zip_archive *pZip, const char *pFilename, size_t *pSize, mz_uint flags, fun_alloc alloc, fun_free free)
{
    mz_uint32 file_index;
    if (!mz_zip_reader_locate_file_v2(pZip, pFilename, NULL, flags, &file_index))
    {
        if (pSize)
            *pSize = 0;
        return MZ_FALSE;
    }
    return mz_zip_reader_extract_to_heap_custom(pZip, file_index, pSize, flags, alloc, free );
}

}

static void * ZoneAlloc( size_t _Size ) {
    return GZoneMemory.Alloc( _Size );
}

static void ZoneFree( void * _Data ) {
    return GZoneMemory.Free( _Data );
}

static void * HeapAlloc( size_t _Size ) {
    return GHeapMemory.Alloc( _Size );
}

static void HeapFree( void * _Data ) {
    return GHeapMemory.Free( _Data );
}

static void * HunkAlloc( size_t _Size ) {
    return GHunkMemory.Alloc( _Size );
}

static void HunkFree( void * _Data ) {
}

bool AArchive::ReadFileToMemory( const char * _FileName, byte ** _MemoryBuffer, int * _SizeInBytes, bool _ZoneMemory ) {
    size_t uncomp_size;

    *_MemoryBuffer = _ZoneMemory ? (byte *)mz_zip_reader_extract_file_to_heap_custom( (mz_zip_archive *)Handle, _FileName, &uncomp_size, 0, ZoneAlloc, ZoneFree )
                                 : (byte *)mz_zip_reader_extract_file_to_heap_custom( (mz_zip_archive *)Handle, _FileName, &uncomp_size, 0, HeapAlloc, HeapFree );
    *_SizeInBytes = uncomp_size;

    return *_MemoryBuffer != nullptr;
#if 0
    int Result = Handle ? unzLocateFile( Handle, _FileName, Case_Strcmpi ) : UNZ_BADZIPFILE;
    if ( Result != UNZ_OK ) {
        GLogger.Printf( "Couldn't open file %s from archive (%s)\n", _FileName, GetUnzipErrorStr( Result ) );
        return false;
    }

    unz_file_info FileInfo;

    Result = unzGetCurrentFileInfo( Handle, &FileInfo, NULL, 0, NULL, 0, NULL, 0 );
    if ( Result != UNZ_OK ) {
        GLogger.Printf( "Failed to read file info %s from archive (%s)\n", _FileName, GetUnzipErrorStr( Result ) );
        return false;
    }

    Result = unzOpenCurrentFile( Handle );
    if ( Result != UNZ_OK ) {
        GLogger.Printf( "Failed to open file %s from archive (%s)\n", _FileName, GetUnzipErrorStr( Result ) );
        return false;
    }

    void * data;
    
    if ( _ZoneMemory ) {
        data = GZoneMemory.Alloc( FileInfo.uncompressed_size );
    } else {
        data = GHeapMemory.Alloc( FileInfo.uncompressed_size, 1 );
    }
    Result = unzReadCurrentFile( Handle, data, FileInfo.uncompressed_size );
    if ( (uLong)Result != FileInfo.uncompressed_size ) {
        GLogger.Printf( "Couldn't read file %s complete from archive: ", _FileName );
        if ( Result == 0 ) {
            GLogger.Print( "the end of file was reached\n" );
        } else {
            GLogger.Printf( "%s\n", GetUnzipErrorStr( Result ) );
        }
        if ( _ZoneMemory ) {
            GZoneMemory.Free( data );
        } else {
            GHeapMemory.Free( data );
        }
        unzCloseCurrentFile( Handle );
        return false;
    }

    Result = unzCloseCurrentFile( Handle );
    if ( Result != UNZ_OK ) {
        GLogger.Printf( "Error during reading file %s (%s)\n", _FileName, GetUnzipErrorStr( Result ) );
        if ( _ZoneMemory ) {
            GZoneMemory.Free( data );
        } else {
            GHeapMemory.Free( data );
        }
        return false;
    }

    *_MemoryBuffer = (byte *)data;
    *_SizeInBytes = FileInfo.uncompressed_size;

    return true;
#endif
}

bool AArchive::ReadFileToHunkMemory( const char * _FileName, byte ** _MemoryBuffer, int * _SizeInBytes, int * _HunkMark ) {
    size_t uncomp_size;

    *_HunkMark = GHunkMemory.SetHunkMark();

    *_MemoryBuffer = ( byte * )mz_zip_reader_extract_file_to_heap_custom( (mz_zip_archive *)Handle, _FileName, &uncomp_size, 0, HunkAlloc, HunkFree );
    *_SizeInBytes = uncomp_size;

    if ( !*_MemoryBuffer ) {
        GHunkMemory.ClearToMark( *_HunkMark );
    }

    return *_MemoryBuffer != nullptr;

#if 0
    int Result = Handle ? unzLocateFile( Handle, _FileName, Case_Strcmpi ) : UNZ_BADZIPFILE;
    if ( Result != UNZ_OK ) {
        GLogger.Printf( "Couldn't open file %s from archive (%s)\n", _FileName, GetUnzipErrorStr( Result ) );
        return false;
    }

    unz_file_info FileInfo;

    Result = unzGetCurrentFileInfo( Handle, &FileInfo, NULL, 0, NULL, 0, NULL, 0 );
    if ( Result != UNZ_OK ) {
        GLogger.Printf( "Failed to read file info %s from archive (%s)\n", _FileName, GetUnzipErrorStr( Result ) );
        return false;
    }

    Result = unzOpenCurrentFile( Handle );
    if ( Result != UNZ_OK ) {
        GLogger.Printf( "Failed to open file %s from archive (%s)\n", _FileName, GetUnzipErrorStr( Result ) );
        return false;
    }

    *_HunkMark = GHunkMemory.SetHunkMark();

    void * data = GHunkMemory.Alloc( FileInfo.uncompressed_size );
    Result = unzReadCurrentFile( Handle, data, FileInfo.uncompressed_size );
    if ( (uLong)Result != FileInfo.uncompressed_size ) {
        GLogger.Printf( "Couldn't read file %s complete from archive: ", _FileName );
        if ( Result == 0 ) {
            GLogger.Print( "the end of file was reached\n" );
        } else {
            GLogger.Printf( "%s\n", GetUnzipErrorStr( Result ) );
        }
        GHunkMemory.ClearToMark( *_HunkMark );
        unzCloseCurrentFile( Handle );
        return false;
    }

    Result = unzCloseCurrentFile( Handle );
    if ( Result != UNZ_OK ) {
        GLogger.Printf( "Error during reading file %s (%s)\n", _FileName, GetUnzipErrorStr( Result ) );
        GHunkMemory.ClearToMark( *_HunkMark );
        return false;
    }

    *_MemoryBuffer = ( byte * )data;
    *_SizeInBytes = FileInfo.uncompressed_size;

    return true;
#endif
}

#if 0
class ANGIE_API FProgressCopyFile {
    AN_FORBID_COPY( FProgressCopyFile )

public:

    // Input parameters
    const char *    ExistingFileName;
    const char *    NewFileName;
    bool            Overwrite;
    bool            CallProgressRoutine;

                    FProgressCopyFile();

    bool            Copy();

    // Current file transfer information
    struct transferInfo_t {
        uint64_t        TotalFileSize;
        uint64_t        TotalBytesTransferred;
    };

    // Call this method to cancel copy operation
    void            Cancel();

    virtual void    ProgressRoutine( const transferInfo_t & _TransferInfo ) {}

private:
    int             m_Cancel;
};

class ANGIE_API FProgressMoveFile {
    AN_FORBID_COPY( FProgressMoveFile )

public:

    // Input parameters
    const char *    ExistingFileName;
    const char *    NewFileName;
    bool            Overwrite;
    bool            CallProgressRoutine;

                    FProgressMoveFile();

    bool            Move();

    // Current file transfer information
    struct transferInfo_t {
        uint64_t        TotalFileSize;
        uint64_t        TotalBytesTransferred;
    };

    // Call this method to cancel copy operation
    //void            Cancel();

    virtual void    ProgressRoutine( const transferInfo_t & _TransferInfo ) {}

private:
    //int             m_Cancel;
};



static DWORD WINAPI COPY_PROGRESS_ROUTINE(
    _In_     LARGE_INTEGER TotalFileSize,
    _In_     LARGE_INTEGER TotalBytesTransferred,
    _In_     LARGE_INTEGER StreamSize,
    _In_     LARGE_INTEGER StreamBytesTransferred,
    _In_     DWORD dwStreamNumber,
    _In_     DWORD dwCallbackReason,
    _In_     HANDLE hSourceFile,
    _In_     HANDLE hDestinationFile,
    _In_opt_ LPVOID lpData
    )
{
    FProgressCopyFile * cf = reinterpret_cast< FProgressCopyFile * >( lpData );
    FProgressCopyFile::transferInfo_t transfer;
    transfer.TotalFileSize = TotalFileSize.QuadPart;
    transfer.TotalBytesTransferred = TotalBytesTransferred.QuadPart;
    cf->ProgressRoutine( transfer );
    //if ( cf->m_Cancel ) {
    //    return PROGRESS_CANCEL;
    //}
    if ( !cf->CallProgressRoutine ) {
        return PROGRESS_QUIET;
    }
    return PROGRESS_CONTINUE;
}

FProgressCopyFile::FProgressCopyFile() {
    ExistingFileName = NULL;
    NewFileName = NULL;
    Overwrite = true;
    CallProgressRoutine = false;
}

bool FProgressCopyFile::Copy() {
    if ( !ExistingFileName || !NewFileName ) {
        return false;
    }

    Core::MakeDir( NewFileName, true );

    if ( !CallProgressRoutine ) {
        return ::CopyFileA( ExistingFileName, NewFileName, !Overwrite ) != FALSE;
    }

    static_assert( sizeof( m_Cancel ) == sizeof( BOOL ), "FProgressCopyFile::Copy" );

    m_Cancel = FALSE;

    DWORD copyFlags = 0;

    if ( !Overwrite ) {
        copyFlags |= COPY_FILE_FAIL_IF_EXISTS;
    }

    return CopyFileExA( ExistingFileName, NewFileName, COPY_PROGRESS_ROUTINE, this, &m_Cancel, copyFlags ) != FALSE;
}

void FProgressCopyFile::Cancel() {
    m_Cancel = TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////

static DWORD WINAPI MOVE_PROGRESS_ROUTINE(
    _In_     LARGE_INTEGER TotalFileSize,
    _In_     LARGE_INTEGER TotalBytesTransferred,
    _In_     LARGE_INTEGER StreamSize,
    _In_     LARGE_INTEGER StreamBytesTransferred,
    _In_     DWORD dwStreamNumber,
    _In_     DWORD dwCallbackReason,
    _In_     HANDLE hSourceFile,
    _In_     HANDLE hDestinationFile,
    _In_opt_ LPVOID lpData
    )
{
    FProgressMoveFile * cf = reinterpret_cast< FProgressMoveFile * >( lpData );
    FProgressMoveFile::transferInfo_t transfer;
    transfer.TotalFileSize = TotalFileSize.QuadPart;
    transfer.TotalBytesTransferred = TotalBytesTransferred.QuadPart;
    cf->ProgressRoutine( transfer );
    //if ( cf->m_Cancel ) {
    //    return PROGRESS_CANCEL;
    //}
    if ( !cf->CallProgressRoutine ) {
        return PROGRESS_QUIET;
    }
    return PROGRESS_CONTINUE;
}

FProgressMoveFile::FProgressMoveFile() {
    ExistingFileName = NULL;
    NewFileName = NULL;
    Overwrite = true;
    CallProgressRoutine = false;
}

bool FProgressMoveFile::Move() {
    if ( !ExistingFileName || !NewFileName ) {
        return false;
    }

    Core::CreateDir( NewFileName, true );

    DWORD flags = MOVEFILE_WRITE_THROUGH;
    if ( Overwrite ) {
        flags |= MOVEFILE_REPLACE_EXISTING;
    }

    if ( !CallProgressRoutine ) {
        return ::MoveFileExA( ExistingFileName, NewFileName, flags ) != FALSE;
    }

    //static_assert( sizeof( m_Cancel ) == sizeof( BOOL ), "FProgressMoveFile::Move" );

    //m_Cancel = FALSE;

    return MoveFileWithProgressA( ExistingFileName, NewFileName, MOVE_PROGRESS_ROUTINE, this, flags ) != FALSE;
}

//void FProgressMoveFile::Cancel() {
//    m_Cancel = TRUE;
//}

void FDiskScanner::ScanDir_r( const char * _Directory, bool _SubDirs, bool _Folders ) {
#if 0
    size_t replaceLen = 1;
    WIN32_FIND_DATAA findData = {};

    AString path = _Directory;
    path += "\\*";

    HANDLE find = FindFirstFileA( path.Str(), &findData );
    if ( find != INVALID_HANDLE_VALUE ) {
        do {
            if ( !(Core::Cmp( findData.cFileName, "." )
                   && Core::Cmp( findData.cFileName, ".." )) ) {
                continue;
            }

            path.Replace( findData.cFileName, path.Length() - replaceLen );
            replaceLen = strlen( findData.cFileName );

            path.UpdateSeparator();

            if ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
                bool scanSubdir = _SubDirs;
                if ( _Folders ) {
                    scanSubdir = EnterFolder( path );
                }
                if ( scanSubdir ) {
                    ScanDir_r( path.Str(), _SubDirs, _Folders );

                    if ( _Folders ) {
                        LeaveFolder( path );
                    }
                }
            }
            else {
                Filter( path );
            }

        } while( FindNextFileA( find, &findData ) );

        FindClose( find );
    }
#else

    // Folders first
    if ( _Folders )
    {
        size_t replaceLen = 1;
        WIN32_FIND_DATAA findData = {};

        AString path = _Directory;
        path += "\\*";
        HANDLE find = FindFirstFileA( path.Str(), &findData );
        if ( find != INVALID_HANDLE_VALUE ) {
            do {
                if ( !(Core::Cmp( findData.cFileName, "." )
                       && Core::Cmp( findData.cFileName, ".." )) ) {
                    continue;
                }

                path.Replace( findData.cFileName, path.Length() - replaceLen );
                replaceLen = strlen( findData.cFileName );

                path.UpdateSeparator();

                if ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
                    bool scanSubdir = _SubDirs;
                    if ( _Folders ) {
                        scanSubdir = EnterFolder( path );
                    }
                    if ( scanSubdir ) {
                        ScanDir_r( path.Str(), _SubDirs, _Folders );

                        if ( _Folders ) {
                            LeaveFolder( path );
                        }
                    }
                }

            } while( FindNextFileA( find, &findData ) );

            FindClose( find );
        }
    }

    // Files
    {
        size_t replaceLen = 1;
        WIN32_FIND_DATAA findData = {};

        AString path = _Directory;
        path += "\\*";

        HANDLE find = FindFirstFileA( path.Str(), &findData );
        if ( find != INVALID_HANDLE_VALUE ) {
            do {
                if ( !(Core::Cmp( findData.cFileName, "." )
                       && Core::Cmp( findData.cFileName, ".." )) ) {
                    continue;
                }

                path.Replace( findData.cFileName, path.Length() - replaceLen );
                replaceLen = strlen( findData.cFileName );

                path.UpdateSeparator();

                if ( !( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) {
                    Filter( path );
                }

            } while( FindNextFileA( find, &findData ) );

            FindClose( find );
        }
    }
#endif
}

bool RemoveDir( const char * _Directory, bool _SubDirs ) {
    bool ret = false;
    size_t replaceLen = 1;
    WIN32_FIND_DATAA findData = {};
    bool hasSubDirs = false;

    std::string path( _Directory );
    path += "\\*";

    HANDLE find = FindFirstFileA( path.c_str(), &findData );
    if ( find != INVALID_HANDLE_VALUE ) {
        ret = true;
        do {
            if ( !(Core::Cmp( findData.cFileName, "." )
                   && Core::Cmp( findData.cFileName, ".." )) ) {
                continue;
            }

            path.replace( path.end() - replaceLen, path.end(), findData.cFileName );
            replaceLen = strlen( findData.cFileName );

            if ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
                hasSubDirs = true;
                if ( _SubDirs ) {
                    ret = RemoveDir( path.c_str() );
                }
            }
            else {
                if ( findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY ) {
                    SetFileAttributesA( path.c_str(), FILE_ATTRIBUTE_NORMAL );
                }
                ret = DeleteFileA( path.c_str() ) != FALSE;
            }

        } while( ret && FindNextFileA( find, &findData ) );

        FindClose( find );

        if ( ret && ( !hasSubDirs || _SubDirs ) ) {
            ret = RemoveDirectoryA( _Directory ) != FALSE;
        }
    }
    return ret;
}

bool IsDirectory( const char * _FileName ) {
    DWORD Attribs = GetFileAttributesA( _FileName ); 

    return !!( Attribs & FILE_ATTRIBUTE_DIRECTORY);
}

#endif
