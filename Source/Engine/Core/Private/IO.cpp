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

#include <Engine/Core/Public/IO.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/WindowsDefs.h>

#ifdef AN_OS_WIN32
#include <direct.h>     // _mkdir
#include <io.h>         // access
#endif
#ifdef AN_OS_LINUX
#include <sys/stat.h>   // _mkdir
#include <unistd.h>     // access
#endif

#include <unzip.h>

//#ifdef AN_OS_ANDROID
//#include <android/asset_manager_jni.h>
//#endif

#ifdef AN_OS_ANDROID
extern AAssetManager * __AssetManager;
#endif

//////////////////////////////////////////////////////////////////////////////////////////
//
// Common IO functions
//
//////////////////////////////////////////////////////////////////////////////////////////

namespace FCore {

void MakeDir( const char * _Directory, bool _FileName ) {
    size_t strLen = strlen( _Directory );
    if ( !strLen ) {
        return;
    }
    char * tmpStr = ( char * )GZoneMemory.Alloc( strLen + 1, 1 );
    memcpy( tmpStr, _Directory, strLen+1 );
    char * p = tmpStr;
    #ifdef AN_OS_WIN32
    if ( strLen >= 3 && _Directory[1] == ':' ) {
        p += 3;
        _Directory += 3;
    }
    #endif
    for ( ; *_Directory ; p++, _Directory++ ) {
        if ( FString::IsPathSeparator( *p ) ) {
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
    GZoneMemory.Dealloc( tmpStr );
}

bool IsFileExists( const char * _FileName ) {
    FString s = _FileName;
    s.UpdateSeparator();
    return access( s.ToConstChar(), 0 ) == 0;
}

void RemoveFile( const char * _FileName ) {
    FString s = _FileName;
    s.UpdateSeparator();
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

FFileStream::FFileStream()
    : Mode( M_Closed )
{
}

FFileStream::~FFileStream() {
    Close();
}

bool FFileStream::OpenRead( const char * _FileName ) {
    return Open( _FileName, M_Read );
}

bool FFileStream::OpenWrite( const char * _FileName ) {
    return Open( _FileName, M_Write );
}

bool FFileStream::OpenAppend( const char * _FileName ) {
    return Open( _FileName, M_Append );
}

bool FFileStream::Open( const char * _FileName, int _Mode ) {
    Close();

#ifdef AN_OS_ANDROID
    if ( _Mode != M_Read ) {
        Out() << "FFileStream::Open: only read mode is supported on current platform";
        return false;
    }
    AAsset * f = AAssetManager_open( __AssetManager, UpdatePathSeparator( _Url ).c_str(), _Access );
#else
    FileName = _FileName;
    FileName.UpdateSeparator();
    if ( FileName.Length() && FileName[ FileName.Length() - 1 ] == '/' ) {

        if ( bVerbose ) {
            GLogger.Printf( "FFileStream::Open: invalid file name %s\n", _FileName );
        }

        FileName.Free();

        return false;
    }

    constexpr const char * fopen_mode[3] = { "rb", "wb", "ab" };

    AN_ASSERT( _Mode >= 0 && _Mode < 3, "Invalid mode" );

    if ( _Mode == M_Write || _Mode == M_Append ) {
        FCore::MakeDir( FileName.ToConstChar(), true );
    }

    FILE * f = fopen( FileName.ToConstChar(), fopen_mode[ _Mode ] );
#endif

    if ( !f ) {
        if ( bVerbose ) {
            GLogger.Printf( "FFileStream::Open: couldn't open %s\n", FileName.ToConstChar() );
        }
        FileName.Free();
        return false;
    }

    FileHandle = f;
    Mode = _Mode;

    return true;
}

void FFileStream::Close() {
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

const char * FFileStream::Impl_GetFileName() const {
    return FileName.ToConstChar();
}

int FFileStream::Impl_Read( void * _Buffer, int _Length ) {
    if ( Mode != M_Read ) {
        GLogger.Printf( "FFileStream::Read: expected read mode for %s\n", FileName.ToConstChar() );
        return 0;
    }

    int count;
    int numTries = 0;
    byte * data = ( byte * )_Buffer;

    for ( int bytesCount = _Length ; bytesCount > 0 ; bytesCount -= count, data += count ) {
#ifdef AN_OS_ANDROID
        count = AAsset_read( ( AAsset * )FileHandle, data, bytesCount );
#else
        count = fread( data, 1, bytesCount, ( FILE * )FileHandle );
#endif
        if ( count == 0 ) {
            if ( numTries > 0 ) {
                if ( ferror( ( FILE * )FileHandle ) ) {
                    GLogger.Printf( "FFileStream::Read: read error %s\n", FileName.ToConstChar() );
                } else if ( feof( ( FILE * )FileHandle ) ) {
                    //GLogger.Printf( "FFileStream::Read: unexpected end of file %s\n", FileName.ToConstChar() );
                }
                bytesCount = _Length - bytesCount;
                return bytesCount;
            }
            numTries++;
        } else  if ( count < 0 ) {
            GLogger.Printf( "FFileStream::Read: failed to read from %s\n", FileName.ToConstChar() );
            return 0;
        }
    }
    return _Length;
}

int FFileStream::Impl_Write( const void * _Buffer, int _Length ) {
#ifdef AN_OS_ANDROID
    #error "write mode is not supported on current platform";
#else

    if ( Mode != M_Write && Mode != M_Append ) {
        GLogger.Printf( "FFileStream::Write: expected write or append mode for %s\n", FileName.ToConstChar() );
        return 0;
    }

    int count;
    int numTries = 0;
    byte * data = ( byte * )_Buffer;

    for ( int bytesCount = _Length ; bytesCount > 0 ; bytesCount -= count, data += count ) {
        count = fwrite( data, 1, bytesCount, ( FILE * )FileHandle );
        if ( count == 0 ) {
            if ( numTries > 0 ) {
                GLogger.Printf( "FFileStream::Write: write error %s\n", FileName.ToConstChar() );
                bytesCount = _Length - bytesCount;
                return bytesCount;
            }
            numTries++;
        } else  if ( count == -1 ) {
            GLogger.Printf( "FFileStream::Write: failed to write to %s\n", FileName.ToConstChar() );
            return 0;
        }
    }

    return _Length;
#endif
}

char * FFileStream::Impl_Gets( char * _StrBuf, int _StrSz ) {
#ifdef AN_OS_ANDROID
    #error "Gets() is not supported on current platform";
#else

    if ( Mode != M_Read ) {
        GLogger.Printf( "FFileStream::Gets: expected read mode for %s\n", FileName.ToConstChar() );
        return nullptr;
    }

    return fgets( _StrBuf, _StrSz, ( FILE * )FileHandle );
#endif
}

void FFileStream::Impl_Flush() {
#ifdef AN_OS_ANDROID
    #error "Flush() is not supported on current platform";
#else
    fflush( ( FILE * )FileHandle );
#endif
}

long FFileStream::Impl_Tell() {
#ifdef AN_OS_ANDROID
    return AAsset_getLength( ( AAsset * )FileHandle ) - AAsset_getRemainingLength( ( AAsset * )FileHandle );
#else
    return ftell( ( FILE * )FileHandle );
#endif
}

int FFileStream::Impl_SeekSet( long _Offset ) {
#ifdef AN_OS_ANDROID
    return AAsset_seek( ( AAsset * )FileHandle, _Offset, SEEK_SET );
#else
    return fseek( ( FILE * )FileHandle, _Offset, SEEK_SET );
#endif
}

int FFileStream::Impl_SeekCur( long _Offset ) {
#ifdef AN_OS_ANDROID
    return AAsset_seek( ( AAsset * )FileHandle, _Offset, SEEK_CUR );
#else
    return fseek( ( FILE * )FileHandle, _Offset, SEEK_CUR );
#endif
}

int FFileStream::Impl_SeekEnd( long _Offset ) {
#ifdef AN_OS_ANDROID
    return AAsset_seek( ( AAsset * )FileHandle, _Offset, SEEK_END );
#else
    return fseek( ( FILE * )FileHandle, _Offset, SEEK_END );
#endif
}

long FFileStream::Impl_Length() {
    long Offset = Tell();
    SeekEnd( 0 );
    long FileSz = Tell();
    SeekSet( Offset );
    return FileSz;
}

bool FFileStream::Impl_Eof() {
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

FMemoryStream::FMemoryStream()
    : Mode( M_Closed )
    , MemoryBuffer( nullptr )
    , MemoryBufferLength( 0 )
    , bMemoryBufferOwner( false )
    , MemoryBufferOffset( 0 )
{
}

FMemoryStream::~FMemoryStream() {
    Close();
}

bool FMemoryStream::OpenRead( const char * _FileName, const byte * _MemoryBuffer, size_t _MemoryBufferLength ) {
    Close();
    FileName = _FileName;
    MemoryBuffer = const_cast< byte * >( _MemoryBuffer );
    MemoryBufferLength = _MemoryBufferLength;
    bMemoryBufferOwner = false;
    MemoryBufferOffset = 0;
    Mode = M_Read;
    return true;
}

bool FMemoryStream::OpenRead( const char * _FileName, FArchive & _Archive ) {
    Close();

    if ( !_Archive.ReadFileToZoneMemory( _FileName, &MemoryBuffer, &MemoryBufferLength ) ) {
        return false;
    }

    FileName = _FileName;
    bMemoryBufferOwner = true;
    MemoryBufferOffset = 0;
    Mode = M_Read;
    return true;
}

bool FMemoryStream::OpenWrite( const char * _FileName, byte * _MemoryBuffer, size_t _MemoryBufferLength ) {
    Close();
    FileName = _FileName;
    MemoryBuffer = _MemoryBuffer;
    MemoryBufferLength = _MemoryBufferLength;
    bMemoryBufferOwner = false;
    MemoryBufferOffset = 0;
    Mode = M_Write;
    return true;
}

bool FMemoryStream::OpenWrite( const char * _FileName, size_t _ReservedSize ) {
    Close();
    FileName = _FileName;
    MemoryBuffer = ( byte * )GZoneMemory.Alloc( _ReservedSize, 1 );
    MemoryBufferLength = _ReservedSize;
    bMemoryBufferOwner = true;
    MemoryBufferOffset = 0;
    Mode = M_Write;
    return true;
}

void FMemoryStream::Close() {
    if ( Mode == M_Closed ) {
        return;
    }

    Mode = M_Closed;

    if ( bMemoryBufferOwner ) {
        GZoneMemory.Dealloc( MemoryBuffer );
    }
}

const char * FMemoryStream::Impl_GetFileName() const {
    return FileName.ToConstChar();
}

int FMemoryStream::Impl_Read( void * _Buffer, int _Length ) {
    if ( Mode != M_Read ) {
        GLogger.Printf( "FMemoryStream::Read: expected read mode for %s\n", FileName.ToConstChar() );
        return 0;
    }

    int bytesCount = _Length;
    if ( MemoryBufferOffset + _Length > MemoryBufferLength ) {
        bytesCount = MemoryBufferLength - MemoryBufferOffset;
        //GLogger.Printf( "FFileStream::Read: unexpected end of file %s\n", FileName.ToConstChar() );
    }

    if ( bytesCount > 0 ) {
        memcpy( _Buffer, MemoryBuffer + MemoryBufferOffset, bytesCount );
        MemoryBufferOffset += bytesCount;
    }

    return bytesCount;
}

int FMemoryStream::Impl_Write( const void * _Buffer, int _Length ) {
    if ( Mode != M_Write ) {
        GLogger.Printf( "FMemoryStream::Write: expected write mode for %s\n", FileName.ToConstChar() );
        return 0;
    }

    int diff = MemoryBufferOffset + _Length - MemoryBufferLength;
    if ( diff > 0 ) {
        if ( !bMemoryBufferOwner ) {
            GLogger.Printf( "FMemoryStream::Write: buffer overflowed for %s\n", FileName.ToConstChar() );
            return 0;
        }
        const int GRANULARITY = 256;
        int newLength = MemoryBufferLength + diff;
        int mod = newLength % GRANULARITY;
        if ( mod ) {
            newLength = newLength + GRANULARITY - mod;
        }
        MemoryBuffer = ( byte * )GZoneMemory.Extend( MemoryBuffer, MemoryBufferLength, newLength, 1, true );
        MemoryBufferLength = newLength;
    }
    memcpy( MemoryBuffer + MemoryBufferOffset, _Buffer, _Length );
    MemoryBufferOffset += _Length;
    return _Length;
}

char * FMemoryStream::Impl_Gets( char * _StrBuf, int _StrSz ) {
    if ( Mode != M_Read ) {
        GLogger.Printf( "FMemoryStream::Gets: expected read mode for %s\n", FileName.ToConstChar() );
        return nullptr;
    }

    if ( _StrSz == 0 || MemoryBufferOffset >= MemoryBufferLength ) {
        return nullptr;
    }

    int charsCount = _StrSz-1;
    if ( MemoryBufferOffset + charsCount > MemoryBufferLength ) {
        charsCount = MemoryBufferLength - MemoryBufferOffset;
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

void FMemoryStream::Impl_Flush() {
}

long FMemoryStream::Impl_Tell() {
    return static_cast< long >( MemoryBufferOffset );
}

int FMemoryStream::Impl_SeekSet( long _Offset ) {
    const int NewOffset = _Offset;

    if ( NewOffset < 0 || NewOffset > MemoryBufferLength ) {
        GLogger.Printf( "FMemoryStream::Seek: bad offset for %s\n", FileName.ToConstChar() );
        return -1;
    }

    MemoryBufferOffset = NewOffset;
    return 0;
}

int FMemoryStream::Impl_SeekCur( long _Offset ) {
    const int NewOffset = MemoryBufferOffset + _Offset;

    if ( NewOffset < 0 || NewOffset > MemoryBufferLength ) {
        GLogger.Printf( "FMemoryStream::Seek: bad offset for %s\n", FileName.ToConstChar() );
        return -1;
    }

    MemoryBufferOffset = NewOffset;
    return 0;
}

int FMemoryStream::Impl_SeekEnd( long _Offset ) {
    const int NewOffset = MemoryBufferLength + _Offset;

    if ( NewOffset < 0 || NewOffset > MemoryBufferLength ) {
        GLogger.Printf( "FMemoryStream::Seek: bad offset for %s\n", FileName.ToConstChar() );
        return -1;
    }

    MemoryBufferOffset = NewOffset;
    return 0;
}

long FMemoryStream::Impl_Length() {
    return static_cast< long >( MemoryBufferLength );
}

bool FMemoryStream::Impl_Eof() {
    return MemoryBufferOffset >= MemoryBufferLength;
}

byte * FMemoryStream::GrabMemory() {
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

FArchive::FArchive()
 : Handle( nullptr )
{

}

FArchive::~FArchive() {
    Close();
}

bool FArchive::Open( const char * _ArchiveName ) {
    Close();

    Handle = unzOpen( _ArchiveName );
    if ( !Handle ) {
        GLogger.Printf( "FArchive::Open: couldn't open %s\n", _ArchiveName );
        return false;
    }

    return true;
}

void FArchive::Close() {
    if ( !Handle ) {
        return;
    }

    unzClose( Handle );
    Handle = nullptr;
}

static const char * GetUnzipErrorStr( int _ErrorCode ) {
    switch( _ErrorCode ) {
    case UNZ_OK:
        return "UNZ_OK";
    case UNZ_END_OF_LIST_OF_FILE:
        return "file not found";
    case UNZ_ERRNO:
        return "UNZ_ERRNO";
    case UNZ_PARAMERROR:
        return "UNZ_PARAMERROR";
    case UNZ_BADZIPFILE:
        return "bad Zip file";
    case UNZ_INTERNALERROR:
        return "UNZ_INTERNALERROR";
    case UNZ_CRCERROR:
        return "CRC error";
    }
    return "unknown error";
}

bool FArchive::LocateFile( const char * _FileName ) const {
    return Handle ? unzLocateFile( Handle, _FileName, Case_Strcmpi ) == UNZ_OK : false;
}

bool FArchive::GoToFirstFile() {
    return Handle ? unzGoToFirstFile( Handle ) == UNZ_OK : false;
}

bool FArchive::GoToNextFile() {
    return Handle ? unzGoToNextFile( Handle ) == UNZ_OK : false;
}

bool FArchive::GetCurrentFileInfo( char * _FileName, size_t _SizeofFileName ) {
    return Handle ? unzGetCurrentFileInfo( Handle, NULL, _FileName, _SizeofFileName, NULL, 0, NULL, 0 ) == UNZ_OK : false;
}

bool FArchive::ReadFileToZoneMemory( const char * _FileName, byte ** _MemoryBuffer, int * _MemoryBufferLength ) {
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

    void * data = GZoneMemory.Alloc( FileInfo.uncompressed_size, 1 );
    Result = unzReadCurrentFile( Handle, data, FileInfo.uncompressed_size );
    if ( (uLong)Result != FileInfo.uncompressed_size ) {
        GLogger.Printf( "Couldn't read file %s complete from archive: ", _FileName );
        if ( Result == 0 ) {
            GLogger.Print( "the end of file was reached\n" );
        } else {
            GLogger.Printf( "%s\n", GetUnzipErrorStr( Result ) );
        }
        GZoneMemory.Dealloc( data );
        unzCloseCurrentFile( Handle );
        return false;
    }

    Result = unzCloseCurrentFile( Handle );
    if ( Result != UNZ_OK ) {
        GLogger.Printf( "Error during reading file %s (%s)\n", _FileName, GetUnzipErrorStr( Result ) );
        GZoneMemory.Dealloc( data );
        return false;
    }

    *_MemoryBuffer = ( byte * )data;
    *_MemoryBufferLength = FileInfo.uncompressed_size;

    return true;
}

bool FArchive::ReadFileToHunkMemory( const char * _FileName, byte ** _MemoryBuffer, int * _MemoryBufferLength, int * _HunkMark ) {
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

    void * data = GHunkMemory.HunkMemory( FileInfo.uncompressed_size, 1 );
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
    *_MemoryBufferLength = FileInfo.uncompressed_size;

    return true;
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

    FCore::MakeDir( NewFileName, true );

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

    FCore::CreateDir( NewFileName, true );

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

    FString path = _Directory;
    path += "\\*";

    HANDLE find = FindFirstFileA( path.Str(), &findData );
    if ( find != INVALID_HANDLE_VALUE ) {
        do {
            if ( !(FString::Cmp( findData.cFileName, "." )
                   && FString::Cmp( findData.cFileName, ".." )) ) {
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

        FString path = _Directory;
        path += "\\*";
        HANDLE find = FindFirstFileA( path.Str(), &findData );
        if ( find != INVALID_HANDLE_VALUE ) {
            do {
                if ( !(FString::Cmp( findData.cFileName, "." )
                       && FString::Cmp( findData.cFileName, ".." )) ) {
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

        FString path = _Directory;
        path += "\\*";

        HANDLE find = FindFirstFileA( path.Str(), &findData );
        if ( find != INVALID_HANDLE_VALUE ) {
            do {
                if ( !(FString::Cmp( findData.cFileName, "." )
                       && FString::Cmp( findData.cFileName, ".." )) ) {
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
            if ( !(FString::Cmp( findData.cFileName, "." )
                   && FString::Cmp( findData.cFileName, ".." )) ) {
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
