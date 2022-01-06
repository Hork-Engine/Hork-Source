/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "Texture.h"

#include <Platform/Utf8.h>

enum EDrawCursor
{
    DRAW_CURSOR_ARROW,
    DRAW_CURSOR_TEXT_INPUT,
    DRAW_CURSOR_RESIZE_ALL,
    DRAW_CURSOR_RESIZE_NS,
    DRAW_CURSOR_RESIZE_EW,
    DRAW_CURSOR_RESIZE_NESW,
    DRAW_CURSOR_RESIZE_NWSE,
    DRAW_CURSOR_RESIZE_HAND
};

struct SFontGlyph
{
    /** 0x0000..0xFFFF */
    SWideChar Codepoint;
    /** Distance to next character (data from font + GlyphExtraSpacing.X baked in). */
    float AdvanceX;
    /** Glyph corners. */
    float X0, Y0, X1, Y1;
    /** Texture coordinates. */
    float U0, V0, U1, V1;
};

struct SFontCustomRect
{
    /**  User ID. Use < 0x110000 to map into a font glyph, >= 0x110000 for other/internal/custom texture data. */
    unsigned int Id;
    /** Rectangle width. */
    unsigned short Width;
    /** Rectangle height. */
    unsigned short Height;
    /** Packed position in Atlas. (read only) */
    unsigned short X;
    /** Packed position in Atlas. (read only) */
    unsigned short Y;
    /** For custom font glyphs only (ID < 0x110000): glyph xadvance. */
    float GlyphAdvanceX;
    /** For custom font glyphs only (ID < 0x110000): glyph display offset. */
    Float2 GlyphOffset;
};

enum EGlyphRange
{
    /** Basic Latin, Extended Latin */
    GLYPH_RANGE_DEFAULT,
    /** Default + Korean characters */
    GLYPH_RANGE_KOREAN,
    /** Default + Hiragana, Katakana, Half-Width, Selection of 1946 Ideographs */
    GLYPH_RANGE_JAPANESE,
    /** Default + Half-Width + Japanese Hiragana/Katakana + full set of about 21000 CJK Unified Ideographs */
    GLYPH_RANGE_CHINESE_FULL,
    /** Default + Half-Width + Japanese Hiragana/Katakana + set of 2500 CJK Unified Ideographs for common simplified Chinese */
    GLYPH_RANGE_CHINESE_SIMPLIFIED_COMMON,
    /** Default + about 400 Cyrillic characters */
    GLYPH_RANGE_CYRILLIC,
    /** Default + Thai characters */
    GLYPH_RANGE_THAI,
    /** Default + Vietname characters */
    GLYPH_RANGE_VIETNAMESE
};

struct SFontCreateInfo
{
    /** Index of font within TTF/OTF file. Default 0. */
    int FontNum;
    /** Size in pixels for rasterizer (more or less maps to the resulting font height). Default 13. */
    float SizePixels;
    /** Rasterize at higher quality for sub-pixel positioning. Default 3. */
    int OversampleH;
    /** Rasterize at higher quality for sub-pixel positioning. We don't use sub-pixel positions on the Y axis. Default 1. */
    int OversampleV;
    /** Align every glyph to pixel boundary. Useful e.g. if you are merging a non-pixel aligned font with the default font. If enabled, you can set OversampleH/V to 1. Default false. */
    bool bPixelSnapH;
    /** Extra spacing (in pixels) between glyphs. Only X axis is supported for now. Default (0,0). */
    Float2 GlyphExtraSpacing;
    /** Offset all glyphs from this font input. Default (0,0). */
    Float2 GlyphOffset;
    /** Unicode range. */
    EGlyphRange GlyphRange;
    /** Minimum AdvanceX for glyphs, set Min to align font icons, set both Min/Max to enforce mono-space font. Default 0. */
    float GlyphMinAdvanceX;
    /** Maximum AdvanceX for glyphs. Default FLT_MAX. */
    float GlyphMaxAdvanceX;
    /** Brighten (>1.0f) or darken (<1.0f) font output. Brightening small fonts may be a good workaround to make them more readable. Default 1. */
    float RasterizerMultiply;
};

