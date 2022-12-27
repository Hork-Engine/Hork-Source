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

#include "VirtualTextureTools.h"
#include "QuadTree.h"

#include <Platform/Logger.h>
#include <Platform/WindowsDefs.h>
#include <Geometry/VectorMath.h>

#include <Image/Image.h>

#define PAGE_EXTENSION ".page"

#ifdef HK_OS_LINUX
#    include <sys/stat.h>
#    include <dirent.h>
#endif

HK_NAMESPACE_BEGIN

VirtualTextureImage::VirtualTextureImage()
{
    Data = NULL;
    NumChannels = 0;
    Width = 0;
    Height = 0;
}
VirtualTextureImage::~VirtualTextureImage()
{
    Platform::GetHeapAllocator<HEAP_TEMP>().Free(Data);
}

bool VirtualTextureImage::OpenImage(const char* FileName, int InWidth, int InHeight, int InNumChannels)
{
    VTFileHandle file;
    if (!file.OpenRead(FileName))
    {
        return false;
    }
    CreateEmpty(InWidth, InHeight, InNumChannels);
    file.Read(Data, InWidth * InHeight * InNumChannels, 0);
    return true;
}

bool VirtualTextureImage::WriteImage(const char* FileName) const
{
    if (!Data)
    {
        return false;
    }
    VTFileHandle file;
    if (!file.OpenWrite(FileName))
    {
        return false;
    }
    file.Write(Data, Width * Height * NumChannels, 0);
    return true;
}

void VirtualTextureImage::CreateEmpty(int InWidth, int InHeight, int InNumChannels)
{
    if (Data == NULL || Width * Height * NumChannels != InWidth * InHeight * InNumChannels)
    {
        Data = (byte*)Platform::GetHeapAllocator<HEAP_TEMP>().Realloc(Data, InWidth * InHeight * InNumChannels, 16, MALLOC_DISCARD);
    }

    Width = InWidth;
    Height = InHeight;
    NumChannels = InNumChannels;
}

VirtualTextureLayer::VirtualTextureLayer()
{
    MaxCachedPages = 1024;
    NumCachedPages = 0;
    bAllowDump = true;
}

VirtualTextureLayer::~VirtualTextureLayer()
{
    VT_FitPageData(*this, true);
    if (NumCachedPages > 0)
    {
        LOG("Warning: have not closed pages\n");
    }

    // Удалить все оставшееся содержимое кеша
    for (auto it = Pages.begin(); it != Pages.end(); it++)
    {
        VirtualTextureLayer::CachedPage* cachedPage = it->second;
        delete cachedPage;
    }
}

bool VT_MakeStructure(VirtualTextureStructure& _Struct,
                      int _PageWidthLog2,
                      const std::vector<ARectangleBinPack::RectSize>& _TextureRects,
                      std::vector<RectangleBinBack_RectNode>& _BinRects,
                      unsigned int& _BinWidth,
                      unsigned int& _BinHeight)
{
    std::vector<ARectangleBinPack::RectSize> tempRects;
    double space = 0.0;

    _Struct.PageResolutionB = 1 << _PageWidthLog2;
    _Struct.PageResolution = _Struct.PageResolutionB - (VT_PAGE_BORDER_WIDTH << 1);

    _BinRects.clear();

    int numTextureRectangles = _TextureRects.size();
    if (!numTextureRectangles)
    {
        LOG("No texture rectangles\n");
        return false;
    }

    tempRects.resize(numTextureRectangles);

    // adjust rect sizes and compute virtual texture space
    for (int i = 0; i < numTextureRectangles; i++)
    {
        const ARectangleBinPack::RectSize& in = _TextureRects[i];
        ARectangleBinPack::RectSize& out = tempRects[i];

        // copy rect
        out = in;

        // adjust rect size
        if (out.width % _Struct.PageResolution != 0)
        {
            out.width = int(out.width / double(_Struct.PageResolution) + 1.0) * _Struct.PageResolution;
        }
        if (out.height % _Struct.PageResolution != 0)
        {
            out.height = int(out.height / double(_Struct.PageResolution) + 1.0) * _Struct.PageResolution;
        }

        // compute virtual texture space
        space += (out.width + (VT_PAGE_BORDER_WIDTH << 1)) * (out.height + (VT_PAGE_BORDER_WIDTH << 1));

        // scale pixels to pages
        out.width /= _Struct.PageResolution;  // перевести из пикселей в страницы
        out.height /= _Struct.PageResolution; // перевести из пикселей в страницы
    }

    _Struct.NumLods = Math::Log2(Math::ToGreaterPowerOfTwo((uint32_t)ceilf(sqrtf(space))) / _Struct.PageResolutionB) + 1;

    while (1)
    {
        _BinWidth = _BinHeight = (1 << (_Struct.NumLods - 1));

        ARectangleBinPack binPack(_BinWidth, _BinHeight);
        std::vector<ARectangleBinPack::RectSize> rectVec = tempRects;
        binPack.Insert(rectVec,
                       false,
                       ARectangleBinPack::RectBestAreaFit,
                       ARectangleBinPack::SplitShorterLeftoverAxis,
                       true);

        if ((int)binPack.GetUsedRectangles().size() == numTextureRectangles)
        {
            _BinRects = binPack.GetUsedRectangles();
            break;
        }

        _Struct.NumLods++;
    }

    //int TextureWidth = ( 1 << ( _Struct.NumLods - 1 ) ) * _Struct.PageResolutionB;

    _Struct.NumQuadTreeNodes = QuadTreeCalcQuadTreeNodes(_Struct.NumLods);

    _Struct.PageBitfield.ResizeInvalidate(_Struct.NumQuadTreeNodes);
    _Struct.PageBitfield.UnmarkAll();

    return true;
}

String VT_FileNameFromRelative(const char* _OutputPath, unsigned int _RelativeIndex, int _Lod)
{
    return HK_FORMAT("{}{}/{}{}", _OutputPath, _Lod, _RelativeIndex, PAGE_EXTENSION);
}

VirtualTextureLayer::CachedPage* VT_FindInCache(VirtualTextureLayer& Layer, unsigned int _AbsoluteIndex)
{
    auto it = Layer.Pages.find(_AbsoluteIndex);
    if (it == Layer.Pages.end())
    {
        return NULL;
    }
    return it->second;
}

bool VT_DumpPageToDisk(const char* _Path, unsigned int _AbsoluteIndex, const VirtualTextureImage& _Image)
{
    String fn;
    int lod;
    unsigned int relativeIndex;

    lod = QuadTreeCalcLod64(_AbsoluteIndex);
    relativeIndex = QuadTreeAbsoluteToRelativeIndex(_AbsoluteIndex, lod);

    fn = VT_FileNameFromRelative(_Path, relativeIndex, lod);

    return _Image.WriteImage(fn.CStr());
}

void VT_FitPageData(VirtualTextureLayer& Layer, bool _ForceFit)
{
    if ((!_ForceFit && Layer.NumCachedPages < Layer.MaxCachedPages) || Layer.MaxCachedPages < 0)
    {
        return;
    }

    int totalDumped = 0;
    int totalCachedPages = Layer.NumCachedPages;

    LOG("Fit page data...\n");

    for (auto it = Layer.Pages.begin(); it != Layer.Pages.end();)
    {
        VirtualTextureLayer::CachedPage* cachedPage = it->second;

        if (cachedPage->Used > 0)
        {
            // now page in use, so keep it in memory
            it++;
            continue;
        }

        if (cachedPage->bNeedToSave && Layer.bAllowDump)
        {
            if (totalDumped == 0)
            {
                LOG("Dumping pages to disk...\n");
            }
            if (VT_DumpPageToDisk(Layer.Path.CStr(), it->first, cachedPage->Image))
            {
                totalDumped++;
            }
        }

        delete cachedPage;
        it = Layer.Pages.erase(it);
        Layer.NumCachedPages--;
    }

    LOG("Total dumped pages: {} from {}\n", totalDumped, totalCachedPages);
}

