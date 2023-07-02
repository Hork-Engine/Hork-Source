/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

HK_NAMESPACE_BEGIN

struct FileHandle
{
    FileHandle() = default;
    
    constexpr explicit FileHandle(int h) :
        h(h)
    {}
    
    operator bool() const
    {
        return h != -1;
    }
    
    bool IsValid() const
    {
        return h != -1;
    }
    
    operator int() const
    {
        return h;
    }

    void Reset()
    {
        h = -1;
    }
    
    constexpr static FileHandle Invalid()
    {
        return FileHandle(-1);
    }
    
private:
    int h{-1};
};


/**

Archive

Read file from archive

*/
class Archive final
{
    HK_FORBID_COPY(Archive)

public:
    Archive() = default;
    ~Archive();

    Archive(Archive&& Rhs) noexcept :
        m_Handle(Rhs.m_Handle)
    {
        Rhs.m_Handle = nullptr;
    }

    Archive& operator=(Archive&& Rhs) noexcept
    {
        Close();
        Core::Swap(m_Handle, Rhs.m_Handle);
        return *this;
    }

    operator bool() const
    {
        return IsOpened();
    }

    /** Open archive from file */
    static Archive Open(StringView ArchiveName, bool bResourcePack = false);

    /** Open archive from memory */
    static Archive OpenFromMemory(const void* pMemory, size_t SizeInBytes);

    /** Close archive */
    void Close();

    /** Check is archive opened */
    bool IsOpened() const { return !!m_Handle; }
    
    bool IsClosed() const { return !m_Handle; }

    /** Get total files in archive */
    int GetNumFiles() const;

    /** Get file handle. Returns an invalid handle if file wasn't found. */
    FileHandle LocateFile(StringView FileName) const;

    /** Get file compressed and uncompressed size */
    bool GetFileSize(FileHandle FileHandle, size_t* pCompressedSize, size_t* pUncompressedSize) const;

    /** Get file name by index */
    bool GetFileName(FileHandle FileHandle, String& FileName) const;

    /** Decompress file to memory buffer */
    bool ExtractFileToMemory(FileHandle FileHandle, void* pMemoryBuffer, size_t SizeInBytes) const;

    /** Decompress file to heap memory */
    bool ExtractFileToHeapMemory(StringView FileName, void** pHeapMemoryPtr, size_t* pSizeInBytes, MemoryHeap& Heap) const;

    /** Decompress file to heap memory */
    bool ExtractFileToHeapMemory(FileHandle FileHandle, void** pHeapMemoryPtr, size_t* pSizeInBytes, MemoryHeap& Heap) const;

private:
    void* m_Handle{};
};

class File final : public IBinaryStreamReadInterface, public IBinaryStreamWriteInterface
{
public:
    /** Open file for reading form specified path. */
    static File OpenRead(StringView FileName);

    /** Read from specified memory buffer. */
    static File OpenRead(StringView FileName, const void* pMemoryBuffer, size_t SizeInBytes);

    /** Read from specified memory buffer. */
    static File OpenRead(StringView FileName, BlobRef Blob);

    /** Read file from archive by file name. */
    static File OpenRead(StringView FileName, Archive const& Archive);

    /** Read file from archive by file index. */
    static File OpenRead(FileHandle FileHandle, Archive const& Archive);

    /** Open file for writing. */
    static File OpenWrite(StringView FileName);

    /** Write to specified memory buffer */
    static File OpenWrite(StringView StreamName, void* pMemoryBuffer, size_t SizeInBytes);

    /** Write to inner memory buffer */
    static File OpenWriteToMemory(StringView StreamName, size_t ReservedSize = 32);

    /** Open or create file for writing at end-of-file. */
    static File OpenAppend(StringView FileName);

    File() = default;

    File(File&& Rhs) noexcept :
        m_Name(std::move(Rhs.m_Name)),
        m_Type(Rhs.m_Type),
        m_Handle(Rhs.m_Handle),
        m_RWOffset(Rhs.m_RWOffset),
        m_FileSize(Rhs.m_FileSize),
        m_ReservedSize(Rhs.m_ReservedSize),
        m_Granularity(Rhs.m_Granularity),
        m_bMemoryBufferOwner(Rhs.m_bMemoryBufferOwner)
    {
        Rhs.m_Type               = FILE_TYPE_UNDEFINED;
        Rhs.m_Handle             = nullptr;
        Rhs.m_RWOffset           = 0;
        Rhs.m_FileSize           = 0;
        Rhs.m_ReservedSize       = 0;
        Rhs.m_Granularity        = 0;
        Rhs.m_bMemoryBufferOwner = true;
    }

