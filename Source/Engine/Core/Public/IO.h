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

#pragma once

#include "BaseTypes.h"
#include "String.h"
#include "EndianSwap.h"
#include "PodArray.h"

/**

AArchive

Read file from archive

*/
class ANGIE_API AArchive final {
    AN_FORBID_COPY( AArchive )

public:
    AArchive();
    ~AArchive();

    /** Open archive */
    bool Open( const char * _ArchiveName );

    void Close();

    bool IsOpened() const;

    /** Check is file present in archive and set offset to this file */
    bool LocateFile( const char * _FileName ) const;

    /** Set offset to first file in archive */
    bool GoToFirstFile();

    /** Set offset to next first file in archive */
    bool GoToNextFile();

    /** Get current located file info */
    bool GetCurrentFileInfo( char * _FileName, size_t _SizeofFileName );

    /** Read file to memory */
    bool ReadFileToZoneMemory( const char * _FileName, byte ** _MemoryBuffer, int * _SizeInBytes );

    /** Read file to memory */
    bool ReadFileToHeapMemory( const char * _FileName, byte ** _MemoryBuffer, int * _SizeInBytes );

    /** Read file to memory */
    bool ReadFileToHunkMemory( const char * _FileName, byte ** _MemoryBuffer, int * _SizeInBytes, int * _HunkMark );

private:
    bool ReadFileToMemory( const char * _FileName, byte ** _MemoryBuffer, int * _SizeInBytes, bool _ZoneMemory );

    void * Handle;
};

AN_FORCEINLINE bool AArchive::IsOpened() const {
    return Handle != nullptr;
}

/**

IStreamBase

Interface class for AFileStream and AMemoryStream

*/
class IStreamBase {
public:
    const char * GetFileName() const {
        return Impl_GetFileName();
    }

    void ReadBuffer( void * _Buffer, int _SizeInBytes ) {
        ReadBytesCount = Impl_Read( _Buffer, _SizeInBytes );
    }

    void ReadWholeFileToString( AString & _Str ) {
        SeekEnd( 0 );
        long fileSz = Tell();
        SeekSet( 0 );
        _Str.ResizeInvalidate( fileSz );
        ReadBuffer( _Str.ToPtr(), fileSz );
    }

    void ReadString( AString & _Str ) {
        int len = ReadUInt32();
        _Str.ResizeInvalidate( len );
        ReadBuffer( _Str.ToPtr(), len );
    }

