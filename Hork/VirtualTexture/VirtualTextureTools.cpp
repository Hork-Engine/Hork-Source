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

#include "VirtualTextureTools.h"
#include "QuadTree.h"

#include <Hork/Core/Logger.h>
#include <Hork/Core/WindowsDefs.h>
#include <Hork/Math/VectorMath.h>

#include <Hork/Image/Image.h>

#define PAGE_EXTENSION ".page"

#ifdef HK_OS_LINUX
#    include <sys/stat.h>
#    include <dirent.h>
#endif

HK_NAMESPACE_BEGIN

VirtualTextureImage::~VirtualTextureImage()
{
    Core::GetHeapAllocator<HEAP_TEMP>().Free(m_Data);
}

bool VirtualTextureImage::OpenImage(const char* fileName, int width, int height, int numChannels)
{
    VTFileHandle file;
    if (!file.OpenRead(fileName))
    {
        return false;
    }
    CreateEmpty(width, height, numChannels);
    file.Read(m_Data, width * height * numChannels, 0);
    return true;
}

bool VirtualTextureImage::WriteImage(const char* fileName) const
{
    if (!m_Data)
    {
        return false;
    }
    VTFileHandle file;
    if (!file.OpenWrite(fileName))
    {
        return false;
    }
    file.Write(m_Data, m_Width * m_Height * m_NumChannels, 0);
    return true;
}

