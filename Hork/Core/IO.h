/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

HK_NAMESPACE_BEGIN

struct FileHandle
{
                                FileHandle() = default;    
    constexpr explicit          FileHandle(int h) : m_Handle(h) {}
    
                                operator bool() const { return m_Handle != -1; }
                                operator int() const { return m_Handle; }
    
    bool                        IsValid() const { return m_Handle != -1; }
    void                        Reset() { m_Handle = -1; }
    
    constexpr static FileHandle sInvalid() { return FileHandle(-1); }
    
private:
    int                         m_Handle{-1};
};


/**

Archive

Read file from archive

*/
class Archive final : public Noncopyable
{
public:
                                Archive() = default;
                                ~Archive();

                                Archive(Archive&& rhs) noexcept;

    Archive&                    operator=(Archive&& rhs) noexcept;

                                operator bool() const { return IsOpened(); }

    /// Open archive from file
    static Archive              sOpen(StringView archiveName, bool isResourcePack = false);

    /// Open archive from memory
    static Archive              sOpenFromMemory(const void* memory, size_t sizeInBytes);

    /// Close archive
    void                        Close();

    /// Check is archive opened
    bool                        IsOpened() const { return !!m_Handle; }
    
    bool                        IsClosed() const { return !m_Handle; }

    /// Get total files in archive
    int                         GetNumFiles() const;

    /// Get file handle. Returns an invalid handle if file wasn't found.
    FileHandle                  LocateFile(StringView fileName) const;

    /// Get file compressed and decompressed size
    bool                        GetFileSize(FileHandle fileHandle, size_t* compressedSize, size_t* decompressedSize) const;

    /// Get file name by index
    bool                        GetFileName(FileHandle fileHandle, String& fileName) const;

    /// Decompress file to memory buffer
    bool                        ExtractFileToMemory(FileHandle fileHandle, void* memory, size_t sizeInBytes) const;

    /// Decompress file to heap memory
    bool                        ExtractFileToHeapMemory(StringView fileName, void** heapMemory, size_t* sizeInBytes, MemoryHeap& heap) const;

    /// Decompress file to heap memory
    bool                        ExtractFileToHeapMemory(FileHandle fileHandle, void** heapMemory, size_t* sizeInBytes, MemoryHeap& heap) const;

private:
    void*                       m_Handle{};
};

class File final : public IBinaryStreamReadInterface, public IBinaryStreamWriteInterface
{
public:
    /// Open file for reading form specified path.
    static File                 sOpenRead(StringView fileName);

    /// Read from specified memory buffer.
    static File                 sOpenRead(StringView fileName, const void* memory, size_t sizeInBytes);

    /// Read file from archive by file name.
    static File                 sOpenRead(StringView fileName, Archive const& archive);

    /// Read file from archive by file index.
    static File                 sOpenRead(FileHandle fileHandle, Archive const& archive);

    /// Open file for writing.
    static File                 sOpenWrite(StringView fileName);

    /// Write to specified memory buffer
    static File                 sOpenWrite(StringView streamName, void* memory, size_t sizeInBytes);

    /// Write to inner memory buffer
    static File                 sOpenWriteToMemory(StringView streamName, size_t reservedSize = 32);

    /// Open or create file for writing at end-of-file.
    static File                 sOpenAppend(StringView fileName);

                                File() = default;
                                File(File&& rhs) noexcept;
                                ~File();

    File&                       operator=(File&& rhs) noexcept;
                                operator bool() const { return IsOpened(); }


    /// Close file
    void                        Close();

    bool                        IsOpened() const { return m_Type != FileType::Undefined; }

    bool                        IsClosed() const { return !IsOpened(); }

    bool                        IsValid() const override { return IsOpened(); }

    bool                        IsMemory() const { return m_Type == FileType::ReadMemory || m_Type == FileType::WriteMemory; }

    bool                        IsFileSystem() const { return m_Type == FileType::ReadFS || m_Type == FileType::WriteFS || m_Type == FileType::AppendFS; }

    bool                        IsReadable() const { return m_Type == FileType::ReadFS || m_Type == FileType::ReadMemory; }

    bool                        IsWritable() const { return m_Type == FileType::WriteFS || m_Type == FileType::AppendFS || m_Type == FileType::WriteMemory; }


    IBinaryStreamReadInterface& ReadInterface() { HK_ASSERT(IsReadable()); return *this; }

    IBinaryStreamWriteInterface&WriteInterface() { HK_ASSERT(IsWritable()); return *this; }

    /// Get memory buffer.
    void*                       GetHeapPtr();

    size_t                      GetMemoryReservedSize() const;

    void                        SetMemoryGrowGranularity(uint32_t granularity) { m_Granularity = granularity; }

    StringView                  GetName() const override { return m_Name; }

    size_t                      Read(void* data, size_t sizeInBytes) override;

    size_t                      Write(const void* data, size_t sizeInBytes) override;

    char*                       Gets(char* str, size_t sizeInBytes) override;

    void                        Flush() override;

    size_t                      GetOffset() const override;

    bool                        SeekSet(int32_t offset) override;

    bool                        SeekCur(int32_t offset) override;
    bool                        SeekEnd(int32_t offset) override;

    size_t                      SizeInBytes() const override;

    bool                        IsEOF() const override;

private:
    enum class FileType : uint8_t
    {
        Undefined,
        ReadFS,
        WriteFS,
        AppendFS,
        ReadMemory,
        WriteMemory
    };

    static File                 sOpenFromFileSystem(StringView fileName, FileType type);
    static void*                sAlloc(size_t sizeInBytes);
    static void*                sRealloc(void* memory, size_t sizeInBytes);
    static void                 sFree(void* memory);

    String                      m_Name;
    FileType                    m_Type{FileType::Undefined};
    union
    {
        FILE*                   m_Handle{};
        byte*                   m_pHeapPtr;
    };
    size_t                      m_RWOffset{};
    mutable size_t              m_FileSize{};
    size_t                      m_ReservedSize{};
    uint32_t                    m_Granularity{1024};
    bool                        m_IsMemoryBufferOwner{true};
};

namespace Core
{

/// Make file system directory
void CreateDirectory(StringView directory, bool isFileName);

/// Check is file exists
bool IsFileExists(StringView fileName);

/// Remove file from disk
void RemoveFile(StringView fileName);

using STraverseDirectoryCB = std::function<void(StringView fileName, bool isDirectory)>;
/// Traverse the directory
void TraverseDirectory(StringView path, bool recursive, STraverseDirectoryCB callback);

/// Write game resource pack
bool WriteResourcePack(StringView sourcePath, StringView resultFile);

} // namespace Core

HK_NAMESPACE_END
