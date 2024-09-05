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

#include "VT.h"
#include "RectangleBinPack.h"

#include <Hork/Core/Containers/Hash.h>

HK_NAMESPACE_BEGIN

struct VirtualTextureStructure
{
    int PageResolutionB;
    int PageResolution;
    int NumLods;
    unsigned int NumQuadTreeNodes;
    VTPageBitfield PageBitfield;
};

struct VirtualTextureImage
{
    VirtualTextureImage();
    ~VirtualTextureImage();

    bool OpenImage(const char* _FileName, int _Width, int _Height, int _NumChannels);
    bool WriteImage(const char* _FileName) const;
    void CreateEmpty(int _Width, int _Height, int _NumChannels);
    byte* GetData() { return Data; }
    const byte* GetData() const { return Data; }
    int GetWidth() const { return Width; }
    int GetHeight() const { return Height; }
    int GetNumChannels() const { return NumChannels; }

private:
    byte* Data;
    int NumChannels;
    int Width;
    int Height;
};

struct VirtualTextureLayer
{
    VirtualTextureLayer();
    ~VirtualTextureLayer();

    // Параметры открытия страницы VT из кеша
    enum OpenMode
    {
        OpenEmpty, // Если страницы нет в кеше, то создает пустую
        OpenActual // Если страницы нет в кеше, то читает из файла. Если файл не удалось открыть, то возвращает NULL
    };

    struct CachedPage
    {

        void* operator new(size_t size)
        {
            return Core::GetHeapAllocator<HEAP_MISC>().Alloc(size);
        }

        void operator delete(void* p)
        {
            Core::GetHeapAllocator<HEAP_MISC>().Free(p);
        }

        VirtualTextureImage Image;
        bool bNeedToSave;
        int Used;
    };

    String Path;

    int MaxCachedPages; // Максимальное количество закешированных страниц.
                        // Если значение < 0, то размер кеша ограничивается
                        // объемом оперативной памяти компьютера
    int NumCachedPages; // Текущее количество закешированных страниц

    bool bAllowDump; // Сохранять страницы на HDD, при переполнении кеша

    int NumChannels;
    int SizeInBytes; // Размер страницы в байтах после компрессии
    int PageDataFormat;

    void (*PageCompressionMethod)(const void* _InputData, void* _OutputData);

    HashMap<unsigned int, CachedPage*> Pages;
};

// Создает структуру виртуальной текстуры, на выходе _struct и binRects
bool VT_MakeStructure(VirtualTextureStructure& _struct,
                      int _PageWidthLog2,
                      const std::vector<RectangleBinPack::RectSize>& _textureRects,
                      std::vector<RectangleBinBack_RectNode>& _binRects,
                      unsigned int& _binWidth,
                      unsigned int& _binHeight);

// Открывает страницу VT из кеша
VirtualTextureLayer::CachedPage* VT_OpenCachedPage(const VirtualTextureStructure& _Struct, VirtualTextureLayer& _cache, unsigned int _absoluteIndex, VirtualTextureLayer::OpenMode _openMode, bool _needToSave);

// Закрывает страницу VT из кеша
void VT_CloseCachedPage(VirtualTextureLayer::CachedPage* _cachedPage);

// Поиск страницы в кеше
VirtualTextureLayer::CachedPage* VT_FindInCache(VirtualTextureLayer& _cache, unsigned int _absoluteIndex);

// Чистка кеша, сброс (запись) страниц кеша на диск в случае переполнения
// если _forceFit = true, то запись страниц кеша на диск в любом случае.
// Если страница кеша в текущий момент открыта, то она остается в кеше в любом случае.
void VT_FitPageData(VirtualTextureLayer& _cache, bool _forceFit = false);

// Получает имя файла страницы по относительному индексу страницы и лоду
String VT_FileNameFromRelative(const char* _outputPath, unsigned int _relativeIndex, int _lod);

// Сохраняет страницу на диск
bool VT_DumpPageToDisk(const char* _path, unsigned int _absoluteIndex, const VirtualTextureImage& _image);

// Режет входное изображение (_LayerData) на страницы и сохраняет в кеше
void VT_PutImageIntoPages(VirtualTextureStructure& _struct,
                          VirtualTextureLayer& _cache,
                          const RectangleBinBack_RectNode& _rect,
                          const byte* _LayerData);

// Загружает заданные четыре страницы для последующего лодирования
bool VT_LoadQuad(const VirtualTextureStructure& _struct,
                 VirtualTextureLayer& _cache,
                 unsigned int _src00,
                 unsigned int _src10,
                 unsigned int _src01,
                 unsigned int _src11,
                 int _sourceLod,
                 VirtualTextureLayer::CachedPage* _page[4]);

