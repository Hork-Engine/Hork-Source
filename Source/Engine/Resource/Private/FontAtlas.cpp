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

static const unsigned short * GGlyphRanges = AFont::GetGlyphRangesDefault();

AN_CLASS_META( AFont )

void AFont::InitializeFromMemoryTTF( const void * _SysMem, size_t _SizeInBytes, float _SizePixels, unsigned short const * _GlyphRanges ) {
    Purge();

    if ( !Atlas.AddFontFromMemoryTTF( const_cast< void * >( _SysMem ), _SizeInBytes, _SizePixels, nullptr, _GlyphRanges ) ) {
        InitializeDefaultObject();
        return;
    }

    CreateTexture();
}

void AFont::InitializeFromMemoryCompressedTTF( const void * _SysMem, size_t _SizeInBytes, float _SizePixels, unsigned short const * _GlyphRanges ) {
    Purge();

    if ( !Atlas.AddFontFromMemoryCompressedTTF( _SysMem, _SizeInBytes, _SizePixels, nullptr, _GlyphRanges ) ) {
        InitializeDefaultObject();
        return;
    }

    CreateTexture();
}

void AFont::InitializeFromMemoryCompressedBase85TTF( const char * _SysMem, float _SizePixels, unsigned short const * _GlyphRanges ) {
    Purge();

    if ( !Atlas.AddFontFromMemoryCompressedBase85TTF( _SysMem, _SizePixels, nullptr, _GlyphRanges ) ) {
        InitializeDefaultObject();
        return;
    }

    CreateTexture();
}

void AFont::LoadInternalResource( const char * _Path ) {
    Purge();

    if ( !AString::Icmp( _Path, "/Default/Fonts/Default" ) ) {
        Atlas.AddFontDefault();

        CreateTexture();
        return;
    }

    GLogger.Printf( "Unknown internal font %s\n", _Path );

    LoadInternalResource( "/Default/Fonts/Default" );
}

bool AFont::LoadResource( AString const & _Path ) {
    Purge();

    int i = _Path.FindExtWithoutDot();
    const char * n = &_Path[i];
    UInt sizePixels;
    
    sizePixels.FromString( n );
    if ( sizePixels < 8 ) {
        sizePixels = 8;
    }

    AString fn = _Path;
    fn.StripExt();

    if ( !Atlas.AddFontFromFileTTF( fn.CStr(), sizePixels, nullptr, GGlyphRanges ) ) {
        return false;
    }

    CreateTexture();

    return true;
}

void AFont::Purge() {
    Atlas.Clear();
    Atlas.TexID = nullptr;
    //AtlasTexture = nullptr;
    Font = nullptr;
}

bool AFont::IsValid() const {
    return Font != nullptr; 
}

int AFont::GetFontSize() const {
    return Font ? Font->FontSize : 8;
}

ImFontGlyph const * AFont::FindGlyph( FWideChar c ) const {
    if ( !Font ) {
        return nullptr;
    }
    return Font->FindGlyph( c );
}

float AFont::GetCharAdvance( FWideChar c ) const {
    if ( !Font ) {
        return 0.0f;
    }
    return ((int)c < Font->IndexAdvanceX.Size) ? Font->IndexAdvanceX[(int)c] : Font->FallbackAdvanceX;
}

Float2 AFont::CalcTextSizeA( float _Size, float _MaxWidth, float _WrapWidth, const char * _TextBegin, const char * _TextEnd, const char** _Remaining ) const {
    if ( !Font ) {
        return Float2::Zero();
    }
    return Font->CalcTextSizeA( _Size, _MaxWidth, _WrapWidth, _TextBegin, _TextEnd, _Remaining );
}

const char * AFont::CalcWordWrapPositionA( float _Scale, const char * _Text, const char * _TextEnd, float _WrapWidth ) const {
    if ( !Font ) {
        return _Text;
    }
    return Font->CalcWordWrapPositionA( _Scale, _Text, _TextEnd, _WrapWidth );
}

FWideChar const * AFont::CalcWordWrapPositionW( float _Scale, FWideChar const * _Text, FWideChar const * _TextEnd, float _WrapWidth ) const {
    if ( !Font ) {
        return _Text;
    }
    return Font->CalcWordWrapPositionW( _Scale, _Text, _TextEnd, _WrapWidth );
}

void AFont::SetDisplayOffset( Float2 const & _Offset ) {
    if ( Font ) {
        Font->DisplayOffset = _Offset;
    }
}

Float2 const & AFont::GetDisplayOffset() const {
    if ( !Font ) {
        return Float2::Zero();
    }
    return *(Float2 *)&Font->DisplayOffset.x;
}

void AFont::CreateTexture() {
    unsigned char * pixels;
    int atlasWidth, atlasHeight;

    // Get atlas raw data
    Atlas.GetTexDataAsAlpha8( &pixels, &atlasWidth, &atlasHeight );

    // Create atlas texture
    if ( !AtlasTexture ) {
        AtlasTexture = NewObject< ATexture >();
    }
    if ( AtlasTexture->GetDimensionX() != atlasWidth
         || AtlasTexture->GetDimensionY() != atlasHeight ) {
        AtlasTexture->Initialize2D( TEXTURE_PF_R8, 1, atlasWidth, atlasHeight );
    }
    AtlasTexture->WriteTextureData2D( 0, 0, atlasWidth, atlasHeight, 0, pixels );

    Atlas.TexID = AtlasTexture->GetGPUResource();

    Font = Atlas.Fonts[0];
}

void AFont::SetGlyphRanges( const unsigned short * _GlyphRanges ) {
    GGlyphRanges = _GlyphRanges;
}

const unsigned short * AFont::GetGlyphRangesDefault() {
    return ImFontAtlas::GetGlyphRangesDefault();
}

const unsigned short * AFont::GetGlyphRangesKorean() {
    return ImFontAtlas::GetGlyphRangesKorean();
}

const unsigned short * AFont::GetGlyphRangesJapanese() {
    return ImFontAtlas::GetGlyphRangesJapanese();
}

const unsigned short * AFont::GetGlyphRangesChineseFull() {
    return ImFontAtlas::GetGlyphRangesChineseFull();
}

const unsigned short * AFont::GetGlyphRangesChineseSimplifiedCommon() {
    return ImFontAtlas::GetGlyphRangesChineseSimplifiedCommon();
}

const unsigned short * AFont::GetGlyphRangesCyrillic() {
    return ImFontAtlas::GetGlyphRangesCyrillic();
}

const unsigned short * AFont::GetGlyphRangesThai() {
    return ImFontAtlas::GetGlyphRangesThai();
}

const unsigned short * AFont::GetGlyphRangesVietnamese() {
    return ImFontAtlas::GetGlyphRangesVietnamese();
}
