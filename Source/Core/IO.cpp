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

AFileStream::~AFileStream()
{
    Close();
}

bool AFileStream::OpenRead(AStringView FileName)
{
    return Open(FileName, FILE_OPEN_MODE_READ);
}

bool AFileStream::OpenWrite(AStringView FileName)
{
    return Open(FileName, FILE_OPEN_MODE_WRITE);
}

bool AFileStream::OpenAppend(AStringView FileName)
{
    return Open(FileName, FILE_OPEN_MODE_APPEND);
}

static FILE* OpenFile(const char* FileName, const char* Mode)
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

    wchar_t* wFilename = (wchar_t*)StackAlloc(n * sizeof(wchar_t));

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

bool AFileStream::Open(AStringView FileName, FILE_OPEN_MODE _Mode)
{
    Close();

    Name = PathUtils::FixPath(FileName);
    if (Name.Length() && Name[Name.Length() - 1] == '/')
    {
        LOG("Invalid file name {}\n", FileName);
        Name.Clear();
        return false;
    }

    if (_Mode == FILE_OPEN_MODE_WRITE || _Mode == FILE_OPEN_MODE_APPEND)
    {
        Core::CreateDirectory(Name, true);
    }

    const char* fopen_mode = "";

    switch (_Mode)
    {
        case FILE_OPEN_MODE_READ:
            fopen_mode = "rb";
            break;
        case FILE_OPEN_MODE_WRITE:
            fopen_mode = "wb";
            break;
        case FILE_OPEN_MODE_APPEND:
            fopen_mode = "ab";
            break;
        default:
            HK_ASSERT(0);
    }

    FILE* f = OpenFile(Name.CStr(), fopen_mode);
    if (!f)
    {
        LOG("Couldn't open {}\n", Name);
        Name.Clear();
        return false;
    }

    Handle = f;
    Mode   = _Mode;

    if (Mode == FILE_OPEN_MODE_READ)
    {
        FileSize = ~0ull;
    }

    return true;
}

void AFileStream::Close()
{
    if (Mode == FILE_OPEN_MODE_CLOSED)
    {
        return;
    }

    Name.Clear();
    Mode = FILE_OPEN_MODE_CLOSED;

    fclose(Handle);
    Handle = nullptr;

    RWOffset = 0;
    FileSize = 0;
}

AString const& AFileStream::GetFileName() const
{
    return Name;
}

size_t AFileStream::Read(void* pBuffer, size_t SizeInBytes)
{
    size_t bytesToRead{0};

    if (Mode != FILE_OPEN_MODE_READ)
    {
        LOG("Failed to read from {} (wrong mode)\n", Name);
    }
    else
    {
        bytesToRead = fread(pBuffer, 1, SizeInBytes, Handle);
    }

    SizeInBytes -= bytesToRead;
    if (SizeInBytes)
        Platform::ZeroMem((uint8_t*)pBuffer + bytesToRead, SizeInBytes);

    RWOffset += bytesToRead;
    return bytesToRead;
}

size_t AFileStream::Write(const void* pBuffer, size_t SizeInBytes)
{
    if (Mode != FILE_OPEN_MODE_WRITE && Mode != FILE_OPEN_MODE_APPEND)
    {
        LOG("Failed to write {} (wrong mode)\n", Name);
        return 0;
    }

    size_t bytesToWrite = fwrite(pBuffer, 1, SizeInBytes, Handle);
    RWOffset += bytesToWrite;
    FileSize = std::max(FileSize, RWOffset);
    return bytesToWrite;
}

char* AFileStream::Gets(char* pBuffer, size_t SizeInBytes)
{
    if (Mode != FILE_OPEN_MODE_READ)
    {
        LOG("Failed to read from {} (wrong mode)\n", Name);
        return nullptr;
    }

    pBuffer = fgets(pBuffer, SizeInBytes, Handle);

    RWOffset = ftell(Handle);

    return pBuffer;
}

void AFileStream::Flush()
{
    fflush(Handle);
}

size_t AFileStream::GetOffset() const
{
    return RWOffset;
}

#define HasFileSize() (FileSize != ~0ull)

bool AFileStream::SeekSet(int32_t Offset)
{
    RWOffset = std::max(int32_t{0}, Offset);

    if (HasFileSize())
    {
        if (RWOffset > FileSize)
            RWOffset = FileSize;
    }

    bool r = fseek(Handle, RWOffset, SEEK_SET) == 0;
    if (!r)
        RWOffset = ftell(Handle);

    return r;
}