void VirtualTextureImage::CreateEmpty(int width, int height, int numChannels)
{
    if (m_Data == NULL || m_Width * m_Height * m_NumChannels != width * height * numChannels)
    {
        m_Data = (byte*)Core::GetHeapAllocator<HEAP_TEMP>().Realloc(m_Data, width * height * numChannels, 16, MALLOC_DISCARD);
    }

    m_Width = width;
    m_Height = height;
    m_NumChannels = numChannels;
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

bool VT_MakeStructure(VirtualTextureStructure& _struct, int pageWidthLog2, const std::vector<RectangleBinPack::RectSize>& textureRects, std::vector<RectangleBinBack_RectNode>& binRects, unsigned int& binWidth, unsigned int& binHeight)
{
    std::vector<RectangleBinPack::RectSize> tempRects;
    double space = 0.0;

    _struct.PageResolutionB = 1 << pageWidthLog2;
    _struct.PageResolution = _struct.PageResolutionB - (VT_PAGE_BORDER_WIDTH << 1);

    binRects.clear();

    int numTextureRectangles = textureRects.size();
    if (!numTextureRectangles)
    {
        LOG("No texture rectangles\n");
        return false;
    }

    tempRects.resize(numTextureRectangles);

    // adjust rect sizes and compute virtual texture space
    for (int i = 0; i < numTextureRectangles; i++)
    {
        const RectangleBinPack::RectSize& in = textureRects[i];
        RectangleBinPack::RectSize& out = tempRects[i];

        // copy rect
        out = in;

        // adjust rect size
        if (out.width % _struct.PageResolution != 0)
        {
            out.width = int(out.width / double(_struct.PageResolution) + 1.0) * _struct.PageResolution;
        }
        if (out.height % _struct.PageResolution != 0)
        {
            out.height = int(out.height / double(_struct.PageResolution) + 1.0) * _struct.PageResolution;
        }

        // compute virtual texture space
        space += (out.width + (VT_PAGE_BORDER_WIDTH << 1)) * (out.height + (VT_PAGE_BORDER_WIDTH << 1));

        // scale pixels to pages
        out.width /= _struct.PageResolution;  // перевести из пикселей в страницы
        out.height /= _struct.PageResolution; // перевести из пикселей в страницы
    }

    _struct.NumLods = Math::Log2(Math::ToGreaterPowerOfTwo((uint32_t)ceilf(sqrtf(space))) / _struct.PageResolutionB) + 1;

    while (1)
    {
        binWidth = binHeight = (1 << (_struct.NumLods - 1));

        RectangleBinPack binPack(binWidth, binHeight);
        std::vector<RectangleBinPack::RectSize> rectVec = tempRects;
        binPack.Insert(rectVec,
                       false,
                       RectangleBinPack::RectBestAreaFit,
                       RectangleBinPack::SplitShorterLeftoverAxis,
                       true);

        if ((int)binPack.GetUsedRectangles().size() == numTextureRectangles)
        {
            binRects = binPack.GetUsedRectangles();
            break;
        }

        _struct.NumLods++;
    }

    //int TextureWidth = ( 1 << ( _struct.NumLods - 1 ) ) * _struct.PageResolutionB;

    _struct.NumQuadTreeNodes = QuadTreeCalcQuadTreeNodes(_struct.NumLods);

    _struct.PageBitfield.ResizeInvalidate(_struct.NumQuadTreeNodes);
    _struct.PageBitfield.UnmarkAll();

    return true;
}

String VT_FileNameFromRelative(const char* outputPath, unsigned int relativeIndex, int lod)
{
    return String(HK_FORMAT("{}{}/{}{}", outputPath, lod, relativeIndex, PAGE_EXTENSION));
}

VirtualTextureLayer::CachedPage* VT_FindInCache(VirtualTextureLayer& layer, unsigned int absoluteIndex)
{
    auto it = layer.Pages.find(absoluteIndex);
    if (it == layer.Pages.end())
    {
        return NULL;
    }
    return it->second;
}

bool VT_DumpPageToDisk(const char* path, unsigned int absoluteIndex, const VirtualTextureImage& image)
{
    String fn;
    int lod;
    unsigned int relativeIndex;

    lod = QuadTreeCalcLod64(absoluteIndex);
    relativeIndex = QuadTreeAbsoluteToRelativeIndex(absoluteIndex, lod);

    fn = VT_FileNameFromRelative(path, relativeIndex, lod);

    return image.WriteImage(fn.CStr());
}

void VT_FitPageData(VirtualTextureLayer& layer, bool forceFit)
{
    if ((!forceFit && layer.NumCachedPages < layer.MaxCachedPages) || layer.MaxCachedPages < 0)
    {
        return;
    }

    int totalDumped = 0;
    int totalCachedPages = layer.NumCachedPages;

    LOG("Fit page data...\n");

    for (auto it = layer.Pages.begin(); it != layer.Pages.end();)
    {
        VirtualTextureLayer::CachedPage* cachedPage = it->second;

        if (cachedPage->Used > 0)
        {
            // now page in use, so keep it in memory
            it++;
            continue;
        }

        if (cachedPage->bNeedToSave && layer.bAllowDump)
        {
            if (totalDumped == 0)
            {
                LOG("Dumping pages to disk...\n");
            }
            if (VT_DumpPageToDisk(layer.Path.CStr(), it->first, cachedPage->Image))
            {
                totalDumped++;
            }
        }

        delete cachedPage;
        it = layer.Pages.erase(it);
        layer.NumCachedPages--;
    }

    LOG("Total dumped pages: {} from {}\n", totalDumped, totalCachedPages);
}

VirtualTextureLayer::CachedPage* VT_OpenCachedPage(const VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int absoluteIndex, VirtualTextureLayer::OpenMode openMode, bool needToSave)
{
    int lod;
    unsigned int relativeIndex;
    String fn;
    VirtualTextureLayer::CachedPage* cachedPage;

    cachedPage = VT_FindInCache(layer, absoluteIndex);
    if (cachedPage)
    {
        if (needToSave)
        {
            cachedPage->bNeedToSave = true;
        }
        cachedPage->Used++;
        return cachedPage;
    }

    VT_FitPageData(layer);

    cachedPage = new VirtualTextureLayer::CachedPage;

    if (openMode == VirtualTextureLayer::OpenEmpty)
    {
        // create empty page
        cachedPage->Image.CreateEmpty(_struct.PageResolutionB, _struct.PageResolutionB, layer.NumChannels);
        int ImageByteSize = _struct.PageResolutionB * _struct.PageResolutionB * layer.NumChannels;
        Core::ZeroMem(cachedPage->Image.GetData(), ImageByteSize);
    }
    else if (openMode == VirtualTextureLayer::OpenActual)
    {
        // open from file
        lod = QuadTreeCalcLod64(absoluteIndex);
        relativeIndex = QuadTreeAbsoluteToRelativeIndex(absoluteIndex, lod);
        fn = VT_FileNameFromRelative(layer.Path.CStr(), relativeIndex, lod);

        bool bOpened = cachedPage->Image.OpenImage(fn.CStr(), _struct.PageResolutionB, _struct.PageResolutionB, layer.NumChannels);
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
    cachedPage->bNeedToSave = needToSave;
    layer.Pages[absoluteIndex] = cachedPage;
    layer.NumCachedPages++;

    return cachedPage;
}

void VT_CloseCachedPage(VirtualTextureLayer::CachedPage* cachedPage)
{
    if (!cachedPage)
    {
        return;
    }
    cachedPage->Used--;
    if (cachedPage->Used < 0)
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

    void CopyRect(const PageRect& rect,
                  const byte* source,
                  int srcWidth,
                  int srcHeight,
                  int dstPositionX,
                  int dstPositionY,
                  byte* dest,
                  int dstWidth,
                  int dstHeight,
                  int elementSize)
    {
        HK_ASSERT_(rect.x + rect.width <= srcWidth && rect.y + rect.height <= srcHeight, "CopyRect");

        HK_ASSERT_(dstPositionX + rect.width <= dstWidth && dstPositionY + rect.height <= dstHeight, "CopyRect");

        const byte* pSource = source + (rect.y * srcWidth + rect.x) * elementSize;
        byte* pDest = dest + (dstPositionY * dstWidth + dstPositionX) * elementSize;

        int rectLineSize = rect.width * elementSize;
        int destStep = dstWidth * elementSize;
        int sourceStep = srcWidth * elementSize;

        for (int y = 0; y < rect.height; y++)
        {
            Core::Memcpy(pDest, pSource, rectLineSize);
            pDest += destStep;
            pSource += sourceStep;
        }
    }
} // namespace

void VT_PutImageIntoPages(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, const RectangleBinBack_RectNode& rect, const byte* layerData)
{
    int numPagesX = rect.width;
    int numPagesY = rect.height;

    PageRect pageRect;

    pageRect.width = _struct.PageResolution;
    pageRect.height = _struct.PageResolution;

    int copyOffsetX = VT_PAGE_BORDER_WIDTH;
    int copyOffsetY = VT_PAGE_BORDER_WIDTH;

    int lod = _struct.NumLods - 1;
    int numVtPages = 1 << lod;

    int LayerWidth = rect.width * _struct.PageResolution;
    int LayerHeight = rect.height * _struct.PageResolution;

    for (int x = 0; x < numPagesX; x++)
    {
        for (int y = 0; y < numPagesY; y++)
        {

            int pageIndexX = rect.x + x;
            int pageIndexY = rect.y + y;

            HK_ASSERT_(pageIndexX < numVtPages, "VT_PutImageIntoPages");
            HK_ASSERT_(pageIndexY < numVtPages, "VT_PutImageIntoPages");
            HK_UNUSED(numVtPages);

            unsigned int relativeIndex = QuadTreeGetRelativeFromXY(pageIndexX, pageIndexY, lod);
            unsigned int absoluteIndex = QuadTreeRelativeToAbsoluteIndex(relativeIndex, lod);

            VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_struct, layer, absoluteIndex, VirtualTextureLayer::OpenEmpty, true);
            if (!cachedPage)
            {
                continue;
            }

            pageRect.x = x * _struct.PageResolution;
            pageRect.y = y * _struct.PageResolution;

            CopyRect(pageRect,
                     layerData,
                     LayerWidth,
                     LayerHeight,
                     copyOffsetX,
                     copyOffsetY,
                     cachedPage->Image.GetData(),
                     _struct.PageResolutionB,
                     _struct.PageResolutionB,
                     layer.NumChannels);

            _struct.PageBitfield.Mark(absoluteIndex);

            //WriteImage( HK_FORMAT("page_{}_{}.bmp",x,y), _struct.PageResolutionB, _struct.PageResolutionB, layer.NumChannels, cachedPage->Image.GetData() );

            VT_CloseCachedPage(cachedPage);
        }
    }
}

bool VT_LoadQuad(const VirtualTextureStructure& _struct,
                 VirtualTextureLayer& layer,
                 unsigned int src00,
                 unsigned int src10,
                 unsigned int src01,
                 unsigned int src11,
                 int sourceLod,
                 VirtualTextureLayer::CachedPage* page[4])
{

    bool b[4];
    unsigned int src[4];

    src[0] = src00;
    src[1] = src01;
    src[2] = src10;
    src[3] = src11;

    for (int i = 0; i < 4; i++)
    {
        unsigned int absoluteIndex = QuadTreeRelativeToAbsoluteIndex(src[i], sourceLod);
        b[i] = _struct.PageBitfield.IsMarked(absoluteIndex);
        page[i] = NULL;
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
            page[i] = VT_OpenCachedPage(_struct, layer, QuadTreeRelativeToAbsoluteIndex(src[i], sourceLod), VirtualTextureLayer::OpenActual, false);
            if (page[i])
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

void VT_Downsample(const VirtualTextureStructure& _struct, VirtualTextureLayer& layer, VirtualTextureLayer::CachedPage* pages[4], byte* downsample)
{
    const int borderOffset = ((VT_PAGE_BORDER_WIDTH)*_struct.PageResolutionB + (VT_PAGE_BORDER_WIDTH)) * layer.NumChannels;
    const byte* src00 = pages[0] ? pages[0]->Image.GetData() + borderOffset : NULL;
    const byte* src01 = pages[1] ? pages[1]->Image.GetData() + borderOffset : NULL;
    const byte* src10 = pages[2] ? pages[2]->Image.GetData() + borderOffset : NULL;
    const byte* src11 = pages[3] ? pages[3]->Image.GetData() + borderOffset : NULL;
    const byte* s;
    byte* d;
    int color;

    downsample += borderOffset;

    for (int ch = 0; ch < layer.NumChannels; ch++)
    {
        for (int y = 0; y < (_struct.PageResolution >> 1); y++)
        {
            for (int x = 0; x < (_struct.PageResolution >> 1); x++)
            {
                color = 0;
                if (src00)
                {
                    s = &src00[(y * _struct.PageResolutionB + x) * layer.NumChannels * 2];

                    color += *(s + ch);

                    s += layer.NumChannels;

                    color += *(s + ch);

                    s += (_struct.PageResolutionB - 1) * layer.NumChannels;

                    color += *(s + ch);

                    s += layer.NumChannels;

                    color += *(s + ch);

                    color >>= 2;
                }

                d = &downsample[(y * _struct.PageResolutionB + x) * layer.NumChannels];

                *(d + ch) = color;

                color = 0;
                if (src10)
                {
                    s = &src10[(y * _struct.PageResolutionB + x) * layer.NumChannels * 2];

                    color += *(s + ch);

                    s += layer.NumChannels;

                    color += *(s + ch);

                    s += (_struct.PageResolutionB - 1) * layer.NumChannels;

                    color += *(s + ch);

                    s += layer.NumChannels;

                    color += *(s + ch);

                    color >>= 2;
                }

                d += (_struct.PageResolution >> 1) * layer.NumChannels;

                *(d + ch) = color;

                color = 0;
                if (src01)
                {
                    s = &src01[(y * _struct.PageResolutionB + x) * layer.NumChannels * 2];

                    color += *(s + ch);

                    s += layer.NumChannels;

                    color += *(s + ch);

                    s += (_struct.PageResolutionB - 1) * layer.NumChannels;

                    color += *(s + ch);

                    s += layer.NumChannels;

                    color += *(s + ch);

                    color >>= 2;
                }

                d = &downsample[((y + (_struct.PageResolution >> 1)) * _struct.PageResolutionB + x) * layer.NumChannels];

                *(d + ch) = color;

                color = 0;
                if (src11)
                {
                    s = &src11[(y * _struct.PageResolutionB + x) * layer.NumChannels * 2];

                    color += *(s + ch);

                    s += layer.NumChannels;

                    color += *(s + ch);

                    s += (_struct.PageResolutionB - 1) * layer.NumChannels;

                    color += *(s + ch);

                    s += layer.NumChannels;

                    color += *(s + ch);

                    color >>= 2;
                }

                d += (_struct.PageResolution >> 1) * layer.NumChannels;

                *(d + ch) = color;
            }
        }
    }
}

void VT_MakeLods(VirtualTextureStructure& _struct, VirtualTextureLayer& layer)
{
    VirtualTextureLayer::CachedPage* pages[4];

    for (int sourceLod = _struct.NumLods - 1; sourceLod > 0; sourceLod--)
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

                if (!VT_LoadQuad(_struct, layer, src00, src10, src01, src11, sourceLod, pages))
                {
                    continue;
                }

                unsigned int dst = QuadTreeGetRelativeFromXY(x >> 1, y >> 1, destLod);
                unsigned int absoluteIndex = QuadTreeRelativeToAbsoluteIndex(dst, destLod);

                VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_struct, layer, absoluteIndex, VirtualTextureLayer::OpenEmpty, true);
                if (!cachedPage)
                {
                    continue;
                }

                VT_Downsample(_struct, layer, pages, cachedPage->Image.GetData());
                _struct.PageBitfield.Mark(absoluteIndex);

                //WriteImage( HK_FORMAT("page_{}.bmp", absoluteIndex), _struct.PageResolutionB, _struct.PageResolutionB, layer.NumChannels, cachedPage->Image.GetData() );

                VT_CloseCachedPage(cachedPage);

                for (int i = 0; i < 4; i++)
                {
                    VT_CloseCachedPage(pages[i]);
                }
            }
        }
    }
}

