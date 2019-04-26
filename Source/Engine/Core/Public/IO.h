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

#include "BaseTypes.h"
#include "String.h"
#include "EndianSwap.h"

/*

FArchive

Read file from archive

*/
class ANGIE_API FArchive final {
    AN_FORBID_COPY( FArchive )

public:
    FArchive();
    ~FArchive();

    // Open archive
    bool Open( const char * _ArchiveName );

    void Close();

    bool IsOpened() const;

    // Check is file present in archive and set offset to this file
    bool LocateFile( const char * _FileName ) const;

    // Set offset to first file in archive
    bool GoToFirstFile();

    // Set offset to next first file in archive
    bool GoToNextFile();

    // Get current located file info
    bool GetCurrentFileInfo( char * _FileName, size_t _SizeofFileName );

    // Read file to memory
    bool ReadFileToZoneMemory( const char * _FileName, byte ** _MemoryBuffer, int * _MemoryBufferLength );

    // Read file to memory
    bool ReadFileToHunkMemory( const char * _FileName, byte ** _MemoryBuffer, int * _MemoryBufferLength, int * _HunkMark );

private:
    void * Handle;
};

AN_FORCEINLINE bool FArchive::IsOpened() const {
    return Handle != nullptr;
}

/*

FStreamBase

Base class for FFileStream and FMemoryStream

*/
template< typename Derived >
class FStreamBase {
    AN_FORBID_COPY( FStreamBase )

public:
    FStreamBase() {}

    const char * GetFileName() const {
        return static_cast< const Derived * >( this )->Impl_GetFileName();
    }

    void Read( void * _Buffer, int _Length ) {
        ReadBytesCount = static_cast< Derived * >( this )->Impl_Read( _Buffer, _Length );
    }

    void Read( FString & _Str ) {
        SeekEnd( 0 );
        long fileSz = Tell();
        SeekSet( 0 );
        _Str.Resize( fileSz );
        Read( _Str.ToPtr(), fileSz );
    }

    void Read( word & i ) {
        Read( &i, sizeof( i ) );
        i = FCore::LittleWord( i );
    }

    void Read( dword & i ) {
        Read( &i, sizeof( i ) );
        i = FCore::LittleDWord( i );
    }

    void Read( ddword & i ) {
        Read( &i, sizeof( i ) );
        i = FCore::LittleDDWord( i );
    }

    void Write( const void *_Buffer, int _Length ) {
        WriteBytesCount = static_cast< Derived * >( this )->Impl_Write( _Buffer, _Length );
    }

    void Write( word i ) {
        i = FCore::LittleWord( i );
        Write( &i, sizeof( i ) );
    }

    void Write( dword i ) {
        i = FCore::LittleDWord( i );
        Write( &i, sizeof( i ) );
    }

    void Write( ddword i ) {
        i = FCore::LittleDDWord( i );
        Write( &i, sizeof( i ) );
    }

    char * Gets( char * _StrBuf, int _StrSz ) {
        return static_cast< Derived * >( this )->Impl_Gets( _StrBuf, _StrSz );
    }

    void Flush() {
        static_cast< Derived * >( this )->Impl_Flush();
    }

    long Tell() {
        return static_cast< Derived * >( this )->Impl_Tell();
    }

    void Rewind() {
        SeekSet( 0 );
    }

    int SeekSet( long _Offset ) {
        return static_cast< Derived * >( this )->Impl_SeekSet( _Offset );
    }

    int SeekCur( long _Offset ) {
        return static_cast< Derived * >( this )->Impl_SeekCur( _Offset );
    }

    int SeekEnd( long _Offset ) {
        return static_cast< Derived * >( this )->Impl_SeekEnd( _Offset );
    }

    long Length() {
        return static_cast< Derived * >( this )->Impl_Length();
    }

    bool Eof() {
        return static_cast< Derived * >( this )->Impl_Eof();
    }

    void Printf( const char * _Format, ... ) {
        extern  thread_local char LogBuffer[16384]; // Use existing log buffer
        va_list VaList;
        va_start( VaList, _Format );
        int len = FString::vsnprintf( LogBuffer, sizeof( LogBuffer ), _Format, VaList );
        va_end( VaList );
        Write( LogBuffer, len );
    }

    FStreamBase & operator<<( const FString & _Str ) {
        Write( _Str.Length() );
        Write( _Str.ToConstChar(), _Str.Length() );
        return *this;
    }

    template< typename Type >
    FStreamBase & operator>>( FString & _Str ) {
        dword length;
        Read( length );
        _Str.ReserveInvalidate( length + 1 );
        _Str.Resize( length );
        Read( _Str.ToPtr(), length );
        return *this;
    }

    FStreamBase & operator<<( const word & i ) {
        Write( i );
        return *this;
    }

    FStreamBase & operator>>( word & i ) {
        Read( i );
        return *this;
    }