bool AFileStream::SeekCur(int32_t Offset)
{
    bool r;

    if (Offset < 0 && -Offset > RWOffset)
    {
        // Rewind
        RWOffset = 0;

        r = fseek(Handle, 0, SEEK_SET) == 0;
    }
    else
    {
        if (HasFileSize())
        {
            if (RWOffset + Offset > FileSize)
            {
                Offset = FileSize - RWOffset;
            }
        }

        RWOffset += Offset;

        r = fseek(Handle, Offset, SEEK_CUR) == 0;
    }

    if (!r)
    {
        RWOffset = ftell(Handle);
    }

    return r;
}

bool AFileStream::SeekEnd(int32_t Offset)
{
    bool r;

    if (Offset >= 0)
    {
        Offset = 0;

        r = fseek(Handle, 0, SEEK_END) == 0;

        if (r && HasFileSize())
        {
            RWOffset = FileSize;
        }
        else
        {
            RWOffset = ftell(Handle);
            if (r)
                FileSize = RWOffset;
        }

        return r;
    }

    if (HasFileSize() && -Offset >= FileSize)
    {
        // Rewind
        RWOffset = 0;
        r        = fseek(Handle, 0, SEEK_SET) == 0;

        if (!r)
            RWOffset = ftell(Handle);

        return r;
    }

    r = fseek(Handle, Offset, SEEK_END) == 0;
    RWOffset = ftell(Handle);

    return r;
}

size_t AFileStream::SizeInBytes() const
{
    if (!HasFileSize())
    {
        fseek(Handle, 0, SEEK_END);
        FileSize = ftell(Handle);
        fseek(Handle, RWOffset, SEEK_SET);
    }
    return FileSize;
}

bool AFileStream::Eof() const
{
    return feof(Handle) != 0;
}

AMemoryStream::~AMemoryStream()
{
    Close();
}

void* AMemoryStream::Alloc(size_t SizeInBytes)
{
    return Platform::GetHeapAllocator<HEAP_MISC>().Alloc(SizeInBytes, 16);
}

void* AMemoryStream::Realloc(void* pMemory, size_t SizeInBytes)
{
    return Platform::GetHeapAllocator<HEAP_MISC>().Realloc(pMemory, SizeInBytes, 16);
}

void AMemoryStream::Free(void* pMemory)
{
    Platform::GetHeapAllocator<HEAP_MISC>().Free(pMemory);
}

bool AMemoryStream::OpenRead(AStringView FileName, const void* pMemoryBuffer, size_t SizeInBytes)
{
    Close();

    Name               = FileName;
    Mode               = FILE_OPEN_MODE_READ;
    pHeapPtr           = reinterpret_cast<byte*>(const_cast<void*>(pMemoryBuffer));
    HeapSize           = SizeInBytes;
    ReservedSize       = SizeInBytes;
    bMemoryBufferOwner = false;
    return true;
}

bool AMemoryStream::OpenRead(AStringView FileName, AArchive const& Archive)
{
    Close();

    if (!Archive.ExtractFileToHeapMemory(FileName, (void**)&pHeapPtr, &HeapSize, Platform::GetHeapAllocator<HEAP_MISC>()))
    {
        LOG("Couldn't open {}\n", FileName);
        return false;
    }

    Name               = FileName;
    Mode               = FILE_OPEN_MODE_READ;
    ReservedSize       = HeapSize;
    bMemoryBufferOwner = true;
    
    return true;
}

bool AMemoryStream::OpenRead(int FileIndex, AArchive const& Archive)
{
    Close();

    Archive.GetFileName(FileIndex, Name);

    if (!Archive.ExtractFileToHeapMemory(FileIndex, (void**)&pHeapPtr, &HeapSize, Platform::GetHeapAllocator<HEAP_MISC>()))
    {
        LOG("Couldn't open {}\n", Name);
        Name.Clear();
        return false;
    }

    Mode               = FILE_OPEN_MODE_READ;
    ReservedSize       = HeapSize;
    bMemoryBufferOwner = true;

    return true;
}

