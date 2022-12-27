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

#include <Core/IO.h>
#include <Core/BaseMath.h>
#include <Platform/WindowsDefs.h>
#include <Platform/Logger.h>

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
FILE* OpenFile(const char* FileName, const char* Mode)
{
    FILE* f;
#if defined(_MSC_VER)
    wchar_t wMode[64];

    if (0 == MultiByteToWideChar(CP_UTF8, 0, Mode, -1, wMode, HK_ARRAY_SIZE(wMode)))
        return NULL;

    int n = MultiByteToWideChar(CP_UTF8, 0, FileName, -1, NULL, 0);
    if (0 == n)
    {
        return NULL;
    }

    wchar_t* wFilename = (wchar_t*)HkStackAlloc(n * sizeof(wchar_t));

    MultiByteToWideChar(CP_UTF8, 0, FileName, -1, wFilename, n);

#    if _MSC_VER >= 1400
    if (0 != _wfopen_s(&f, wFilename, wMode))
        f = NULL;
#    else
    f = _wfopen(wFilename, wMode);
#    endif
#else
    f = fopen(FileName, Mode);
#endif
    return f;
}
}

HK_NAMESPACE_BEGIN

File::~File()
{
    Close();
}

File File::OpenRead(StringView FileName)
{
    return OpenFromFileSystem(FileName, FILE_TYPE_READ_FILE_SYSTEM);
}

File File::OpenWrite(StringView FileName)
{
    return OpenFromFileSystem(FileName, FILE_TYPE_WRITE_FILE_SYSTEM);
}

File File::OpenAppend(StringView FileName)
{
    return OpenFromFileSystem(FileName, FILE_TYPE_APPEND_FILE_SYSTEM);
}

File File::OpenFromFileSystem(StringView FileName, FILE_TYPE Type)
{
    File f;

    f.m_Name = PathUtils::FixPath(FileName);
    if (f.m_Name.Length() && f.m_Name[f.m_Name.Length() - 1] == '/')
    {
        LOG("Invalid file name {}\n", FileName);
        return {};
    }

    if (Type == FILE_TYPE_WRITE_FILE_SYSTEM || Type == FILE_TYPE_APPEND_FILE_SYSTEM)
    {
        Core::CreateDirectory(f.m_Name, true);
    }

    const char* fopen_mode = "";

    switch (Type)
    {
        case FILE_TYPE_READ_FILE_SYSTEM:
            fopen_mode = "rb";
            break;
        case FILE_TYPE_WRITE_FILE_SYSTEM:
            fopen_mode = "wb";
            break;
        case FILE_TYPE_APPEND_FILE_SYSTEM:
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

    f.m_Type = Type;

    if (f.m_Type == FILE_TYPE_READ_FILE_SYSTEM)
    {
        f.m_FileSize = ~0ull;
    }

    return f;
}

void File::Close()
{
    if (m_Type == FILE_TYPE_UNDEFINED)
    {
        return;
    }

    if (IsFileSystem())
    {
        fclose(m_Handle);
    }
    else
    {
        if (m_bMemoryBufferOwner)
        {
            Free(m_pHeapPtr);
        }
    }

    m_Name.Clear();
    m_Type               = FILE_TYPE_UNDEFINED;
    m_RWOffset           = 0;
    m_FileSize           = 0;
    m_Handle             = nullptr;
    m_ReservedSize       = 0;
    m_bMemoryBufferOwner = true;
}

size_t File::Read(void* pBuffer, size_t SizeInBytes)
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
            bytesToRead = fread(pBuffer, 1, SizeInBytes, m_Handle);
        }
        else
        {
            bytesToRead = std::min(SizeInBytes, m_FileSize - m_RWOffset);
            Platform::Memcpy(pBuffer, m_pHeapPtr + m_RWOffset, bytesToRead);
        }
    }

    SizeInBytes -= bytesToRead;
    if (SizeInBytes)
        Platform::ZeroMem((uint8_t*)pBuffer + bytesToRead, SizeInBytes);

    m_RWOffset += bytesToRead;
    return bytesToRead;
}

