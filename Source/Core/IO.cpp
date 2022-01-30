/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <Core/IO.h>
#include <Core/BaseMath.h>
#include <Platform/WindowsDefs.h>
#include <Platform/Logger.h>

#ifdef AN_OS_WIN32
#    include <direct.h> // _mkdir
#    include <io.h>     // access
#endif
#ifdef AN_OS_LINUX
#    include <dirent.h>
#    include <sys/stat.h> // _mkdir
#    include <unistd.h>   // access
#endif

#include <miniz/miniz.h>

AFileStream::~AFileStream()
{
    Close();
}

bool AFileStream::OpenRead(AStringView _FileName)
{
    return Open(_FileName, FILE_OPEN_MODE_READ);
}

bool AFileStream::OpenWrite(AStringView _FileName)
{
    return Open(_FileName, FILE_OPEN_MODE_WRITE);
}

bool AFileStream::OpenAppend(AStringView _FileName)
{
    return Open(_FileName, FILE_OPEN_MODE_APPEND);
}

static FILE* OpenFile(const char* _FileName, const char* _Mode)
{
    FILE* f;
#if defined(_MSC_VER)
    wchar_t wMode[64];

    if (0 == MultiByteToWideChar(CP_UTF8, 0, _Mode, -1, wMode, AN_ARRAY_SIZE(wMode)))
        return NULL;

    int n = MultiByteToWideChar(CP_UTF8, 0, _FileName, -1, NULL, 0);
    if (0 == n)
    {
        return NULL;
    }

    wchar_t* wFilename = (wchar_t*)StackAlloc(n * sizeof(wchar_t));

    MultiByteToWideChar(CP_UTF8, 0, _FileName, -1, wFilename, n);

#    if _MSC_VER >= 1400
    if (0 != _wfopen_s(&f, wFilename, wMode))
        f = NULL;
#    else
    f = _wfopen(wFilename, wMode);
#    endif
#else
    f = fopen(_FileName, _Mode);
#endif
    return f;
}

bool AFileStream::Open(AStringView _FileName, int _Mode)
{
    AN_ASSERT(_Mode >= 0 && _Mode < 3);

    Close();

    Name = _FileName;
    Name.FixPath();
    if (Name.Length() && Name[Name.Length() - 1] == '/')
    {
        GLogger.Printf("Invalid file name %s\n", AString(_FileName).CStr());
        Name.Clear();
        return false;
    }

    if (_Mode == FILE_OPEN_MODE_WRITE || _Mode == FILE_OPEN_MODE_APPEND)
    {
        Core::MakeDir(Name.CStr(), true);
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
            AN_ASSERT(0);
    }

    FILE* f = OpenFile(Name.CStr(), fopen_mode);
    if (!f)
    {
        GLogger.Printf("Couldn't open %s\n", Name.CStr());
        Name.Clear();
        return false;
    }

    Handle = f;
    Mode   = _Mode;

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

    fclose((FILE*)Handle);
    Handle = nullptr;
}

const char* AFileStream::Impl_GetFileName() const
{
    return Name.CStr();
}

int AFileStream::Impl_Read(void* _Buffer, int _SizeInBytes)
{
    if (Mode != FILE_OPEN_MODE_READ)
    {
        GLogger.Printf("Failed to read from %s (wrong mode)\n", Name.CStr());
        return 0;
    }
    return fread(_Buffer, 1, _SizeInBytes, (FILE*)Handle);
}

int AFileStream::Impl_Write(const void* _Buffer, int _SizeInBytes)
{
    if (Mode != FILE_OPEN_MODE_WRITE && Mode != FILE_OPEN_MODE_APPEND)
    {
        GLogger.Printf("Failed to write %s (wrong mode)\n", Name.CStr());
        return 0;
    }

    return fwrite(_Buffer, 1, _SizeInBytes, (FILE*)Handle);
}

char* AFileStream::Impl_Gets(char* _StrBuf, int _StrSz)
{
    if (Mode != FILE_OPEN_MODE_READ)
    {
        GLogger.Printf("Failed to read from %s (wrong mode)\n", Name.CStr());
        return nullptr;
    }

    return fgets(_StrBuf, _StrSz, (FILE*)Handle);
}

void AFileStream::Impl_Flush()
{
    fflush((FILE*)Handle);
}

long AFileStream::Impl_Tell()
{
    return ftell((FILE*)Handle);
}

bool AFileStream::Impl_SeekSet(long _Offset)
{
    return fseek((FILE*)Handle, _Offset, SEEK_SET) == 0;
}

