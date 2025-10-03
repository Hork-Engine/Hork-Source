/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include "IO.h"
#include "BaseMath.h"
#include "WindowsDefs.h"
#include "Logger.h"

#ifdef HK_OS_WIN32
#    include <direct.h> // _mkdir
#    include <io.h>     // access
#endif
#ifdef HK_OS_LINUX
#    include <dirent.h>
#    include <sys/stat.h> // _mkdir
#    include <unistd.h>   // access
#endif

#include <miniz/miniz.h>

namespace
{
FILE* OpenFile(const char* fileName, const char* Mode)
{
    FILE* f;
#if defined(_MSC_VER)
    wchar_t wMode[64];

    if (0 == MultiByteToWideChar(CP_UTF8, 0, Mode, -1, wMode, HK_ARRAY_SIZE(wMode)))
        return NULL;

    int n = MultiByteToWideChar(CP_UTF8, 0, fileName, -1, NULL, 0);
    if (0 == n)
    {
        return NULL;
    }

    wchar_t* wFilename = (wchar_t*)HkStackAlloc(n * sizeof(wchar_t));

    MultiByteToWideChar(CP_UTF8, 0, fileName, -1, wFilename, n);

#    if _MSC_VER >= 1400
    if (0 != _wfopen_s(&f, wFilename, wMode))
        f = NULL;
#    else
    f = _wfopen(wFilename, wMode);
#    endif
#else
    f = fopen(fileName, Mode);
#endif
    return f;
}
}

HK_NAMESPACE_BEGIN

File::File(File&& rhs) noexcept :
    m_Name(std::move(rhs.m_Name)),
    m_Type(rhs.m_Type),
    m_Handle(rhs.m_Handle),
    m_RWOffset(rhs.m_RWOffset),
    m_FileSize(rhs.m_FileSize),
    m_ReservedSize(rhs.m_ReservedSize),
    m_Granularity(rhs.m_Granularity),
    m_IsMemoryBufferOwner(rhs.m_IsMemoryBufferOwner)
{
    rhs.m_Type               = FileType::Undefined;
    rhs.m_Handle             = nullptr;
    rhs.m_RWOffset           = 0;
    rhs.m_FileSize           = 0;
    rhs.m_ReservedSize       = 0;
    rhs.m_Granularity        = 0;
    rhs.m_IsMemoryBufferOwner = true;
}

File& File::operator=(File&& rhs) noexcept
{
    Close();

    m_Name               = std::move(rhs.m_Name);
    m_Type               = rhs.m_Type;
    m_Handle             = rhs.m_Handle;
    m_RWOffset           = rhs.m_RWOffset;
    m_FileSize           = rhs.m_FileSize;
    m_ReservedSize       = rhs.m_ReservedSize;
    m_Granularity        = rhs.m_Granularity;
    m_IsMemoryBufferOwner = rhs.m_IsMemoryBufferOwner;

    rhs.m_Type               = FileType::Undefined;
    rhs.m_Handle             = nullptr;
    rhs.m_RWOffset           = 0;
    rhs.m_FileSize           = 0;
    rhs.m_ReservedSize       = 0;
    rhs.m_Granularity        = 0;
    rhs.m_IsMemoryBufferOwner = true;

    return *this;
}

File::~File()
{
    Close();
}

File File::sOpenRead(StringView fileName)
{
    return sOpenFromFileSystem(fileName, FileType::ReadFS);
}

File File::sOpenWrite(StringView fileName)
{
    return sOpenFromFileSystem(fileName, FileType::WriteFS);
}

File File::sOpenAppend(StringView fileName)
{
    return sOpenFromFileSystem(fileName, FileType::AppendFS);
}