VirtualTextureLayer::CachedPage* VT_OpenCachedPage(const VirtualTextureStructure& _Struct, VirtualTextureLayer& Layer, unsigned int _AbsoluteIndex, VirtualTextureLayer::OpenMode _OpenMode, bool _NeedToSave)
{
    int lod;
    unsigned int relativeIndex;
    String fn;
    VirtualTextureLayer::CachedPage* cachedPage;

    cachedPage = VT_FindInCache(Layer, _AbsoluteIndex);
    if (cachedPage)
    {
        if (_NeedToSave)
        {
            cachedPage->bNeedToSave = true;
        }
        cachedPage->Used++;
        return cachedPage;
    }

    VT_FitPageData(Layer);

    cachedPage = new VirtualTextureLayer::CachedPage;

    if (_OpenMode == VirtualTextureLayer::OpenEmpty)
    {
        // create empty page
        cachedPage->Image.CreateEmpty(_Struct.PageResolutionB, _Struct.PageResolutionB, Layer.NumChannels);
        int ImageByteSize = _Struct.PageResolutionB * _Struct.PageResolutionB * Layer.NumChannels;
        Platform::ZeroMem(cachedPage->Image.GetData(), ImageByteSize);
    }
    else if (_OpenMode == VirtualTextureLayer::OpenActual)
    {
        // open from file
        lod = QuadTreeCalcLod64(_AbsoluteIndex);
        relativeIndex = QuadTreeAbsoluteToRelativeIndex(_AbsoluteIndex, lod);
        fn = VT_FileNameFromRelative(Layer.Path.CStr(), relativeIndex, lod);

        bool bOpened = cachedPage->Image.OpenImage(fn.CStr(), _Struct.PageResolutionB, _Struct.PageResolutionB, Layer.NumChannels);
        if (!bOpened)
        {
            LOG("VT_OpenCachedPage: can't open page\n");
            delete cachedPage;
            return NULL;
        }
    }
    else
    {
        LOG("VT_OpenCachedPage: unknown open mode\n");
        delete cachedPage;
        return NULL;
    }

    cachedPage->Used = 1;
    cachedPage->bNeedToSave = _NeedToSave;
    Layer.Pages[_AbsoluteIndex] = cachedPage;
    Layer.NumCachedPages++;

    return cachedPage;
}

void VT_CloseCachedPage(VirtualTextureLayer::CachedPage* _CachedPage)
{
    if (!_CachedPage)
    {
        return;
    }
    _CachedPage->Used--;
    if (_CachedPage->Used < 0)
    {
        LOG("Warning: VT_CloseCachedPage: trying to close closed page\n");
    }
}

namespace
{

struct PageRect
{
    int x, y;
    int width, height;
};

void CopyRect(const PageRect& _Rect,
              const byte* _Source,
              int _SourceWidth,
              int _SourceHeight,
              int _DestPositionX,
              int _DestPositionY,
              byte* _Dest,
              int _DestWidth,
              int _DestHeight,
              int _ElementSize)
{
    HK_ASSERT_(_Rect.x + _Rect.width <= _SourceWidth && _Rect.y + _Rect.height <= _SourceHeight, "CopyRect");

    HK_ASSERT_(_DestPositionX + _Rect.width <= _DestWidth && _DestPositionY + _Rect.height <= _DestHeight, "CopyRect");

    const byte* pSource = _Source + (_Rect.y * _SourceWidth + _Rect.x) * _ElementSize;
    byte* pDest = _Dest + (_DestPositionY * _DestWidth + _DestPositionX) * _ElementSize;

    int rectLineSize = _Rect.width * _ElementSize;
    int destStep = _DestWidth * _ElementSize;
    int sourceStep = _SourceWidth * _ElementSize;

    for (int y = 0; y < _Rect.height; y++)
    {
        Platform::Memcpy(pDest, pSource, rectLineSize);
        pDest += destStep;
        pSource += sourceStep;
    }
}
} // namespace

void VT_PutImageIntoPages(VirtualTextureStructure& _Struct,
                          VirtualTextureLayer& Layer,
                          const RectangleBinBack_RectNode& _Rect,
                          const byte* _LayerData)
{

    int numPagesX = _Rect.width;
    int numPagesY = _Rect.height;

    PageRect pageRect;

    pageRect.width = _Struct.PageResolution;
    pageRect.height = _Struct.PageResolution;

    int copyOffsetX = VT_PAGE_BORDER_WIDTH;
    int copyOffsetY = VT_PAGE_BORDER_WIDTH;

    int lod = _Struct.NumLods - 1;
    int numVtPages = 1 << lod;

    int LayerWidth = _Rect.width * _Struct.PageResolution;
    int LayerHeight = _Rect.height * _Struct.PageResolution;

    for (int x = 0; x < numPagesX; x++)
    {
        for (int y = 0; y < numPagesY; y++)
        {

            int pageIndexX = _Rect.x + x;
            int pageIndexY = _Rect.y + y;

            HK_ASSERT_(pageIndexX < numVtPages, "VT_PutImageIntoPages");
            HK_ASSERT_(pageIndexY < numVtPages, "VT_PutImageIntoPages");
            HK_UNUSED(numVtPages);

            unsigned int relativeIndex = QuadTreeGetRelativeFromXY(pageIndexX, pageIndexY, lod);
            unsigned int absoluteIndex = QuadTreeRelativeToAbsoluteIndex(relativeIndex, lod);

            VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_Struct, Layer, absoluteIndex, VirtualTextureLayer::OpenEmpty, true);
            if (!cachedPage)
            {
                continue;
            }

            pageRect.x = x * _Struct.PageResolution;
            pageRect.y = y * _Struct.PageResolution;

            CopyRect(pageRect,
                     _LayerData,
                     LayerWidth,
                     LayerHeight,
                     copyOffsetX,
                     copyOffsetY,
                     cachedPage->Image.GetData(),
                     _Struct.PageResolutionB,
                     _Struct.PageResolutionB,
                     Layer.NumChannels);

            _Struct.PageBitfield.Mark(absoluteIndex);

            //WriteImage( HK_FORMAT("page_{}_{}.bmp",x,y), _Struct.PageResolutionB, _Struct.PageResolutionB, Layer.NumChannels, cachedPage->Image.GetData() );

            VT_CloseCachedPage(cachedPage);
        }
    }
}

bool VT_LoadQuad(const VirtualTextureStructure& _Struct,
                 VirtualTextureLayer& Layer,
                 unsigned int _Src00,
                 unsigned int _Src10,
                 unsigned int _Src01,
                 unsigned int _Src11,
                 int _SourceLod,
                 VirtualTextureLayer::CachedPage* _Page[4])
{

    bool b[4];
    unsigned int src[4];

    src[0] = _Src00;
    src[1] = _Src01;
    src[2] = _Src10;
    src[3] = _Src11;

    for (int i = 0; i < 4; i++)
    {
        unsigned int absoluteIndex = QuadTreeRelativeToAbsoluteIndex(src[i], _SourceLod);
        b[i] = _Struct.PageBitfield.IsMarked(absoluteIndex);
        _Page[i] = NULL;
    }

    if (!b[0] && !b[1] && !b[2] && !b[3])
    {
        return false;
    }

    int validPages = 0;

    for (int i = 0; i < 4; i++)
    {
        if (b[i])
        {
            _Page[i] = VT_OpenCachedPage(_Struct, Layer, QuadTreeRelativeToAbsoluteIndex(src[i], _SourceLod), VirtualTextureLayer::OpenActual, false);
            if (_Page[i])
            {
                validPages++;
            }
        }
    }

    if (validPages == 0)
    {
        return false;
    }

    return true;
}