static void VT_SynchronizePageBitfieldWithHDD_Lod(VTPageBitfield& bitField, int lod, const char* lodPath)
{
#ifdef HK_OS_WIN32
    // TODO: Not tested on Windows
    unsigned int relativeIndex;
    unsigned int absoluteIndex;
    unsigned int validMax = QuadTreeCalcLodNodes(lod);

    HANDLE fh;
    WIN32_FIND_DATAA fd;

    // TODO: Rewrite to support FindFirstFileW

    if ((fh = FindFirstFileA(String(HK_FORMAT("{}*.png", lodPath)).CStr(), &fd)) != INVALID_HANDLE_VALUE)
    {
        do {
            int len = strlen(fd.cFileName);
            if (len < 4)
            {
                // Invalid name
                HK_ASSERT_(0, "Never reached");
                continue;
            }

            if (!Core::Stricmp(&fd.cFileName[len - 4], ".png"))
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

            absoluteIndex = QuadTreeRelativeToAbsoluteIndex(relativeIndex, lod);

            bitField.Mark(absoluteIndex);

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

    if ((dp = opendir(lodPath)) == NULL)
    {
        // directory does not exist
        return;
    }

    unsigned int validMax = QuadTreeCalcLodNodes(lod);

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

        absoluteIndex = QuadTreeRelativeToAbsoluteIndex(relativeIndex, lod);

        bitField.Mark(absoluteIndex);
    }

    closedir(dp);
#endif
}

void VT_SynchronizePageBitfieldWithHDD(VirtualTextureStructure& _struct, VirtualTextureLayer& layer)
{
    String lodPath;

    _struct.PageBitfield.ResizeInvalidate(_struct.NumQuadTreeNodes);
    _struct.PageBitfield.UnmarkAll();

    for (int lod = 0; lod < _struct.NumLods; lod++)
    {
        lodPath = layer.Path + Core::ToString(lod) + "/";

        VT_SynchronizePageBitfieldWithHDD_Lod(_struct.PageBitfield, lod, lodPath.CStr());
    }
}

static HK_FORCEINLINE unsigned int __GetAbsoluteFromXY(int x, int y, int lod)
{
    return QuadTreeRelativeToAbsoluteIndex(QuadTreeGetRelativeFromXY(x, y, lod), lod);
}

static HK_FORCEINLINE int __PageByteOffset(const VirtualTextureStructure& _struct, const VirtualTextureLayer& layer, const int& x, const int& y)
{
    return (y * _struct.PageResolutionB + x) * layer.NumChannels;
}

void VT_GenerateBorder_U(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int relativeIndex, int lod, byte* pageData)
{
    int x = QuadTreeGetXFromRelative(relativeIndex, lod);
    int y = QuadTreeGetYFromRelative(relativeIndex, lod) - 1;

    unsigned int sourcePageIndex = __GetAbsoluteFromXY(x, y, lod);

    bool pageNotExist = y < 0 || !_struct.PageBitfield.IsMarked(sourcePageIndex);

    int destPositionX = VT_PAGE_BORDER_WIDTH;
    int destPositionY = 0;

    if (pageNotExist)
    {
        int dstOffset = __PageByteOffset(_struct, layer, destPositionX, destPositionY);
        int srcOffset = __PageByteOffset(_struct, layer, VT_PAGE_BORDER_WIDTH, VT_PAGE_BORDER_WIDTH);

        for (int i = 0; i < VT_PAGE_BORDER_WIDTH; i++)
        {
            Core::Memcpy(pageData + dstOffset + i * _struct.PageResolutionB * layer.NumChannels, pageData + srcOffset, _struct.PageResolution * layer.NumChannels);
        }
        return;
    }

    VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_struct, layer, sourcePageIndex, VirtualTextureLayer::OpenActual, false);
    if (!cachedPage)
    {
        return;
    }

    PageRect rect;

    rect.x = VT_PAGE_BORDER_WIDTH;
    rect.y = _struct.PageResolution;
    rect.width = _struct.PageResolution;
    rect.height = VT_PAGE_BORDER_WIDTH;

    CopyRect(rect,
             cachedPage->Image.GetData(),
             _struct.PageResolutionB,
             _struct.PageResolutionB,
             destPositionX,
             destPositionY,
             pageData,
             _struct.PageResolutionB,
             _struct.PageResolutionB,
             layer.NumChannels);

    VT_CloseCachedPage(cachedPage);
}