File File::sOpenFromFileSystem(StringView fileName, FileType type)
{
    File f;

    f.m_Name = PathUtils::sFixPath(fileName);
    if (f.m_Name.Length() && f.m_Name[f.m_Name.Length() - 1] == '/')
    {
        LOG("Invalid file name {}\n", fileName);
        return {};
    }

    if (type == FileType::WriteFS || type == FileType::AppendFS)
    {
        Core::CreateDirectory(f.m_Name, true);
    }

    const char* fopen_mode = "";

    switch (type)
    {
        case FileType::ReadFS:
            fopen_mode = "rb";
            break;
        case FileType::WriteFS:
            fopen_mode = "wb";
            break;
        case FileType::AppendFS:
            fopen_mode = "ab";
            break;
        default:
            HK_ASSERT(0);
            return {};
    }

    f.m_Handle = OpenFile(f.m_Name.CStr(), fopen_mode);
    if (!f.m_Handle)
    {
        LOG("Couldn't open {}\n", f.m_Name);
        return {};
    }

    f.m_Type = type;

    if (f.m_Type == FileType::ReadFS)
    {
        f.m_FileSize = ~0ull;
    }

    return f;
}

void File::Close()
{
    if (m_Type == FileType::Undefined)
    {
        return;
    }

    if (IsFileSystem())
    {
        fclose(m_Handle);
    }
    else
    {
        if (m_IsMemoryBufferOwner)
        {
            sFree(m_pHeapPtr);
        }
    }

    m_Name.Clear();
    m_Type               = FileType::Undefined;
    m_RWOffset           = 0;
    m_FileSize           = 0;
    m_Handle             = nullptr;
    m_ReservedSize       = 0;
    m_IsMemoryBufferOwner = true;
}

size_t File::Read(void* data, size_t sizeInBytes)
{
    size_t bytesToRead{};

    if (!IsReadable())
    {
        LOG("Reading from {} is not allowed. The file must be opened in read mode.\n", m_Name);
    }
    else
    {
        if (IsFileSystem())
        {
            bytesToRead = fread(data, 1, sizeInBytes, m_Handle);
        }
        else
        {
            bytesToRead = std::min(sizeInBytes, m_FileSize - m_RWOffset);
            Core::Memcpy(data, m_pHeapPtr + m_RWOffset, bytesToRead);
        }
    }

    sizeInBytes -= bytesToRead;
    if (sizeInBytes)
        Core::ZeroMem((uint8_t*)data + bytesToRead, sizeInBytes);

    m_RWOffset += bytesToRead;
    return bytesToRead;
}

size_t File::Write(const void* data, size_t sizeInBytes)
{
    size_t bytesToWrite{};

    if (!IsWritable())
    {
        LOG("Writing to {} is not allowed. The file must be opened in write mode.\n", m_Name);
        return 0;
    }

    if (IsFileSystem())
    {
        bytesToWrite = fwrite(data, 1, sizeInBytes, m_Handle);
    }
    else
    {
        size_t requiredSize = m_RWOffset + sizeInBytes;
        if (requiredSize > m_ReservedSize)
        {
            if (!m_IsMemoryBufferOwner)
            {
                LOG("Failed to write {} (buffer overflowed)\n", m_Name);
                return 0;
            }
            const int mod  = requiredSize % m_Granularity;
            m_ReservedSize = mod ? requiredSize + m_Granularity - mod : requiredSize;
            m_pHeapPtr     = (byte*)sRealloc(m_pHeapPtr, m_ReservedSize);
        }
        Core::Memcpy(m_pHeapPtr + m_RWOffset, data, sizeInBytes);
        bytesToWrite = sizeInBytes;
    }

    m_RWOffset += bytesToWrite;
    m_FileSize = std::max(m_FileSize, m_RWOffset);
    return bytesToWrite;
}

char* File::Gets(char* str, size_t sizeInBytes)
{
    if (!IsReadable())
    {
        LOG("Reading from {} is not allowed. The file must be opened in read mode.\n", m_Name);
        return nullptr;
    }

    if (IsFileSystem())
    {
        str = fgets(str, sizeInBytes, m_Handle);

        m_RWOffset = ftell(m_Handle);
    }
    else
    {
        if (sizeInBytes == 0 || m_RWOffset >= m_FileSize)
        {
            return nullptr;
        }

        size_t maxChars = sizeInBytes - 1;
        if (m_RWOffset + maxChars > m_FileSize)
        {
            maxChars = m_FileSize - m_RWOffset;
        }

        char *memoryPointer, *memory = (char*)&m_pHeapPtr[m_RWOffset];
        char* stringPointer = str;

        for (memoryPointer = memory; memoryPointer < &memory[maxChars]; memoryPointer++)
        {
            if (*memoryPointer == '\n')
            {
                *stringPointer++ = *memoryPointer++;
                break;
            }
            *stringPointer++ = *memoryPointer;
        }

        *stringPointer = '\0';
        m_RWOffset += memoryPointer - memory;
    }

    return str;
}