void VT_Downsample(const VirtualTextureStructure& _Struct, VirtualTextureLayer& Layer, VirtualTextureLayer::CachedPage* _Pages[4], byte* _Downsample)
{
    const int borderOffset = ((VT_PAGE_BORDER_WIDTH)*_Struct.PageResolutionB + (VT_PAGE_BORDER_WIDTH)) * Layer.NumChannels;
    const byte* src00 = _Pages[0] ? _Pages[0]->Image.GetData() + borderOffset : NULL;
    const byte* src01 = _Pages[1] ? _Pages[1]->Image.GetData() + borderOffset : NULL;
    const byte* src10 = _Pages[2] ? _Pages[2]->Image.GetData() + borderOffset : NULL;
    const byte* src11 = _Pages[3] ? _Pages[3]->Image.GetData() + borderOffset : NULL;
    const byte* s;
    byte* d;
    int color;

    _Downsample += borderOffset;

    for (int ch = 0; ch < Layer.NumChannels; ch++)
    {
        for (int y = 0; y < (_Struct.PageResolution >> 1); y++)
        {
            for (int x = 0; x < (_Struct.PageResolution >> 1); x++)
            {
                color = 0;
                if (src00)
                {
                    s = &src00[(y * _Struct.PageResolutionB + x) * Layer.NumChannels * 2];

                    color += *(s + ch);

                    s += Layer.NumChannels;

                    color += *(s + ch);

                    s += (_Struct.PageResolutionB - 1) * Layer.NumChannels;

                    color += *(s + ch);

                    s += Layer.NumChannels;

                    color += *(s + ch);

                    color >>= 2;
                }

                d = &_Downsample[(y * _Struct.PageResolutionB + x) * Layer.NumChannels];

                *(d + ch) = color;

                color = 0;
                if (src10)
                {
                    s = &src10[(y * _Struct.PageResolutionB + x) * Layer.NumChannels * 2];

                    color += *(s + ch);

                    s += Layer.NumChannels;

                    color += *(s + ch);

                    s += (_Struct.PageResolutionB - 1) * Layer.NumChannels;

                    color += *(s + ch);

                    s += Layer.NumChannels;

                    color += *(s + ch);

                    color >>= 2;
                }

                d += (_Struct.PageResolution >> 1) * Layer.NumChannels;

                *(d + ch) = color;

                color = 0;
                if (src01)
                {
                    s = &src01[(y * _Struct.PageResolutionB + x) * Layer.NumChannels * 2];

                    color += *(s + ch);

                    s += Layer.NumChannels;

                    color += *(s + ch);

                    s += (_Struct.PageResolutionB - 1) * Layer.NumChannels;

                    color += *(s + ch);

                    s += Layer.NumChannels;

                    color += *(s + ch);

                    color >>= 2;
                }

                d = &_Downsample[((y + (_Struct.PageResolution >> 1)) * _Struct.PageResolutionB + x) * Layer.NumChannels];

                *(d + ch) = color;

                color = 0;
                if (src11)
                {
                    s = &src11[(y * _Struct.PageResolutionB + x) * Layer.NumChannels * 2];

                    color += *(s + ch);

                    s += Layer.NumChannels;

                    color += *(s + ch);

                    s += (_Struct.PageResolutionB - 1) * Layer.NumChannels;

                    color += *(s + ch);

                    s += Layer.NumChannels;

                    color += *(s + ch);

                    color >>= 2;
                }

                d += (_Struct.PageResolution >> 1) * Layer.NumChannels;

                *(d + ch) = color;
            }
        }
    }
}

void VT_MakeLods(VirtualTextureStructure& _Struct, VirtualTextureLayer& Layer)
{
    VirtualTextureLayer::CachedPage* pages[4];

    for (int sourceLod = _Struct.NumLods - 1; sourceLod > 0; sourceLod--)
    {

        unsigned int numLodPages = 1 << sourceLod;

        int destLod = sourceLod - 1;

        for (unsigned int y = 0; y < numLodPages; y += 2)
        {
            for (unsigned int x = 0; x < numLodPages; x += 2)
            {

                unsigned int src00 = QuadTreeGetRelativeFromXY(x, y, sourceLod);
                unsigned int src10 = QuadTreeGetRelativeFromXY(x + 1, y, sourceLod);
                unsigned int src01 = QuadTreeGetRelativeFromXY(x, y + 1, sourceLod);
                unsigned int src11 = QuadTreeGetRelativeFromXY(x + 1, y + 1, sourceLod);

                if (!VT_LoadQuad(_Struct, Layer, src00, src10, src01, src11, sourceLod, pages))
                {
                    continue;
                }

                unsigned int dst = QuadTreeGetRelativeFromXY(x >> 1, y >> 1, destLod);
                unsigned int absoluteIndex = QuadTreeRelativeToAbsoluteIndex(dst, destLod);

                VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_Struct, Layer, absoluteIndex, VirtualTextureLayer::OpenEmpty, true);
                if (!cachedPage)
                {
                    continue;
                }

                VT_Downsample(_Struct, Layer, pages, cachedPage->Image.GetData());
                _Struct.PageBitfield.Mark(absoluteIndex);

                //WriteImage( HK_FORMAT("page_{}.bmp", absoluteIndex), _Struct.PageResolutionB, _Struct.PageResolutionB, Layer.NumChannels, cachedPage->Image.GetData() );

                VT_CloseCachedPage(cachedPage);

                for (int i = 0; i < 4; i++)
                {
                    VT_CloseCachedPage(pages[i]);
                }
            }
        }
    }
}

static void VT_SynchronizePageBitfieldWithHDD_Lod(VTPageBitfield& BitField, int _Lod, const char* _LodPath)
{
#ifdef HK_OS_WIN32
    // TODO: Not tested on Windows
    unsigned int relativeIndex;
    unsigned int absoluteIndex;
    unsigned int validMax = QuadTreeCalcLodNodes(_Lod);

    HANDLE fh;
    WIN32_FIND_DATAA fd;

    // TODO: Rewrite to support FindFirstFileW

    if ((fh = FindFirstFileA(String(HK_FORMAT("{}*.png", _LodPath)).CStr(), &fd)) != INVALID_HANDLE_VALUE)
    {
        do {
            int len = strlen(fd.cFileName);
            if (len < 4)
            {
                // Invalid name
                HK_ASSERT_(0, "Never reached");
                continue;
            }

            if (!Platform::Stricmp(&fd.cFileName[len - 4], ".png"))
            {
                // extension is not .png
                HK_ASSERT_(0, "Never reached");
                continue;
            }

            fd.cFileName[len - 4] = 0;
            relativeIndex = atoi(fd.cFileName);
            fd.cFileName[len - 4] = '.';

            if (relativeIndex >= validMax)
            {
                // invalid index (out of range)
                continue;
            }

            absoluteIndex = QuadTreeRelativeToAbsoluteIndex(relativeIndex, _Lod);

            BitField.Mark(absoluteIndex);

        } while (FindNextFileA(fh, &fd));

        FindClose(fh);
    }
#else
    //size_t          len = strlen( "0000000000000000" );
    //size_t          extLen = strlen( VT_PAGE_RASTER_EXT );
    DIR* dp;
    struct dirent* entry;
    struct stat statBuff;
    unsigned int relativeIndex;
    unsigned int absoluteIndex;

    if ((dp = opendir(_LodPath)) == NULL)
    {
        // directory does not exist
        return;
    }

    unsigned int validMax = QuadTreeCalcLodNodes(_Lod);

    while ((entry = readdir(dp)) != NULL)
    {
        lstat(entry->d_name, &statBuff);
        //        if ( strlen( entry->d_name ) != len + extLen ) {
        //            continue;
        //        }

        //        relativeIndex64 = common::HexToInt64( entry->d_name, len );

        int l = strlen(entry->d_name);
        if (l < 4)
        {
            continue; // no extension
        }

        if (!strcasecmp(&entry->d_name[l - 4], PAGE_EXTENSION))
        {
            // extension is not PAGE_EXTENSION
            continue;
        }

        entry->d_name[l - 4] = 0;
        relativeIndex = atoi(entry->d_name);
        entry->d_name[l - 4] = '.';

        if (relativeIndex >= validMax)
        {
            // invalid index (out of range)
            continue;
        }

        absoluteIndex = QuadTreeRelativeToAbsoluteIndex(relativeIndex, _Lod);

        BitField.Mark(absoluteIndex);
    }

    closedir(dp);
#endif
}

void VT_SynchronizePageBitfieldWithHDD(VirtualTextureStructure& _Struct, VirtualTextureLayer& Layer)
{
    String lodPath;

    _Struct.PageBitfield.ResizeInvalidate(_Struct.NumQuadTreeNodes);
    _Struct.PageBitfield.UnmarkAll();

    for (int lod = 0; lod < _Struct.NumLods; lod++)
    {
        lodPath = Layer.Path + Core::ToString(lod) + "/";

        VT_SynchronizePageBitfieldWithHDD_Lod(_Struct.PageBitfield, lod, lodPath.CStr());
    }
}

static HK_FORCEINLINE unsigned int __GetAbsoluteFromXY(int _X, int _Y, int _Lod)
{
    return QuadTreeRelativeToAbsoluteIndex(QuadTreeGetRelativeFromXY(_X, _Y, _Lod), _Lod);
}

static HK_FORCEINLINE int __PageByteOffset(const VirtualTextureStructure& _Struct, const VirtualTextureLayer& Layer, const int& _X, const int& _Y)
{
    return (_Y * _Struct.PageResolutionB + _X) * Layer.NumChannels;
}