bool AMemoryStream::OpenWrite(AStringView FileName, void* pMemoryBuffer, size_t SizeInBytes)
{
    Close();

    Name               = FileName;
    Mode               = FILE_OPEN_MODE_WRITE;
    pHeapPtr           = reinterpret_cast<byte*>(pMemoryBuffer);
    HeapSize           = 0;
    ReservedSize       = SizeInBytes;
    bMemoryBufferOwner = false;
    
    return true;
}

bool AMemoryStream::OpenWrite(AStringView FileName, size_t _ReservedSize)
{
    Close();

    Name               = FileName;
    Mode               = FILE_OPEN_MODE_WRITE;
    pHeapPtr           = (byte*)Alloc(_ReservedSize);
    HeapSize           = 0;
    ReservedSize       = _ReservedSize;
    bMemoryBufferOwner = true;
    
    return true;
}

void AMemoryStream::Close()
{
    if (Mode == FILE_OPEN_MODE_CLOSED)
    {
        return;
    }

    Name.Clear();
    Mode = FILE_OPEN_MODE_CLOSED;

    if (bMemoryBufferOwner)
    {
        Free(pHeapPtr);
    }

    pHeapPtr           = nullptr;
    HeapSize           = 0;
    ReservedSize       = 0;
    RWOffset           = 0;
    bMemoryBufferOwner = true;
}

AString const& AMemoryStream::GetFileName() const
{
    return Name;
}

size_t AMemoryStream::Read(void* pBuffer, size_t SizeInBytes)
{
    if (Mode != FILE_OPEN_MODE_READ)
    {
        LOG("Failed to read from {} (wrong mode)\n", Name);
        return 0;
    }

    size_t bytesToRead = std::min(SizeInBytes, HeapSize - RWOffset);

    Platform::Memcpy(pBuffer, pHeapPtr + RWOffset, bytesToRead);
    RWOffset += bytesToRead;

    SizeInBytes -= bytesToRead;
    if (SizeInBytes)
        Platform::ZeroMem((uint8_t*)pBuffer + bytesToRead, SizeInBytes);

    return bytesToRead;
}

size_t AMemoryStream::Write(const void* pBuffer, size_t SizeInBytes)
{
    if (Mode != FILE_OPEN_MODE_WRITE)
    {
        LOG("Failed to write {} (wrong mode)\n", Name);
        return 0;
    }

    size_t requiredSize = RWOffset + SizeInBytes;
    if (requiredSize > ReservedSize)
    {
        if (!bMemoryBufferOwner)
        {
            LOG("Failed to write {} (buffer overflowed)\n", Name);
            return 0;
        }
        const int mod = requiredSize % Granularity;
        ReservedSize  = mod ? requiredSize + Granularity - mod : requiredSize;
        pHeapPtr      = (byte*)Realloc(pHeapPtr, ReservedSize);
    }
    Platform::Memcpy(pHeapPtr + RWOffset, pBuffer, SizeInBytes);
    RWOffset += SizeInBytes;
    HeapSize = std::max(HeapSize, RWOffset);
    return SizeInBytes;
}

char* AMemoryStream::Gets(char* pBuffer, size_t SizeInBytes)
{
    if (Mode != FILE_OPEN_MODE_READ)
    {
        LOG("Failed to read from {} (wrong mode)\n", Name);
        return nullptr;
    }

    if (SizeInBytes == 0 || RWOffset >= HeapSize)
    {
        return nullptr;
    }

    size_t maxChars = SizeInBytes - 1;
    if (RWOffset + maxChars > HeapSize)
    {
        maxChars = HeapSize - RWOffset;
    }

    char *memoryPointer, *memory = (char*)&pHeapPtr[RWOffset];
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
    RWOffset += memoryPointer - memory;

    return pBuffer;
}

void AMemoryStream::Flush()
{
    // nop
}

size_t AMemoryStream::GetOffset() const
{
    return RWOffset;
}

bool AMemoryStream::SeekSet(int32_t Offset)
{
    Offset = std::max(int32_t{0}, Offset);
    if (Offset > HeapSize)
        Offset = HeapSize;
    RWOffset = Offset;
    return true;
}

bool AMemoryStream::SeekCur(int32_t Offset)
{
    if (Offset < 0 && -Offset > RWOffset)
    {
        RWOffset = 0;
    }
    else
    {
        size_t prevOffset = RWOffset;
        RWOffset += Offset;
        if (RWOffset > HeapSize || (Offset > 0 && prevOffset > RWOffset))
            RWOffset = HeapSize;
    }
    return true;
}