void VT_GenerateBorder_D(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int relativeIndex, int lod, byte* pageData)
{
    int maxPages = 1 << lod;

    int x = QuadTreeGetXFromRelative(relativeIndex, lod);
    int y = QuadTreeGetYFromRelative(relativeIndex, lod) + 1;

    unsigned int sourcePageIndex = __GetAbsoluteFromXY(x, y, lod);

    bool pageNotExist = y >= maxPages || !_struct.PageBitfield.IsMarked(sourcePageIndex);

    int destPositionX = VT_PAGE_BORDER_WIDTH;
    int destPositionY = _struct.PageResolutionB - VT_PAGE_BORDER_WIDTH;

    if (pageNotExist)
    {
        int dstOffset = __PageByteOffset(_struct, layer, destPositionX, destPositionY);
        int srcOffset = __PageByteOffset(_struct, layer, VT_PAGE_BORDER_WIDTH, _struct.PageResolutionB - VT_PAGE_BORDER_WIDTH - 1);

        for (int i = 0; i < VT_PAGE_BORDER_WIDTH; i++)
        {
            Core::Memcpy(pageData + dstOffset + i * _struct.PageResolutionB * layer.NumChannels, pageData + srcOffset, _struct.PageResolution * layer.NumChannels);
        }
        return;
    }

    VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_struct, layer, sourcePageIndex, VirtualTextureLayer::OpenActual, false);
    if (!cachedPage)
    {
        return;
    }

    PageRect rect;

    rect.x = VT_PAGE_BORDER_WIDTH;
    rect.y = VT_PAGE_BORDER_WIDTH;
    rect.width = _struct.PageResolution;
    rect.height = VT_PAGE_BORDER_WIDTH;

    CopyRect(rect,
             cachedPage->Image.GetData(),
             _struct.PageResolutionB,
             _struct.PageResolutionB,
             destPositionX,
             destPositionY,
             pageData,
             _struct.PageResolutionB,
             _struct.PageResolutionB,
             layer.NumChannels);

    VT_CloseCachedPage(cachedPage);
}

void VT_GenerateBorder_L(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int relativeIndex, int lod, byte* pageData)
{
    int x = QuadTreeGetXFromRelative(relativeIndex, lod) - 1;
    int y = QuadTreeGetYFromRelative(relativeIndex, lod);

    unsigned int sourcePageIndex = __GetAbsoluteFromXY(x, y, lod);

    bool pageNotExist = x < 0 || !_struct.PageBitfield.IsMarked(sourcePageIndex);

    int destPositionX = 0;
    int destPositionY = VT_PAGE_BORDER_WIDTH;

    if (pageNotExist)
    {
        pageData += __PageByteOffset(_struct, layer, destPositionX, destPositionY);

        byte* srcData = pageData + VT_PAGE_BORDER_WIDTH * layer.NumChannels;

        for (int j = 0; j < _struct.PageResolution; j++)
        {
            for (int i = 0; i < VT_PAGE_BORDER_WIDTH; i++)
            {
                Core::Memcpy(pageData + i * layer.NumChannels, srcData, layer.NumChannels);
            }
            pageData += _struct.PageResolutionB * layer.NumChannels;
            srcData += _struct.PageResolutionB * layer.NumChannels;
        }
        return;
    }

    VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_struct, layer, sourcePageIndex, VirtualTextureLayer::OpenActual, false);
    if (!cachedPage)
    {
        return;
    }

    PageRect rect;

    rect.x = _struct.PageResolution;
    rect.y = VT_PAGE_BORDER_WIDTH;
    rect.width = VT_PAGE_BORDER_WIDTH;
    rect.height = _struct.PageResolution;

    CopyRect(rect,
             cachedPage->Image.GetData(),
             _struct.PageResolutionB,
             _struct.PageResolutionB,
             destPositionX,
             destPositionY,
             pageData,
             _struct.PageResolutionB,
             _struct.PageResolutionB,
             layer.NumChannels);

    VT_CloseCachedPage(cachedPage);
}