void VT_GenerateBorder_U(VirtualTextureStructure& _Struct, VirtualTextureLayer& Layer, unsigned int _RelativeIndex, int _Lod, byte* PageData)
{
    int x = QuadTreeGetXFromRelative(_RelativeIndex, _Lod);
    int y = QuadTreeGetYFromRelative(_RelativeIndex, _Lod) - 1;

    unsigned int sourcePageIndex = __GetAbsoluteFromXY(x, y, _Lod);

    bool pageNotExist = y < 0 || !_Struct.PageBitfield.IsMarked(sourcePageIndex);

    int destPositionX = VT_PAGE_BORDER_WIDTH;
    int destPositionY = 0;

    if (pageNotExist)
    {
        int dstOffset = __PageByteOffset(_Struct, Layer, destPositionX, destPositionY);
        int srcOffset = __PageByteOffset(_Struct, Layer, VT_PAGE_BORDER_WIDTH, VT_PAGE_BORDER_WIDTH);

        for (int i = 0; i < VT_PAGE_BORDER_WIDTH; i++)
        {
            Platform::Memcpy(PageData + dstOffset + i * _Struct.PageResolutionB * Layer.NumChannels, PageData + srcOffset, _Struct.PageResolution * Layer.NumChannels);
        }
        return;
    }

    VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_Struct, Layer, sourcePageIndex, VirtualTextureLayer::OpenActual, false);
    if (!cachedPage)
    {
        return;
    }

    PageRect rect;

    rect.x = VT_PAGE_BORDER_WIDTH;
    rect.y = _Struct.PageResolution;
    rect.width = _Struct.PageResolution;
    rect.height = VT_PAGE_BORDER_WIDTH;

    CopyRect(rect,
             cachedPage->Image.GetData(),
             _Struct.PageResolutionB,
             _Struct.PageResolutionB,
             destPositionX,
             destPositionY,
             PageData,
             _Struct.PageResolutionB,
             _Struct.PageResolutionB,
             Layer.NumChannels);

    VT_CloseCachedPage(cachedPage);
}

void VT_GenerateBorder_D(VirtualTextureStructure& _Struct, VirtualTextureLayer& Layer, unsigned int _RelativeIndex, int _Lod, byte* PageData)
{
    int maxPages = 1 << _Lod;

    int x = QuadTreeGetXFromRelative(_RelativeIndex, _Lod);
    int y = QuadTreeGetYFromRelative(_RelativeIndex, _Lod) + 1;

    unsigned int sourcePageIndex = __GetAbsoluteFromXY(x, y, _Lod);

    bool pageNotExist = y >= maxPages || !_Struct.PageBitfield.IsMarked(sourcePageIndex);

    int destPositionX = VT_PAGE_BORDER_WIDTH;
    int destPositionY = _Struct.PageResolutionB - VT_PAGE_BORDER_WIDTH;

    if (pageNotExist)
    {
        int dstOffset = __PageByteOffset(_Struct, Layer, destPositionX, destPositionY);
        int srcOffset = __PageByteOffset(_Struct, Layer, VT_PAGE_BORDER_WIDTH, _Struct.PageResolutionB - VT_PAGE_BORDER_WIDTH - 1);

        for (int i = 0; i < VT_PAGE_BORDER_WIDTH; i++)
        {
            Platform::Memcpy(PageData + dstOffset + i * _Struct.PageResolutionB * Layer.NumChannels, PageData + srcOffset, _Struct.PageResolution * Layer.NumChannels);
        }
        return;
    }

    VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_Struct, Layer, sourcePageIndex, VirtualTextureLayer::OpenActual, false);
    if (!cachedPage)
    {
        return;
    }

    PageRect rect;

    rect.x = VT_PAGE_BORDER_WIDTH;
    rect.y = VT_PAGE_BORDER_WIDTH;
    rect.width = _Struct.PageResolution;
    rect.height = VT_PAGE_BORDER_WIDTH;

    CopyRect(rect,
             cachedPage->Image.GetData(),
             _Struct.PageResolutionB,
             _Struct.PageResolutionB,
             destPositionX,
             destPositionY,
             PageData,
             _Struct.PageResolutionB,
             _Struct.PageResolutionB,
             Layer.NumChannels);

    VT_CloseCachedPage(cachedPage);
}

void VT_GenerateBorder_L(VirtualTextureStructure& _Struct, VirtualTextureLayer& Layer, unsigned int _RelativeIndex, int _Lod, byte* PageData)
{
    int x = QuadTreeGetXFromRelative(_RelativeIndex, _Lod) - 1;
    int y = QuadTreeGetYFromRelative(_RelativeIndex, _Lod);

    unsigned int sourcePageIndex = __GetAbsoluteFromXY(x, y, _Lod);

    bool pageNotExist = x < 0 || !_Struct.PageBitfield.IsMarked(sourcePageIndex);

    int destPositionX = 0;
    int destPositionY = VT_PAGE_BORDER_WIDTH;

    if (pageNotExist)
    {
        PageData += __PageByteOffset(_Struct, Layer, destPositionX, destPositionY);

        byte* srcData = PageData + VT_PAGE_BORDER_WIDTH * Layer.NumChannels;

        for (int j = 0; j < _Struct.PageResolution; j++)
        {
            for (int i = 0; i < VT_PAGE_BORDER_WIDTH; i++)
            {
                Platform::Memcpy(PageData + i * Layer.NumChannels, srcData, Layer.NumChannels);
            }
            PageData += _Struct.PageResolutionB * Layer.NumChannels;
            srcData += _Struct.PageResolutionB * Layer.NumChannels;
        }
        return;
    }

    VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_Struct, Layer, sourcePageIndex, VirtualTextureLayer::OpenActual, false);
    if (!cachedPage)
    {
        return;
    }

    PageRect rect;

    rect.x = _Struct.PageResolution;
    rect.y = VT_PAGE_BORDER_WIDTH;
    rect.width = VT_PAGE_BORDER_WIDTH;
    rect.height = _Struct.PageResolution;

    CopyRect(rect,
             cachedPage->Image.GetData(),
             _Struct.PageResolutionB,
             _Struct.PageResolutionB,
             destPositionX,
             destPositionY,
             PageData,
             _Struct.PageResolutionB,
             _Struct.PageResolutionB,
             Layer.NumChannels);

    VT_CloseCachedPage(cachedPage);
}

void VT_GenerateBorder_R(VirtualTextureStructure& _Struct, VirtualTextureLayer& Layer, unsigned int _RelativeIndex, int _Lod, byte* PageData)
{
    int maxPages = 1 << _Lod;

    int x = QuadTreeGetXFromRelative(_RelativeIndex, _Lod) + 1;
    int y = QuadTreeGetYFromRelative(_RelativeIndex, _Lod);

    unsigned int sourcePageIndex = __GetAbsoluteFromXY(x, y, _Lod);

    bool pageNotExist = x >= maxPages || !_Struct.PageBitfield.IsMarked(sourcePageIndex);

    int destPositionX = _Struct.PageResolution + VT_PAGE_BORDER_WIDTH;
    int destPositionY = VT_PAGE_BORDER_WIDTH;

    if (pageNotExist)
    {
        PageData += __PageByteOffset(_Struct, Layer, destPositionX, destPositionY);

        byte* srcData = PageData - Layer.NumChannels;

        for (int j = 0; j < _Struct.PageResolution; j++)
        {
            for (int i = 0; i < VT_PAGE_BORDER_WIDTH; i++)
            {
                Platform::Memcpy(PageData + i * Layer.NumChannels, srcData, Layer.NumChannels);
            }
            PageData += _Struct.PageResolutionB * Layer.NumChannels;
            srcData += _Struct.PageResolutionB * Layer.NumChannels;
        }
        return;
    }

    VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_Struct, Layer, sourcePageIndex, VirtualTextureLayer::OpenActual, false);
    if (!cachedPage)
    {
        return;
    }

    PageRect rect;

    rect.x = VT_PAGE_BORDER_WIDTH;
    rect.y = VT_PAGE_BORDER_WIDTH;
    rect.width = VT_PAGE_BORDER_WIDTH;
    rect.height = _Struct.PageResolution;

    CopyRect(rect,
             cachedPage->Image.GetData(),
             _Struct.PageResolutionB,
             _Struct.PageResolutionB,
             destPositionX,
             destPositionY,
             PageData,
             _Struct.PageResolutionB,
             _Struct.PageResolutionB,
             Layer.NumChannels);

    VT_CloseCachedPage(cachedPage);
}

