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

#include "BinaryStream.h"
#include "String.h"

/**

AArchive

Read file from archive

*/
class AArchive final
{
    AN_FORBID_COPY( AArchive )

public:
    AArchive();
    AArchive( AStringView _ArchiveName, bool bResourcePack = false );
    AArchive( const void * pMemory, size_t SizeInBytes );

    ~AArchive();

    /** Open archive from file */
    bool Open( AStringView _ArchiveName, bool bResourcePack = false );

    /** Open archive from memory */
    bool OpenFromMemory( const void * pMemory, size_t SizeInBytes );

    /** Close archive */
    void Close();

    /** Check is archive opened */
    bool IsOpened() const { return !!Handle; }

    /** Get total files in archive */
    int GetNumFiles() const;

    /** Get file index. Return -1 if file wasn't found */
    int LocateFile( AStringView _FileName ) const;

    /** Get file compressed and uncompressed size */
    bool GetFileSize( int _FileIndex, size_t * _CompressedSize, size_t * _UncompressedSize ) const;

    /** Get file name by index */
    bool GetFileName( int _FileIndex, AString & FileName ) const;

    /** Decompress file to memory buffer */
    bool ExtractFileToMemory( int _FileIndex, void * _MemoryBuffer, size_t _SizeInBytes ) const;

    /** Decompress file to heap memory */
    bool ExtractFileToHeapMemory( AStringView _FileName, void ** _HeapMemoryPtr, int * _SizeInBytes ) const;
    /** Decompress file to heap memory */
    bool ExtractFileToHeapMemory( int _FileIndex, void ** _HeapMemoryPtr, int * _SizeInBytes ) const;

    /** Decompress file to hunk memory */
    bool ExtractFileToHunkMemory( AStringView _FileName, void ** _HunkMemoryPtr, int * _SizeInBytes, int * _HunkMark ) const;

private:
    void * Handle;
};


/*

AFileStream

Read/Write to file

*/
class AFileStream final : public IBinaryStream
{
    AN_FORBID_COPY( AFileStream )

public:
    AFileStream()
    {}

    AFileStream( AFileStream && BinaryStream ) noexcept :
        Name( std::move( BinaryStream.Name ) ), Handle( BinaryStream.Handle ), Mode( BinaryStream.Mode )
    {
        ReadBytesCount  = BinaryStream.ReadBytesCount;
        WriteBytesCount = BinaryStream.WriteBytesCount;

        BinaryStream.ReadBytesCount  = 0;
        BinaryStream.WriteBytesCount = 0;

        BinaryStream.Handle = nullptr;
        BinaryStream.Mode   = FILE_OPEN_MODE_CLOSED;
    }

    AFileStream & operator=( AFileStream && BinaryStream ) noexcept
    {
        Close();

        ReadBytesCount  = BinaryStream.ReadBytesCount;
        WriteBytesCount = BinaryStream.WriteBytesCount;
        Name            = std::move( BinaryStream.Name );
        Handle          = BinaryStream.Handle;
        Mode            = BinaryStream.Mode;

        BinaryStream.ReadBytesCount  = 0;
        BinaryStream.WriteBytesCount = 0;
        BinaryStream.Handle          = nullptr;
        BinaryStream.Mode            = FILE_OPEN_MODE_CLOSED;

        return *this;
    }

    ~AFileStream();

    /** Open file for read form specified path */
    bool OpenRead( AStringView _FileName );

    /** Open file for write */
    bool OpenWrite( AStringView _FileName );

    /** Open file for append */
    bool OpenAppend( AStringView _FileName );

    /** Close file */
    void Close();

    /** Check is file opened */
    bool IsOpened() const { return Mode != FILE_OPEN_MODE_CLOSED; }

protected:
    const char * Impl_GetFileName() const override;
    int          Impl_Read( void * _Buffer, int _SizeInBytes ) override;
    int          Impl_Write( const void * _Buffer, int _SizeInBytes ) override;
    char *       Impl_Gets( char * _StrBuf, int _StrSz ) override;
    void         Impl_Flush() override;
    long         Impl_Tell() override;
    bool         Impl_SeekSet( long _Offset ) override;
    bool         Impl_SeekCur( long _Offset ) override;
    bool         Impl_SeekEnd( long _Offset ) override;
    size_t       Impl_SizeInBytes() override;
    bool         Impl_Eof() override;

private:
    bool Open( AStringView _FileName, int _Mode );

    enum FILE_OPEN_MODE
    {
        FILE_OPEN_MODE_CLOSED,
        FILE_OPEN_MODE_READ,
        FILE_OPEN_MODE_WRITE,
        FILE_OPEN_MODE_APPEND
    };

    AString Name;
    void *  Handle = nullptr;
    int     Mode   = FILE_OPEN_MODE_CLOSED;
};


/**

AMemoryStream

Read/Write to memory

*/
class AMemoryStream final : public IBinaryStream
{
    AN_FORBID_COPY( AMemoryStream )

public:
    AMemoryStream()
    {}

