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

#include <Engine/Base/Public/BaseObject.h>
#include <Engine/Runtime/Public/RenderBackend.h>

/*

FSoftwareMipmapGenerator

Software mipmap generator

*/
struct FSoftwareMipmapGenerator {
    void * SourceImage;
    int Width;
    int Height;
    int NumChannels;
    bool bLinearSpace;
    bool bHDRI;

    void ComputeRequiredMemorySize( int & _RequiredMemory, int & _NumLods ) const;
    void GenerateMipmaps( void * _Data );
};

/*

FImage

Image loader

*/
class ANGIE_API FImage {
public:
    void * pRawData;
    int Width;
    int Height;
    int NumChannels;
    bool bHDRI;
    bool bLinearSpace;
    bool bHalf; // Only for HDRI images
    int NumLods;

    FImage();
    ~FImage();

    // Load image as byte*
    bool LoadRawImage( const char * _Path, bool _SRGB, bool _GenerateMipmaps, int _NumDesiredChannels = 0 );
    bool LoadRawImage( FFileStream & _Stream, bool _SRGB, bool _GenerateMipmaps, int _NumDesiredChannels = 0 );
    bool LoadRawImage( FMemoryStream & _Stream, bool _SRGB, bool _GenerateMipmaps, int _NumDesiredChannels = 0 );

    // Load image as float* in linear space
    bool LoadRawImageHDRI( const char * _Path, bool _HalfFloat, bool _GenerateMipmaps, int _NumDesiredChannels = 0 );
    bool LoadRawImageHDRI( FFileStream & _Stream, bool _HalfFloat, bool _GenerateMipmaps, int _NumDesiredChannels = 0 );
    bool LoadRawImageHDRI( FMemoryStream & _Stream, bool _HalfFloat, bool _GenerateMipmaps, int _NumDesiredChannels = 0 );

    void Free();
};

/*

FTexture

Textures are used for materials

*/
class ANGIE_API FTexture : public FBaseObject, public IRenderProxyOwner {
    AN_CLASS( FTexture, FBaseObject )

public:
    bool InitializeFromImage( FImage const & _Image );
    bool InitializeCubemapFromImages( FImage const * _Faces[6] );

    void Initialize1D( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArrayLength = 1 );
    void Initialize2D( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArrayLength = 1 );
    void Initialize3D( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth );
    void InitializeCubemap( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArrayLength = 1 );
    void InitializeRect( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height );

    // Create texture from string (*white* *black* *gray* *normal* *cubemap*)
    void InitializeInternalTexture( const char * _Name );

    // Initialize default object representation
    void InitializeDefaultObject() override;

    // Initialize object from file
    bool InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails = true ) override;

    void Purge();

    ETexturePixelFormat GetPixelFormat() const;
    int GetDimensionCount() const;
    int GetWidth() const;
    int GetHeight() const;
    int GetDepth() const;
    int GetNumLods() const;
    bool IsCubemap() const;
    int GetNumComponents() const;
    bool IsCompressed() const;
    bool IsSRGB() const;
    size_t UncompressedPixelByteLength() const;
    size_t CompressedTextureBlockLength() const;

    void * WriteTextureData( int _LocationX, int _LocationY, int _LocationZ, int _Width, int _Height, int _Lod );
    //bool WriteTextureData( void const * _Pixels, int _LocationX, int _LocationY, int _LocationZ, int _Width, int _Height, int _Lod );

    // Utilites
    static size_t TextureByteLength1D( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArrayLength );
    static size_t TextureByteLength2D( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArrayLength );
    static size_t TextureByteLength3D( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth );
    static size_t TextureByteLengthCubemap( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArrayLength );
    static size_t TextureByteLengthRect( ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height );

    FRenderProxy_Texture * GetRenderProxy() { return RenderProxy; }

protected:
    FTexture();
    ~FTexture();

    // IRenderProxyOwner interface
    void OnLost() override { /* ... */ }

private:
    FRenderProxy_Texture * RenderProxy;
    int TextureType;
    ETexturePixelFormat PixelFormat;
    int Width;
    int Height;
    int Depth;
    int NumLods;
};

AN_FORCEINLINE ETexturePixelFormat FTexture::GetPixelFormat() const {
    return PixelFormat;
}

AN_FORCEINLINE int FTexture::GetWidth() const {
    return Width;
}

AN_FORCEINLINE int FTexture::GetHeight() const {
    return Height;
}

AN_FORCEINLINE int FTexture::GetDepth() const {
    return Depth;
}

AN_FORCEINLINE int FTexture::GetNumLods() const {
    return NumLods;
}

AN_FORCEINLINE int FTexture::GetNumComponents() const {
    return ::NumPixelComponents( PixelFormat );
}

AN_FORCEINLINE bool FTexture::IsCompressed() const {
    return ::IsTextureCompressed( PixelFormat );
}

AN_FORCEINLINE bool FTexture::IsSRGB() const {
    return ::IsTextureSRGB( PixelFormat );
}

AN_FORCEINLINE size_t FTexture::UncompressedPixelByteLength() const {
    return ::UncompressedPixelByteLength( PixelFormat );
}

AN_FORCEINLINE size_t FTexture::CompressedTextureBlockLength() const {
    return ::CompressedTextureBlockLength( PixelFormat );
}
