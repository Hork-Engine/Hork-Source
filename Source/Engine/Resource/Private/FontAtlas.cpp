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

static const unsigned short * GGlyphRanges = FFont::GetGlyphRangesDefault();

AN_CLASS_META( FFont )

void FFont::InitializeFromMemoryTTF( const void * _SysMem, size_t _SizeInBytes, float _SizePixels, unsigned short const * _GlyphRanges ) {
    Purge();

    if ( !Atlas.AddFontFromMemoryTTF( const_cast< void * >( _SysMem ), _SizeInBytes, _SizePixels, nullptr, _GlyphRanges ) ) {
        InitializeDefaultObject();
        return;
    }

    CreateTexture();
}

void FFont::InitializeFromMemoryCompressedTTF( const void * _SysMem, size_t _SizeInBytes, float _SizePixels, unsigned short const * _GlyphRanges ) {
    Purge();

    if ( !Atlas.AddFontFromMemoryCompressedTTF( _SysMem, _SizeInBytes, _SizePixels, nullptr, _GlyphRanges ) ) {
        InitializeDefaultObject();
        return;
    }

    CreateTexture();
}

void FFont::InitializeFromMemoryCompressedBase85TTF( const char * _SysMem, float _SizePixels, unsigned short const * _GlyphRanges ) {
    Purge();

    if ( !Atlas.AddFontFromMemoryCompressedBase85TTF( _SysMem, _SizePixels, nullptr, _GlyphRanges ) ) {
        InitializeDefaultObject();
        return;
    }

    CreateTexture();
}

void FFont::InitializeInternalResource( const char * _InternalResourceName ) {
    Purge();

    if ( !FString::Icmp( _InternalResourceName, "FFont.Default" ) ) {
        Atlas.AddFontDefault();

        CreateTexture();
        return;
    }

    GLogger.Printf( "Unknown internal font %s\n", _InternalResourceName );
}

bool FFont::InitializeFromFile( const char * _Path, bool _CreateDefultObjectIfFails ) {
    Purge();

    int i = FString::FindExtWithoutDot( _Path );
    const char * n = _Path + i;
    UInt sizePixels;
    
    sizePixels.FromString( n );
    if ( sizePixels < 8 ) {
        sizePixels = 8;
    }

    FString fn = _Path;
    fn.StripExt();

    if ( !Atlas.AddFontFromFileTTF( fn.ToConstChar(), sizePixels, nullptr, GGlyphRanges ) ) {

        if ( _CreateDefultObjectIfFails ) {
            InitializeDefaultObject();
            return true;
        }

        return false;
    }

    CreateTexture();

    return true;
}

void FFont::Purge() {
    Atlas.Clear();
    Atlas.TexID = nullptr;
    //AtlasTexture = nullptr;
    Font = nullptr;
}

bool FFont::IsValid() const {
    return Font != nullptr; 
}

int FFont::GetFontSize() const {
    return Font ? Font->FontSize : 8;
}

ImFontGlyph const * FFont::FindGlyph( FWideChar c ) const {
    if ( !Font ) {
        return nullptr;
    }
    return Font->FindGlyph( c );
}

float FFont::GetCharAdvance( FWideChar c ) const {
    if ( !Font ) {
        return 0.0f;
    }
    return ((int)c < Font->IndexAdvanceX.Size) ? Font->IndexAdvanceX[(int)c] : Font->FallbackAdvanceX;
}

Float2 FFont::CalcTextSizeA( float _Size, float _MaxWidth, float _WrapWidth, const char * _TextBegin, const char * _TextEnd, const char** _Remaining ) const {
    if ( !Font ) {
        return Float2::Zero();
    }
    return Font->CalcTextSizeA( _Size, _MaxWidth, _WrapWidth, _TextBegin, _TextEnd, _Remaining );
}

const char * FFont::CalcWordWrapPositionA( float _Scale, const char * _Text, const char * _TextEnd, float _WrapWidth ) const {
    if ( !Font ) {
        return _Text;
    }
    return Font->CalcWordWrapPositionA( _Scale, _Text, _TextEnd, _WrapWidth );
}

FWideChar const * FFont::CalcWordWrapPositionW( float _Scale, FWideChar const * _Text, FWideChar const * _TextEnd, float _WrapWidth ) const {
    if ( !Font ) {
        return _Text;
    }
    return Font->CalcWordWrapPositionW( _Scale, _Text, _TextEnd, _WrapWidth );
}

void FFont::SetDisplayOffset( Float2 const & _Offset ) {
    if ( Font ) {
        Font->DisplayOffset = _Offset;
    }
}

Float2 const & FFont::GetDisplayOffset() const {
    if ( !Font ) {
        return Float2::Zero();
    }
    return *(Float2 *)&Font->DisplayOffset.x;
}

void FFont::CreateTexture() {
    unsigned char * pixels;
    int atlasWidth, atlasHeight;

    // Get atlas raw data
    Atlas.GetTexDataAsAlpha8( &pixels, &atlasWidth, &atlasHeight );

    // Create atlas texture
    if ( !AtlasTexture ) {
        AtlasTexture = NewObject< FTexture2D >();
    }
    if ( AtlasTexture->GetWidth() != atlasWidth
         || AtlasTexture->GetHeight() != atlasHeight ) {
        AtlasTexture->Initialize( TEXTURE_PF_R8, 1, atlasWidth, atlasHeight );
    }
    AtlasTexture->WriteTextureData( 0, 0, atlasWidth, atlasHeight, 0, pixels );

    Atlas.TexID = AtlasTexture->GetGPUResource();

    Font = Atlas.Fonts[0];
}

void FFont::SetGlyphRanges( const unsigned short * _GlyphRanges ) {
    GGlyphRanges = _GlyphRanges;
}

const unsigned short * FFont::GetGlyphRangesDefault() {
    return ImFontAtlas::GetGlyphRangesDefault();
}

const unsigned short * FFont::GetGlyphRangesKorean() {
    return ImFontAtlas::GetGlyphRangesKorean();
}

const unsigned short * FFont::GetGlyphRangesJapanese() {
    return ImFontAtlas::GetGlyphRangesJapanese();
}

const unsigned short * FFont::GetGlyphRangesChineseFull() {
    return ImFontAtlas::GetGlyphRangesChineseFull();
}

const unsigned short * FFont::GetGlyphRangesChineseSimplifiedCommon() {
    return ImFontAtlas::GetGlyphRangesChineseSimplifiedCommon();
}

const unsigned short * FFont::GetGlyphRangesCyrillic() {
    return ImFontAtlas::GetGlyphRangesCyrillic();
}

const unsigned short * FFont::GetGlyphRangesThai() {
    return ImFontAtlas::GetGlyphRangesThai();
}

const unsigned short * FFont::GetGlyphRangesVietnamese() {
    return ImFontAtlas::GetGlyphRangesVietnamese();
}