bool AFileStream::Impl_SeekCur(long _Offset)
{
    return fseek((FILE*)Handle, _Offset, SEEK_CUR) == 0;
}

bool AFileStream::Impl_SeekEnd(long _Offset)
{
    return fseek((FILE*)Handle, _Offset, SEEK_END) == 0;
}

size_t AFileStream::Impl_SizeInBytes()
{
    long offset = Tell();
    SeekEnd(0);
    long sizeInBytes = Tell();
    SeekSet(offset);
    return (size_t)sizeInBytes;
}

bool AFileStream::Impl_Eof()
{
    return feof((FILE*)Handle) != 0;
}

AMemoryStream::~AMemoryStream()
{
    Close();
}

void* AMemoryStream::Alloc(size_t _SizeInBytes)
{
    return GHeapMemory.Alloc(_SizeInBytes, 16);
}

void* AMemoryStream::Realloc(void* _Data, size_t _SizeInBytes)
{
    return GHeapMemory.Realloc(_Data, _SizeInBytes, 16, true);
}

void AMemoryStream::Free(void* _Data)
{
    GHeapMemory.Free(_Data);
}

bool AMemoryStream::OpenRead(AStringView _FileName, const void* _MemoryBuffer, size_t _SizeInBytes)
{
    Close();
    Name               = _FileName;
    MemoryBuffer       = reinterpret_cast<byte*>(const_cast<void*>(_MemoryBuffer));
    MemoryBufferSize   = _SizeInBytes;
    bMemoryBufferOwner = false;
    MemoryBufferOffset = 0;
    Mode               = FILE_OPEN_MODE_READ;
    return true;
}

bool AMemoryStream::OpenRead(AStringView _FileName, AArchive const& _Archive)
{
    Close();

    if (!_Archive.ExtractFileToHeapMemory(_FileName, (void**)&MemoryBuffer, &MemoryBufferSize))
    {
        GLogger.Printf("Couldn't open %s\n", AString(_FileName).CStr());
        return false;
    }

    Name               = _FileName;
    bMemoryBufferOwner = true;
    MemoryBufferOffset = 0;
    Mode               = FILE_OPEN_MODE_READ;
    return true;
}

bool AMemoryStream::OpenRead(int _FileIndex, AArchive const& _Archive)
{
    Close();

    _Archive.GetFileName(_FileIndex, Name);

    if (!_Archive.ExtractFileToHeapMemory(_FileIndex, (void**)&MemoryBuffer, &MemoryBufferSize))
    {
        GLogger.Printf("Couldn't open %s\n", Name.CStr());
        Name.Clear();
        return false;
    }

    bMemoryBufferOwner = true;
    MemoryBufferOffset = 0;
    Mode               = FILE_OPEN_MODE_READ;
    return true;
}

bool AMemoryStream::OpenWrite(AStringView _FileName, void* _MemoryBuffer, size_t _SizeInBytes)
{
    Close();
    Name               = _FileName;
    MemoryBuffer       = reinterpret_cast<byte*>(_MemoryBuffer);
    MemoryBufferSize   = _SizeInBytes;
    bMemoryBufferOwner = false;
    MemoryBufferOffset = 0;
    Mode               = FILE_OPEN_MODE_WRITE;
    return true;
}

bool AMemoryStream::OpenWrite(AStringView _FileName, size_t _ReservedSize)
{
    Close();
    Name               = _FileName;
    MemoryBuffer       = (byte*)Alloc(_ReservedSize);
    MemoryBufferSize   = _ReservedSize;
    bMemoryBufferOwner = true;
    MemoryBufferOffset = 0;
    Mode               = FILE_OPEN_MODE_WRITE;
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
        Free(MemoryBuffer);
    }
}

const char* AMemoryStream::Impl_GetFileName() const
{
    return Name.CStr();
}

int AMemoryStream::Impl_Read(void* _Buffer, int _SizeInBytes)
{
    if (Mode != FILE_OPEN_MODE_READ)
    {
        GLogger.Printf("Failed to read from %s (wrong mode)\n", Name.CStr());
        return 0;
    }

    int bytesRemaining = MemoryBufferSize - MemoryBufferOffset;
    if (_SizeInBytes > bytesRemaining)
    {
        _SizeInBytes = bytesRemaining;
    }

    if (_SizeInBytes > 0)
    {
        Platform::Memcpy(_Buffer, MemoryBuffer + MemoryBufferOffset, _SizeInBytes);
        MemoryBufferOffset += _SizeInBytes;
    }

    return _SizeInBytes;
}

