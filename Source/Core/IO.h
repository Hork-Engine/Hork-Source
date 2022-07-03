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

#pragma once

#include "BinaryStream.h"
#include "String.h"

/**

AArchive

Read file from archive

*/
class AArchive final
{
    HK_FORBID_COPY(AArchive)

public:
    AArchive();
    AArchive(AStringView ArchiveName, bool bResourcePack = false);
    AArchive(const void* pMemory, size_t SizeInBytes);

    ~AArchive();

    /** Open archive from file */
    bool Open(AStringView ArchiveName, bool bResourcePack = false);

    /** Open archive from memory */
    bool OpenFromMemory(const void* pMemory, size_t SizeInBytes);

    /** Close archive */
    void Close();

    /** Check is archive opened */
    bool IsOpened() const { return !!Handle; }

    /** Get total files in archive */
    int GetNumFiles() const;

    /** Get file index. Return -1 if file wasn't found */
    int LocateFile(AStringView FileName) const;

    /** Get file compressed and uncompressed size */
    bool GetFileSize(int FileIndex, size_t* pCompressedSize, size_t* pUncompressedSize) const;

    /** Get file name by index */
    bool GetFileName(int FileIndex, AString& FileName) const;

    /** Decompress file to memory buffer */
    bool ExtractFileToMemory(int FileIndex, void* pMemoryBuffer, size_t SizeInBytes) const;

    /** Decompress file to heap memory */
    bool ExtractFileToHeapMemory(AStringView FileName, void** pHeapMemoryPtr, size_t* pSizeInBytes, MemoryHeap& Heap) const;

    /** Decompress file to heap memory */
    bool ExtractFileToHeapMemory(int FileIndex, void** pHeapMemoryPtr, size_t* pSizeInBytes, MemoryHeap& Heap) const;

private:
    void* Handle{};
};


/*

AFileStream

Read/Write to file

*/
class AFileStream final : public IBinaryStreamReadInterface, public IBinaryStreamWriteInterface
{
    HK_FORBID_COPY(AFileStream)

public:
    AFileStream()
    {}

    AFileStream(AFileStream&& BinaryStream) noexcept :
        Name(std::move(BinaryStream.Name)), Mode(BinaryStream.Mode), Handle(BinaryStream.Handle), RWOffset(BinaryStream.RWOffset), FileSize(BinaryStream.FileSize)
    {
        BinaryStream.Mode     = FILE_OPEN_MODE_CLOSED;
        BinaryStream.Handle   = nullptr;
        BinaryStream.RWOffset = 0;
        BinaryStream.FileSize = 0;
    }

    AFileStream& operator=(AFileStream&& BinaryStream) noexcept
    {
        Close();

        Name     = std::move(BinaryStream.Name);
        Mode     = BinaryStream.Mode;
        Handle   = BinaryStream.Handle;
        RWOffset = BinaryStream.RWOffset;
        FileSize = BinaryStream.FileSize;

        BinaryStream.Mode     = FILE_OPEN_MODE_CLOSED;
        BinaryStream.Handle   = nullptr;
        BinaryStream.RWOffset = 0;
        BinaryStream.FileSize = 0;

        return *this;
    }

    ~AFileStream();

    /** Open file for read form specified path */
    bool OpenRead(AStringView FileName);

    /** Open file for write */
    bool OpenWrite(AStringView FileName);

    /** Open file for append */
    bool OpenAppend(AStringView FileName);

    /** Close file */
    void Close();

    /** Check is file opened */
    bool IsOpened() const { return Mode != FILE_OPEN_MODE_CLOSED; }

    AString const& GetFileName() const override;
    size_t      Read(void* pBuffer, size_t SizeInBytes) override;
    size_t      Write(const void* pBuffer, size_t SizeInBytes) override;
    char*       Gets(char* pBuffer, size_t SizeInBytes) override;
    void        Flush() override;
    size_t      GetOffset() const override;
    bool        SeekSet(int32_t Offset) override;
    bool        SeekCur(int32_t Offset) override;
    bool        SeekEnd(int32_t Offset) override;
    size_t      SizeInBytes() const override;
    bool        Eof() const override;

private:
    enum FILE_OPEN_MODE : uint8_t
    {
        FILE_OPEN_MODE_CLOSED,
        FILE_OPEN_MODE_READ,
        FILE_OPEN_MODE_WRITE,
        FILE_OPEN_MODE_APPEND
    };

    bool Open(AStringView FileName, FILE_OPEN_MODE Mode);

    AString        Name;
    FILE_OPEN_MODE Mode{FILE_OPEN_MODE_CLOSED};
    FILE*          Handle{};
    size_t         RWOffset{};
    mutable size_t FileSize{};
};


/**

AMemoryStream

Read/Write to memory

*/
class AMemoryStream final : public IBinaryStreamReadInterface, public IBinaryStreamWriteInterface
{
    HK_FORBID_COPY(AMemoryStream)

public:
    AMemoryStream()
    {}