    int8_t ReadInt8() {
        int8_t i;
        ReadBuffer( &i, sizeof( i ) );
        return i;
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void ReadArrayInt8( TPodArray< int8_t, BASE_CAPACITY, GRANULARITY, Allocator > & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        ReadBuffer( _Array.ToPtr(), _Array.Size() );
    }

    uint8_t ReadUInt8() {
        uint8_t i;
        ReadBuffer( &i, sizeof( i ) );
        return i;
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void ReadArrayUInt8( TPodArray< uint8_t, BASE_CAPACITY, GRANULARITY, Allocator > & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        ReadBuffer( _Array.ToPtr(), _Array.Size() );
    }

    int16_t ReadInt16() {
        int16_t i;
        ReadBuffer( &i, sizeof( i ) );
        return Core::LittleWord( i );
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void ReadArrayInt16( TPodArray< int16_t, BASE_CAPACITY, GRANULARITY, Allocator > & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            _Array[i] = ReadInt16();
        }
    }

    uint16_t ReadUInt16() {
        uint16_t i;
        ReadBuffer( &i, sizeof( i ) );
        return Core::LittleWord( i );
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void ReadArrayUInt16( TPodArray< uint16_t, BASE_CAPACITY, GRANULARITY, Allocator > & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            _Array[i] = ReadUInt16();
        }
    }

    int32_t ReadInt32() {
        int32_t i;
        ReadBuffer( &i, sizeof( i ) );
        return Core::LittleDWord( i );
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void ReadArrayInt32( TPodArray< int32_t, BASE_CAPACITY, GRANULARITY, Allocator > & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            _Array[i] = ReadInt32();
        }
    }

    uint32_t ReadUInt32() {
        uint32_t i;
        ReadBuffer( &i, sizeof( i ) );
        return Core::LittleDWord( i );
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void ReadArrayUInt32( TPodArray< uint32_t, BASE_CAPACITY, GRANULARITY, Allocator > & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            _Array[i] = ReadUInt32();
        }
    }

    int64_t ReadInt64() {
        int64_t i;
        ReadBuffer( &i, sizeof( i ) );
        return Core::LittleDDWord( i );
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void ReadArrayInt64( TPodArray< int64_t, BASE_CAPACITY, GRANULARITY, Allocator > & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            _Array[i] = ReadInt64();
        }
    }

    uint64_t ReadUInt64() {
        uint64_t i;
        ReadBuffer( &i, sizeof( i ) );
        return Core::LittleDDWord( i );
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void ReadArrayUInt64( TPodArray< uint64_t, BASE_CAPACITY, GRANULARITY, Allocator > & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            _Array[i] = ReadUInt64();
        }
    }

    float ReadFloat() {
        uint32_t i = ReadUInt32();
        return *(float *)&i;
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void ReadArrayFloat( TPodArray< float, BASE_CAPACITY, GRANULARITY, Allocator > & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            _Array[i] = ReadFloat();
        }
    }

    double ReadDouble() {
        uint64_t i = ReadUInt64();
        return *(double *)&i;
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void ReadArrayDouble( TPodArray< double, BASE_CAPACITY, GRANULARITY, Allocator > & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            _Array[i] = ReadDouble();
        }
    }

    bool ReadBool() {
        return ReadUInt8();
    }

    template< typename T >
    void ReadObject( T & _Object ) {
        _Object.Read( *this );
    }

    template< typename T, int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void ReadArrayOfStructs( TPodArray< T, BASE_CAPACITY, GRANULARITY, Allocator > & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            ReadObject( _Array[i] );
        }
    }

    void WriteBuffer( const void *_Buffer, int _SizeInBytes ) {
        WriteBytesCount = Impl_Write( _Buffer, _SizeInBytes );
    }

    void WriteString( AString const & _Str ) {
        WriteUInt32( _Str.Length() );
        WriteBuffer( _Str.CStr(), _Str.Length() );
    }

    void WriteString( const char * _Str ) {
        int len = AString::Length( _Str );
        WriteUInt32( len );
        WriteBuffer( _Str, len );
    }

    void WriteInt8( int8_t i ) {
        WriteBuffer( &i, sizeof( i ) );
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void WriteArrayInt8( TPodArray< int8_t, BASE_CAPACITY, GRANULARITY, Allocator > const & _Array ) {
        WriteUInt32( _Array.Size() );
        WriteBuffer( _Array.ToPtr(), _Array.Size() );
    }

    void WriteUInt8( uint8_t i ) {
        WriteBuffer( &i, sizeof( i ) );
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void WriteArrayUInt8( TPodArray< uint8_t, BASE_CAPACITY, GRANULARITY, Allocator > const & _Array ) {
        WriteUInt32( _Array.Size() );
        WriteBuffer( _Array.ToPtr(), _Array.Size() );
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void WriteInt16( int16_t i ) {
        i = Core::LittleWord( i );
        WriteBuffer( &i, sizeof( i ) );
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void WriteArrayInt16( TPodArray< int16_t, BASE_CAPACITY, GRANULARITY, Allocator > const & _Array ) {
        WriteUInt32( _Array.Size() );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            WriteInt16( _Array[i] );
        }
    }

    void WriteUInt16( uint16_t i ) {
        i = Core::LittleWord( i );
        WriteBuffer( &i, sizeof( i ) );
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void WriteArrayUInt16( TPodArray< uint16_t, BASE_CAPACITY, GRANULARITY, Allocator > const & _Array ) {
        WriteUInt32( _Array.Size() );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            WriteUInt16( _Array[i] );
        }
    }

    void WriteInt32( int32_t i ) {
        i = Core::LittleDWord( i );
        WriteBuffer( &i, sizeof( i ) );
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void WriteArrayInt32( TPodArray< int32_t, BASE_CAPACITY, GRANULARITY, Allocator > const & _Array ) {
        WriteUInt32( _Array.Size() );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            WriteInt32( _Array[i] );
        }
    }

    void WriteUInt32( uint32_t i ) {
        i = Core::LittleDWord( i );
        WriteBuffer( &i, sizeof( i ) );
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void WriteArrayUInt32( TPodArray< uint32_t, BASE_CAPACITY, GRANULARITY, Allocator > const & _Array ) {
        WriteUInt32( _Array.Size() );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            WriteUInt32( _Array[i] );
        }
    }

    void WriteInt64( int64_t i ) {
        i = Core::LittleDDWord( i );
        WriteBuffer( &i, sizeof( i ) );
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void WriteArrayInt64( TPodArray< int64_t, BASE_CAPACITY, GRANULARITY, Allocator > const & _Array ) {
        WriteUInt32( _Array.Size() );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            WriteInt64( _Array[i] );
        }
    }

    void WriteUInt64( uint64_t i ) {
        i = Core::LittleDDWord( i );
        WriteBuffer( &i, sizeof( i ) );
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void WriteArrayUInt64( TPodArray< uint64_t, BASE_CAPACITY, GRANULARITY, Allocator > const & _Array ) {
        WriteUInt32( _Array.Size() );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            WriteUInt64( _Array[i] );
        }
    }

    void WriteFloat( float f ) {
        WriteUInt32( *(uint32_t *)&f );
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void WriteArrayFloat( TPodArray< float, BASE_CAPACITY, GRANULARITY, Allocator > const & _Array ) {
        WriteUInt32( _Array.Size() );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            WriteFloat( _Array[i] );
        }
    }

    void WriteDouble( double f ) {
        WriteUInt64( *(uint64_t *)&f );
    }

    template< int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void WriteArrayDouble( TPodArray< double, BASE_CAPACITY, GRANULARITY, Allocator > const & _Array ) {
        WriteUInt32( _Array.Size() );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            WriteDouble( _Array[i] );
        }
    }

    void WriteBool( bool b ) {
        WriteUInt8( uint8_t(b) );
    }

    template< typename T >
    void WriteObject( T & _Object ) {
        _Object.Write( *this );
    }

    template< typename T, int BASE_CAPACITY = 32, int GRANULARITY = 32, typename Allocator = AZoneAllocator >
    void WriteArrayOfStructs( TPodArray< T, BASE_CAPACITY, GRANULARITY, Allocator > const & _Array ) {
        WriteUInt32( _Array.Size() );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            WriteObject( _Array[i] );
        }
    }

    char * Gets( char * _StrBuf, int _StrSz ) {
        return Impl_Gets( _StrBuf, _StrSz );
    }

    void Flush() {
        Impl_Flush();
    }

    long Tell() {
        return Impl_Tell();
    }

    void Rewind() {
        SeekSet( 0 );
    }

    int SeekSet( long _Offset ) {
        return Impl_SeekSet( _Offset );
    }

    int SeekCur( long _Offset ) {
        return Impl_SeekCur( _Offset );
    }

    int SeekEnd( long _Offset ) {
        return Impl_SeekEnd( _Offset );
    }

    long SizeInBytes() {
        return Impl_SizeInBytes();
    }

    bool Eof() {
        return Impl_Eof();
    }

    void Printf( const char * _Format, ... ) {
        extern  thread_local char LogBuffer[16384]; // Use existing log buffer
        va_list VaList;
        va_start( VaList, _Format );
        int len = AString::vsnprintf( LogBuffer, sizeof( LogBuffer ), _Format, VaList );
        va_end( VaList );
        WriteBuffer( LogBuffer, len );
    }

    int GetReadBytesCount() const { return ReadBytesCount; }
    int GetWriteBytesCount() const { return WriteBytesCount; }

protected:
    virtual const char * Impl_GetFileName() const = 0;
    virtual int Impl_Read( void * _Buffer, int _SizeInBytes ) = 0;
    virtual int	Impl_Write( const void *_Buffer, int _SizeInBytes ) = 0;
    virtual char * Impl_Gets( char * _StrBuf, int _StrSz ) = 0;
    virtual void Impl_Flush() = 0;
    virtual long Impl_Tell() = 0;
    virtual int Impl_SeekSet( long _Offset ) = 0;
    virtual int Impl_SeekCur( long _Offset ) = 0;
    virtual int Impl_SeekEnd( long _Offset ) = 0;
    virtual long Impl_SizeInBytes() = 0;
    virtual bool Impl_Eof() = 0;

private:
    int ReadBytesCount = 0;
    int WriteBytesCount = 0;
};

/*

AFileStream

Read/Write to file

*/
class ANGIE_API AFileStream final : public IStreamBase {
    AN_FORBID_COPY( AFileStream )

public:
    /** File access mode (for Android platforms)  */
    enum EAccess {
        A_Random	= 1,    // Чтение кусками из файла, перемещение по файлу вперед/назад
        A_Streaming	= 2,    // Последовательное чтение из файла, редкий Seek()
        A_Buffer	= 3     // Пытается загрузить содержимое в память, для быстрых небольших считываний
    };

    EAccess Access = A_Random;     // set this before open
    bool    bVerbose = true;

    AFileStream();
    ~AFileStream();

    bool OpenRead( const char * _FileName );
    bool OpenRead( AString const & _FileName ) { return OpenRead( _FileName.CStr() ); }
    bool OpenWrite( const char * _FileName );
    bool OpenWrite( AString const & _FileName ) { return OpenWrite( _FileName.CStr() ); }
    bool OpenAppend( const char * _FileName );
    bool OpenAppend( AString const & _FileName ) { return OpenAppend( _FileName.CStr() ); }

    void Close();

    bool IsOpened() const;

protected:
    const char * Impl_GetFileName() const override;
    int Impl_Read( void * _Buffer, int _SizeInBytes ) override;
    int	Impl_Write( const void *_Buffer, int _SizeInBytes ) override;
    char * Impl_Gets( char * _StrBuf, int _StrSz ) override;
    void Impl_Flush() override;
    long Impl_Tell() override;
    int Impl_SeekSet( long _Offset ) override;
    int Impl_SeekCur( long _Offset ) override;
    int Impl_SeekEnd( long _Offset ) override;
    long Impl_SizeInBytes() override;
    bool Impl_Eof() override;

private:
    bool Open( const char * _FileName, int _Mode );

    enum EMode {
        M_Read,
        M_Write,
        M_Append,
        M_Closed
    };

    AString FileName;
    void *  FileHandle = nullptr;
    int     Mode;
};

AN_FORCEINLINE bool AFileStream::IsOpened() const {
    return Mode != M_Closed;
}

/**

AMemoryStream

Read/Write to memory

*/
class ANGIE_API AMemoryStream final : public IStreamBase {
    AN_FORBID_COPY( AMemoryStream )

public:
    AMemoryStream();
    ~AMemoryStream();

    bool OpenRead( const char * _FileName, const byte * _MemoryBuffer, size_t _SizeInBytes );
    bool OpenRead( AString const & _FileName, const byte * _MemoryBuffer, size_t _SizeInBytes );

    bool OpenRead( const char * _FileName, AArchive & _Archive );
    bool OpenRead( AString const & _FileName, AArchive & _Archive );

    bool OpenWrite( const char * _FileName, byte * _MemoryBuffer, size_t _SizeInBytes );
    bool OpenWrite( AString const & _FileName, byte * _MemoryBuffer, size_t _SizeInBytes );

    bool OpenWrite( const char * _FileName, size_t _ReservedSize = 32 );
    bool OpenWrite( AString const & _FileName, size_t _ReservedSize = 32 );

    void Close();

    bool IsOpened() const;

    byte * GrabMemory();

protected:
    const char * Impl_GetFileName() const override;
    int Impl_Read( void * _Buffer, int _SizeInBytes ) override;
    int	Impl_Write( const void *_Buffer, int _SizeInBytes ) override;
    char * Impl_Gets( char * _StrBuf, int _StrSz ) override;
    void Impl_Flush() override;
    long Impl_Tell() override;
    int Impl_SeekSet( long _Offset ) override;
    int Impl_SeekCur( long _Offset ) override;
    int Impl_SeekEnd( long _Offset ) override;
    long Impl_SizeInBytes() override;
    bool Impl_Eof() override;

private:
    enum EMode {
        M_Read,
        M_Write,
        //M_Append,
        M_Closed
    };

    AString FileName;
    int     Mode;
    byte *  MemoryBuffer;
    int     MemoryBufferSize;
    bool    bMemoryBufferOwner;
    int     MemoryBufferOffset;
};

AN_FORCEINLINE bool AMemoryStream::IsOpened() const {
    return Mode != M_Closed;
}

namespace Core {

void MakeDir( const char * _Directory, bool _FileName );
bool IsFileExists( const char * _FileName );
void RemoveFile( const char * _FileName );

}