    AMemoryStream( AMemoryStream && BinaryStream ) noexcept :
        Name( std::move( BinaryStream.Name ) ),
        Mode( BinaryStream.Mode ),
        MemoryBuffer( BinaryStream.MemoryBuffer ),
        MemoryBufferSize( BinaryStream.MemoryBufferSize ),
        bMemoryBufferOwner( BinaryStream.bMemoryBufferOwner ),
        MemoryBufferOffset( BinaryStream.MemoryBufferOffset ),
        Granularity( BinaryStream.Granularity )
    {
        ReadBytesCount  = BinaryStream.ReadBytesCount;
        WriteBytesCount = BinaryStream.WriteBytesCount;

        BinaryStream.ReadBytesCount     = 0;
        BinaryStream.WriteBytesCount    = 0;
        BinaryStream.Mode               = FILE_OPEN_MODE_CLOSED;
        BinaryStream.MemoryBuffer       = nullptr;
        BinaryStream.MemoryBufferSize   = 0;
        BinaryStream.bMemoryBufferOwner = false;
        BinaryStream.MemoryBufferOffset = 0;
        BinaryStream.Granularity        = 0;
    }

    ~AMemoryStream();

    AMemoryStream & operator=( AMemoryStream && BinaryStream ) noexcept
    {
        Close();

        ReadBytesCount     = BinaryStream.ReadBytesCount;
        WriteBytesCount    = BinaryStream.WriteBytesCount;
        Name               = std::move( BinaryStream.Name );
        Mode               = BinaryStream.Mode;
        MemoryBuffer       = BinaryStream.MemoryBuffer;
        MemoryBufferSize   = BinaryStream.MemoryBufferSize;
        bMemoryBufferOwner = BinaryStream.bMemoryBufferOwner;
        MemoryBufferOffset = BinaryStream.MemoryBufferOffset;
        Granularity        = BinaryStream.Granularity;

        BinaryStream.ReadBytesCount     = 0;
        BinaryStream.WriteBytesCount    = 0;
        BinaryStream.Mode               = FILE_OPEN_MODE_CLOSED;
        BinaryStream.MemoryBuffer       = nullptr;
        BinaryStream.MemoryBufferSize   = 0;
        BinaryStream.bMemoryBufferOwner = false;
        BinaryStream.MemoryBufferOffset = 0;
        BinaryStream.Granularity        = 0;

        return *this;
    }

    /** Read from specified memory buffer */
    bool OpenRead( AStringView _FileName, const void * _MemoryBuffer, size_t _SizeInBytes );

    /** Read file from archive */
    bool OpenRead( AStringView _FileName, AArchive const & _Archive );

    /** Read file from archive */
    bool OpenRead( int _FileIndex, AArchive const & _Archive );

    /** Write to specified memory buffer */
    bool OpenWrite( AStringView _FileName, void * _MemoryBuffer, size_t _SizeInBytes );

    /** Write to inner memory buffer */
    bool OpenWrite( AStringView _FileName, size_t _ReservedSize = 32 );

    /** Close file */
    void Close();

    /** Check is file opened */
    bool IsOpened() const { return Mode != FILE_OPEN_MODE_CLOSED; }

    /** Grab memory buffer */
    void * GrabMemory();

    void SetGrowGranularity( int _Granularity ) { Granularity = _Granularity; }

protected:
    const char * Impl_GetFileName() const override;
    int          Impl_Read( void * _Buffer, int _SizeInBytes ) override;
    int          Impl_Write( const void * _Buffer, int _SizeInBytes ) override;
    char *       Impl_Gets( char * _StrBuf, int _StrSz ) override;
    void         Impl_Flush() override;
    long         Impl_Tell() override;
    bool         Impl_SeekSet( long _Offset ) override;
    bool         Impl_SeekCur( long _Offset ) override;
    bool         Impl_SeekEnd( long _Offset ) override;
    size_t       Impl_SizeInBytes() override;
    bool         Impl_Eof() override;

private:
    enum FILE_OPEN_MODE
    {
        FILE_OPEN_MODE_CLOSED,
        FILE_OPEN_MODE_READ,
        FILE_OPEN_MODE_WRITE
    };

    void * Alloc( size_t _SizeInBytes );
    void * Realloc( void * _Data, size_t _SizeInBytes );
    void   Free( void * _Data );

    AString Name;
    int     Mode               = FILE_OPEN_MODE_CLOSED;
    byte *  MemoryBuffer       = nullptr;
    int     MemoryBufferSize   = 0;
    bool    bMemoryBufferOwner = false;
    int     MemoryBufferOffset = 0;
    int     Granularity        = 1024;
};


namespace Core
{

/** Make file system directory */
void MakeDir( const char * _Directory, bool _FileName );

/** Check is file exists */
bool IsFileExists( const char * _FileName );

/** Remove file from disk */
void RemoveFile( const char * _FileName );

using STraverseDirectoryCB = std::function< void( AStringView FileName, bool bIsDirectory ) >;
/** Traverse the directory */
void TraverseDirectory( AStringView Path, bool bSubDirs, STraverseDirectoryCB Callback );

/** Write game resource pack */
bool WriteResourcePack( const char * SourcePath, const char * ResultFile );

} // namespace Core