bool AMemoryStream::SeekEnd(int32_t Offset)
{
    if (Offset >= 0)
        RWOffset = HeapSize;
    else if (-Offset > HeapSize)
        RWOffset = 0;
    else
        RWOffset = HeapSize + Offset;
    return true;
}

size_t AMemoryStream::SizeInBytes() const
{
    return HeapSize;
}

size_t AMemoryStream::GetReservedSize() const
{
    return ReservedSize;
}

bool AMemoryStream::Eof() const
{
    return RWOffset >= HeapSize;
}

void* AMemoryStream::GetHeapPtr()
{
    return pHeapPtr;
}


AArchive::AArchive()
{}

AArchive::AArchive(AStringView ArchiveName, bool bResourcePack) :
    AArchive()
{
    Open(ArchiveName, bResourcePack);
}

AArchive::AArchive(const void* pMemory, size_t SizeInBytes) :
    AArchive()
{
    OpenFromMemory(pMemory, SizeInBytes);
}

AArchive::~AArchive()
{
    Close();
}

bool AArchive::Open(AStringView ArchiveName, bool bResourcePack)
{
    mz_zip_archive arch;
    mz_uint64      fileStartOffset = 0;
    mz_uint64      archiveSize     = 0;

    Close();

    if (bResourcePack)
    {
        AFileStream f;
        if (!f.OpenRead(ArchiveName))
        {
            return false;
        }

        uint64_t    magic = f.ReadUInt64();
        const char* MAGIC = "ARESPACK";
        if (std::memcmp(&magic, MAGIC, sizeof(uint64_t)) != 0)
        {
            LOG("Invalid file format {}\n", ArchiveName);
            return false;
        }

        fileStartOffset += sizeof(magic);
        archiveSize = f.SizeInBytes() - fileStartOffset;
    }

    Platform::ZeroMem(&arch, sizeof(arch));

    mz_bool status = mz_zip_reader_init_file_v2(&arch, ArchiveName.IsNullTerminated() ? ArchiveName.Begin() : AString(ArchiveName).CStr(), 0, fileStartOffset, archiveSize);
    if (!status)
    {
        LOG("Couldn't open archive {}\n", ArchiveName);
        return false;
    }

    Handle = Platform::GetHeapAllocator<HEAP_MISC>().Alloc(sizeof(mz_zip_archive));
    Platform::Memcpy(Handle, &arch, sizeof(arch));

    // Keep pointer valid
    ((mz_zip_archive*)Handle)->m_pIO_opaque = Handle;

    return true;
}

bool AArchive::OpenFromMemory(const void* pMemory, size_t SizeInBytes)
{
    Close();

    mz_zip_archive arch;

    Platform::ZeroMem(&arch, sizeof(arch));

    mz_bool status = mz_zip_reader_init_mem(&arch, pMemory, SizeInBytes, 0);
    if (!status)
    {
        LOG("Couldn't open archive from memory\n");
        return false;
    }

    Handle = Platform::GetHeapAllocator<HEAP_MISC>().Alloc(sizeof(mz_zip_archive));
    Platform::Memcpy(Handle, &arch, sizeof(arch));

    // Keep pointer valid
    ((mz_zip_archive*)Handle)->m_pIO_opaque = Handle;

    return true;
}

void AArchive::Close()
{
    if (!Handle)
    {
        return;
    }

    mz_zip_reader_end((mz_zip_archive*)Handle);

    Platform::GetHeapAllocator<HEAP_MISC>().Free(Handle);
    Handle = nullptr;
}

int AArchive::GetNumFiles() const
{
    return mz_zip_reader_get_num_files((mz_zip_archive*)Handle);
}

int AArchive::LocateFile(AStringView FileName) const
{
    return mz_zip_reader_locate_file((mz_zip_archive*)Handle, FileName.IsNullTerminated() ? FileName.Begin() : AString(FileName).CStr(), NULL, 0);
}

#define MZ_ZIP_CDH_COMPRESSED_SIZE_OFS   20
#define MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS 24
#define MZ_ZIP_CDH_FILENAME_LEN_OFS      28
#define MZ_ZIP_CENTRAL_DIR_HEADER_SIZE   46