void VT_GenerateBorder_UL(VirtualTextureStructure& _Struct, VirtualTextureLayer& Layer, unsigned int _RelativeIndex, int _Lod, byte* PageData)
{
    int x = QuadTreeGetXFromRelative(_RelativeIndex, _Lod) - 1;
    int y = QuadTreeGetYFromRelative(_RelativeIndex, _Lod) - 1;

    unsigned int sourcePageIndex = __GetAbsoluteFromXY(x, y, _Lod);

    bool pageNotExist = x < 0 || y < 0 || !_Struct.PageBitfield.IsMarked(sourcePageIndex);

    int destPositionX = 0;
    int destPositionY = 0;

    if (pageNotExist)
    {
        byte* srcData = PageData + __PageByteOffset(_Struct, Layer, VT_PAGE_BORDER_WIDTH, VT_PAGE_BORDER_WIDTH);

        for (int j = 0; j < VT_PAGE_BORDER_WIDTH; j++)
        {
            for (int i = 0; i < VT_PAGE_BORDER_WIDTH; i++)
            {
                Platform::Memcpy(PageData + (j * _Struct.PageResolutionB + i) * Layer.NumChannels, srcData, Layer.NumChannels);
            }
        }
        return;
    }

    VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_Struct, Layer, sourcePageIndex, VirtualTextureLayer::OpenActual, false);
    if (!cachedPage)
    {
        return;
    }

    PageRect rect;

    rect.x = _Struct.PageResolution;
    rect.y = _Struct.PageResolution;
    rect.width = VT_PAGE_BORDER_WIDTH;
    rect.height = VT_PAGE_BORDER_WIDTH;

    CopyRect(rect,
             cachedPage->Image.GetData(),
             _Struct.PageResolutionB,
             _Struct.PageResolutionB,
             destPositionX,
             destPositionY,
             PageData,
             _Struct.PageResolutionB,
             _Struct.PageResolutionB,
             Layer.NumChannels);

    VT_CloseCachedPage(cachedPage);
}

void VT_GenerateBorder_UR(VirtualTextureStructure& _Struct, VirtualTextureLayer& Layer, unsigned int _RelativeIndex, int _Lod, byte* PageData)
{
    int maxPages = 1 << _Lod;

    int x = QuadTreeGetXFromRelative(_RelativeIndex, _Lod) + 1;
    int y = QuadTreeGetYFromRelative(_RelativeIndex, _Lod) - 1;

    unsigned int sourcePageIndex = __GetAbsoluteFromXY(x, y, _Lod);

    bool pageNotExist = x >= maxPages || y < 0 || !_Struct.PageBitfield.IsMarked(sourcePageIndex);

    int destPositionX = _Struct.PageResolutionB - VT_PAGE_BORDER_WIDTH;
    int destPositionY = 0;

    if (pageNotExist)
    {
        byte* srcData = PageData + __PageByteOffset(_Struct, Layer, _Struct.PageResolutionB - VT_PAGE_BORDER_WIDTH - 1, VT_PAGE_BORDER_WIDTH);

        PageData += __PageByteOffset(_Struct, Layer, destPositionX, destPositionY);

        for (int j = 0; j < VT_PAGE_BORDER_WIDTH; j++)
        {
            for (int i = 0; i < VT_PAGE_BORDER_WIDTH; i++)
            {
                Platform::Memcpy(PageData + (j * _Struct.PageResolutionB + i) * Layer.NumChannels, srcData, Layer.NumChannels);
            }
        }
        return;
    }

    VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_Struct, Layer, sourcePageIndex, VirtualTextureLayer::OpenActual, false);
    if (!cachedPage)
    {
        return;
    }

    PageRect rect;

    rect.x = VT_PAGE_BORDER_WIDTH;
    rect.y = _Struct.PageResolution;
    rect.width = VT_PAGE_BORDER_WIDTH;
    rect.height = VT_PAGE_BORDER_WIDTH;

    CopyRect(rect,
             cachedPage->Image.GetData(),
             _Struct.PageResolutionB,
             _Struct.PageResolutionB,
             destPositionX,
             destPositionY,
             PageData,
             _Struct.PageResolutionB,
             _Struct.PageResolutionB,
             Layer.NumChannels);

    VT_CloseCachedPage(cachedPage);
}

void VT_GenerateBorder_DL(VirtualTextureStructure& _Struct, VirtualTextureLayer& Layer, unsigned int _RelativeIndex, int _Lod, byte* PageData)
{
    int maxPages = 1 << _Lod;

    int x = QuadTreeGetXFromRelative(_RelativeIndex, _Lod) - 1;
    int y = QuadTreeGetYFromRelative(_RelativeIndex, _Lod) + 1;

    unsigned int sourcePageIndex = __GetAbsoluteFromXY(x, y, _Lod);

    bool pageNotExist = x < 0 || y >= maxPages || !_Struct.PageBitfield.IsMarked(sourcePageIndex);

    int destPositionX = 0;
    int destPositionY = _Struct.PageResolutionB - VT_PAGE_BORDER_WIDTH;

    if (pageNotExist)
    {
        byte* srcData = PageData + __PageByteOffset(_Struct, Layer, VT_PAGE_BORDER_WIDTH, _Struct.PageResolutionB - VT_PAGE_BORDER_WIDTH - 1);

        PageData += __PageByteOffset(_Struct, Layer, destPositionX, destPositionY);

        for (int j = 0; j < VT_PAGE_BORDER_WIDTH; j++)
        {
            for (int i = 0; i < VT_PAGE_BORDER_WIDTH; i++)
            {
                Platform::Memcpy(PageData + (j * _Struct.PageResolutionB + i) * Layer.NumChannels, srcData, Layer.NumChannels);
            }
        }
        return;
    }

    VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_Struct, Layer, sourcePageIndex, VirtualTextureLayer::OpenActual, false);
    if (!cachedPage)
    {
        return;
    }

    PageRect rect;

    rect.x = _Struct.PageResolution;
    rect.y = VT_PAGE_BORDER_WIDTH;
    rect.width = VT_PAGE_BORDER_WIDTH;
    rect.height = VT_PAGE_BORDER_WIDTH;

    CopyRect(rect,
             cachedPage->Image.GetData(),
             _Struct.PageResolutionB,
             _Struct.PageResolutionB,
             destPositionX,
             destPositionY,
             PageData,
             _Struct.PageResolutionB,
             _Struct.PageResolutionB,
             Layer.NumChannels);

    VT_CloseCachedPage(cachedPage);
}

void VT_GenerateBorder_DR(VirtualTextureStructure& _Struct, VirtualTextureLayer& Layer, unsigned int _RelativeIndex, int _Lod, byte* PageData)
{
    int maxPages = 1 << _Lod;

    int x = QuadTreeGetXFromRelative(_RelativeIndex, _Lod) + 1;
    int y = QuadTreeGetYFromRelative(_RelativeIndex, _Lod) + 1;

    unsigned int sourcePageIndex = __GetAbsoluteFromXY(x, y, _Lod);

    bool pageNotExist = x >= maxPages || y >= maxPages || !_Struct.PageBitfield.IsMarked(sourcePageIndex);

    int destPositionX = _Struct.PageResolutionB - VT_PAGE_BORDER_WIDTH;
    int destPositionY = _Struct.PageResolutionB - VT_PAGE_BORDER_WIDTH;

    if (pageNotExist)
    {
        byte* srcData = PageData + __PageByteOffset(_Struct, Layer, _Struct.PageResolutionB - VT_PAGE_BORDER_WIDTH - 1, _Struct.PageResolutionB - VT_PAGE_BORDER_WIDTH - 1);

        PageData += __PageByteOffset(_Struct, Layer, destPositionX, destPositionY);

        for (int j = 0; j < VT_PAGE_BORDER_WIDTH; j++)
        {
            for (int i = 0; i < VT_PAGE_BORDER_WIDTH; i++)
            {
                Platform::Memcpy(PageData + (j * _Struct.PageResolutionB + i) * Layer.NumChannels, srcData, Layer.NumChannels);
            }
        }
        return;
    }

    VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_Struct, Layer, sourcePageIndex, VirtualTextureLayer::OpenActual, false);
    if (!cachedPage)
    {
        return;
    }

    PageRect rect;

    rect.x = VT_PAGE_BORDER_WIDTH;
    rect.y = VT_PAGE_BORDER_WIDTH;
    rect.width = VT_PAGE_BORDER_WIDTH;
    rect.height = VT_PAGE_BORDER_WIDTH;

    CopyRect(rect,
             cachedPage->Image.GetData(),
             _Struct.PageResolutionB,
             _Struct.PageResolutionB,
             destPositionX,
             destPositionY,
             PageData,
             _Struct.PageResolutionB,
             _Struct.PageResolutionB,
             Layer.NumChannels);

    VT_CloseCachedPage(cachedPage);
}