void File::Flush()
{
    if (IsFileSystem() && IsWritable())
    {
        fflush(m_Handle);
    }
}

size_t File::GetOffset() const
{
    return m_RWOffset;
}

#define HasFileSize() (m_FileSize != ~0ull)

bool File::SeekSet(int32_t offset)
{
    if (!IsOpened())
        return false;

    m_RWOffset = std::max(int32_t{0}, offset);

    if (IsFileSystem())
    {
        if (HasFileSize())
        {
            if (m_RWOffset > m_FileSize)
                m_RWOffset = m_FileSize;
        }

        bool r = fseek(m_Handle, m_RWOffset, SEEK_SET) == 0;
        if (!r)
            m_RWOffset = ftell(m_Handle);

        return r;
    }
    else
    {
        if (m_RWOffset > m_FileSize)
            m_RWOffset = m_FileSize;
        return true;
    }
}

bool File::SeekCur(int32_t offset)
{
    if (!IsOpened())
        return false;

    if (IsFileSystem())
    {
        bool r;

        if (offset < 0 && -offset > m_RWOffset)
        {
            // Rewind
            m_RWOffset = 0;

            r = fseek(m_Handle, 0, SEEK_SET) == 0;
        }
        else
        {
            if (HasFileSize())
            {
                if (m_RWOffset + offset > m_FileSize)
                {
                    offset = m_FileSize - m_RWOffset;
                }
            }

            m_RWOffset += offset;

            r = fseek(m_Handle, offset, SEEK_CUR) == 0;
        }

        if (!r)
        {
            m_RWOffset = ftell(m_Handle);
        }

        return r;
    }
    else
    {
        if (offset < 0 && -offset > m_RWOffset)
        {
            m_RWOffset = 0;
        }
        else
        {
            size_t prevOffset = m_RWOffset;
            m_RWOffset += offset;
            if (m_RWOffset > m_FileSize || (offset > 0 && prevOffset > m_RWOffset))
                m_RWOffset = m_FileSize;
        }
        return true;
    }
}

bool File::SeekEnd(int32_t offset)
{
    if (!IsOpened())
        return false;

    if (IsFileSystem())
    {
        bool r;

        if (offset >= 0)
        {
            offset = 0;

            r = fseek(m_Handle, 0, SEEK_END) == 0;

            if (r && HasFileSize())
            {
                m_RWOffset = m_FileSize;
            }
            else
            {
                m_RWOffset = ftell(m_Handle);
                if (r)
                    m_FileSize = m_RWOffset;
            }

            return r;
        }

        if (HasFileSize() && -offset >= m_FileSize)
        {
            // Rewind
            m_RWOffset = 0;
            r          = fseek(m_Handle, 0, SEEK_SET) == 0;

            if (!r)
                m_RWOffset = ftell(m_Handle);

            return r;
        }

        r          = fseek(m_Handle, offset, SEEK_END) == 0;
        m_RWOffset = ftell(m_Handle);

        return r;
    }
    else
    {
        if (offset >= 0)
            m_RWOffset = m_FileSize;
        else if (-offset > m_FileSize)
            m_RWOffset = 0;
        else
            m_RWOffset = m_FileSize + offset;
        return true;
    }
}

size_t File::SizeInBytes() const
{
    if (!IsOpened())
        return 0;

    if (IsFileSystem())
    {
        if (!HasFileSize())
        {
            fseek(m_Handle, 0, SEEK_END);
            m_FileSize = ftell(m_Handle);
            fseek(m_Handle, m_RWOffset, SEEK_SET);
        }
    }
    return m_FileSize;
}