    File& operator=(File&& Rhs) noexcept
    {
        Close();

        m_Name               = std::move(Rhs.m_Name);
        m_Type               = Rhs.m_Type;
        m_Handle             = Rhs.m_Handle;
        m_RWOffset           = Rhs.m_RWOffset;
        m_FileSize           = Rhs.m_FileSize;
        m_ReservedSize       = Rhs.m_ReservedSize;
        m_Granularity        = Rhs.m_Granularity;
        m_bMemoryBufferOwner = Rhs.m_bMemoryBufferOwner;

        Rhs.m_Type               = FILE_TYPE_UNDEFINED;
        Rhs.m_Handle             = nullptr;
        Rhs.m_RWOffset           = 0;
        Rhs.m_FileSize           = 0;
        Rhs.m_ReservedSize       = 0;
        Rhs.m_Granularity        = 0;
        Rhs.m_bMemoryBufferOwner = true;

        return *this;
    }

    ~File();

    /** Close file */
    void Close();

    bool IsOpened() const { return m_Type != FILE_TYPE_UNDEFINED; }

    bool IsClosed() const { return !IsOpened(); }

    bool IsValid() const override { return IsOpened(); }

    bool IsMemory() const
    {
        return m_Type == FILE_TYPE_READ_MEMORY || m_Type == FILE_TYPE_WRITE_MEMORY;
    }

    bool IsFileSystem() const
    {
        return m_Type == FILE_TYPE_READ_FILE_SYSTEM || m_Type == FILE_TYPE_WRITE_FILE_SYSTEM || m_Type == FILE_TYPE_APPEND_FILE_SYSTEM;
    }

    bool IsReadable() const
    {
        return m_Type == FILE_TYPE_READ_FILE_SYSTEM || m_Type == FILE_TYPE_READ_MEMORY;
    }

    bool IsWritable() const
    {
        return m_Type == FILE_TYPE_WRITE_FILE_SYSTEM || m_Type == FILE_TYPE_APPEND_FILE_SYSTEM || m_Type == FILE_TYPE_WRITE_MEMORY;
    }

    operator bool() const
    {
        return IsOpened();
    }

    IBinaryStreamReadInterface& ReadInterface()
    {
        HK_ASSERT(IsReadable());
        return *this;
    }

    IBinaryStreamWriteInterface& WriteInterface()
    {
        HK_ASSERT(IsWritable());
        return *this;
    }

    /** Get memory buffer. */
    void* GetHeapPtr();

    size_t GetMemoryReservedSize() const;

    void SetMemoryGrowGranularity(uint32_t Granularity) { m_Granularity = Granularity; }

    String const& GetName() const override
    {
        return m_Name;
    }

    size_t Read(void* pBuffer, size_t SizeInBytes) override;
    size_t Write(const void* pBuffer, size_t SizeInBytes) override;
    char*  Gets(char* pBuffer, size_t SizeInBytes) override;
    void   Flush() override;
    size_t GetOffset() const override;
    bool   SeekSet(int32_t Offset) override;
    bool   SeekCur(int32_t Offset) override;
    bool   SeekEnd(int32_t Offset) override;
    size_t SizeInBytes() const override;
    bool   Eof() const override;

private:
    enum FILE_TYPE : uint8_t
    {
        FILE_TYPE_UNDEFINED,
        FILE_TYPE_READ_FILE_SYSTEM,
        FILE_TYPE_READ_MEMORY,
        FILE_TYPE_WRITE_MEMORY,
        FILE_TYPE_WRITE_FILE_SYSTEM,
        FILE_TYPE_APPEND_FILE_SYSTEM
    };

    static File OpenFromFileSystem(StringView FileName, FILE_TYPE Type);
    static void* Alloc(size_t SizeInBytes);
    static void* Realloc(void* pMemory, size_t SizeInBytes);
    static void  Free(void* pMemory);

    String    m_Name;
    FILE_TYPE m_Type{FILE_TYPE_UNDEFINED};
    union
    {
        FILE* m_Handle{};
        byte* m_pHeapPtr;
    };
    size_t         m_RWOffset{};
    mutable size_t m_FileSize{};
    size_t         m_ReservedSize{};
    uint32_t       m_Granularity{1024};
    bool           m_bMemoryBufferOwner{true};
};

namespace Core
{

/** Make file system directory */
void CreateDirectory(StringView Directory, bool bFileName);

/** Check is file exists */
bool IsFileExists(StringView FileName);

/** Remove file from disk */
void RemoveFile(StringView FileName);

using STraverseDirectoryCB = std::function<void(StringView FileName, bool bIsDirectory)>;
/** Traverse the directory */
void TraverseDirectory(StringView Path, bool bSubDirs, STraverseDirectoryCB Callback);

/** Write game resource pack */
bool WriteResourcePack(StringView SourcePath, StringView ResultFile);

} // namespace Core

HK_NAMESPACE_END