void VT_GenerateBordersLod(VirtualTextureStructure& _Struct, VirtualTextureLayer& Layer, int _Lod)
{
    int numLodPages = QuadTreeCalcLodNodes(_Lod);
    unsigned int absoluteIndex = QuadTreeRelativeToAbsoluteIndex(0, _Lod);

    for (int i = 0; i < numLodPages; i++)
    {
        unsigned int pageIndex = absoluteIndex + i;

        if (!_Struct.PageBitfield.IsMarked(pageIndex))
        {
            continue;
        }

        VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_Struct, Layer, pageIndex, VirtualTextureLayer::OpenActual, true);
        if (!cachedPage)
        {
            continue;
        }

        byte* ImageData = cachedPage->Image.GetData();

        // borders
        VT_GenerateBorder_L(_Struct, Layer, i, _Lod, ImageData);
        VT_GenerateBorder_R(_Struct, Layer, i, _Lod, ImageData);
        VT_GenerateBorder_U(_Struct, Layer, i, _Lod, ImageData);
        VT_GenerateBorder_D(_Struct, Layer, i, _Lod, ImageData);

        // corners
        VT_GenerateBorder_UL(_Struct, Layer, i, _Lod, ImageData);
        VT_GenerateBorder_UR(_Struct, Layer, i, _Lod, ImageData);
        VT_GenerateBorder_DL(_Struct, Layer, i, _Lod, ImageData);
        VT_GenerateBorder_DR(_Struct, Layer, i, _Lod, ImageData);

        //WriteImage( HK_FORMAT("page_{}.bmp", pageIndex), _Struct.PageResolutionB, _Struct.PageResolutionB, Layer.NumChannels, cachedPage->Image.GetData() );

        VT_CloseCachedPage(cachedPage);
    }
}

void VT_GenerateBorders(VirtualTextureStructure& _Struct, VirtualTextureLayer& Layer)
{
    for (int i = 0; i < _Struct.NumLods; i++)
    {
        VT_GenerateBordersLod(_Struct, Layer, i);
    }
}

SFileOffset VT_WritePage(VTFileHandle* File, SFileOffset Offset, const VirtualTextureStructure& _Struct, VirtualTextureLayer* _Layers, int _NumLayers, unsigned int _PageIndex)
{
    int CompressedDataSize = 0;
    for (int Layer = 0; Layer < _NumLayers; Layer++)
    {
        CompressedDataSize = Math::Max(CompressedDataSize, _Layers[Layer].SizeInBytes);
    }

    byte* CompressedData = NULL;

    for (int Layer = 0; Layer < _NumLayers; Layer++)
    {
        VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_Struct, _Layers[Layer], _PageIndex, VirtualTextureLayer::OpenActual, false);
        if (!cachedPage)
        {
            LOG("VT_WritePage: couldn't open page Layer {} : {}\n", Layer, _PageIndex);
            Offset += _Layers[Layer].SizeInBytes;
            continue;
        }

        if (_Layers[Layer].PageCompressionMethod)
        {

            if (!CompressedData)
            {
                CompressedData = (byte*)Platform::GetHeapAllocator<HEAP_TEMP>().Alloc(CompressedDataSize);
            }

            _Layers[Layer].PageCompressionMethod(cachedPage->Image.GetData(), CompressedData);

            File->Write(CompressedData, _Layers[Layer].SizeInBytes, Offset);

            //Platform::ZeroMem( CompressedData, _Layers[Layer].SizeInBytes );
            //File->Read( CompressedData, _Layers[Layer].SizeInBytes, Offset );

            //WriteImage( HK_FORMAT("page_{}_{}.bmp", Layer, Offset), 128, 128, 3, CompressedData );
        }
        else
        {
            File->Write(cachedPage->Image.GetData(), _Layers[Layer].SizeInBytes, Offset);
        }


        Offset += _Layers[Layer].SizeInBytes;

        VT_CloseCachedPage(cachedPage);
    }

    Platform::GetHeapAllocator<HEAP_TEMP>().Free(CompressedData);

    return Offset;
}

bool VT_WriteFile(const VirtualTextureStructure& _Struct, int _MaxLods, VirtualTextureLayer* _Layers, int _NumLayers, StringView FileName)
{
    VTFileHandle fileHandle;
    SFileOffset fileOffset;
    VirtualTexturePIT pit;
    VirtualTextureAddressTable addressTable;
    int storedLods;
    uint32_t version = VT_FILE_ID;
    byte tmp;

    Core::CreateDirectory(FileName, true);

    if (!fileHandle.OpenWrite(FileName))
    {
        LOG("VT_WriteFile: couldn't write {}\n", FileName);
        return false;
    }

    int numLods = Math::Min(_Struct.NumLods, _MaxLods);
    unsigned int numQuadTreeNodes = QuadTreeCalcQuadTreeNodes(numLods);

    pit.Create(numQuadTreeNodes);
    pit.Generate(_Struct.PageBitfield, storedLods);

    addressTable.Create(storedLods);
    addressTable.Generate(_Struct.PageBitfield);

    // Пишем заголовок
    fileOffset = 0;

    // write version
    fileHandle.Write(&version, sizeof(version), fileOffset);
    fileOffset += sizeof(version);

    // write num Layers
    tmp = _NumLayers;
    fileHandle.Write(&tmp, sizeof(byte), fileOffset);
    fileOffset += sizeof(byte);

    for (int i = 0; i < _NumLayers; i++)
    {
        fileHandle.Write(&_Layers[i].SizeInBytes, sizeof(_Layers[i].SizeInBytes), fileOffset);
        fileOffset += sizeof(_Layers[i].SizeInBytes);

        fileHandle.Write(&_Layers[i].PageDataFormat, sizeof(_Layers[i].PageDataFormat), fileOffset);
        fileOffset += sizeof(_Layers[i].PageDataFormat);

        //fileHandle.Write( &_Layers[i].NumChannels, sizeof( _Layers[i].NumChannels ), fileOffset );
        //fileOffset += sizeof( _Layers[i].NumChannels );
    }

    // write page width
    fileHandle.Write(&_Struct.PageResolutionB, sizeof(_Struct.PageResolutionB), fileOffset);
    fileOffset += sizeof(_Struct.PageResolutionB);

    // write num lods
    //tmp = _Struct.NumLods;
    //fileHandle.Write( &tmp, sizeof( byte ), fileOffset );
    //fileOffset += sizeof( byte );

    // write stored lods
    //tmp = storedLods;
    //fileHandle.Write( &tmp, sizeof( byte ), fileOffset );
    //fileOffset += sizeof( byte );

    // write page bit field
    //fileOffset += _Struct.PageBitfield.write( &fileHandle, fileOffset );

    // write page info table
    fileOffset += pit.Write(&fileHandle, fileOffset);

    // write page address tables
    fileOffset += addressTable.Write(&fileHandle, fileOffset);

    // Кол-во страниц в LOD'ах от 0 до 4
    unsigned int numFirstPages = Math::Min<unsigned int>(85, addressTable.TotalPages);

    // Записываем страницы LOD'ов 0-4
    for (unsigned int i = 0; i < numFirstPages; i++)
    {
        if (_Struct.PageBitfield.IsMarked(i))
        {
            fileOffset = VT_WritePage(&fileHandle, fileOffset, _Struct, _Layers, _NumLayers, i);
        }
    }

    if (addressTable.TableSize)
    {
        // Записываем остальные страницы
        for (int lodNum = 4; lodNum < addressTable.NumLods; lodNum++)
        {
            int addrTableLod = lodNum - 4;
            unsigned int numNodes = 1 << (addrTableLod + addrTableLod);

            for (unsigned int node = 0; node < numNodes; node++)
            {
                int nodeX = QuadTreeGetXFromRelative(node, addrTableLod);
                int nodeY = QuadTreeGetYFromRelative(node, addrTableLod);
                nodeX <<= 4;
                nodeY <<= 4;

                for (int i = 0; i < 256; i++)
                {
                    unsigned int relativeIndex = QuadTreeGetRelativeFromXY(nodeX + (i & 15), nodeY + (i >> 4), lodNum);
                    unsigned int absoluteIndex = QuadTreeRelativeToAbsoluteIndex(relativeIndex, lodNum);

                    if (_Struct.PageBitfield.IsMarked(absoluteIndex))
                    {
                        fileOffset = VT_WritePage(&fileHandle, fileOffset, _Struct, _Layers, _NumLayers, absoluteIndex);
                    }
                }
            }
        }
    }

    return true;
}