    AMemoryStream(AMemoryStream&& BinaryStream) noexcept :
        Name(std::move(BinaryStream.Name)),
        Mode(BinaryStream.Mode),
        pHeapPtr(BinaryStream.pHeapPtr),
        HeapSize(BinaryStream.HeapSize),
        ReservedSize(BinaryStream.ReservedSize),
        RWOffset(BinaryStream.RWOffset),
        Granularity(BinaryStream.Granularity),
        bMemoryBufferOwner(BinaryStream.bMemoryBufferOwner)
    {
        BinaryStream.Mode               = FILE_OPEN_MODE_CLOSED;
        BinaryStream.pHeapPtr           = nullptr;
        BinaryStream.HeapSize           = 0;
        BinaryStream.ReservedSize       = 0;
        BinaryStream.RWOffset           = 0;
        BinaryStream.Granularity        = 0;
        BinaryStream.bMemoryBufferOwner = true;
    }

    ~AMemoryStream();

    AMemoryStream& operator=(AMemoryStream&& BinaryStream) noexcept
    {
        Close();

        Name               = std::move(BinaryStream.Name);
        Mode               = BinaryStream.Mode;
        pHeapPtr           = BinaryStream.pHeapPtr;
        HeapSize           = BinaryStream.HeapSize;
        ReservedSize       = BinaryStream.ReservedSize;
        RWOffset           = BinaryStream.RWOffset;
        Granularity        = BinaryStream.Granularity;
        bMemoryBufferOwner = BinaryStream.bMemoryBufferOwner;

        BinaryStream.Mode               = FILE_OPEN_MODE_CLOSED;
        BinaryStream.pHeapPtr           = nullptr;
        BinaryStream.HeapSize           = 0;
        BinaryStream.ReservedSize       = 0;
        BinaryStream.RWOffset           = 0;
        BinaryStream.Granularity        = 0;
        BinaryStream.bMemoryBufferOwner = true;

        return *this;
    }

    /** Read from specified memory buffer */
    bool OpenRead(AStringView FileName, const void* pMemoryBuffer, size_t SizeInBytes);

    /** Read file from archive */
    bool OpenRead(AStringView FileName, AArchive const& Archive);

    /** Read file from archive */
    bool OpenRead(int FileIndex, AArchive const& Archive);

    /** Write to specified memory buffer */
    bool OpenWrite(AStringView FileName, void* pMemoryBuffer, size_t SizeInBytes);

    /** Write to inner memory buffer */
    bool OpenWrite(AStringView FileName, size_t ReservedSize = 32);

    /** Close file */
    void Close();

    /** Check is file opened */
    bool IsOpened() const { return Mode != FILE_OPEN_MODE_CLOSED; }

    /** Get memory buffer */
    void* GetHeapPtr();

    size_t GetReservedSize() const;

    void SetGrowGranularity(int _Granularity) { Granularity = _Granularity; }

    AString const& GetFileName() const override;
    size_t      Read(void* pBuffer, size_t SizeInBytes) override;
    size_t      Write(const void* pBuffer, size_t SizeInBytes) override;
    char*       Gets(char* pBuffer, size_t SizeInBytes) override;
    void        Flush() override;
    size_t      GetOffset() const override;
    bool        SeekSet(int32_t Offset) override;
    bool        SeekCur(int32_t Offset) override;
    bool        SeekEnd(int32_t Offset) override;
    size_t      SizeInBytes() const override;
    bool        Eof() const override;

private:
    enum FILE_OPEN_MODE : uint8_t
    {
        FILE_OPEN_MODE_CLOSED,
        FILE_OPEN_MODE_READ,
        FILE_OPEN_MODE_WRITE
    };

    void* Alloc(size_t SizeInBytes);
    void* Realloc(void* pMemory, size_t SizeInBytes);
    void  Free(void* pMemory);

    AString        Name;
    FILE_OPEN_MODE Mode{FILE_OPEN_MODE_CLOSED};
    byte*          pHeapPtr{};
    size_t         HeapSize{};
    size_t         ReservedSize{};
    size_t         RWOffset{};
    int            Granularity{1024};
    bool           bMemoryBufferOwner{true};
};


namespace Core
{

/** Make file system directory */
void CreateDirectory(AStringView Directory, bool bFileName);

/** Check is file exists */
bool IsFileExists(AStringView FileName);

/** Remove file from disk */
void RemoveFile(AStringView FileName);

using STraverseDirectoryCB = std::function<void(AStringView FileName, bool bIsDirectory)>;
/** Traverse the directory */
void TraverseDirectory(AStringView Path, bool bSubDirs, STraverseDirectoryCB Callback);

/** Write game resource pack */
bool WriteResourcePack(AStringView SourcePath, AStringView ResultFile);

} // namespace Core
