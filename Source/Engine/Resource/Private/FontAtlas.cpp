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

#include <Engine/Resource/Public/FontAtlas.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

AN_CLASS_META( FFontAtlas )

FFontAtlas::FFontAtlas() {

}

FFontAtlas::~FFontAtlas() {
}

int FFontAtlas::AddFontDefault() {
    if ( !Atlas.AddFontDefault() ) {
        return -1;
    }
    return Atlas.Fonts.size() - 1;
}

int FFontAtlas::AddFontFromFileTTF( const char * _FileName, float _SizePixels, unsigned short const * _GlyphRanges ) {
    if ( !Atlas.AddFontFromFileTTF( _FileName, _SizePixels, nullptr, _GlyphRanges ) ) {
        return -1;
    }
    return Atlas.Fonts.size() - 1;
}

int FFontAtlas::AddFontFromMemoryTTF( void * _FontData, int _FontSize, float _SizePixels, unsigned short const * _GlyphRanges ) {
    if ( !Atlas.AddFontFromMemoryTTF( _FontData, _FontSize, _SizePixels, nullptr, _GlyphRanges ) ) {
        return -1;
    }
    return Atlas.Fonts.size() - 1;
}

int FFontAtlas::AddFontFromMemoryCompressedTTF( const void * _CompressedFontData, int _CompressedFontSize, float _SizePixels, unsigned short const * _GlyphRanges ) {
    if ( !Atlas.AddFontFromMemoryCompressedTTF( _CompressedFontData, _CompressedFontSize, _SizePixels, nullptr, _GlyphRanges ) ) {
         return -1;
     }
    return Atlas.Fonts.size() - 1;
}

int FFontAtlas::AddFontFromMemoryCompressedBase85TTF( const char * _CompressedFontData, float _SizePixels, unsigned short const * _GlyphRanges ) {
    if ( !Atlas.AddFontFromMemoryCompressedBase85TTF( _CompressedFontData, _SizePixels, nullptr, _GlyphRanges ) ) {
        return -1;
    }
    return Atlas.Fonts.size() - 1;
}

//FFont * FFontAtlas::GetFont( int _Id ) {
//    if ( _Id >= 0 && _Id < Atlas.Fonts.size() ) {
//        return Atlas.Fonts[_Id];
//    }
//    return nullptr;
//}

FFont const * FFontAtlas::GetFont( int _Id ) const {
    if ( _Id >= 0 && _Id < Atlas.Fonts.size() ) {
        return Atlas.Fonts[_Id];
    }
    return nullptr;
}

void FFontAtlas::Build() {
    unsigned char * pixels;
    int atlasWidth, atlasHeight;

    // Get atlas raw data
    Atlas.GetTexDataAsAlpha8( &pixels, &atlasWidth, &atlasHeight );

    // Create atlas texture
    if ( !AtlasTexture ) {
        AtlasTexture = NewObject< FTexture >();
    }
    AtlasTexture->Initialize2D( TEXTURE_PF_R8, 1, atlasWidth, atlasHeight );
    void * pPixels = AtlasTexture->WriteTextureData( 0,0,0, atlasWidth, atlasHeight, 0 );
    if ( pPixels ) {
        memcpy( pPixels, pixels, atlasWidth*atlasHeight );
    }

    Atlas.TexID = AtlasTexture->GetRenderProxy();
}

void FFontAtlas::Purge() {
    Atlas.Clear();
    Atlas.TexID = nullptr;
    AtlasTexture = nullptr;
}

const unsigned short * FFontAtlas::GetGlyphRangesDefault() {
    return ImFontAtlas::GetGlyphRangesDefault();
}

const unsigned short * FFontAtlas::GetGlyphRangesKorean() {
    return ImFontAtlas::GetGlyphRangesKorean();
}

const unsigned short * FFontAtlas::GetGlyphRangesJapanese() {
    return ImFontAtlas::GetGlyphRangesJapanese();
}

const unsigned short * FFontAtlas::GetGlyphRangesChineseFull() {
    return ImFontAtlas::GetGlyphRangesChineseFull();
}

const unsigned short * FFontAtlas::GetGlyphRangesChineseSimplifiedCommon() {
    return ImFontAtlas::GetGlyphRangesChineseSimplifiedCommon();
}

const unsigned short * FFontAtlas::GetGlyphRangesCyrillic() {
    return ImFontAtlas::GetGlyphRangesCyrillic();
}

const unsigned short * FFontAtlas::GetGlyphRangesThai() {
    return ImFontAtlas::GetGlyphRangesThai();
}

const unsigned short * FFontAtlas::GetGlyphRangesVietnamese() {
    return ImFontAtlas::GetGlyphRangesVietnamese();
}