void VT_RemoveHDDData(VirtualTextureStructure& _Struct, VirtualTextureLayer& Layer, bool _SynchPageBitfield, bool _UnmarkRemoved)
{
    String fileName;

    if (_SynchPageBitfield)
    {
        VT_SynchronizePageBitfieldWithHDD(_Struct, Layer);
    }

    for (unsigned int absoluteIndex = 0; absoluteIndex < _Struct.NumQuadTreeNodes; absoluteIndex++)
    {
        if (_Struct.PageBitfield.IsMarked(absoluteIndex))
        {

            if (_UnmarkRemoved)
            {
                _Struct.PageBitfield.Unmark(absoluteIndex);
            }

            int lod = QuadTreeCalcLod64(absoluteIndex);
            unsigned int relativeIndex = QuadTreeAbsoluteToRelativeIndex(absoluteIndex, lod);

            fileName = VT_FileNameFromRelative(Layer.Path.CStr(), relativeIndex, lod);

            Core::RemoveFile(fileName);
        }
    }
}





//static byte __Symbols[10][5][4] = {
//    // 0
//    {
//    { 0,1,1,0 },
//    { 1,0,0,1 },
//    { 1,0,0,1 },
//    { 1,0,0,1 },
//    { 0,1,1,0 }
//    },

//    // 1
//    {
//    { 0,0,1,0 },
//    { 0,1,1,0 },
//    { 1,0,1,0 },
//    { 0,0,1,0 },
//    { 0,0,1,0 }
//    },

//    // 2
//    {
//    { 0,1,1,0 },
//    { 1,0,0,1 },
//    { 0,0,1,0 },
//    { 0,1,0,0 },
//    { 1,1,1,1 }
//    },

//    // 3
//    {
//    { 0,1,1,0 },
//    { 1,0,0,1 },
//    { 0,0,1,0 },
//    { 1,0,0,1 },
//    { 0,1,1,0 }
//    },

//    // 4
//    {
//    { 0,0,1,1 },
//    { 0,1,0,1 },
//    { 1,1,1,1 },
//    { 0,0,0,1 },
//    { 0,0,0,1 }
//    },

//    // 5
//    {
//    { 0,1,1,1 },
//    { 0,1,0,0 },
//    { 0,1,1,0 },
//    { 0,0,1,0 },
//    { 1,1,1,0 }
//    },

//    // 6
//    {
//    { 0,0,1,1 },
//    { 0,1,0,0 },
//    { 1,1,1,1 },
//    { 1,0,0,1 },
//    { 1,1,1,1 }
//    },

//    // 7
//    {
//    { 1,1,1,1 },
//    { 0,0,1,0 },
//    { 0,1,1,1 },
//    { 0,0,1,0 },
//    { 0,0,1,0 }
//    },

//    // 8
//    {
//    { 0,1,1,0 },
//    { 1,0,0,1 },
//    { 0,1,1,0 },
//    { 1,0,0,1 },
//    { 0,1,1,0 }
//    },

//    // 9
//    {
//    { 0,1,1,1 },
//    { 0,1,0,1 },
//    { 0,0,1,1 },
//    { 0,0,1,0 },
//    { 1,1,0,0 }
//    },
//};

//void PrintNum( int _Num, int _X, int _Y, byte * PageData ) {
//    char buff[32];
//    sprintf( buff, "%d", _Num );
//    char * s = buff;
//    while ( *s ) {
//        PrintSymbol( *s, _X, _Y, PageData );
//        _X += 5;
//    }
//}

//void PrintSymbol( char _Symbol, int _X, int _Y, byte * PageData ) {
//    if ( _Symbol < '0' || _Symbol > '9' ) {
//        return;
//    }

//    _Symbol -= '0';

//    byte * src = &__Symbols[_Symbol][0][0];
//}


//static void ComputeTextureRects( const std::vector< ngMeshMaterial > & _materials,
//                                 std::vector< FRectangleBinPack::rectSize_t > & _TextureRects ) {
//    ngImageProperties imageProperties;
//    int numMaterials = _materials.size();

//    _TextureRects.reserve( numMaterials );

//    ngDbg() << "Computing texture rects...";

//    for ( int i = 0 ; i < numMaterials ; i++ ) {
//        const ngMeshMaterial & m = _materials[ i ];

//        ngFileAbstract * file = ngFiles::OpenFileFromUrl( m.diffuseMap.c_str(), ngFileAbstract::M_Read );
//        if ( !file ) {
//            ngDbg() << "No diffuse map";
//            continue;
//        }

//        ngImageFileFormat::Type fileFormat = ngImageUtils::GetImageExtensionFromSignature( file );
//        if ( !ngImageUtils::ReadImageProperties( file, fileFormat, imageProperties ) ) {
//            ngDbg() << "Invalid diffuse map";
//            delete file;
//            continue;
//        }

//        delete file;

//        FRectangleBinPack::rectSize_t rect;
//        rect.width = imageProperties.width;
//        rect.height = imageProperties.height;
//        rect.userdata = ( void * ) (size_t)i;
//        _TextureRects.push_back( rect );
//    }
//}

bool VT_CreateVirtualTexture(const VirtualTextureLayerDesc* _Layers,
                             int _NumLayers,
                             const char* _OutputFileName,
                             const char* _TempDir,
                             int _MaxLods,
                             int _PageWidthLog2,
                             const std::vector<ARectangleBinPack::RectSize>& _TextureRects,
                             std::vector<RectangleBinBack_RectNode>& _BinRects,
                             unsigned int& _BinWidth,
                             unsigned int& _BinHeight,
                             int _MaxCachedPages)
{
    //_MaxCachedPages=1;// FIXME: for debug
    Core::CreateDirectory(_OutputFileName, true);

    std::vector<VirtualTextureLayer> vtLayers(_NumLayers);

    int pageDataNumPixelsB = (1 << _PageWidthLog2) * (1 << _PageWidthLog2);

    for (int LayerIndex = 0; LayerIndex < _NumLayers; LayerIndex++)
    {
        String layerPath = HK_FORMAT("{}/layer{}/", _TempDir, LayerIndex);

        for (int lodIndex = 0; lodIndex < _MaxLods; lodIndex++)
        {
            Core::CreateDirectory(HK_FORMAT("{}{}", layerPath, lodIndex), false);
        }

        vtLayers[LayerIndex].NumCachedPages = 0;
        vtLayers[LayerIndex].MaxCachedPages = _MaxCachedPages;
        vtLayers[LayerIndex].Path = layerPath;
        vtLayers[LayerIndex].NumChannels = _Layers[LayerIndex].NumChannels;

        if (_Layers[LayerIndex].PageCompressionMethod)
        {
            vtLayers[LayerIndex].SizeInBytes = _Layers[LayerIndex].SizeInBytes;
            vtLayers[LayerIndex].PageCompressionMethod = _Layers[LayerIndex].PageCompressionMethod;
        }
        else
        {
            vtLayers[LayerIndex].SizeInBytes = pageDataNumPixelsB * _Layers[LayerIndex].NumChannels;
            vtLayers[LayerIndex].PageCompressionMethod = NULL;
        }

        vtLayers[LayerIndex].PageDataFormat = _Layers[LayerIndex].PageDataFormat;
    }

    VirtualTextureStructure vtStruct;
    if (!VT_MakeStructure(vtStruct, _PageWidthLog2, _TextureRects, _BinRects, _BinWidth, _BinHeight))
    {
        return false;
    }

    int numRects = _BinRects.size();

    for (int rectIndex = 0; rectIndex < numRects; rectIndex++)
    {
        RectangleBinBack_RectNode& rect = _BinRects[rectIndex];

        for (int layerIndex = 0; layerIndex < _NumLayers; layerIndex++)
        {

            void* imageData = _Layers[layerIndex].LoadLayerImage(rect.userdata, rect.width * vtStruct.PageResolution, rect.height * vtStruct.PageResolution);

            if (imageData)
            {

                VT_PutImageIntoPages(vtStruct, vtLayers[layerIndex], rect, (const byte*)imageData);

                _Layers[layerIndex].FreeLayerImage(imageData);
            }
        }
    }
    //VT_FitPageData( vtLayers[ 0],true);// FIXME: for debug
    for (int layerIndex = 0; layerIndex < _NumLayers; layerIndex++)
    {
        VT_MakeLods(vtStruct, vtLayers[layerIndex]);

        //VT_FitPageData( vtLayers[ layerIndex],true);// FIXME: for debug
    }

    for (int layerIndex = 0; layerIndex < _NumLayers; layerIndex++)
    {
        VT_GenerateBorders(vtStruct, vtLayers[layerIndex]);

        //VT_FitPageData( vtLayers[ layerIndex],true);// FIXME: for debug
    }

    if (!VT_WriteFile(vtStruct, _MaxLods, &vtLayers[0], vtLayers.size(), HK_FORMAT("{}.vt3", _OutputFileName)))
    {
        return false;
    }

#if 0
    CreateMinImage( vtCache0, (String( _outputFileName ) + "_0.png").CStr() );
    CreateMinImage( vtCache1, (String( _outputFileName ) + "_1.png").CStr() );
#endif

    for (int layerIndex = 0; layerIndex < _NumLayers; layerIndex++)
    {
        // Запрещаем дамп страниц кеша, которые еще находятся в оперативной памяти
        vtLayers[layerIndex].bAllowDump = false;

        // Удаляем страницы, которые уже были сброшены на диск
        VT_RemoveHDDData(vtStruct, vtLayers[layerIndex], false, false);
    }

    return true;
}