bool AArchive::GetFileSize(int FileIndex, size_t* pCompressedSize, size_t* pUncompressedSize) const
{
    // All checks are processd in mz_zip_get_cdh
    const mz_uint8* p = mz_zip_get_cdh_public((mz_zip_archive*)Handle, FileIndex);
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

bool AArchive::GetFileName(int FileIndex, AString& FileName) const
{
    // All checks are processd in mz_zip_get_cdh
    const mz_uint8* p = mz_zip_get_cdh_public((mz_zip_archive*)Handle, FileIndex);
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

bool AArchive::ExtractFileToMemory(int FileIndex, void* pMemoryBuffer, size_t SizeInBytes) const
{
    // All checks are processd in mz_zip_reader_extract_to_mem
    return !!mz_zip_reader_extract_to_mem((mz_zip_archive*)Handle, FileIndex, pMemoryBuffer, SizeInBytes, 0);
}

bool AArchive::ExtractFileToHeapMemory(AStringView FileName, void** pHeapMemoryPtr, size_t* pSizeInBytes, MemoryHeap& Heap) const
{
    size_t uncompSize;

    *pHeapMemoryPtr = nullptr;
    *pSizeInBytes   = 0;

    int fileIndex = LocateFile(FileName);
    if (fileIndex < 0)
    {
        return false;
    }

    if (!GetFileSize(fileIndex, NULL, &uncompSize))
    {
        return false;
    }

    void* pBuf = Heap.Alloc(uncompSize);

    if (!ExtractFileToMemory(fileIndex, pBuf, uncompSize))
    {
        Heap.Free(pBuf);
        return false;
    }

    *pHeapMemoryPtr = pBuf;
    *pSizeInBytes   = uncompSize;

    return true;
}

bool AArchive::ExtractFileToHeapMemory(int FileIndex, void** pHeapMemoryPtr, size_t* pSizeInBytes, MemoryHeap& Heap) const
{
    size_t uncompSize;

    *pHeapMemoryPtr = nullptr;
    *pSizeInBytes   = 0;

    if (FileIndex < 0)
    {
        return false;
    }

    if (!GetFileSize(FileIndex, NULL, &uncompSize))
    {
        return false;
    }

    void* pBuf = Heap.Alloc(uncompSize);

    if (!ExtractFileToMemory(FileIndex, pBuf, uncompSize))
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

void CreateDirectory(AStringView Directory, bool bFileName)
{
    size_t len = Directory.Length();
    if (!len)
    {
        return;
    }
    const char* dir = Directory.ToPtr();
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

bool IsFileExists(AStringView FileName)
{
    return access(PathUtils::FixSeparator(FileName).CStr(), 0) == 0;
}

void RemoveFile(AStringView FileName)
{
    AString s = PathUtils::FixPath(FileName);
#if defined HK_OS_LINUX
    ::remove(s.CStr());
#elif defined HK_OS_WIN32
    int n = MultiByteToWideChar(CP_UTF8, 0, s.CStr(), -1, NULL, 0);
    if (0 != n)
    {
        wchar_t* wFilename = (wchar_t*)StackAlloc(n * sizeof(wchar_t));

        MultiByteToWideChar(CP_UTF8, 0, s.CStr(), -1, wFilename, n);

        ::DeleteFile(wFilename);
    }
#else
    static_assert(0, "TODO: Implement RemoveFile for current build settings");
#endif
}

#ifdef HK_OS_LINUX
void TraverseDirectory(AStringView Path, bool bSubDirs, STraverseDirectoryCB Callback)
{
    AString fn;
    DIR*    dir = opendir(Path.IsNullTerminated() ? Path.Begin() : AString(Path).CStr());
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
void TraverseDirectory(AStringView Path, bool bSubDirs, STraverseDirectoryCB Callback)
{
    AString fn;

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

bool WriteResourcePack(AStringView SourcePath, AStringView ResultFile)
{
    AString path = PathUtils::FixSeparator(SourcePath);
    AString result = PathUtils::FixSeparator(ResultFile);

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
                          [&zip, &path](AStringView FileName, bool bIsDirectory)
                          {
                              if (bIsDirectory)
                              {
                                  return;
                              }

                              if (PathUtils::CompareExt(FileName, ".resources", true))
                              {
                                  return;
                              }

                              AString fn = FileName.TruncateHead(path.Length() + 1);

                              LOG("Writing '{}'\n", fn);

                              mz_bool status = mz_zip_writer_add_file(&zip, fn.CStr(), FileName.IsNullTerminated() ? FileName.Begin() : AString(FileName).CStr(), nullptr, 0, MZ_UBER_COMPRESSION);
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
