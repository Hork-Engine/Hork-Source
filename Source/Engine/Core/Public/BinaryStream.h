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

#pragma once

#include "EndianSwap.h"
#include "CString.h"

/**

IBinaryStream

Interface class for binary stream

*/
class IBinaryStream {
public:
    const char * GetFileName() const {
        return Impl_GetFileName();
    }

    void ReadBuffer( void * _Buffer, int _SizeInBytes ) {
        ReadBytesCount = Impl_Read( _Buffer, _SizeInBytes );
    }

    void ReadCString( char * _Str, int _Sizeof ) {
        int32_t size = ReadInt32();
        int32_t capacity = size + 1;
        if ( capacity > _Sizeof ) {
            capacity = _Sizeof;
        }
        ReadBuffer( _Str, capacity - 1 );
        _Str[capacity - 1] = 0;
        int32_t skipBytes = size - ( _Sizeof - 1 );
        if ( skipBytes > 0 ) {
            SeekCur( skipBytes );
        }
    }

    int8_t ReadInt8() {
        int8_t i;
        ReadBuffer( &i, sizeof( i ) );
        return i;
    }

    template< typename T >
    void ReadArrayInt8( T & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        ReadInt8ToBuffer( _Array.ToPtr(), _Array.Size() );
    }

    void ReadInt8ToBuffer( int8_t * _Buffer, int _Count ) {
        ReadBuffer( _Buffer, _Count );
    }

    uint8_t ReadUInt8() {
        uint8_t i;
        ReadBuffer( &i, sizeof( i ) );
        return i;
    }

    template< typename T >
    void ReadArrayUInt8( T & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        ReadUInt8ToBuffer( _Array.ToPtr(), _Array.Size() );
    }

    void ReadUInt8ToBuffer( uint8_t * _Buffer, int _Count ) {
        ReadBuffer( _Buffer, _Count );
    }

    int16_t ReadInt16() {
        int16_t i;
        ReadBuffer( &i, sizeof( i ) );
        return Core::LittleWord( i );
    }

    template< typename T >
    void ReadArrayInt16( T & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        ReadInt16ToBuffer( _Array.ToPtr(), _Array.Size() );
    }

    void ReadInt16ToBuffer( int16_t * _Buffer, int _Count ) {
        ReadBuffer( _Buffer, _Count * sizeof( _Buffer[0] ) );
        for ( int i = 0 ; i < _Count ; i++ ) {
            _Buffer[i] = Core::LittleWord( _Buffer[i] );
        }
    }

    uint16_t ReadUInt16() {
        uint16_t i;
        ReadBuffer( &i, sizeof( i ) );
        return Core::LittleWord( i );
    }

    template< typename T >
    void ReadArrayUInt16( T & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        ReadUInt16ToBuffer( _Array.ToPtr(), _Array.Size() );
    }

    void ReadUInt16ToBuffer( uint16_t * _Buffer, int _Count ) {
        ReadBuffer( _Buffer, _Count * sizeof( _Buffer[0] ) );
        for ( int i = 0 ; i < _Count ; i++ ) {
            _Buffer[i] = Core::LittleWord( _Buffer[i] );
        }
    }

    int32_t ReadInt32() {
        int32_t i;
        ReadBuffer( &i, sizeof( i ) );
        return Core::LittleDWord( i );
    }

    template< typename T >
    void ReadArrayInt32( T & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        ReadInt32ToBuffer( _Array.ToPtr(), _Array.Size() );
    }

    void ReadInt32ToBuffer( int32_t * _Buffer, int _Count ) {
        ReadBuffer( _Buffer, _Count * sizeof( _Buffer[0] ) );
        for ( int i = 0 ; i < _Count ; i++ ) {
            _Buffer[i] = Core::LittleDWord( _Buffer[i] );
        }
    }

    uint32_t ReadUInt32() {
        uint32_t i;
        ReadBuffer( &i, sizeof( i ) );
        return Core::LittleDWord( i );
    }

    template< typename T >
    void ReadArrayUInt32( T & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        ReadUInt32ToBuffer( _Array.ToPtr(), _Array.Size() );
    }

    void ReadUInt32ToBuffer( uint32_t * _Buffer, int _Count ) {
        ReadBuffer( _Buffer, _Count * sizeof( _Buffer[0] ) );
        for ( int i = 0 ; i < _Count ; i++ ) {
            _Buffer[i] = Core::LittleDWord( _Buffer[i] );
        }
    }