    FStreamBase & operator<<( const dword & i ) {
        Write( i );
        return *this;
    }

    FStreamBase & operator>>( dword & i ) {
        Read( i );
        return *this;
    }

    FStreamBase & operator<<( const ddword & i ) {
        Write( i );
        return *this;
    }

    FStreamBase & operator>>( ddword & i ) {
        Read( i );
        return *this;
    }

    FStreamBase & operator<<( const float & i ) {
        Write( *(dword *)&i );
        return *this;
    }

    FStreamBase & operator>>( float & i ) {
        Read( *(dword *)&i );
        return *this;
    }

    FStreamBase & operator<<( const double & i ) {
        Write( *(ddword *)&i );
        return *this;
    }

    FStreamBase & operator>>( double & i ) {
        Read( *(ddword *)&i );
        return *this;
    }

    template< typename Type >
    FStreamBase & operator<<( const Type & _Value ) {
        _Value.Write( *this );
        return *this;
    }

    template< typename Type >
    FStreamBase & operator>>( Type & _Value ) {
        _Value.Read( *this );
        return *this;
    }

    int GetReadBytesCount() const { return ReadBytesCount; }
    int GetWriteBytesCount() const { return WriteBytesCount; }

private:
    int ReadBytesCount = 0;
    int WriteBytesCount = 0;
};

/*

FFileStream

Read/Write to file

*/
class ANGIE_API FFileStream final : public FStreamBase< FFileStream > {
    AN_FORBID_COPY( FFileStream )

    friend class FStreamBase< FFileStream >;
public:
    // File access mode (for Android platforms)
    enum EAccess {
        A_Random	= 1,    // Чтение кусками из файла, перемещение по файлу вперед/назад
        A_Streaming	= 2,    // Последовательное чтение из файла, редкий Seek()
        A_Buffer	= 3     // Пытается загрузить содержимое в память, для быстрых небольших считываний
    };

    EAccess Access = A_Random;     // set this before open
    bool    bVerbose = true;

    FFileStream();
    ~FFileStream();

    bool OpenRead( const char * _FileName );
    bool OpenWrite( const char * _FileName );
    bool OpenAppend( const char * _FileName );

    void Close();

    bool IsOpened() const;

private:
    bool Open( const char * _FileName, int _Mode );

    const char * Impl_GetFileName() const;
    int Impl_Read( void * _Buffer, int _Length );
    int	Impl_Write( const void *_Buffer, int _Length );
    char * Impl_Gets( char * _StrBuf, int _StrSz );
    void Impl_Flush();
    long Impl_Tell();
    int Impl_SeekSet( long _Offset );
    int Impl_SeekCur( long _Offset );
    int Impl_SeekEnd( long _Offset );
    long Impl_Length();
    bool Impl_Eof();

    enum EMode {
        M_Read,
        M_Write,
        M_Append,
        M_Closed
    };

    FString FileName;
    void *  FileHandle = nullptr;
    int     Mode;
};

AN_FORCEINLINE bool FFileStream::IsOpened() const {
    return Mode != M_Closed;
}

/*

FMemoryStream

Read/Write to memory

*/
class ANGIE_API FMemoryStream final : public FStreamBase< FMemoryStream > {
    AN_FORBID_COPY( FMemoryStream )

    friend class FStreamBase< FMemoryStream >;
public:
    FMemoryStream();
    ~FMemoryStream();

    bool OpenRead( const char * _FileName, const byte * _MemoryBuffer, size_t _MemoryBufferLength );
    bool OpenRead( const char * _FileName, FArchive & _Archive );

    bool OpenWrite( const char * _FileName, byte * _MemoryBuffer, size_t _MemoryBufferLength );
    bool OpenWrite( const char * _FileName, size_t _ReservedSize = 32 );

    void Close();

    bool IsOpened() const;

    byte * GrabMemory();

private:
    const char * Impl_GetFileName() const;
    int Impl_Read( void * _Buffer, int _Length );
    int Impl_Write( const void *_Buffer, int _Length );
    char * Impl_Gets( char * _StrBuf, int _StrSz );
    void Impl_Flush();
    long Impl_Tell();
    int Impl_SeekSet( long _Offset );
    int Impl_SeekCur( long _Offset );
    int Impl_SeekEnd( long _Offset );
    long Impl_Length();
    bool Impl_Eof();

    enum EMode {
        M_Read,
        M_Write,
        //M_Append,
        M_Closed
    };

    FString FileName;
    int     Mode;
    byte *  MemoryBuffer;
    int     MemoryBufferLength;
    bool    bMemoryBufferOwner;
    int     MemoryBufferOffset;
};

AN_FORCEINLINE bool FMemoryStream::IsOpened() const {
    return Mode != M_Closed;
}

namespace FCore {

void MakeDir( const char * _Directory, bool _FileName );
bool IsFileExists( const char * _FileName );
void RemoveFile( const char * _FileName );

}