void VT_TransformTextureCoords(float* _TexCoord,
                               unsigned int _NumVerts,
                               int _VertexStride,
                               const RectangleBinBack_RectNode& _BinRect,
                               unsigned int _BinWidth,
                               unsigned int _BinHeight)
{
    double ScaleX = (double)_BinRect.x / _BinWidth;
    double ScaleY = (double)_BinRect.y / _BinHeight;
    double OffsetX = (double)_BinRect.width / _BinWidth;
    double OffsetY = (double)_BinRect.height / _BinHeight;

    for (unsigned int i = 0; i < _NumVerts; i++)
    {
#if 0
        // TODO: transpose!!!
        if ( _BinRect.transposed ) {
            std::swap( _TexCoord[0], _TexCoord[1] );
        }
#endif
        _TexCoord[0] *= ScaleX;
        _TexCoord[1] *= ScaleY;
        _TexCoord[0] += OffsetX;
        _TexCoord[1] += OffsetY;

        _TexCoord = reinterpret_cast<float*>(reinterpret_cast<byte*>(_TexCoord) + _VertexStride);
    }
}






struct TextureLayers
{
    // Inputs
    const char* Diffuse;
    const char* Ambient;
    const char* Specular;
    const char* Normal;
    int Width;
    int Height;

    // Outputs
    Float2 UVScale;
    Float2 UVOffset;
};

void* LoadDiffuseImage(void* _RectUserData, int Width, int Height)
{
    TextureLayers* layers = (TextureLayers*)_RectUserData;

    ImageStorage image = CreateImage(layers->Diffuse, nullptr, IMAGE_STORAGE_FLAGS_DEFAULT, TEXTURE_FORMAT_SRGBA8_UNORM);

    if (!image)
    {
        return nullptr;
    }

    void* pScaledImage = Platform::GetHeapAllocator<HEAP_TEMP>().Alloc(Width * Height * 4);

    // Scale source image to match required width and height
    ImageResampleParams resample;
    resample.pImage = image.GetData();
    resample.Width = image.GetDesc().Width;
    resample.Height = image.GetDesc().Height;
    resample.Format = TEXTURE_FORMAT_SRGBA8_UNORM;
    resample.bHasAlpha = true;
    resample.bPremultipliedAlpha = false;
    resample.HorizontalEdgeMode = IMAGE_RESAMPLE_EDGE_CLAMP;
    resample.VerticalEdgeMode = IMAGE_RESAMPLE_EDGE_CLAMP;
    resample.HorizontalFilter = IMAGE_RESAMPLE_FILTER_MITCHELL;
    resample.VerticalFilter = IMAGE_RESAMPLE_FILTER_MITCHELL;
    resample.ScaledWidth = Width;
    resample.ScaledHeight = Height;
    ResampleImage(resample, pScaledImage);

    return pScaledImage;
}

void FreeImage(void* ImageData)
{
    Platform::GetHeapAllocator<HEAP_TEMP>().Free(ImageData);
}

#define VT_PAGE_SIZE_LOG2 7 // page size 128x128
#define VT_PAGE_SIZE_B    (1 << VT_PAGE_SIZE_LOG2)

void CompressDiffusePage(const void* _InputData, void* _OutputData)
{
    Platform::Memcpy(_OutputData, _InputData, VT_PAGE_SIZE_B * VT_PAGE_SIZE_B * 4);
}

enum VIRTUAL_TEXTURE_PAGE_FORMAT
{
    VIRTUAL_TEXTURE_PAGE_FORMAT_RGBA
};

void TestVT()
{
    VirtualTextureLayerDesc Layers[1];

    // Diffuse layer
    Layers[0].SizeInBytes = VT_PAGE_SIZE_B * VT_PAGE_SIZE_B * 4;
    Layers[0].PageDataFormat = VIRTUAL_TEXTURE_PAGE_FORMAT_RGBA;
    Layers[0].NumChannels = 4;
    Layers[0].LoadLayerImage = LoadDiffuseImage;
    Layers[0].FreeLayerImage = FreeImage;
    Layers[0].PageCompressionMethod = CompressDiffusePage;

    //// Ambient layer
    //Layers[1].SizeInBytes = VT_PAGE_SIZE_B * VT_PAGE_SIZE_B;
    //Layers[1].PageDataFormat = GHI_PF_UByte_R;
    //Layers[1].NumChannels = 1;
    //Layers[1].LoadLayerImage = LoadAmbientImage;
    //Layers[1].FreeLayerImage = FreeImage;
    //Layers[1].PageCompressionMethod = CompressAmbientPage;

    //// Specular layer
    //Layers[2].SizeInBytes = VT_PAGE_SIZE_B * VT_PAGE_SIZE_B;
    //Layers[2].PageDataFormat = GHI_PF_UByte_R;
    //Layers[2].NumChannels = 1;
    //Layers[2].LoadLayerImage = LoadSpecularImage;
    //Layers[2].FreeLayerImage = FreeImage;
    //Layers[2].PageCompressionMethod = CompressSpecularPage;

    //// Normal layer
    //Layers[3].SizeInBytes = VT_PAGE_SIZE_B * VT_PAGE_SIZE_B * 3;
    //Layers[3].PageDataFormat = GHI_PF_UByte_RGB;
    //Layers[3].NumChannels = 3;
    //Layers[3].LoadLayerImage = LoadNormalImage;
    //Layers[3].FreeLayerImage = FreeImage;
    //Layers[3].PageCompressionMethod = CompressNormalPage;

    TextureLayers textureLayers[1];
    int NumTextures = HK_ARRAY_SIZE(textureLayers);

#ifdef HK_OS_LINUX
    textureLayers[0].Diffuse = "vt_test.jpg"; //"D:/portret.png";
    //textureLayers[0].Ambient = "";
    //textureLayers[0].Specular = "";
    //textureLayers[0].Normal = "normal.png";
    textureLayers[0].Width = 1920;  //1240;
    textureLayers[0].Height = 1080; //1416;
#else
    textureLayers[0].Diffuse = "D:/portret.png";
    textureLayers[0].Width = 1240;
    textureLayers[0].Height = 1416;
#endif

    std::vector<ARectangleBinPack::RectSize> inputRects(NumTextures);
    for (unsigned int i = 0; i < NumTextures; i++)
    {
        ARectangleBinPack::RectSize& rect = inputRects[i];

        rect.width = textureLayers[i].Width;
        rect.height = textureLayers[i].Height;
        rect.userdata = &textureLayers[i];
    }

    std::vector<RectangleBinBack_RectNode> outputRects;
    unsigned int BinWidth;
    unsigned int BinHeight;

    VT_CreateVirtualTexture(Layers, HK_ARRAY_SIZE(Layers), "Test", "TmpVT", 11, VT_PAGE_SIZE_LOG2, inputRects, outputRects, BinWidth, BinHeight);

    for (unsigned int i = 0; i < outputRects.size(); i++)
    {
        TextureLayers* layers = (TextureLayers*)outputRects[i].userdata;

        layers->UVOffset.X = (double)outputRects[i].x / BinWidth;
        layers->UVOffset.Y = (double)outputRects[i].y / BinHeight;

        layers->UVScale.X = (double)outputRects[i].width / BinWidth;
        layers->UVScale.Y = (double)outputRects[i].height / BinHeight;
    }
}

HK_NAMESPACE_END