class AFont : public AResource {
    AN_CLASS( AFont, AResource )

public:
    /** Initialize from memory */
    void InitializeFromMemoryTTF( const void * _SysMem, size_t _SizeInBytes, SFontCreateInfo const * _pCreateInfo = nullptr );

    /** Purge font data */
    void Purge();

    bool IsValid() const {
        return AtlasTexture.GetObject() != nullptr;
    }

    int GetFontSize() const {
        return FontSize;
    }

    void SetDrawOffset( Float2 const & _Offset );

    Float2 const & GetDrawOffset() const {
        return DrawOffset;
    }

    Float2 const & GetUVWhitePixel() const {
        return TexUvWhitePixel;
    }

    SFontGlyph const * GetGlyph( SWideChar c ) const {
        return ((int)c < WideCharToGlyph.Size()) ? Glyphs.ToPtr() + WideCharToGlyph[(int)c] : FallbackGlyph;
    }

    float GetCharAdvance( SWideChar c ) const {
        return ((int)c < WideCharAdvanceX.Size()) ? WideCharAdvanceX[(int)c] : FallbackAdvanceX;
    }

    ATexture * GetTexture() const {
        return AtlasTexture;
    }

    bool GetMouseCursorTexData( EDrawCursor cursor_type, Float2* out_offset, Float2* out_size, Float2 out_uv_border[2], Float2 out_uv_fill[2] ) const;

    Float2 CalcTextSizeA( float _Size, float _MaxWidth, float _WrapWidth, const char * _TextBegin, const char * _TextEnd = nullptr, const char** _Remaining = nullptr ) const; // utf8

    const char * CalcWordWrapPositionA( float _Scale, const char * _Text, const char * _TextEnd, float _WrapWidth ) const;

    SWideChar const * CalcWordWrapPositionW( float _Scale, SWideChar const * _Text, SWideChar const * _TextEnd, float _WrapWidth ) const;

    static void SetGlyphRanges( EGlyphRange _GlyphRange );

protected:
    AFont();
    ~AFont();

    /** Load resource from file */
    bool LoadResource( IBinaryStream & Stream ) override;

    /** Create internal resource */
    void LoadInternalResource( const char * _Path ) override;

    const char * GetDefaultResourcePath() const override { return "/Default/Fonts/Default"; }

private:
    bool Build( const void * _SysMem, size_t _SizeInBytes, SFontCreateInfo const * _CreateInfo );

    void AddGlyph( SFontCreateInfo const & cfg, SWideChar c, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, float advance_x );

    int AddCustomRect( unsigned int id, int width, int height );

    // Cache-friendly glyph advanceX
    TPodVector< float > WideCharAdvanceX;
    // AdvanceX for fallback character
    float FallbackAdvanceX = 0.0f;
    // Indexed by widechar, holds indices for corresponding glyphs
    TPodVector< unsigned short > WideCharToGlyph;
    // Font glyphs
    TPodVector< SFontGlyph > Glyphs;
    // Glyph for fallback character
    SFontGlyph const * FallbackGlyph = nullptr;
    // Font size in pixels
    float FontSize = 0.0f;
    // Offset for font rendering in pixels
    Float2 DrawOffset = Float2( 0.0f );
    // Texture raw data
    byte * TexPixelsAlpha8 = nullptr;
    // Texture width
    int TexWidth = 0;
    // Texture height
    int TexHeight = 0;
    // 1.0f/TexWidth, 1.0f/TexHeight
    Float2 TexUvScale = Float2( 0.0f );
    // Texture coordinates to a white pixel
    Float2 TexUvWhitePixel = Float2( 0.0f );
    // Texture object
    TRef< ATexture > AtlasTexture;
    // Rectangles for packing custom texture data into the atlas.
    TPodVector< SFontCustomRect > CustomRects;
};