void VT_GenerateBorder_R(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int relativeIndex, int lod, byte* pageData)
{
    int maxPages = 1 << lod;

    int x = QuadTreeGetXFromRelative(relativeIndex, lod) + 1;
    int y = QuadTreeGetYFromRelative(relativeIndex, lod);

    unsigned int sourcePageIndex = __GetAbsoluteFromXY(x, y, lod);

    bool pageNotExist = x >= maxPages || !_struct.PageBitfield.IsMarked(sourcePageIndex);

    int destPositionX = _struct.PageResolution + VT_PAGE_BORDER_WIDTH;
    int destPositionY = VT_PAGE_BORDER_WIDTH;

    if (pageNotExist)
    {
        pageData += __PageByteOffset(_struct, layer, destPositionX, destPositionY);

        byte* srcData = pageData - layer.NumChannels;

        for (int j = 0; j < _struct.PageResolution; j++)
        {
            for (int i = 0; i < VT_PAGE_BORDER_WIDTH; i++)
            {
                Core::Memcpy(pageData + i * layer.NumChannels, srcData, layer.NumChannels);
            }
            pageData += _struct.PageResolutionB * layer.NumChannels;
            srcData += _struct.PageResolutionB * layer.NumChannels;
        }
        return;
    }

    VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_struct, layer, sourcePageIndex, VirtualTextureLayer::OpenActual, false);
    if (!cachedPage)
    {
        return;
    }

    PageRect rect;

    rect.x = VT_PAGE_BORDER_WIDTH;
    rect.y = VT_PAGE_BORDER_WIDTH;
    rect.width = VT_PAGE_BORDER_WIDTH;
    rect.height = _struct.PageResolution;

    CopyRect(rect,
             cachedPage->Image.GetData(),
             _struct.PageResolutionB,
             _struct.PageResolutionB,
             destPositionX,
             destPositionY,
             pageData,
             _struct.PageResolutionB,
             _struct.PageResolutionB,
             layer.NumChannels);

    VT_CloseCachedPage(cachedPage);
}

void VT_GenerateBorder_UL(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int relativeIndex, int lod, byte* pageData)
{
    int x = QuadTreeGetXFromRelative(relativeIndex, lod) - 1;
    int y = QuadTreeGetYFromRelative(relativeIndex, lod) - 1;

    unsigned int sourcePageIndex = __GetAbsoluteFromXY(x, y, lod);

    bool pageNotExist = x < 0 || y < 0 || !_struct.PageBitfield.IsMarked(sourcePageIndex);

    int destPositionX = 0;
    int destPositionY = 0;

    if (pageNotExist)
    {
        byte* srcData = pageData + __PageByteOffset(_struct, layer, VT_PAGE_BORDER_WIDTH, VT_PAGE_BORDER_WIDTH);

        for (int j = 0; j < VT_PAGE_BORDER_WIDTH; j++)
        {
            for (int i = 0; i < VT_PAGE_BORDER_WIDTH; i++)
            {
                Core::Memcpy(pageData + (j * _struct.PageResolutionB + i) * layer.NumChannels, srcData, layer.NumChannels);
            }
        }
        return;
    }

    VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_struct, layer, sourcePageIndex, VirtualTextureLayer::OpenActual, false);
    if (!cachedPage)
    {
        return;
    }

    PageRect rect;

    rect.x = _struct.PageResolution;
    rect.y = _struct.PageResolution;
    rect.width = VT_PAGE_BORDER_WIDTH;
    rect.height = VT_PAGE_BORDER_WIDTH;

    CopyRect(rect,
             cachedPage->Image.GetData(),
             _struct.PageResolutionB,
             _struct.PageResolutionB,
             destPositionX,
             destPositionY,
             pageData,
             _struct.PageResolutionB,
             _struct.PageResolutionB,
             layer.NumChannels);

    VT_CloseCachedPage(cachedPage);
}

void VT_GenerateBorder_UR(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int relativeIndex, int lod, byte* pageData)
{
    int maxPages = 1 << lod;

    int x = QuadTreeGetXFromRelative(relativeIndex, lod) + 1;
    int y = QuadTreeGetYFromRelative(relativeIndex, lod) - 1;

    unsigned int sourcePageIndex = __GetAbsoluteFromXY(x, y, lod);

    bool pageNotExist = x >= maxPages || y < 0 || !_struct.PageBitfield.IsMarked(sourcePageIndex);

    int destPositionX = _struct.PageResolutionB - VT_PAGE_BORDER_WIDTH;
    int destPositionY = 0;

    if (pageNotExist)
    {
        byte* srcData = pageData + __PageByteOffset(_struct, layer, _struct.PageResolutionB - VT_PAGE_BORDER_WIDTH - 1, VT_PAGE_BORDER_WIDTH);

        pageData += __PageByteOffset(_struct, layer, destPositionX, destPositionY);

        for (int j = 0; j < VT_PAGE_BORDER_WIDTH; j++)
        {
            for (int i = 0; i < VT_PAGE_BORDER_WIDTH; i++)
            {
                Core::Memcpy(pageData + (j * _struct.PageResolutionB + i) * layer.NumChannels, srcData, layer.NumChannels);
            }
        }
        return;
    }

    VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_struct, layer, sourcePageIndex, VirtualTextureLayer::OpenActual, false);
    if (!cachedPage)
    {
        return;
    }

    PageRect rect;

    rect.x = VT_PAGE_BORDER_WIDTH;
    rect.y = _struct.PageResolution;
    rect.width = VT_PAGE_BORDER_WIDTH;
    rect.height = VT_PAGE_BORDER_WIDTH;

    CopyRect(rect,
             cachedPage->Image.GetData(),
             _struct.PageResolutionB,
             _struct.PageResolutionB,
             destPositionX,
             destPositionY,
             pageData,
             _struct.PageResolutionB,
             _struct.PageResolutionB,
             layer.NumChannels);

    VT_CloseCachedPage(cachedPage);
}

