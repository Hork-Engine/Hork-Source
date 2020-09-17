/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#pragma once

#include <World/Public/Base/Resource.h>
#include <Runtime/Public/RenderCore.h>

class AImage;

struct SColorGradingPreset {
    Float3 Gain;
    Float3 Gamma;
    Float3 Lift;
    Float3 Presaturation;
    Float3 ColorTemperatureStrength;
    float ColorTemperature; // in K
    float ColorTemperatureBrightnessNormalization;
};

/**

ATexture

*/
class ANGIE_API ATexture : public AResource {
    AN_CLASS( ATexture, AResource )

public:
    /** Create empty 1D texture */
    void Initialize1D( STexturePixelFormat _PixelFormat, int _NumLods, int _Width );

    /** Create empty 1D array texture */
    void Initialize1DArray( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize );

    /** Create empty 2D texture */
    void Initialize2D( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height );

    /** Create 2D texture */
    bool InitializeFromImage( AImage const & _Image );

    /** Create empty 2D array texture */
    void Initialize2DArray( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArraySize );

    /** Create empty 3D texture */
    void Initialize3D( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth );

    /** Create 3D color grading LUT */
    void InitializeColorGradingLUT( const char * _Path );

    /** Create 3D color grading LUT */
    void InitializeColorGradingLUT( SColorGradingPreset const & _Preset );

    /** Create empty cubemap texture */
    void InitializeCubemap( STexturePixelFormat _PixelFormat, int _NumLods, int _Width );

    /** Create cubemap texture */
    bool InitializeCubemapFromImages( AImage const * _Faces[6] );

    /** Create empty cubemap array texture */
    void InitializeCubemapArray( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize );

    /** Fill texture data for any texture type. */
    bool WriteArbitraryData( int _LocationX, int _LocationY, int _LocationZ, int _Width, int _Height, int _Depth, int _Lod, const void * _SysMem );

    /** Helper. Fill texture data. */
    bool WriteTextureData1D( int _LocationX, int _Width, int _Lod, const void * _SysMem );

    /** Helper. Fill texture data. */
    bool WriteTextureData1DArray( int _LocationX, int _Width, int _ArrayLayer, int _Lod, const void * _SysMem );

    /** Helper. Fill texture data. */
    bool WriteTextureData2D( int _LocationX, int _LocationY, int _Width, int _Height, int _Lod, const void * _SysMem );

    /** Helper. Fill texture data. */
    bool WriteTextureData2DArray( int _LocationX, int _LocationY, int _Width, int _Height, int _ArrayLayer, int _Lod, const void * _SysMem );

    /** Helper. Fill texture data. */
    bool WriteTextureData3D( int _LocationX, int _LocationY, int _LocationZ, int _Width, int _Height, int _Depth, int _Lod, const void * _SysMem );

    /** Helper. Fill texture data. */
    bool WriteTextureDataCubemap( int _LocationX, int _LocationY, int _Width, int _Height, int _FaceIndex, int _Lod, const void * _SysMem );

    /** Helper. Fill texture data. */
    bool WriteTextureDataCubemapArray( int _LocationX, int _LocationY, int _Width, int _Height, int _FaceIndex, int _ArrayLayer, int _Lod, const void * _SysMem );

    ETextureType GetType() const { return (ETextureType)TextureType; }

    STexturePixelFormat const & GetPixelFormat() const { return PixelFormat; }

    int GetDimensionX() const { return Width; }

    int GetDimensionY() const { return Height; }

    int GetDimensionZ() const { return Depth; }

    int GetArraySize() const;

    int GetNumLods() const { return NumLods; }

    bool IsCubemap() const;

    int GetNumComponents() const { return PixelFormat.NumComponents(); }

    bool IsCompressed() const { return PixelFormat.IsCompressed(); }

    bool IsSRGB() const { return PixelFormat.IsSRGB(); }

    size_t SizeInBytesUncompressed() const { return PixelFormat.SizeInBytesUncompressed(); }

    size_t BlockSizeCompressed() const { return PixelFormat.BlockSizeCompressed(); }

    size_t GetSizeInBytes() const;

    // Utilites
    static size_t TextureSizeInBytes1D( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize );
    static size_t TextureSizeInBytes2D( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArraySize );
    static size_t TextureSizeInBytes3D( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth );
    static size_t TextureSizeInBytesCubemap( STexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize );

    RenderCore::ITexture * GetGPUResource() { return TextureGPU; }

    void Purge();

protected:
    ATexture();
    ~ATexture();

    /** Load resource from file */
    bool LoadResource( AString const & _Path ) override;

    /** Create internal resource */
    void LoadInternalResource( const char * _Path ) override;

    const char * GetDefaultResourcePath() const override { return "/Default/Textures/Default2D"; }

    TRef< RenderCore::ITexture > TextureGPU;
    int TextureType = 0;
    STexturePixelFormat PixelFormat = TEXTURE_PF_BGRA8_SRGB;
    int Width = 0;
    int Height = 0;
    int Depth = 0;
    int NumLods = 0;
};