int AMemoryStream::Impl_Write(const void* _Buffer, int _SizeInBytes)
{
    if (Mode != FILE_OPEN_MODE_WRITE)
    {
        GLogger.Printf("Failed to write %s (wrong mode)\n", Name.CStr());
        return 0;
    }

    int requiredSize = MemoryBufferOffset + _SizeInBytes;
    if (requiredSize > MemoryBufferSize)
    {
        if (!bMemoryBufferOwner)
        {
            GLogger.Printf("Failed to write %s (buffer overflowed)\n", Name.CStr());
            return 0;
        }
        const int mod    = requiredSize % Granularity;
        requiredSize     = mod ? requiredSize + Granularity - mod : requiredSize;
        MemoryBuffer     = (byte*)Realloc(MemoryBuffer, requiredSize);
        MemoryBufferSize = requiredSize;
    }
    Platform::Memcpy(MemoryBuffer + MemoryBufferOffset, _Buffer, _SizeInBytes);
    MemoryBufferOffset += _SizeInBytes;
    return _SizeInBytes;
}

char* AMemoryStream::Impl_Gets(char* _StrBuf, int _StrSz)
{
    if (Mode != FILE_OPEN_MODE_READ)
    {
        GLogger.Printf("Failed to read from %s (wrong mode)\n", Name.CStr());
        return nullptr;
    }

    if (_StrSz == 0 || MemoryBufferOffset >= MemoryBufferSize)
    {
        return nullptr;
    }

    int charsCount = _StrSz - 1;
    if (MemoryBufferOffset + charsCount > MemoryBufferSize)
    {
        charsCount = MemoryBufferSize - MemoryBufferOffset;
    }

    char *memoryPointer, *memory = (char*)&MemoryBuffer[MemoryBufferOffset];
    char* stringPointer = _StrBuf;

    for (memoryPointer = memory; memoryPointer < &memory[charsCount]; memoryPointer++)
    {
        if (*memoryPointer == '\n')
        {
            *stringPointer++ = *memoryPointer++;
            break;
        }
        *stringPointer++ = *memoryPointer;
    }

    *stringPointer = '\0';
    MemoryBufferOffset += memoryPointer - memory;

    return _StrBuf;
}

void AMemoryStream::Impl_Flush()
{
}

long AMemoryStream::Impl_Tell()
{
    return static_cast<long>(MemoryBufferOffset);
}

bool AMemoryStream::Impl_SeekSet(long _Offset)
{
    const int newOffset = _Offset;

#if 0
    if ( newOffset < 0 || newOffset > MemoryBufferSize ) {
        GLogger.Printf( "Bad seek offset for %s\n", Name.CStr() );
        return false;
    }
#endif

    MemoryBufferOffset = Math::Clamp(newOffset, 0, MemoryBufferSize);
    return true;
}

bool AMemoryStream::Impl_SeekCur(long _Offset)
{
    const int newOffset = MemoryBufferOffset + _Offset;

#if 0
    if ( newOffset < 0 || newOffset > MemoryBufferSize ) {
        GLogger.Printf( "Bad seek offset for %s\n", Name.CStr() );
        return false;
    }
#endif

    MemoryBufferOffset = Math::Clamp(newOffset, 0, MemoryBufferSize);
    return true;
}

bool AMemoryStream::Impl_SeekEnd(long _Offset)
{
    const int newOffset = MemoryBufferSize + _Offset;

#if 0
    if ( newOffset < 0 || newOffset > MemoryBufferSize ) {
        GLogger.Printf( "Bad seek offset for %s\n", Name.CStr() );
        return false;
    }
#endif

    MemoryBufferOffset = Math::Clamp(newOffset, 0, MemoryBufferSize);
    return true;
}

size_t AMemoryStream::Impl_SizeInBytes()
{
    return static_cast<size_t>(MemoryBufferSize);
}

bool AMemoryStream::Impl_Eof()
{
    return MemoryBufferOffset >= MemoryBufferSize;
}

void* AMemoryStream::GrabMemory()
{
    return MemoryBuffer;
}


AArchive::AArchive() :
    Handle(nullptr)
{
}