bool File::IsEOF() const
{
    if (!IsOpened())
        return false;

    if (IsFileSystem())
        return feof(m_Handle) != 0;
    else
        return m_RWOffset >= m_FileSize;
}

void* File::sAlloc(size_t sizeInBytes)
{
    return Core::GetHeapAllocator<HEAP_MISC>().Alloc(sizeInBytes, 16);
}

void* File::sRealloc(void* memory, size_t sizeInBytes)
{
    return Core::GetHeapAllocator<HEAP_MISC>().Realloc(memory, sizeInBytes, 16);
}

void File::sFree(void* memory)
{
    Core::GetHeapAllocator<HEAP_MISC>().Free(memory);
}

File File::sOpenRead(StringView fileName, const void* memory, size_t sizeInBytes)
{
    File f;

    f.m_Name               = fileName;
    f.m_Type               = FileType::ReadMemory;
    f.m_pHeapPtr           = reinterpret_cast<byte*>(const_cast<void*>(memory));
    f.m_FileSize           = sizeInBytes;
    f.m_ReservedSize       = sizeInBytes;
    f.m_IsMemoryBufferOwner = false;

    return f;
}

File File::sOpenRead(StringView fileName, Archive const& archive)
{
    File f;

    if (!archive.ExtractFileToHeapMemory(fileName, (void**)&f.m_pHeapPtr, &f.m_FileSize, Core::GetHeapAllocator<HEAP_MISC>()))
    {
        LOG("Couldn't open {}\n", fileName);
        return {};
    }

    f.m_Name               = fileName;
    f.m_Type               = FileType::ReadMemory;
    f.m_ReservedSize       = f.m_FileSize;
    f.m_IsMemoryBufferOwner = true;

    return f;
}

File File::sOpenRead(FileHandle fileHandle, Archive const& archive)
{
    File f;

    archive.GetFileName(fileHandle, f.m_Name);

    if (!archive.ExtractFileToHeapMemory(fileHandle, (void**)&f.m_pHeapPtr, &f.m_FileSize, Core::GetHeapAllocator<HEAP_MISC>()))
    {
        LOG("Couldn't open {}\n", f.m_Name);
        return {};
    }

    f.m_Type               = FileType::ReadMemory;
    f.m_ReservedSize       = f.m_FileSize;
    f.m_IsMemoryBufferOwner = true;

    return f;
}

File File::sOpenWrite(StringView streamName, void* memory, size_t sizeInBytes)
{
    File f;

    f.m_Name               = streamName;
    f.m_Type               = FileType::WriteMemory;
    f.m_pHeapPtr           = reinterpret_cast<byte*>(memory);
    f.m_FileSize           = 0;
    f.m_ReservedSize       = sizeInBytes;
    f.m_IsMemoryBufferOwner = false;

    return f;
}

File File::sOpenWriteToMemory(StringView streamName, size_t reservedSize)
{
    File f;

    f.m_Name               = streamName;
    f.m_Type               = FileType::WriteMemory;
    f.m_pHeapPtr           = (byte*)sAlloc(reservedSize);
    f.m_FileSize           = 0;
    f.m_ReservedSize       = reservedSize;
    f.m_IsMemoryBufferOwner = true;

    return f;
}

size_t File::GetMemoryReservedSize() const
{
    return m_ReservedSize;
}

void* File::GetHeapPtr()
{
    if (IsFileSystem())
        return nullptr;
    return m_pHeapPtr;
}

Archive::Archive(Archive&& rhs) noexcept :
    m_Handle(rhs.m_Handle)
{
    rhs.m_Handle = nullptr;
}

Archive& Archive::operator=(Archive&& rhs) noexcept
{
    Close();
    Core::Swap(m_Handle, rhs.m_Handle);
    return *this;
}

Archive::~Archive()
{
    Close();
}