// Лодирование четырех страниц
void VT_Downsample(const VirtualTextureStructure& _Struct, VirtualTextureLayer::CachedPage* _pages[4], byte* _downsample);

// Создает лоды VT
void VT_MakeLods(VirtualTextureStructure& _struct, VirtualTextureLayer& _cache);

// Синхронизирует page bitfield с жестким диском (заново заполняет pageBitfield на основе страниц,
// хранящихся на диске
void VT_SynchronizePageBitfieldWithHDD(VirtualTextureStructure& _struct, VirtualTextureLayer& _cache);

// Удаляет страницы кеша, сброшенные на диск
// Если _synchPageBitfield = true, то перед удалением обновляется pageBitfield в соответствии с данными,
// хранящимися на диске
// Если _unmarkRemoved = true, то при удалении страницы с диска, снимается бит с pageBitfield-а
void VT_RemoveHDDData(VirtualTextureStructure& _struct, VirtualTextureLayer& _cache, bool _synchPageBitfield = true, bool _unmarkRemoved = true);

// Функции создания бордеров
void VT_GenerateBorder_U(VirtualTextureStructure& _struct, VirtualTextureLayer& _cache, unsigned int _relativeIndex, int _lod, byte* _pageData);
void VT_GenerateBorder_D(VirtualTextureStructure& _struct, VirtualTextureLayer& _cache, unsigned int _relativeIndex, int _lod, byte* _pageData);
void VT_GenerateBorder_L(VirtualTextureStructure& _struct, VirtualTextureLayer& _cache, unsigned int _relativeIndex, int _lod, byte* _pageData);
void VT_GenerateBorder_R(VirtualTextureStructure& _struct, VirtualTextureLayer& _cache, unsigned int _relativeIndex, int _lod, byte* _pageData);
void VT_GenerateBorder_UL(VirtualTextureStructure& _struct, VirtualTextureLayer& _cache, unsigned int _relativeIndex, int _lod, byte* _pageData);
void VT_GenerateBorder_UR(VirtualTextureStructure& _struct, VirtualTextureLayer& _cache, unsigned int _relativeIndex, int _lod, byte* _pageData);
void VT_GenerateBorder_DL(VirtualTextureStructure& _struct, VirtualTextureLayer& _cache, unsigned int _relativeIndex, int _lod, byte* _pageData);
void VT_GenerateBorder_DR(VirtualTextureStructure& _struct, VirtualTextureLayer& _cache, unsigned int _relativeIndex, int _lod, byte* _pageData);
void VT_GenerateBordersLod(VirtualTextureStructure& _struct, VirtualTextureLayer& _cache, int _lod);
void VT_GenerateBorders(VirtualTextureStructure& _struct, VirtualTextureLayer& _cache);

// Пишет страницу в файл VT
SFileOffset VT_WritePage(VTFileHandle* File, SFileOffset _offset, const VirtualTextureStructure& _Struct, VirtualTextureLayer* _Layers, int _numLayers, unsigned int _PageIndex);

// Пишет файл VT
bool VT_WriteFile(const VirtualTextureStructure& _struct, int _maxLods, VirtualTextureLayer* _Layers, int _numLayers, StringView _fileName);

struct VirtualTextureLayerDesc
{
    int SizeInBytes; // Размер страницы после компрессии
    int PageDataFormat;

    int NumChannels;

    void* (*LoadLayerImage)(void* _RectUserData, int _Width, int _Height);
    void (*FreeLayerImage)(void* _ImageData);
    void (*PageCompressionMethod)(const void* _InputData, void* _OutputData);
};

bool VT_CreateVirtualTexture(const VirtualTextureLayerDesc* _Layers,
                             int _NumLayers,
                             const char* _OutputFileName,
                             const char* _TempDir,
                             int _MaxLods,
                             int _PageWidthLog2,
                             const std::vector<RectangleBinPack::RectSize>& _TextureRects,
                             std::vector<RectangleBinBack_RectNode>& _BinRects,
                             unsigned int& _BinWidth,
                             unsigned int& _BinHeight,
                             int _MaxCachedPages = 32768);

void VT_TransformTextureCoords(float* _TexCoord,
                               unsigned int _NumVerts,
                               int _VertexStride,
                               const RectangleBinBack_RectNode& _BinRect,
                               unsigned int _BinWidth,
                               unsigned int _BinHeight);

HK_NAMESPACE_END
