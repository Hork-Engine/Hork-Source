/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include <Engine/Core/Public/Image.h>
#include <Engine/Base/Public/BaseObject.h>
#include <Engine/Runtime/Public/RenderCore.h>

/**

FTexture

Texture base class. Don't create objects with this class. Use FTexture2D,FTexture3D,etc instead.

*/
class ANGIE_API FTexture : public FBaseObject, public IGPUResourceOwner {
    AN_CLASS( FTexture, FBaseObject )

public:
    ETextureType GetType() const { return (ETextureType)TextureType; }

    FTexturePixelFormat const & GetPixelFormat() const { return PixelFormat; }

    int GetDimensionX() const { return Width; }

    int GetDimensionY() const { return Height; }

    int GetDimensionZ() const { return Depth; }

    int GetNumLods() const { return NumLods; }

    bool IsCubemap() const;

    int GetNumComponents() const { return PixelFormat.NumComponents(); }

    bool IsCompressed() const { return PixelFormat.IsCompressed(); }

    bool IsSRGB() const { return PixelFormat.IsSRGB(); }

    size_t SizeInBytesUncompressed() const { return PixelFormat.SizeInBytesUncompressed(); }

    size_t BlockSizeCompressed() const { return PixelFormat.BlockSizeCompressed(); }

    // Utilites
    static size_t TextureByteLength1D( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize );
    static size_t TextureByteLength2D( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArraySize );
    static size_t TextureByteLength3D( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth );
    static size_t TextureByteLengthCubemap( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize );
    static size_t TextureByteLength2DNPOT( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height );

    FTextureGPU * GetGPUResource() { return TextureGPU; }

    void Purge();

protected:
    FTexture();
    ~FTexture();

    void SendTextureDataToGPU( int _LocationX, int _LocationY, int _LocationZ, int _Width, int _Height, int _Lod, const void * _SysMem );

    // IGPUResourceOwner interface
    void UploadResourceGPU( FResourceGPU * _Resource ) override {}

    FTextureGPU * TextureGPU;
    int TextureType;
    FTexturePixelFormat PixelFormat;
    int Width;
    int Height;
    int Depth;
    int NumLods;
};

class FTexture1D : public FTexture {
    AN_CLASS( FTexture1D, FTexture )

public:
    void Initialize( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width );

    // TODO:
    // Initialize object from file
    //bool InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails = true ) override;

    int GetWidth() const { return Width; }

    size_t GetSizeInBytes() const { return TextureByteLength1D( PixelFormat, NumLods, Width, 1 ); }

    void WriteTextureData( int _LocationX, int _Width, int _Lod, const void * _SysMem );

protected:
    FTexture1D() {}
    ~FTexture1D() {}
};

class FTexture1DArray : public FTexture {
    AN_CLASS( FTexture1DArray, FTexture )

public:
    void Initialize( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize );

    // TODO:
    // Initialize object from file
    //bool InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails = true ) override;

    int GetWidth() const { return Width; }
    int GetArraySize() const { return Height; }

    size_t GetSizeInBytes() const { return TextureByteLength1D( PixelFormat, NumLods, Width, GetArraySize() ); }

    void WriteTextureData( int _LocationX, int _Width, int _ArrayLayer, int _Lod, const void * _SysMem );

protected:
    FTexture1DArray() {}
    ~FTexture1DArray() {}
};

class FTexture2D : public FTexture {
    AN_CLASS( FTexture2D, FTexture )

public:
    void Initialize( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height );

    /** Create texture from string (FTexture2D.***) */
    void InitializeInternalResource( const char * _InternalResourceName ) override;

    /** Initialize object from file */
    bool InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails = true ) override;

    bool InitializeFromImage( FImage const & _Image );

    int GetWidth() const { return Width; }
    int GetHeight() const { return Height; }

    size_t GetSizeInBytes() const { return TextureByteLength2D( PixelFormat, NumLods, Width, Height, 1 ); }

    void WriteTextureData( int _LocationX, int _LocationY, int _Width, int _Height, int _Lod, const void * _SysMem );

protected:
    FTexture2D() {}
    ~FTexture2D() {}
};