Archive Archive::sOpen(StringView archiveName, bool isResourcePack)
{
    mz_zip_archive arch;
    mz_uint64      fileStartOffset = 0;
    mz_uint64      archiveSize     = 0;

    if (isResourcePack)
    {
        File f = File::sOpenRead(archiveName);
        if (!f)
        {
            return {};
        }

        uint64_t    magic = f.ReadUInt64();
        const char* MAGIC = "ARESPACK";
        if (std::memcmp(&magic, MAGIC, sizeof(uint64_t)) != 0)
        {
            LOG("Invalid file format {}\n", archiveName);
            return {};
        }

        fileStartOffset += sizeof(magic);
        archiveSize = f.SizeInBytes() - fileStartOffset;
    }

    Core::ZeroMem(&arch, sizeof(arch));

    mz_bool status = mz_zip_reader_init_file_v2(&arch, archiveName.IsNullTerminated() ? archiveName.Begin() : String(archiveName).CStr(), 0, fileStartOffset, archiveSize);
    if (!status)
    {
        LOG("Couldn't open archive {}\n", archiveName);
        return {};
    }

    Archive archive;
    archive.m_Handle = Core::GetHeapAllocator<HEAP_MISC>().Alloc(sizeof(mz_zip_archive));
    Core::Memcpy(archive.m_Handle, &arch, sizeof(arch));

    // Keep pointer valid
    ((mz_zip_archive*)archive.m_Handle)->m_pIO_opaque = archive.m_Handle;

    return archive;
}

Archive Archive::sOpenFromMemory(const void* memory, size_t sizeInBytes)
{
    mz_zip_archive arch;

    Core::ZeroMem(&arch, sizeof(arch));

    mz_bool status = mz_zip_reader_init_mem(&arch, memory, sizeInBytes, 0);
    if (!status)
    {
        LOG("Couldn't open archive from memory\n");
        return {};
    }

    Archive archive;
    archive.m_Handle = Core::GetHeapAllocator<HEAP_MISC>().Alloc(sizeof(mz_zip_archive));
    Core::Memcpy(archive.m_Handle, &arch, sizeof(arch));

    // Keep pointer valid
    ((mz_zip_archive*)archive.m_Handle)->m_pIO_opaque = archive.m_Handle;

    return archive;
}

void Archive::Close()
{
    if (!m_Handle)
    {
        return;
    }

    mz_zip_reader_end((mz_zip_archive*)m_Handle);

    Core::GetHeapAllocator<HEAP_MISC>().Free(m_Handle);
    m_Handle = nullptr;
}

int Archive::GetNumFiles() const
{
    return mz_zip_reader_get_num_files((mz_zip_archive*)m_Handle);
}

FileHandle Archive::LocateFile(StringView fileName) const
{
    return FileHandle(mz_zip_reader_locate_file((mz_zip_archive*)m_Handle, fileName.IsNullTerminated() ? fileName.Begin() : String(fileName).CStr(), NULL, 0));
}

#define MZ_ZIP_CDH_COMPRESSED_SIZE_OFS   20
#define MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS 24
#define MZ_ZIP_CDH_FILENAME_LEN_OFS      28
#define MZ_ZIP_CENTRAL_DIR_HEADER_SIZE   46

bool Archive::GetFileSize(FileHandle fileHandle, size_t* compressedSize, size_t* decompressedSize) const
{
    // All checks are processd in mz_zip_get_cdh
    const mz_uint8* p = mz_zip_get_cdh_public((mz_zip_archive*)m_Handle, int(fileHandle));
    if (!p)
    {
        return false;
    }

    if (compressedSize)
    {
        *compressedSize = MZ_READ_LE32(p + MZ_ZIP_CDH_COMPRESSED_SIZE_OFS);
    }

    if (decompressedSize)
    {
        *decompressedSize = MZ_READ_LE32(p + MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS);
    }
    return true;
}

