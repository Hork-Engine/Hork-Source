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
    int                         PageResolutionB;
    int                         PageResolution;
    int                         NumLods;
    unsigned int                NumQuadTreeNodes;
    VTPageBitfield              PageBitfield;
};

class VirtualTextureImage final
{
public:
                                VirtualTextureImage() = default;
                                ~VirtualTextureImage();

    bool                        OpenImage(const char* fileName, int width, int height, int numChannels);
    bool                        WriteImage(const char* fileName) const;
    void                        CreateEmpty(int width, int height, int numChannels);
    byte*                       GetData() { return m_Data; }
    const byte*                 GetData() const { return m_Data; }
    int                         GetWidth() const { return m_Width; }
    int                         GetHeight() const { return m_Height; }
    int                         GetNumChannels() const { return m_NumChannels; }

private:
    byte*                       m_Data{};
    int                         m_NumChannels{};
    int                         m_Width{};
    int                         m_Height{};
};

class VirtualTextureLayer final
{
public:
                                VirtualTextureLayer() = default;
                                ~VirtualTextureLayer();

    // Параметры открытия страницы VT из кеша
    enum OpenMode
    {
        OpenEmpty, // Если страницы нет в кеше, то создает пустую
        OpenActual // Если страницы нет в кеше, то читает из файла. Если файл не удалось открыть, то возвращает NULL
    };

    struct CachedPage
    {
        void* operator new(size_t size) { return Core::GetHeapAllocator<HEAP_MISC>().Alloc(size); }
        void operator delete(void* p) { Core::GetHeapAllocator<HEAP_MISC>().Free(p); }

        VirtualTextureImage     Image;
        bool                    bNeedToSave;
        int                     Used;
    };

    String                      Path;

    int                         MaxCachedPages = 1024; // Максимальное количество закешированных страниц.
                                                       // Если значение < 0, то размер кеша ограничивается
                                                       // объемом оперативной памяти компьютера
    int                         NumCachedPages = 0; // Текущее количество закешированных страниц

    bool                        bAllowDump = true; // Сохранять страницы на HDD, при переполнении кеша

    int                         NumChannels;
    int                         SizeInBytes; // Размер страницы в байтах после компрессии
    int                         PageDataFormat;

    void                        (*PageCompressionMethod)(const void* inputData, void* outputData);

    HashMap<unsigned int, CachedPage*> Pages;
};

// Создает структуру виртуальной текстуры, на выходе _struct и binRects
bool VT_MakeStructure(VirtualTextureStructure& _struct, int pageWidthLog2, const std::vector<RectangleBinPack::RectSize>& textureRects, std::vector<RectangleBinBack_RectNode>& binRects, unsigned int& binWidth, unsigned int& binHeight);

// Открывает страницу VT из кеша
VirtualTextureLayer::CachedPage* VT_OpenCachedPage(const VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int absoluteIndex, VirtualTextureLayer::OpenMode openMode, bool needToSave);

// Закрывает страницу VT из кеша
void VT_CloseCachedPage(VirtualTextureLayer::CachedPage* cachedPage);

// Поиск страницы в кеше
VirtualTextureLayer::CachedPage* VT_FindInCache(VirtualTextureLayer& layer, unsigned int absoluteIndex);

// Чистка кеша, сброс (запись) страниц кеша на диск в случае переполнения
// если _forceFit = true, то запись страниц кеша на диск в любом случае.
// Если страница кеша в текущий момент открыта, то она остается в кеше в любом случае.
void VT_FitPageData(VirtualTextureLayer& layer, bool forceFit = false);

// Получает имя файла страницы по относительному индексу страницы и лоду
String VT_FileNameFromRelative(const char* outputPath, unsigned int relativeIndex, int lod);

// Сохраняет страницу на диск
bool VT_DumpPageToDisk(const char* path, unsigned int absoluteIndex, const VirtualTextureImage& image);

// Режет входное изображение (layerData) на страницы и сохраняет в кеше
void VT_PutImageIntoPages(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, const RectangleBinBack_RectNode& rect, const byte* layerData);

// Загружает заданные четыре страницы для последующего лодирования
bool VT_LoadQuad(const VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int src00, unsigned int src10, unsigned int src01, unsigned int src11, int sourceLod, VirtualTextureLayer::CachedPage* page[4]);

// Лодирование четырех страниц
void VT_Downsample(const VirtualTextureStructure& _struct, VirtualTextureLayer::CachedPage* pages[4], byte* downsample);

// Создает лоды VT
void VT_MakeLods(VirtualTextureStructure& _struct, VirtualTextureLayer& layer);

// Синхронизирует page bitfield с жестким диском (заново заполняет pageBitfield на основе страниц,
// хранящихся на диске
void VT_SynchronizePageBitfieldWithHDD(VirtualTextureStructure& _struct, VirtualTextureLayer& layer);

// Удаляет страницы кеша, сброшенные на диск
// Если _synchPageBitfield = true, то перед удалением обновляется pageBitfield в соответствии с данными,
// хранящимися на диске
// Если _unmarkRemoved = true, то при удалении страницы с диска, снимается бит с pageBitfield-а
void VT_RemoveHDDData(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, bool synchPageBitfield = true, bool unmarkRemoved = true);

// Функции создания бордеров
void VT_GenerateBorder_U(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int relativeIndex, int lod, byte* pageData);
void VT_GenerateBorder_D(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int relativeIndex, int lod, byte* pageData);
void VT_GenerateBorder_L(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int relativeIndex, int lod, byte* pageData);
void VT_GenerateBorder_R(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int relativeIndex, int lod, byte* pageData);
void VT_GenerateBorder_UL(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int relativeIndex, int lod, byte* pageData);
void VT_GenerateBorder_UR(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int relativeIndex, int lod, byte* pageData);
void VT_GenerateBorder_DL(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int relativeIndex, int lod, byte* pageData);
void VT_GenerateBorder_DR(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, unsigned int relativeIndex, int lod, byte* pageData);
void VT_GenerateBordersLod(VirtualTextureStructure& _struct, VirtualTextureLayer& layer, int lod);
void VT_GenerateBorders(VirtualTextureStructure& _struct, VirtualTextureLayer& layer);

// Пишет страницу в файл VT
VTFileOffset VT_WritePage(VTFileHandle* file, VTFileOffset offset, const VirtualTextureStructure& _struct, VirtualTextureLayer* layers, int c, unsigned int pageIndex);

// Пишет файл VT
bool VT_WriteFile(const VirtualTextureStructure& _struct, int maxLods, VirtualTextureLayer* layers, int pageData, StringView fileName);

struct VirtualTextureLayerDesc
{
    int                         SizeInBytes; // Размер страницы после компрессии
    int                         PageDataFormat;

    int                         NumChannels;

    void*                       (*LoadLayerImage)(void* rectUserData, int width, int height);
    void                        (*FreeLayerImage)(void* imageData);
    void                        (*PageCompressionMethod)(const void* inputData, void* outputData);
};

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
                             int maxCachedPages = 32768);

void VT_TransformTextureCoords(float* texCoord,
                               unsigned int numVerts,
                               int vertexStride,
                               const RectangleBinBack_RectNode& binRect,
                               unsigned int binWidth,
                               unsigned int binHeight);

HK_NAMESPACE_END