class FTexture2DArray : public FTexture {
    AN_CLASS( FTexture2DArray, FTexture )

public:
    void Initialize( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArraySize );

    // TODO:
    // Initialize object from file
    //bool InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails = true ) override;

    int GetWidth() const { return Width; }
    int GetHeight() const { return Height; }
    int GetArraySize() const { return Depth; }

    size_t GetSizeInBytes() const { return TextureByteLength2D( PixelFormat, NumLods, Width, Height, GetArraySize() ); }

    void WriteTextureData( int _LocationX, int _LocationY, int _Width, int _Height, int _ArrayLayer, int _Lod, const void * _SysMem );

protected:
    FTexture2DArray() {}
    ~FTexture2DArray() {}
};

struct FColorGradingPreset {
    Float3 Gain;
    Float3 Gamma;
    Float3 Lift;
    Float3 Presaturation;
    Float3 ColorTemperatureStrength;
    float ColorTemperature; // in K
    float ColorTemperatureBrightnessNormalization;
};

class FTexture3D : public FTexture {
    AN_CLASS( FTexture3D, FTexture )

public:
    void Initialize( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth );

    /** Create texture from string (FTexture3D.***) */
    void InitializeInternalResource( const char * _InternalResourceName ) override;

    // TODO:
    // Initialize object from file
    //bool InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails = true ) override;

    void InitializeColorGradingLUT( const char * _Path );

    void InitializeColorGradingLUT( FColorGradingPreset const & _Preset );

    int GetWidth() const { return Width; }
    int GetHeight() const { return Height; }
    int GetDepth() const { return Depth; }

    size_t GetSizeInBytes() const { return TextureByteLength3D( PixelFormat, NumLods, Width, Height, Depth ); }

    void WriteTextureData( int _LocationX, int _LocationY, int _LocationZ, int _Width, int _Height, int _Lod, const void * _SysMem );

protected:
    FTexture3D() {}
    ~FTexture3D() {}
};

class FTextureCubemap : public FTexture {
    AN_CLASS( FTextureCubemap, FTexture )

public:
    void Initialize( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width );

    // TODO:
    // Initialize object from file
    //bool InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails = true ) override;

    void InitializeInternalResource( const char * _InternalResourceName );

    bool InitializeCubemapFromImages( FImage const * _Faces[6] );

    int GetWidth() const { return Width; }

    size_t GetSizeInBytes() const { return TextureByteLengthCubemap( PixelFormat, NumLods, Width, 1 ); }

    void WriteTextureData( int _LocationX, int _LocationY, int _Width, int _Height, int _FaceIndex, int _Lod, const void * _SysMem );

protected:
    FTextureCubemap() {}
    ~FTextureCubemap() {}
};

class FTextureCubemapArray : public FTexture {
    AN_CLASS( FTextureCubemapArray, FTexture )

public:
    void Initialize( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize );

    // TODO:
    // Initialize object from file
    //bool InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails = true ) override;

    int GetWidth() const { return Width; }
    int GetArraySize() const { return Depth; }

    size_t GetSizeInBytes() const { return TextureByteLengthCubemap( PixelFormat, NumLods, Width, GetArraySize() ); }

    void WriteTextureData( int _LocationX, int _LocationY, int _Width, int _Height, int _FaceIndex, int _ArrayLayer, int _Lod, const void * _SysMem );

protected:
    FTextureCubemapArray() {}
    ~FTextureCubemapArray() {}
};

class FTexture2DNPOT : public FTexture {
    AN_CLASS( FTexture2DNPOT, FTexture )

public:
    void Initialize( FTexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height );

    // TODO:
    // Initialize object from file
    //bool InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails = true ) override;

    int GetWidth() const { return Width; }
    int GetHeight() const { return Height; }

    size_t GetSizeInBytes() const { return TextureByteLength2DNPOT( PixelFormat, NumLods, Width, Height ); }

    void WriteTextureData( int _LocationX, int _LocationY, int _Width, int _Height, int _Lod, const void * _SysMem );

protected:
    FTexture2DNPOT() {}
    ~FTexture2DNPOT() {}
};