void VT_GenerateBorder_DL(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int relativeIndex, int lod, byte* pageData)
{
    int maxPages = 1 << lod;

    int x = QuadTreeGetXFromRelative(relativeIndex, lod) - 1;
    int y = QuadTreeGetYFromRelative(relativeIndex, lod) + 1;

    unsigned int sourcePageIndex = __GetAbsoluteFromXY(x, y, lod);

    bool pageNotExist = x < 0 || y >= maxPages || !_struct.PageBitfield.IsMarked(sourcePageIndex);

    int destPositionX = 0;
    int destPositionY = _struct.PageResolutionB - VT_PAGE_BORDER_WIDTH;

    if (pageNotExist)
    {
        byte* srcData = pageData + __PageByteOffset(_struct, layer, VT_PAGE_BORDER_WIDTH, _struct.PageResolutionB - VT_PAGE_BORDER_WIDTH - 1);

        pageData += __PageByteOffset(_struct, layer, destPositionX, destPositionY);

        for (int j = 0; j < VT_PAGE_BORDER_WIDTH; j++)
        {
            for (int i = 0; i < VT_PAGE_BORDER_WIDTH; i++)
            {
                Core::Memcpy(pageData + (j * _struct.PageResolutionB + i) * layer.NumChannels, srcData, layer.NumChannels);
            }
        }
        return;
    }

    VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_struct, layer, sourcePageIndex, VirtualTextureLayer::OpenActual, false);
    if (!cachedPage)
    {
        return;
    }

    PageRect rect;

    rect.x = _struct.PageResolution;
    rect.y = VT_PAGE_BORDER_WIDTH;
    rect.width = VT_PAGE_BORDER_WIDTH;
    rect.height = VT_PAGE_BORDER_WIDTH;

    CopyRect(rect,
             cachedPage->Image.GetData(),
             _struct.PageResolutionB,
             _struct.PageResolutionB,
             destPositionX,
             destPositionY,
             pageData,
             _struct.PageResolutionB,
             _struct.PageResolutionB,
             layer.NumChannels);

    VT_CloseCachedPage(cachedPage);
}

void VT_GenerateBorder_DR(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int relativeIndex, int lod, byte* pageData)
{
    int maxPages = 1 << lod;

    int x = QuadTreeGetXFromRelative(relativeIndex, lod) + 1;
    int y = QuadTreeGetYFromRelative(relativeIndex, lod) + 1;

    unsigned int sourcePageIndex = __GetAbsoluteFromXY(x, y, lod);

    bool pageNotExist = x >= maxPages || y >= maxPages || !_struct.PageBitfield.IsMarked(sourcePageIndex);

    int destPositionX = _struct.PageResolutionB - VT_PAGE_BORDER_WIDTH;
    int destPositionY = _struct.PageResolutionB - VT_PAGE_BORDER_WIDTH;

    if (pageNotExist)
    {
        byte* srcData = pageData + __PageByteOffset(_struct, layer, _struct.PageResolutionB - VT_PAGE_BORDER_WIDTH - 1, _struct.PageResolutionB - VT_PAGE_BORDER_WIDTH - 1);

        pageData += __PageByteOffset(_struct, layer, destPositionX, destPositionY);

        for (int j = 0; j < VT_PAGE_BORDER_WIDTH; j++)
        {
            for (int i = 0; i < VT_PAGE_BORDER_WIDTH; i++)
            {
                Core::Memcpy(pageData + (j * _struct.PageResolutionB + i) * layer.NumChannels, srcData, layer.NumChannels);
            }
        }
        return;
    }

    VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_struct, layer, sourcePageIndex, VirtualTextureLayer::OpenActual, false);
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
             _struct.PageResolutionB,
             _struct.PageResolutionB,
             destPositionX,
             destPositionY,
             pageData,
             _struct.PageResolutionB,
             _struct.PageResolutionB,
             layer.NumChannels);

    VT_CloseCachedPage(cachedPage);
}

void VT_GenerateBordersLod(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, int lod)
{
    int numLodPages = QuadTreeCalcLodNodes(lod);
    unsigned int absoluteIndex = QuadTreeRelativeToAbsoluteIndex(0, lod);

    for (int i = 0; i < numLodPages; i++)
    {
        unsigned int pageIndex = absoluteIndex + i;

        if (!_struct.PageBitfield.IsMarked(pageIndex))
        {
            continue;
        }

        VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_struct, layer, pageIndex, VirtualTextureLayer::OpenActual, true);
        if (!cachedPage)
        {
            continue;
        }

        byte* ImageData = cachedPage->Image.GetData();

        // borders
        VT_GenerateBorder_L(_struct, layer, i, lod, ImageData);
        VT_GenerateBorder_R(_struct, layer, i, lod, ImageData);
        VT_GenerateBorder_U(_struct, layer, i, lod, ImageData);
        VT_GenerateBorder_D(_struct, layer, i, lod, ImageData);

        // corners
        VT_GenerateBorder_UL(_struct, layer, i, lod, ImageData);
        VT_GenerateBorder_UR(_struct, layer, i, lod, ImageData);
        VT_GenerateBorder_DL(_struct, layer, i, lod, ImageData);
        VT_GenerateBorder_DR(_struct, layer, i, lod, ImageData);

        //WriteImage( HK_FORMAT("page_{}.bmp", pageIndex), _struct.PageResolutionB, _struct.PageResolutionB, layer.NumChannels, cachedPage->Image.GetData() );

        VT_CloseCachedPage(cachedPage);
    }
}

void VT_GenerateBorders(VirtualTextureStructure& _struct, VirtualTextureLayer& layer)
{
    for (int i = 0; i < _struct.NumLods; i++)
    {
        VT_GenerateBordersLod(_struct, layer, i);
    }
}