size_t File::Write(const void* pBuffer, size_t SizeInBytes)
{
    size_t bytesToWrite{};

    if (!IsWritable())
    {
        LOG("Writing to {} is not allowed. The file must be opened in write mode.\n", m_Name);
        return 0;
    }

    if (IsFileSystem())
    {
        bytesToWrite = fwrite(pBuffer, 1, SizeInBytes, m_Handle);
    }
    else
    {
        size_t requiredSize = m_RWOffset + SizeInBytes;
        if (requiredSize > m_ReservedSize)
        {
            if (!m_bMemoryBufferOwner)
            {
                LOG("Failed to write {} (buffer overflowed)\n", m_Name);
                return 0;
            }
            const int mod  = requiredSize % m_Granularity;
            m_ReservedSize = mod ? requiredSize + m_Granularity - mod : requiredSize;
            m_pHeapPtr     = (byte*)Realloc(m_pHeapPtr, m_ReservedSize);
        }
        Platform::Memcpy(m_pHeapPtr + m_RWOffset, pBuffer, SizeInBytes);
        bytesToWrite = SizeInBytes;
    }

    m_RWOffset += bytesToWrite;
    m_FileSize = std::max(m_FileSize, m_RWOffset);
    return bytesToWrite;
}

char* File::Gets(char* pBuffer, size_t SizeInBytes)
{
    if (!IsReadable())
    {
        LOG("Reading from {} is not allowed. The file must be opened in read mode.\n", m_Name);
        return nullptr;
    }

    if (IsFileSystem())
    {
        pBuffer = fgets(pBuffer, SizeInBytes, m_Handle);

        m_RWOffset = ftell(m_Handle);
    }
    else
    {
        if (SizeInBytes == 0 || m_RWOffset >= m_FileSize)
        {
            return nullptr;
        }

        size_t maxChars = SizeInBytes - 1;
        if (m_RWOffset + maxChars > m_FileSize)
        {
            maxChars = m_FileSize - m_RWOffset;
        }

        char *memoryPointer, *memory = (char*)&m_pHeapPtr[m_RWOffset];
        char* stringPointer = pBuffer;

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

    return pBuffer;
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

bool File::SeekSet(int32_t Offset)
{
    if (!IsOpened())
        return false;

    m_RWOffset = std::max(int32_t{0}, Offset);

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

bool File::SeekCur(int32_t Offset)
{
    if (!IsOpened())
        return false;

    if (IsFileSystem())
    {
        bool r;

        if (Offset < 0 && -Offset > m_RWOffset)
        {
            // Rewind
            m_RWOffset = 0;

            r = fseek(m_Handle, 0, SEEK_SET) == 0;
        }
        else
        {
            if (HasFileSize())
            {
                if (m_RWOffset + Offset > m_FileSize)
                {
                    Offset = m_FileSize - m_RWOffset;
                }
            }

            m_RWOffset += Offset;

            r = fseek(m_Handle, Offset, SEEK_CUR) == 0;
        }

        if (!r)
        {
            m_RWOffset = ftell(m_Handle);
        }

        return r;
    }
    else
    {
        if (Offset < 0 && -Offset > m_RWOffset)
        {
            m_RWOffset = 0;
        }
        else
        {
            size_t prevOffset = m_RWOffset;
            m_RWOffset += Offset;
            if (m_RWOffset > m_FileSize || (Offset > 0 && prevOffset > m_RWOffset))
                m_RWOffset = m_FileSize;
        }
        return true;
    }
}

bool File::SeekEnd(int32_t Offset)
{
    if (!IsOpened())
        return false;

    if (IsFileSystem())
    {
        bool r;

        if (Offset >= 0)
        {
            Offset = 0;

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

        if (HasFileSize() && -Offset >= m_FileSize)
        {
            // Rewind
            m_RWOffset = 0;
            r          = fseek(m_Handle, 0, SEEK_SET) == 0;

            if (!r)
                m_RWOffset = ftell(m_Handle);

            return r;
        }

        r          = fseek(m_Handle, Offset, SEEK_END) == 0;
        m_RWOffset = ftell(m_Handle);

        return r;
    }
    else
    {
        if (Offset >= 0)
            m_RWOffset = m_FileSize;
        else if (-Offset > m_FileSize)
            m_RWOffset = 0;
        else
            m_RWOffset = m_FileSize + Offset;
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

bool File::Eof() const
{
    if (!IsOpened())
        return false;

    if (IsFileSystem())
        return feof(m_Handle) != 0;
    else
        return m_RWOffset >= m_FileSize;
}

void* File::Alloc(size_t SizeInBytes)
{
    return Platform::GetHeapAllocator<HEAP_MISC>().Alloc(SizeInBytes, 16);
}

void* File::Realloc(void* pMemory, size_t SizeInBytes)
{
    return Platform::GetHeapAllocator<HEAP_MISC>().Realloc(pMemory, SizeInBytes, 16);
}

void File::Free(void* pMemory)
{
    Platform::GetHeapAllocator<HEAP_MISC>().Free(pMemory);
}

File File::OpenRead(StringView FileName, const void* pMemoryBuffer, size_t SizeInBytes)
{
    File f;

    f.m_Name               = FileName;
    f.m_Type               = FILE_TYPE_READ_MEMORY;
    f.m_pHeapPtr           = reinterpret_cast<byte*>(const_cast<void*>(pMemoryBuffer));
    f.m_FileSize           = SizeInBytes;
    f.m_ReservedSize       = SizeInBytes;
    f.m_bMemoryBufferOwner = false;

    return f;
}

File File::OpenRead(StringView FileName, BlobRef Blob)
{
    File f;

    f.m_Name = FileName;
    f.m_Type = FILE_TYPE_READ_MEMORY;
    f.m_pHeapPtr = (byte*)Alloc(Blob.Size());
    f.m_FileSize = Blob.Size();
    f.m_ReservedSize = Blob.Size();
    f.m_bMemoryBufferOwner = true;

    Platform::Memcpy(f.m_pHeapPtr, Blob.GetData(), Blob.Size());

    return f;
}

File File::OpenRead(StringView FileName, Archive const& Archive)
{
    File f;

    if (!Archive.ExtractFileToHeapMemory(FileName, (void**)&f.m_pHeapPtr, &f.m_FileSize, Platform::GetHeapAllocator<HEAP_MISC>()))
    {
        LOG("Couldn't open {}\n", FileName);
        return {};
    }

    f.m_Name               = FileName;
    f.m_Type               = FILE_TYPE_READ_MEMORY;
    f.m_ReservedSize       = f.m_FileSize;
    f.m_bMemoryBufferOwner = true;

    return f;
}

File File::OpenRead(FileHandle FileHandle, Archive const& Archive)
{
    File f;

    Archive.GetFileName(FileHandle, f.m_Name);

    if (!Archive.ExtractFileToHeapMemory(FileHandle, (void**)&f.m_pHeapPtr, &f.m_FileSize, Platform::GetHeapAllocator<HEAP_MISC>()))
    {
        LOG("Couldn't open {}\n", f.m_Name);
        return {};
    }

    f.m_Type               = FILE_TYPE_READ_MEMORY;
    f.m_ReservedSize       = f.m_FileSize;
    f.m_bMemoryBufferOwner = true;

    return f;
}

File File::OpenWrite(StringView StreamName, void* pMemoryBuffer, size_t SizeInBytes)
{
    File f;

    f.m_Name               = StreamName;
    f.m_Type               = FILE_TYPE_WRITE_MEMORY;
    f.m_pHeapPtr           = reinterpret_cast<byte*>(pMemoryBuffer);
    f.m_FileSize           = 0;
    f.m_ReservedSize       = SizeInBytes;
    f.m_bMemoryBufferOwner = false;

    return f;
}

File File::OpenWriteToMemory(StringView StreamName, size_t ReservedSize)
{
    File f;

    f.m_Name               = StreamName;
    f.m_Type               = FILE_TYPE_WRITE_MEMORY;
    f.m_pHeapPtr           = (byte*)Alloc(ReservedSize);
    f.m_FileSize           = 0;
    f.m_ReservedSize       = ReservedSize;
    f.m_bMemoryBufferOwner = true;

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

Archive::~Archive()
{
    Close();
}

Archive Archive::Open(StringView ArchiveName, bool bResourcePack)
{
    mz_zip_archive arch;
    mz_uint64      fileStartOffset = 0;
    mz_uint64      archiveSize     = 0;

    if (bResourcePack)
    {
        File f = File::OpenRead(ArchiveName);
        if (!f)
        {
            return {};
        }

        uint64_t    magic = f.ReadUInt64();
        const char* MAGIC = "ARESPACK";
        if (std::memcmp(&magic, MAGIC, sizeof(uint64_t)) != 0)
        {
            LOG("Invalid file format {}\n", ArchiveName);
            return {};
        }

        fileStartOffset += sizeof(magic);
        archiveSize = f.SizeInBytes() - fileStartOffset;
    }

    Platform::ZeroMem(&arch, sizeof(arch));

    mz_bool status = mz_zip_reader_init_file_v2(&arch, ArchiveName.IsNullTerminated() ? ArchiveName.Begin() : String(ArchiveName).CStr(), 0, fileStartOffset, archiveSize);
    if (!status)
    {
        LOG("Couldn't open archive {}\n", ArchiveName);
        return {};
    }

    Archive archive;
    archive.m_Handle = Platform::GetHeapAllocator<HEAP_MISC>().Alloc(sizeof(mz_zip_archive));
    Platform::Memcpy(archive.m_Handle, &arch, sizeof(arch));

    // Keep pointer valid
    ((mz_zip_archive*)archive.m_Handle)->m_pIO_opaque = archive.m_Handle;

    return archive;
}

Archive Archive::OpenFromMemory(const void* pMemory, size_t SizeInBytes)
{
    mz_zip_archive arch;

    Platform::ZeroMem(&arch, sizeof(arch));

    mz_bool status = mz_zip_reader_init_mem(&arch, pMemory, SizeInBytes, 0);
    if (!status)
    {
        LOG("Couldn't open archive from memory\n");
        return {};
    }

    Archive archive;
    archive.m_Handle = Platform::GetHeapAllocator<HEAP_MISC>().Alloc(sizeof(mz_zip_archive));
    Platform::Memcpy(archive.m_Handle, &arch, sizeof(arch));

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

    Platform::GetHeapAllocator<HEAP_MISC>().Free(m_Handle);
    m_Handle = nullptr;
}

int Archive::GetNumFiles() const
{
    return mz_zip_reader_get_num_files((mz_zip_archive*)m_Handle);
}

FileHandle Archive::LocateFile(StringView FileName) const
{
    return FileHandle(mz_zip_reader_locate_file((mz_zip_archive*)m_Handle, FileName.IsNullTerminated() ? FileName.Begin() : String(FileName).CStr(), NULL, 0));
}

#define MZ_ZIP_CDH_COMPRESSED_SIZE_OFS   20
#define MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS 24
#define MZ_ZIP_CDH_FILENAME_LEN_OFS      28
#define MZ_ZIP_CENTRAL_DIR_HEADER_SIZE   46

bool Archive::GetFileSize(FileHandle FileHandle, size_t* pCompressedSize, size_t* pUncompressedSize) const
{
    // All checks are processd in mz_zip_get_cdh
    const mz_uint8* p = mz_zip_get_cdh_public((mz_zip_archive*)m_Handle, int(FileHandle));
    if (!p)
    {
        return false;
    }

    if (pCompressedSize)
    {
        *pCompressedSize = MZ_READ_LE32(p + MZ_ZIP_CDH_COMPRESSED_SIZE_OFS);
    }

    if (pUncompressedSize)
    {
        *pUncompressedSize = MZ_READ_LE32(p + MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS);
    }
    return true;
}

bool Archive::GetFileName(FileHandle FileHandle, String& FileName) const
{
    // All checks are processd in mz_zip_get_cdh
    const mz_uint8* p = mz_zip_get_cdh_public((mz_zip_archive*)m_Handle, int(FileHandle));
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

    FileName.Resize(filename_len);
    Platform::Memcpy(FileName.ToPtr(), pFilename, filename_len);

    return true;
}

bool Archive::ExtractFileToMemory(FileHandle FileHandle, void* pMemoryBuffer, size_t SizeInBytes) const
{
    // All checks are processd in mz_zip_reader_extract_to_mem
    return !!mz_zip_reader_extract_to_mem((mz_zip_archive*)m_Handle, int(FileHandle), pMemoryBuffer, SizeInBytes, 0);
}

bool Archive::ExtractFileToHeapMemory(StringView FileName, void** pHeapMemoryPtr, size_t* pSizeInBytes, MemoryHeap& Heap) const
{
    size_t uncompSize;

    *pHeapMemoryPtr = nullptr;
    *pSizeInBytes   = 0;

    FileHandle fileHandle = LocateFile(FileName);
    if (!fileHandle)
    {
        return false;
    }

    if (!GetFileSize(fileHandle, NULL, &uncompSize))
    {
        return false;
    }

    void* pBuf = Heap.Alloc(uncompSize);

    if (!ExtractFileToMemory(fileHandle, pBuf, uncompSize))
    {
        Heap.Free(pBuf);
        return false;
    }

    *pHeapMemoryPtr = pBuf;
    *pSizeInBytes   = uncompSize;

    return true;
}

bool Archive::ExtractFileToHeapMemory(FileHandle FileHandle, void** pHeapMemoryPtr, size_t* pSizeInBytes, MemoryHeap& Heap) const
{
    size_t uncompSize;

    *pHeapMemoryPtr = nullptr;
    *pSizeInBytes   = 0;

    if (!FileHandle)
    {
        return false;
    }

    if (!GetFileSize(FileHandle, NULL, &uncompSize))
    {
        return false;
    }

    void* pBuf = Heap.Alloc(uncompSize);

    if (!ExtractFileToMemory(FileHandle, pBuf, uncompSize))
    {
        Heap.Free(pBuf);
        return false;
    }

    *pHeapMemoryPtr = pBuf;
    *pSizeInBytes   = uncompSize;

    return true;
}

namespace Core
{

void CreateDirectory(StringView Directory, bool bFileName)
{
    size_t len = Directory.Length();
    if (!len)
    {
        return;
    }
    const char* dir    = Directory.ToPtr();
    char*       tmpStr = (char*)Platform::GetHeapAllocator<HEAP_TEMP>().Alloc(len + 1);
    Platform::Memcpy(tmpStr, dir, len + 1);
    char* p = tmpStr;
#ifdef HK_OS_WIN32
    if (len >= 3 && dir[1] == ':')
    {
        p += 3;
        dir += 3;
    }
#endif
    for (; dir < Directory.End(); p++, dir++)
    {
        if (Platform::IsPathSeparator(*p))
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
    if (!bFileName)
    {
#ifdef HK_COMPILER_MSVC
        mkdir(tmpStr);
#else
        mkdir(tmpStr, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
    }
    Platform::GetHeapAllocator<HEAP_TEMP>().Free(tmpStr);
}

bool IsFileExists(StringView FileName)
{
    return access(PathUtils::FixSeparator(FileName).CStr(), 0) == 0;
}

void RemoveFile(StringView FileName)
{
    String s = PathUtils::FixPath(FileName);
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
void TraverseDirectory(StringView Path, bool bSubDirs, STraverseDirectoryCB Callback)
{
    String fn;
    DIR*    dir = opendir(Path.IsNullTerminated() ? Path.Begin() : String(Path).CStr());
    if (dir)
    {
        while (1)
        {
            dirent* entry = readdir(dir);
            if (!entry)
            {
                break;
            }

            fn      = Path;
            int len = Path.Length();
            if (len > 1 && Path[len - 1] != '/')
            {
                fn += "/";
            }
            fn += entry->d_name;

            struct stat statInfo;
            lstat(fn.CStr(), &statInfo);

            if (S_ISDIR(statInfo.st_mode))
            {
                if (bSubDirs)
                {
                    if (!strcmp(".", entry->d_name) || !strcmp("..", entry->d_name))
                    {
                        continue;
                    }
                    TraverseDirectory(fn, bSubDirs, Callback);
                }
                Callback(fn, true);
            }
            else
            {
                Callback(Path / entry->d_name, false);
            }
        }
        closedir(dir);
    }
}
#endif

#ifdef HK_OS_WIN32
void TraverseDirectory(StringView Path, bool bSubDirs, STraverseDirectoryCB Callback)
{
    String fn;

    HANDLE           fh;
    WIN32_FIND_DATAA fd;

    if ((fh = FindFirstFileA((Path + "/*.*").CStr(), &fd)) != INVALID_HANDLE_VALUE)
    {
        do {
            fn      = Path;
            int len = (int)Path.Length();
            if (len > 1 && Path[len - 1] != '/')
            {
                fn += "/";
            }
            fn += fd.cFileName;

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (bSubDirs)
                {
                    if (!strcmp(".", fd.cFileName) || !strcmp("..", fd.cFileName))
                    {
                        continue;
                    }
                    TraverseDirectory(fn, bSubDirs, Callback);
                }
                Callback(fn, true);
            }
            else
            {
                Callback(Path / fd.cFileName, false);
            }
        } while (FindNextFileA(fh, &fd));

        FindClose(fh);
    }
}
#endif

bool WriteResourcePack(StringView SourcePath, StringView ResultFile)
{
    String path   = PathUtils::FixSeparator(SourcePath);
    String result = PathUtils::FixSeparator(ResultFile);

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
    Platform::Memcpy(&magic, "ARESPACK", sizeof(magic));
    magic = Core::LittleDDWord(magic);
    fwrite(&magic, 1, sizeof(magic), file);

    mz_zip_archive zip;
    Platform::ZeroMem(&zip, sizeof(zip));

    if (mz_zip_writer_init_cfile(&zip, file, 0))
    {
        TraverseDirectory(path, true,
                          [&zip, &path](StringView FileName, bool bIsDirectory)
                          {
                              if (bIsDirectory)
                              {
                                  return;
                              }

                              if (PathUtils::CompareExt(FileName, ".resources", true))
                              {
                                  return;
                              }

                              String fn = FileName.TruncateHead(path.Length() + 1);

                              LOG("Writing '{}'\n", fn);

                              mz_bool status = mz_zip_writer_add_file(&zip, fn.CStr(), FileName.IsNullTerminated() ? FileName.Begin() : String(FileName).CStr(), nullptr, 0, MZ_UBER_COMPRESSION);
                              if (!status)
                              {
                                  LOG("Failed to archive {}\n", FileName);
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
