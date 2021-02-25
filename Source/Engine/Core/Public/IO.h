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
class AArchive final {
    AN_FORBID_COPY( AArchive )

public:
    AArchive();
    ~AArchive();

    /** Open archive from file */
    bool Open( const char * _ArchiveName, bool bResourcePack = false );

    /** Open archive from memory */
    bool OpenFromMemory( const void * pMemory, size_t SizeInBytes );

    /** Close archive */
    void Close();

    /** Check is archive opened */
    bool IsOpened() const { return !!Handle; }

    /** Get total files in archive */
    int GetNumFiles() const;

    /** Get file index. Return -1 if file wasn't found */
    int LocateFile( const char * _FileName ) const;

    /** Get file compressed and uncompressed size */
    bool GetFileSize( int _FileIndex, size_t * _CompressedSize, size_t * _UncompressedSize ) const;

    /** Decompress file to memory buffer */
    bool ExtractFileToMemory( int _FileIndex, void * _MemoryBuffer, size_t _SizeInBytes ) const;

    /** Decompress file to heap memory */
    bool ExtractFileToHeapMemory( const char * _FileName, void ** _HeapMemoryPtr, int * _SizeInBytes ) const;

    /** Decompress file to hunk memory */
    bool ExtractFileToHunkMemory( const char * _FileName, void ** _HunkMemoryPtr, int * _SizeInBytes, int * _HunkMark ) const;

private:
    void * Handle;
};


/*

AFileStream

Read/Write to file

*/
class AFileStream final : public IBinaryStream {
    AN_FORBID_COPY( AFileStream )

public:
    AFileStream();
    ~AFileStream();

    /** Open file for read form specified path */
    bool OpenRead( const char * _FileName );
    /** Open file for read form specified path */
    bool OpenRead( AString const & _FileName ) { return OpenRead( _FileName.CStr() ); }

    /** Open file for write */
    bool OpenWrite( const char * _FileName );
    /** Open file for write */
    bool OpenWrite( AString const & _FileName ) { return OpenWrite( _FileName.CStr() ); }

    /** Open file for append */
    bool OpenAppend( const char * _FileName );
    /** Open file for append */
    bool OpenAppend( AString const & _FileName ) { return OpenAppend( _FileName.CStr() ); }

    /** Close file */
    void Close();

    /** Check is file opened */
    bool IsOpened() const { return Mode != M_Closed; }

protected:
    const char * Impl_GetFileName() const override;
    int Impl_Read( void * _Buffer, int _SizeInBytes ) override;
    int	Impl_Write( const void *_Buffer, int _SizeInBytes ) override;
    char * Impl_Gets( char * _StrBuf, int _StrSz ) override;
    void Impl_Flush() override;
    long Impl_Tell() override;
    bool Impl_SeekSet( long _Offset ) override;
    bool Impl_SeekCur( long _Offset ) override;
    bool Impl_SeekEnd( long _Offset ) override;
    size_t Impl_SizeInBytes() override;
    bool Impl_Eof() override;

private:
    bool Open( const char * _FileName, int _Mode );

    enum EMode {
        M_Read,
        M_Write,
        M_Append,
        M_Closed
    };

    AString Name;
    void *  Handle;
    int     Mode;
};


/**

AMemoryStream

Read/Write to memory

*/
class AMemoryStream final : public IBinaryStream {
    AN_FORBID_COPY( AMemoryStream )

public:
    AMemoryStream();
    ~AMemoryStream();

    /** Read from specified memory buffer */
    bool OpenRead( const char * _FileName, const void * _MemoryBuffer, size_t _SizeInBytes );
    /** Read from specified memory buffer */
    bool OpenRead( AString const & _FileName, const void * _MemoryBuffer, size_t _SizeInBytes );

    /** Read file from archive */
    bool OpenRead( const char * _FileName, AArchive const & _Archive );
    /** Read file from archive */
    bool OpenRead( AString const & _FileName, AArchive const & _Archive );

    /** Write to specified memory buffer */
    bool OpenWrite( const char * _FileName, void * _MemoryBuffer, size_t _SizeInBytes );
    /** Write to specified memory buffer */
    bool OpenWrite( AString const & _FileName, void * _MemoryBuffer, size_t _SizeInBytes );

    /** Write to inner memory buffer */
    bool OpenWrite( const char * _FileName, size_t _ReservedSize = 32 );
    /** Write to inner memory buffer */
    bool OpenWrite( AString const & _FileName, size_t _ReservedSize = 32 );

    /** Close file */
    void Close();

    /** Check is file opened */
    bool IsOpened() const { return Mode != M_Closed; }

    /** Grab memory buffer */
    void * GrabMemory();

    void SetGrowGranularity( int _Granularity ) { Granularity = _Granularity; }

protected:
    const char * Impl_GetFileName() const override;
    int Impl_Read( void * _Buffer, int _SizeInBytes ) override;
    int	Impl_Write( const void *_Buffer, int _SizeInBytes ) override;
    char * Impl_Gets( char * _StrBuf, int _StrSz ) override;
    void Impl_Flush() override;
    long Impl_Tell() override;
    bool Impl_SeekSet( long _Offset ) override;
    bool Impl_SeekCur( long _Offset ) override;
    bool Impl_SeekEnd( long _Offset ) override;
    size_t Impl_SizeInBytes() override;
    bool Impl_Eof() override;

private:
    enum EMode {
        M_Read,
        M_Write,
        M_Closed
    };

    void * Alloc( size_t _SizeInBytes );
    void * Realloc( void * _Data, size_t _SizeInBytes );
    void Free( void * _Data );

    AString Name;
    int     Mode;
    byte *  MemoryBuffer;
    int     MemoryBufferSize;
    bool    bMemoryBufferOwner;
    int     MemoryBufferOffset;
    int     Granularity;
};


namespace Core {

/** Make file system directory */
void MakeDir( const char * _Directory, bool _FileName );

/** Check is file exists */
bool IsFileExists( const char * _FileName );

/** Remove file from disk */
void RemoveFile( const char * _FileName );

using STraverseDirectoryCB = std::function< void( AString const & FileName, bool bIsDirectory ) >;
/** Traverse the directory */
void TraverseDirectory( AString const & Path, bool bSubDirs, STraverseDirectoryCB Callback );

/** Write game resource pack */
bool WriteResourcePack( const char * SourcePath, const char * ResultFile );

}