VTFileOffset VT_WritePage(VTFileHandle* file, VTFileOffset offset, const VirtualTextureStructure& _struct, VirtualTextureLayer* layers, int numLayers, unsigned int pageIndex)
{
    int CompressedDataSize = 0;
    for (int layer = 0; layer < numLayers; layer++)
    {
        CompressedDataSize = Math::Max(CompressedDataSize, layers[layer].SizeInBytes);
    }

    byte* CompressedData = NULL;

    for (int layer = 0; layer < numLayers; layer++)
    {
        VirtualTextureLayer::CachedPage* cachedPage = VT_OpenCachedPage(_struct, layers[layer], pageIndex, VirtualTextureLayer::OpenActual, false);
        if (!cachedPage)
        {
            LOG("VT_WritePage: couldn't open page layer {} : {}\n", layer, pageIndex);
            offset += layers[layer].SizeInBytes;
            continue;
        }

        if (layers[layer].PageCompressionMethod)
        {

            if (!CompressedData)
            {
                CompressedData = (byte*)Core::GetHeapAllocator<HEAP_TEMP>().Alloc(CompressedDataSize);
            }

            layers[layer].PageCompressionMethod(cachedPage->Image.GetData(), CompressedData);

            file->Write(CompressedData, layers[layer].SizeInBytes, offset);

            //Core::ZeroMem( CompressedData, layers[layer].SizeInBytes );
            //file->Read( CompressedData, layers[layer].SizeInBytes, offset );

            //WriteImage( HK_FORMAT("page_{}_{}.bmp", layer, offset), 128, 128, 3, CompressedData );
        }
        else
        {
            file->Write(cachedPage->Image.GetData(), layers[layer].SizeInBytes, offset);
        }


        offset += layers[layer].SizeInBytes;

        VT_CloseCachedPage(cachedPage);
    }

    Core::GetHeapAllocator<HEAP_TEMP>().Free(CompressedData);

    return offset;
}

bool VT_WriteFile(const VirtualTextureStructure& _struct, int maxLods, VirtualTextureLayer* layers, int numLayers, StringView fileName)
{
    VTFileHandle fileHandle;
    VTFileOffset fileOffset;
    VirtualTexturePIT pit;
    VirtualTextureAddressTable addressTable;
    int storedLods;
    uint32_t version = VT_FILE_ID;
    byte tmp;

    Core::CreateDirectory(fileName, true);

    if (!fileHandle.OpenWrite(fileName))
    {
        LOG("VT_WriteFile: couldn't write {}\n", fileName);
        return false;
    }

    int numLods = Math::Min(_struct.NumLods, maxLods);
    unsigned int numQuadTreeNodes = QuadTreeCalcQuadTreeNodes(numLods);

    pit.Create(numQuadTreeNodes);
    pit.Generate(_struct.PageBitfield, storedLods);

    addressTable.Create(storedLods);
    addressTable.Generate(_struct.PageBitfield);

    // Пишем заголовок
    fileOffset = 0;

    // write version
    fileHandle.Write(&version, sizeof(version), fileOffset);
    fileOffset += sizeof(version);

    // write num Layers
    tmp = numLayers;
    fileHandle.Write(&tmp, sizeof(byte), fileOffset);
    fileOffset += sizeof(byte);

    for (int i = 0; i < numLayers; i++)
    {
        fileHandle.Write(&layers[i].SizeInBytes, sizeof(layers[i].SizeInBytes), fileOffset);
        fileOffset += sizeof(layers[i].SizeInBytes);

        fileHandle.Write(&layers[i].PageDataFormat, sizeof(layers[i].PageDataFormat), fileOffset);
        fileOffset += sizeof(layers[i].PageDataFormat);

        //fileHandle.Write( &layers[i].NumChannels, sizeof( layers[i].NumChannels ), fileOffset );
        //fileOffset += sizeof( layers[i].NumChannels );
    }

    // write page width
    fileHandle.Write(&_struct.PageResolutionB, sizeof(_struct.PageResolutionB), fileOffset);
    fileOffset += sizeof(_struct.PageResolutionB);

    // write num lods
    //tmp = _struct.NumLods;
    //fileHandle.Write( &tmp, sizeof( byte ), fileOffset );
    //fileOffset += sizeof( byte );

    // write stored lods
    //tmp = storedLods;
    //fileHandle.Write( &tmp, sizeof( byte ), fileOffset );
    //fileOffset += sizeof( byte );

    // write page bit field
    //fileOffset += _struct.PageBitfield.write( &fileHandle, fileOffset );

    // write page info table
    fileOffset += pit.Write(&fileHandle, fileOffset);

    // write page address tables
    fileOffset += addressTable.Write(&fileHandle, fileOffset);

    // Кол-во страниц в LOD'ах от 0 до 4
    unsigned int numFirstPages = Math::Min<unsigned int>(85, addressTable.TotalPages);

    // Записываем страницы LOD'ов 0-4
    for (unsigned int i = 0; i < numFirstPages; i++)
    {
        if (_struct.PageBitfield.IsMarked(i))
        {
            fileOffset = VT_WritePage(&fileHandle, fileOffset, _struct, layers, numLayers, i);
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

                    if (_struct.PageBitfield.IsMarked(absoluteIndex))
                    {
                        fileOffset = VT_WritePage(&fileHandle, fileOffset, _struct, layers, numLayers, absoluteIndex);
                    }
                }
            }
        }
    }

    return true;
}