AArchive::AArchive(AStringView _ArchiveName, bool bResourcePack) :
    AArchive()
{
    Open(_ArchiveName, bResourcePack);
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

bool AArchive::Open(AStringView _ArchiveName, bool bResourcePack)
{
    mz_zip_archive arch;
    mz_uint64      fileStartOffset = 0;
    mz_uint64      archiveSize     = 0;

    Close();

    if (bResourcePack)
    {
        AFileStream f;
        if (!f.OpenRead(_ArchiveName))
        {
            return false;
        }

        uint64_t    magic = f.ReadUInt64();
        const char* MAGIC = "ARESPACK";
        if (std::memcmp(&magic, MAGIC, sizeof(uint64_t)) != 0)
        {
            GLogger.Printf("Invalid file format %s\n", _ArchiveName);
            return false;
        }

        fileStartOffset += sizeof(magic);
        archiveSize = f.SizeInBytes() - fileStartOffset;
    }

    Platform::ZeroMem(&arch, sizeof(arch));

    mz_bool status = mz_zip_reader_init_file_v2(&arch, AString(_ArchiveName).CStr(), 0, fileStartOffset, archiveSize);
    if (!status)
    {
        GLogger.Printf("Couldn't open archive %s\n", AString(_ArchiveName).CStr());
        return false;
    }

    Handle = GZoneMemory.Alloc(sizeof(mz_zip_archive));
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
        GLogger.Printf("Couldn't open archive from memory\n");
        return false;
    }

    Handle = GZoneMemory.Alloc(sizeof(mz_zip_archive));
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

    GZoneMemory.Free(Handle);
    Handle = nullptr;
}

int AArchive::GetNumFiles() const
{
    return mz_zip_reader_get_num_files((mz_zip_archive*)Handle);
}

int AArchive::LocateFile(AStringView _FileName) const
{
    return mz_zip_reader_locate_file((mz_zip_archive*)Handle, AString(_FileName).CStr(), NULL, 0);
}

#define MZ_ZIP_CDH_COMPRESSED_SIZE_OFS   20
#define MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS 24
#define MZ_ZIP_CDH_FILENAME_LEN_OFS      28
#define MZ_ZIP_CENTRAL_DIR_HEADER_SIZE   46

bool AArchive::GetFileSize(int _FileIndex, size_t* _CompressedSize, size_t* _UncompressedSize) const
{
    // All checks are processd in mz_zip_get_cdh
    const mz_uint8* p = mz_zip_get_cdh_public((mz_zip_archive*)Handle, _FileIndex);
    if (!p)
    {
        return false;
    }

    if (_CompressedSize)
    {
        *_CompressedSize = MZ_READ_LE32(p + MZ_ZIP_CDH_COMPRESSED_SIZE_OFS);
    }

    if (_UncompressedSize)
    {
        *_UncompressedSize = MZ_READ_LE32(p + MZ_ZIP_CDH_DECOMPRESSED_SIZE_OFS);
    }
    return true;
}