    int64_t ReadInt64() {
        int64_t i;
        ReadBuffer( &i, sizeof( i ) );
        return Core::LittleDDWord( i );
    }

    template< typename T >
    void ReadArrayInt64( T & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        ReadInt64ToBuffer( _Array.ToPtr(), _Array.Size() );
    }

    void ReadInt64ToBuffer( int64_t * _Buffer, int _Count ) {
        ReadBuffer( _Buffer, _Count * sizeof( _Buffer[0] ) );
        for ( int i = 0 ; i < _Count ; i++ ) {
            _Buffer[i] = Core::LittleDDWord( _Buffer[i] );
        }
    }

    uint64_t ReadUInt64() {
        uint64_t i;
        ReadBuffer( &i, sizeof( i ) );
        return Core::LittleDDWord( i );
    }

    template< typename T >
    void ReadArrayUInt64( T & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        ReadUInt64ToBuffer( _Array.ToPtr(), _Array.Size() );
    }

    void ReadUInt64ToBuffer( uint64_t * _Buffer, int _Count ) {
        ReadBuffer( _Buffer, _Count * sizeof( _Buffer[0] ) );
        for ( int i = 0 ; i < _Count ; i++ ) {
            _Buffer[i] = Core::LittleDDWord( _Buffer[i] );
        }
    }

    float ReadFloat() {
        uint32_t i = ReadUInt32();
        return *(float *)&i;
    }

    template< typename T >
    void ReadArrayFloat( T & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        ReadFloatToBuffer( _Array.ToPtr(), _Array.Size() );
    }

    void ReadFloatToBuffer( float * _Buffer, int _Count ) {
        ReadBuffer( _Buffer, _Count * sizeof( _Buffer[0] ) );
        for ( int i = 0 ; i < _Count ; i++ ) {
            _Buffer[i] = Core::LittleFloat( _Buffer[i] );
        }
    }

    double ReadDouble() {
        uint64_t i = ReadUInt64();
        return *(double *)&i;
    }

    template< typename T >
    void ReadArrayDouble( T & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        ReadDoubleToBuffer( _Array.ToPtr(), _Array.Size() );
    }

    void ReadDoubleToBuffer( double * _Buffer, int _Count ) {
        ReadBuffer( _Buffer, _Count * sizeof( _Buffer[0] ) );
        for ( int i = 0 ; i < _Count ; i++ ) {
            _Buffer[i] = Core::LittleDouble( _Buffer[i] );
        }
    }

    bool ReadBool() {
        return !!ReadUInt8();
    }

    template< typename T >
    void ReadObject( T & _Object ) {
        _Object.Read( *this );
    }