void VT_RemoveHDDData(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, bool synchPageBitfield, bool unmarkRemoved)
{
    String fileName;

    if (synchPageBitfield)
    {
        VT_SynchronizePageBitfieldWithHDD(_struct, layer);
    }

    for (unsigned int absoluteIndex = 0; absoluteIndex < _struct.NumQuadTreeNodes; absoluteIndex++)
    {
        if (_struct.PageBitfield.IsMarked(absoluteIndex))
        {

            if (unmarkRemoved)
            {
                _struct.PageBitfield.Unmark(absoluteIndex);
            }

            int lod = QuadTreeCalcLod64(absoluteIndex);
            unsigned int relativeIndex = QuadTreeAbsoluteToRelativeIndex(absoluteIndex, lod);

            fileName = VT_FileNameFromRelative(layer.Path.CStr(), relativeIndex, lod);

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

//void PrintNum( int num, int x, int y, byte * pageData ) {
//    char buff[32];
//    sprintf( buff, "%d", num );
//    char * s = buff;
//    while ( *s ) {
//        PrintSymbol( *s, x, y, pageData );
//        x += 5;
//    }
//}

//void PrintSymbol( char symbol, int x, int y, byte * pageData ) {
//    if ( symbol < '0' || symbol > '9' ) {
//        return;
//    }

//    symbol -= '0';

//    byte * src = &__Symbols[symbol][0][0];
//}


//static void ComputeTextureRects( const std::vector< ngMeshMaterial > & _materials,
//                                 std::vector< FRectangleBinPack::rectSize_t > & textureRects ) {
//    ngImageProperties imageProperties;
//    int numMaterials = _materials.size();

//    textureRects.reserve( numMaterials );

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
//        textureRects.push_back( rect );
//    }
//}

bool VT_CreateVirtualTexture(const VirtualTextureLayerDesc* layers,
                             int numLayers,
                             const char* outputFileName,
                             const char* tempDir,
                             int maxLods,
                             int pageWidthLog2,
                             const std::vector<RectangleBinPack::RectSize>& textureRects,
                             std::vector<RectangleBinBack_RectNode>& binRects,
                             unsigned int& binWidth,
                             unsigned int& binHeight,
                             int maxCachedPages)
{
    //maxCachedPages=1;// FIXME: for debug
    Core::CreateDirectory(outputFileName, true);

    std::vector<VirtualTextureLayer> vtLayers(numLayers);

    int pageDataNumPixelsB = (1 << pageWidthLog2) * (1 << pageWidthLog2);

    for (int LayerIndex = 0; LayerIndex < numLayers; LayerIndex++)
    {
        String layerPath(HK_FORMAT("{}/layer{}/", tempDir, LayerIndex));

        for (int lodIndex = 0; lodIndex < maxLods; lodIndex++)
        {
            Core::CreateDirectory(HK_FORMAT("{}{}", layerPath, lodIndex), false);
        }

        vtLayers[LayerIndex].NumCachedPages = 0;
        vtLayers[LayerIndex].MaxCachedPages = maxCachedPages;
        vtLayers[LayerIndex].Path = layerPath;
        vtLayers[LayerIndex].NumChannels = layers[LayerIndex].NumChannels;

        if (layers[LayerIndex].PageCompressionMethod)
        {
            vtLayers[LayerIndex].SizeInBytes = layers[LayerIndex].SizeInBytes;
            vtLayers[LayerIndex].PageCompressionMethod = layers[LayerIndex].PageCompressionMethod;
        }
        else
        {
            vtLayers[LayerIndex].SizeInBytes = pageDataNumPixelsB * layers[LayerIndex].NumChannels;
            vtLayers[LayerIndex].PageCompressionMethod = NULL;
        }

        vtLayers[LayerIndex].PageDataFormat = layers[LayerIndex].PageDataFormat;
    }

    VirtualTextureStructure vtStruct;
    if (!VT_MakeStructure(vtStruct, pageWidthLog2, textureRects, binRects, binWidth, binHeight))
    {
        return false;
    }

    int numRects = binRects.size();

    for (int rectIndex = 0; rectIndex < numRects; rectIndex++)
    {
        RectangleBinBack_RectNode& rect = binRects[rectIndex];

        for (int layerIndex = 0; layerIndex < numLayers; layerIndex++)
        {

            void* imageData = layers[layerIndex].LoadLayerImage(rect.userdata, rect.width * vtStruct.PageResolution, rect.height * vtStruct.PageResolution);

            if (imageData)
            {

                VT_PutImageIntoPages(vtStruct, vtLayers[layerIndex], rect, (const byte*)imageData);

                layers[layerIndex].FreeLayerImage(imageData);
            }
        }
    }
    //VT_FitPageData( vtLayers[ 0],true);// FIXME: for debug
    for (int layerIndex = 0; layerIndex < numLayers; layerIndex++)
    {
        VT_MakeLods(vtStruct, vtLayers[layerIndex]);

        //VT_FitPageData( vtLayers[ layerIndex],true);// FIXME: for debug
    }

    for (int layerIndex = 0; layerIndex < numLayers; layerIndex++)
    {
        VT_GenerateBorders(vtStruct, vtLayers[layerIndex]);

        //VT_FitPageData( vtLayers[ layerIndex],true);// FIXME: for debug
    }

    if (!VT_WriteFile(vtStruct, maxLods, &vtLayers[0], vtLayers.size(), HK_FORMAT("{}.vt3", outputFileName)))
    {
        return false;
    }

#if 0
    CreateMinImage( vtCache0, (String( _outputFileName ) + "_0.png").CStr() );
    CreateMinImage( vtCache1, (String( _outputFileName ) + "_1.png").CStr() );
#endif

    for (int layerIndex = 0; layerIndex < numLayers; layerIndex++)
    {
        // Запрещаем дамп страниц кеша, которые еще находятся в оперативной памяти
        vtLayers[layerIndex].bAllowDump = false;

        // Удаляем страницы, которые уже были сброшены на диск
        VT_RemoveHDDData(vtStruct, vtLayers[layerIndex], false, false);
    }

    return true;
}

void VT_TransformTextureCoords(float* texCoord,
                               unsigned int numVerts,
                               int vertexStride,
                               const RectangleBinBack_RectNode& binRect,
                               unsigned int binWidth,
                               unsigned int binHeight)
{
    double ScaleX = (double)binRect.x / binWidth;
    double ScaleY = (double)binRect.y / binHeight;
    double OffsetX = (double)binRect.width / binWidth;
    double OffsetY = (double)binRect.height / binHeight;

    for (unsigned int i = 0; i < numVerts; i++)
    {
#if 0
        // TODO: transpose!!!
        if ( binRect.transposed ) {
            std::swap( texCoord[0], texCoord[1] );
        }
#endif
        texCoord[0] *= ScaleX;
        texCoord[1] *= ScaleY;
        texCoord[0] += OffsetX;
        texCoord[1] += OffsetY;

        texCoord = reinterpret_cast<float*>(reinterpret_cast<byte*>(texCoord) + vertexStride);
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

void* LoadDiffuseImage(void* rectUserData, int width, int height)
{
    TextureLayers* layers = (TextureLayers*)rectUserData;

    ImageStorage image = CreateImage(layers->Diffuse, nullptr, IMAGE_STORAGE_FLAGS_DEFAULT, TEXTURE_FORMAT_SRGBA8_UNORM);

    if (!image)
    {
        return nullptr;
    }

    void* scaledImage = Core::GetHeapAllocator<HEAP_TEMP>().Alloc(width * height * 4);

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
    resample.ScaledWidth = width;
    resample.ScaledHeight = height;
    ResampleImage(resample, scaledImage);

    return scaledImage;
}

void FreeImage(void* imageData)
{
    Core::GetHeapAllocator<HEAP_TEMP>().Free(imageData);
}

#define VT_PAGE_SIZE_LOG2 7 // page size 128x128
#define VT_PAGE_SIZE_B    (1 << VT_PAGE_SIZE_LOG2)

void CompressDiffusePage(const void* inputData, void* outputData)
{
    Core::Memcpy(outputData, inputData, VT_PAGE_SIZE_B * VT_PAGE_SIZE_B * 4);
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

    std::vector<RectangleBinPack::RectSize> inputRects(NumTextures);
    for (unsigned int i = 0; i < NumTextures; i++)
    {
        RectangleBinPack::RectSize& rect = inputRects[i];

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