bool AArchive::GetFileName(int _FileIndex, AString& FileName) const
{
    // All checks are processd in mz_zip_get_cdh
    const mz_uint8* p = mz_zip_get_cdh_public((mz_zip_archive*)Handle, _FileIndex);
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

bool AArchive::ExtractFileToMemory(int _FileIndex, void* _MemoryBuffer, size_t _SizeInBytes) const
{
    // All checks are processd in mz_zip_reader_extract_to_mem
    return !!mz_zip_reader_extract_to_mem((mz_zip_archive*)Handle, _FileIndex, _MemoryBuffer, _SizeInBytes, 0);
}

bool AArchive::ExtractFileToHeapMemory(AStringView _FileName, void** _HeapMemoryPtr, int* _SizeInBytes) const
{
    size_t uncompSize;

    *_HeapMemoryPtr = nullptr;
    *_SizeInBytes   = 0;

    int fileIndex = LocateFile(_FileName);
    if (fileIndex < 0)
    {
        return false;
    }

    if (!GetFileSize(fileIndex, NULL, &uncompSize))
    {
        return false;
    }

    void* pBuf = GHeapMemory.Alloc(uncompSize);

    if (!ExtractFileToMemory(fileIndex, pBuf, uncompSize))
    {
        GHeapMemory.Free(pBuf);
        return false;
    }

    *_HeapMemoryPtr = pBuf;
    *_SizeInBytes   = uncompSize;

    return true;
}

bool AArchive::ExtractFileToHeapMemory(int FileIndex, void** _HeapMemoryPtr, int* _SizeInBytes) const
{
    size_t uncompSize;

    *_HeapMemoryPtr = nullptr;
    *_SizeInBytes   = 0;

    if (FileIndex < 0)
    {
        return false;
    }

    if (!GetFileSize(FileIndex, NULL, &uncompSize))
    {
        return false;
    }

    void* pBuf = GHeapMemory.Alloc(uncompSize);

    if (!ExtractFileToMemory(FileIndex, pBuf, uncompSize))
    {
        GHeapMemory.Free(pBuf);
        return false;
    }

    *_HeapMemoryPtr = pBuf;
    *_SizeInBytes   = uncompSize;

    return true;
}

bool AArchive::ExtractFileToHunkMemory(AStringView _FileName, void** _HunkMemoryPtr, int* _SizeInBytes, int* _HunkMark) const
{
    size_t uncompSize;

    *_HunkMark = GHunkMemory.SetHunkMark();

    *_HunkMemoryPtr = nullptr;
    *_SizeInBytes   = 0;

    int fileIndex = LocateFile(_FileName);
    if (fileIndex < 0)
    {
        return false;
    }

    if (!GetFileSize(fileIndex, NULL, &uncompSize))
    {
        return false;
    }

    void* pBuf = GHunkMemory.Alloc(uncompSize);

    if (!ExtractFileToMemory(fileIndex, pBuf, uncompSize))
    {
        GHunkMemory.ClearToMark(*_HunkMark);
        return false;
    }

    *_HunkMemoryPtr = pBuf;
    *_SizeInBytes   = uncompSize;

    return true;
}


namespace Core
{

void MakeDir(const char* _Directory, bool _FileName)
{
    size_t len = Platform::Strlen(_Directory);
    if (!len)
    {
        return;
    }
    char* tmpStr = (char*)GZoneMemory.Alloc(len + 1);
    Platform::Memcpy(tmpStr, _Directory, len + 1);
    char* p = tmpStr;
#ifdef AN_OS_WIN32
    if (len >= 3 && _Directory[1] == ':')
    {
        p += 3;
        _Directory += 3;
    }
#endif
    for (; *_Directory; p++, _Directory++)
    {
        if (Platform::IsPathSeparator(*p))
        {
            *p = 0;
#ifdef AN_COMPILER_MSVC
            mkdir(tmpStr);
#else
            mkdir(tmpStr, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
            *p = *_Directory;
        }
    }
    if (!_FileName)
    {
#ifdef AN_COMPILER_MSVC
        mkdir(tmpStr);
#else
        mkdir(tmpStr, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
    }
    GZoneMemory.Free(tmpStr);
}

bool IsFileExists(const char* _FileName)
{
    AString s = _FileName;
    s.FixSeparator();
    return access(s.CStr(), 0) == 0;
}

void RemoveFile(const char* _FileName)
{
    AString s = _FileName;
    s.FixPath();
#if defined AN_OS_LINUX
    ::remove(s.CStr());
#elif defined AN_OS_WIN32
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

#ifdef AN_OS_LINUX
void TraverseDirectory(AStringView Path, bool bSubDirs, STraverseDirectoryCB Callback)
{
    AString fn;
    DIR*    dir = opendir(Path.CStr());
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
                Callback(Path + "/" + entry->d_name, false);
            }
        }
        closedir(dir);
    }
}
#endif

#ifdef AN_OS_WIN32
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
                Callback(Path + "/" + fd.cFileName, false);
            }
        } while (FindNextFileA(fh, &fd));

        FindClose(fh);
    }
}
#endif

bool WriteResourcePack(const char* SourcePath, const char* ResultFile)
{
    AString path = SourcePath;

    path.FixSeparator();

    GLogger.Printf("==== WriteResourcePack ====\n"
                   "Source '%s'\n"
                   "Destination: '%s'\n",
                   SourcePath, ResultFile);

    FILE* file = OpenFile(ResultFile, "wb");
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

                              if (FileName.CompareExt(".resources", true))
                              {
                                  return;
                              }

                              AString fn = FileName.TruncateHead(path.Length() + 1);

                              GLogger.Printf("Writing '%s'\n", fn.CStr());

                              mz_bool status = mz_zip_writer_add_file(&zip, fn.CStr(), AString(FileName).CStr(), nullptr, 0, MZ_UBER_COMPRESSION);
                              if (!status)
                              {
                                  GLogger.Printf("Failed to archive %s\n", AString(FileName).CStr());
                              }
                          });

        mz_zip_writer_finalize_archive(&zip);
        mz_zip_writer_end(&zip);
    }

    fclose(file);

    GLogger.Printf("===========================\n");

    return true;
}

} // namespace Core