    template< typename T >
    void ReadArrayOfStructs( T & _Array ) {
        uint32_t size = ReadUInt32();
        _Array.ResizeInvalidate( size );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            ReadObject( _Array[i] );
        }
    }

    void WriteBuffer( const void *_Buffer, int _SizeInBytes ) {
        WriteBytesCount = Impl_Write( _Buffer, _SizeInBytes );
    }

    void WriteCString( const char * _Str ) {
        int len = Core::Strlen( _Str );
        WriteUInt32( len );
        WriteBuffer( _Str, len );
    }

    void WriteInt8( int8_t i ) {
        WriteBuffer( &i, sizeof( i ) );
    }

    template< typename T >
    void WriteArrayInt8( T const & _Array ) {
        WriteUInt32( _Array.Size() );
        WriteBuffer( _Array.ToPtr(), _Array.Size() );
    }

    void WriteUInt8( uint8_t i ) {
        WriteBuffer( &i, sizeof( i ) );
    }

    template< typename T >
    void WriteArrayUInt8( T const & _Array ) {
        WriteUInt32( _Array.Size() );
        WriteBuffer( _Array.ToPtr(), _Array.Size() );
    }

    void WriteInt16( int16_t i ) {
        i = Core::LittleWord( i );
        WriteBuffer( &i, sizeof( i ) );
    }

    template< typename T >
    void WriteArrayInt16( T const & _Array ) {
        WriteUInt32( _Array.Size() );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            WriteInt16( _Array[i] );
        }
    }

    void WriteUInt16( uint16_t i ) {
        i = Core::LittleWord( i );
        WriteBuffer( &i, sizeof( i ) );
    }

    template< typename T >
    void WriteArrayUInt16( T const & _Array ) {
        WriteUInt32( _Array.Size() );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            WriteUInt16( _Array[i] );
        }
    }

    void WriteInt32( int32_t i ) {
        i = Core::LittleDWord( i );
        WriteBuffer( &i, sizeof( i ) );
    }

    template< typename T >
    void WriteArrayInt32( T const & _Array ) {
        WriteUInt32( _Array.Size() );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            WriteInt32( _Array[i] );
        }
    }

    void WriteUInt32( uint32_t i ) {
        i = Core::LittleDWord( i );
        WriteBuffer( &i, sizeof( i ) );
    }

    template< typename T >
    void WriteArrayUInt32( T const & _Array ) {
        WriteUInt32( _Array.Size() );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            WriteUInt32( _Array[i] );
        }
    }

    void WriteInt64( int64_t i ) {
        i = Core::LittleDDWord( i );
        WriteBuffer( &i, sizeof( i ) );
    }

    template< typename T >
    void WriteArrayInt64( T const & _Array ) {
        WriteUInt32( _Array.Size() );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            WriteInt64( _Array[i] );
        }
    }

    void WriteUInt64( uint64_t i ) {
        i = Core::LittleDDWord( i );
        WriteBuffer( &i, sizeof( i ) );
    }

    template< typename T >
    void WriteArrayUInt64( T const & _Array ) {
        WriteUInt32( _Array.Size() );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            WriteUInt64( _Array[i] );
        }
    }

    void WriteFloat( float f ) {
        WriteUInt32( *(uint32_t *)&f );
    }

    template< typename T >
    void WriteArrayFloat( T const & _Array ) {
        WriteUInt32( _Array.Size() );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            WriteFloat( _Array[i] );
        }
    }

    void WriteDouble( double f ) {
        WriteUInt64( *(uint64_t *)&f );
    }

    template< typename T >
    void WriteArrayDouble( T const & _Array ) {
        WriteUInt32( _Array.Size() );
        for ( int i = 0 ; i < _Array.Size() ; i++ ) {
            WriteDouble( _Array[i] );
        }
    }

    void WriteBool( bool b ) {
        WriteUInt8( uint8_t(b) );
    }

    template< typename T >
    void WriteObject( T const & _Object ) {
        _Object.Write( *this );
    }

    template< typename T >
    void WriteArrayOfStructs( T const & _Array ) {
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

    bool SeekSet( long _Offset ) {
        return Impl_SeekSet( _Offset );
    }

    bool SeekCur( long _Offset ) {
        return Impl_SeekCur( _Offset );
    }

    bool SeekEnd( long _Offset ) {
        return Impl_SeekEnd( _Offset );
    }

    size_t SizeInBytes() {
        return Impl_SizeInBytes();
    }

    bool Eof() {
        return Impl_Eof();
    }

    void Printf( const char * _Format, ... ) {
        extern  thread_local char LogBuffer[16384]; // Use existing log buffer
        va_list VaList;
        va_start( VaList, _Format );
        int len = Core::VSprintf( LogBuffer, sizeof( LogBuffer ), _Format, VaList );
        va_end( VaList );
        WriteBuffer( LogBuffer, len );
    }

    int GetReadBytesCount() const { return ReadBytesCount; }
    int GetWriteBytesCount() const { return WriteBytesCount; }

protected:
    IBinaryStream() {}
    ~IBinaryStream() {}

    virtual const char * Impl_GetFileName() const = 0;
    virtual int Impl_Read( void * _Buffer, int _SizeInBytes ) = 0;
    virtual int	Impl_Write( const void *_Buffer, int _SizeInBytes ) = 0;
    virtual char * Impl_Gets( char * _StrBuf, int _StrSz ) = 0;
    virtual void Impl_Flush() = 0;
    virtual long Impl_Tell() = 0;
    virtual bool Impl_SeekSet( long _Offset ) = 0;
    virtual bool Impl_SeekCur( long _Offset ) = 0;
    virtual bool Impl_SeekEnd( long _Offset ) = 0;
    virtual size_t Impl_SizeInBytes() = 0;
    virtual bool Impl_Eof() = 0;

private:
    int ReadBytesCount = 0;
    int WriteBytesCount = 0;
};