bool Archive::GetFileName(FileHandle fileHandle, String& fileName) const
{
    // All checks are processd in mz_zip_get_cdh
    const mz_uint8* p = mz_zip_get_cdh_public((mz_zip_archive*)m_Handle, int(fileHandle));
    if (!p)
    {
        return false;
    }

    mz_uint     filename_len = MZ_READ_LE16(p + MZ_ZIP_CDH_FILENAME_LEN_OFS);
    const char* pFilename    = (const char*)p + MZ_ZIP_CENTRAL_DIR_HEADER_SIZE;

    if (filename_len < 1)
    {
        return false;
    }

    fileName.Resize(filename_len);
    Core::Memcpy(fileName.ToPtr(), pFilename, filename_len);

    return true;
}

bool Archive::ExtractFileToMemory(FileHandle fileHandle, void* memory, size_t sizeInBytes) const
{
    // All checks are processd in mz_zip_reader_extract_to_mem
    return !!mz_zip_reader_extract_to_mem((mz_zip_archive*)m_Handle, int(fileHandle), memory, sizeInBytes, 0);
}

bool Archive::ExtractFileToHeapMemory(StringView fileName, void** heapMemory, size_t* sizeInBytes, MemoryHeap& heap) const
{
    size_t uncompSize;

    *heapMemory = nullptr;
    *sizeInBytes = 0;

    FileHandle fileHandle = LocateFile(fileName);
    if (!fileHandle)
    {
        return false;
    }

    if (!GetFileSize(fileHandle, NULL, &uncompSize))
    {
        return false;
    }

    void* buf = heap.Alloc(uncompSize);

    if (!ExtractFileToMemory(fileHandle, buf, uncompSize))
    {
        heap.Free(buf);
        return false;
    }

    *heapMemory = buf;
    *sizeInBytes = uncompSize;

    return true;
}

bool Archive::ExtractFileToHeapMemory(FileHandle fileHandle, void** heapMemory, size_t* sizeInBytes, MemoryHeap& heap) const
{
    size_t uncompSize;

    *heapMemory = nullptr;
    *sizeInBytes = 0;

    if (!fileHandle)
    {
        return false;
    }

    if (!GetFileSize(fileHandle, NULL, &uncompSize))
    {
        return false;
    }

    void* buf = heap.Alloc(uncompSize);

    if (!ExtractFileToMemory(fileHandle, buf, uncompSize))
    {
        heap.Free(buf);
        return false;
    }

    *heapMemory = buf;
    *sizeInBytes = uncompSize;

    return true;
}

namespace Core
{

void CreateDirectory(StringView directory, bool isFileName)
{
    size_t len = directory.Length();
    if (!len)
    {
        return;
    }
    const char* dir    = directory.ToPtr();
    char*       tmpStr = (char*)Core::GetHeapAllocator<HEAP_TEMP>().Alloc(len + 1);
    Core::Memcpy(tmpStr, dir, len + 1);
    char* p = tmpStr;
#ifdef HK_OS_WIN32
    if (len >= 3 && dir[1] == ':')
    {
        p += 3;
        dir += 3;
    }
#endif
    for (; dir < directory.End(); p++, dir++)
    {
        if (IsPathSeparator(*p))
        {
            *p = 0;
#ifdef HK_COMPILER_MSVC
            mkdir(tmpStr);
#else
            mkdir(tmpStr, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
            *p = *dir;
        }
    }
    if (!isFileName)
    {
#ifdef HK_COMPILER_MSVC
        mkdir(tmpStr);
#else
        mkdir(tmpStr, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
    }
    Core::GetHeapAllocator<HEAP_TEMP>().Free(tmpStr);
}

bool IsFileExists(StringView fileName)
{
    return access(PathUtils::sFixSeparator(fileName).CStr(), 0) == 0;
}

void RemoveFile(StringView fileName)
{
    String s = PathUtils::sFixPath(fileName);
#if defined HK_OS_LINUX
    ::remove(s.CStr());
#elif defined HK_OS_WIN32
    int n = MultiByteToWideChar(CP_UTF8, 0, s.CStr(), -1, NULL, 0);
    if (0 != n)
    {
        wchar_t* wFilename = (wchar_t*)HkStackAlloc(n * sizeof(wchar_t));

        MultiByteToWideChar(CP_UTF8, 0, s.CStr(), -1, wFilename, n);

        ::DeleteFile(wFilename);
    }
#else
    static_assert(0, "TODO: Implement RemoveFile for current build settings");
#endif
}

#ifdef HK_OS_LINUX
void TraverseDirectory(StringView path, bool recursive, STraverseDirectoryCB callback)
{
    String fn;
    DIR*    dir = opendir(path.IsNullTerminated() ? path.Begin() : String(path).CStr());
    if (dir)
    {
        while (1)
        {
            dirent* entry = readdir(dir);
            if (!entry)
            {
                break;
            }

            fn      = path;
            int len = path.Length();
            if (len > 1 && path[len - 1] != '/')
            {
                fn += "/";
            }
            fn += entry->d_name;

            struct stat statInfo;
            lstat(fn.CStr(), &statInfo);

            if (S_ISDIR(statInfo.st_mode))
            {
                if (recursive)
                {
                    if (!strcmp(".", entry->d_name) || !strcmp("..", entry->d_name))
                    {
                        continue;
                    }
                    TraverseDirectory(fn, recursive, callback);
                }
                callback(fn, true);
            }
            else
            {
                callback(path / entry->d_name, false);
            }
        }
        closedir(dir);
    }
}
#endif

#ifdef HK_OS_WIN32
void TraverseDirectory(StringView path, bool recursive, STraverseDirectoryCB callback)
{
    String fn;

    HANDLE           fh;
    WIN32_FIND_DATAA fd;

    if ((fh = FindFirstFileA((path + "/*.*").CStr(), &fd)) != INVALID_HANDLE_VALUE)
    {
        do {
            fn      = path;
            int len = (int)path.Length();
            if (len > 1 && path[len - 1] != '/')
            {
                fn += "/";
            }
            fn += fd.cFileName;

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (recursive)
                {
                    if (!strcmp(".", fd.cFileName) || !strcmp("..", fd.cFileName))
                    {
                        continue;
                    }
                    TraverseDirectory(fn, recursive, callback);
                }
                callback(fn, true);
            }
            else
            {
                callback(path / fd.cFileName, false);
            }
        } while (FindNextFileA(fh, &fd));

        FindClose(fh);
    }
}
#endif

bool WriteResourcePack(StringView sourcePath, StringView resultFile)
{
    String path   = PathUtils::sFixSeparator(sourcePath);
    String result = PathUtils::sFixSeparator(resultFile);

    LOG("==== WriteResourcePack ====\n"
        "Source '{}'\n"
        "Destination: '{}'\n",
        path, result);

    FILE* file = OpenFile(result.CStr(), "wb");
    if (!file)
    {
        return false;
    }

    uint64_t magic;
    Core::Memcpy(&magic, "ARESPACK", sizeof(magic));
    magic = Core::LittleDDWord(magic);
    fwrite(&magic, 1, sizeof(magic), file);

    mz_zip_archive zip;
    Core::ZeroMem(&zip, sizeof(zip));

    if (mz_zip_writer_init_cfile(&zip, file, 0))
    {
        TraverseDirectory(path, true,
                          [&zip, &path](StringView fileName, bool bIsDirectory)
                          {
                              if (bIsDirectory)
                              {
                                  return;
                              }

                              if (PathUtils::sCompareExt(fileName, ".resources", true))
                              {
                                  return;
                              }

                              String fn(fileName.TruncateHead(path.Length() + 1));

                              LOG("Writing '{}'\n", fn);

                              mz_bool status = mz_zip_writer_add_file(&zip, fn.CStr(), fileName.IsNullTerminated() ? fileName.Begin() : String(fileName).CStr(), nullptr, 0, MZ_UBER_COMPRESSION);
                              if (!status)
                              {
                                  LOG("Failed to archive {}\n", fileName);
                              }
                          });

        mz_zip_writer_finalize_archive(&zip);
        mz_zip_writer_end(&zip);
    }

    fclose(file);

    LOG("===========================\n");

    return true;
}

} // namespace Core

HK_NAMESPACE_END
