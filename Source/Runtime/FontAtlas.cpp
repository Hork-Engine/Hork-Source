/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

// NOTE: Some code in this file is based on Dear ImGui

#include "FontAtlas.h"
#include "EmbeddedResources.h"
#include "Engine.h"

#include <Platform/Logger.h>
#include <Platform/Platform.h>
#include <Core/IntrusiveLinkedListMacro.h>
#include <Core/Base85.h>
#include <Core/Compress.h>
#include <Core/Parse.h>

// Padding between glyphs within texture in pixels. Defaults to 1. If your rendering method doesn't rely on bilinear filtering you may set this to 0.
static const int TexGlyphPadding = 1;

static const int TabSize = 4;

static const bool bTexNoPowerOfTwoHeight = false;

static const float DefaultFontSize = 13;

// Replacement character if a glyph isn't found
static const WideChar FallbackChar = (WideChar)'?';

static EGlyphRange GGlyphRange = GLYPH_RANGE_DEFAULT;

#ifndef STB_RECT_PACK_IMPLEMENTATION
#    define STBRP_STATIC
#    define STBRP_ASSERT(x) HK_ASSERT(x)
#    define STBRP_SORT      qsort
#    define STB_RECT_PACK_IMPLEMENTATION
#    include "stb/stb_rect_pack.h"
#endif

#ifndef STB_TRUETYPE_IMPLEMENTATION
#    define STBTT_malloc(x, u) ((void)(u), Platform::GetHeapAllocator<HEAP_IMAGE>().Alloc(x))
#    define STBTT_free(x, u)   ((void)(u), Platform::GetHeapAllocator<HEAP_IMAGE>().Free(x))
#    define STBTT_assert(x)    HK_ASSERT(x)
#    define STBTT_fmod(x, y)   Math::FMod(x, y)
#    define STBTT_sqrt(x)      std::sqrt(x)
#    define STBTT_pow(x, y)    Math::Pow(x, y)
#    define STBTT_fabs(x)      Math::Abs(x)
#    define STBTT_ifloor(x)    (int)Math::Floor(x)
#    define STBTT_iceil(x)     (int)Math::Ceil(x)
#    define STBTT_STATIC
#    define STB_TRUETYPE_IMPLEMENTATION
#    include "stb/stb_truetype.h"
#endif

// A work of art lies ahead! (. = white layer, X = black layer, others are blank)
// The white texels on the top left are the ones we'll use everywhere to render filled shapes.
const int          FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF = 108;
const int          FONT_ATLAS_DEFAULT_TEX_DATA_H      = 27;
const unsigned int FONT_ATLAS_DEFAULT_TEX_DATA_ID     = 0x80000000;
static const char  FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS[FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF * FONT_ATLAS_DEFAULT_TEX_DATA_H + 1] =
    {
        "..-         -XXXXXXX-    X    -           X           -XXXXXXX          -          XXXXXXX-     XX          "
        "..-         -X.....X-   X.X   -          X.X          -X.....X          -          X.....X-    X..X         "
        "---         -XXX.XXX-  X...X  -         X...X         -X....X           -           X....X-    X..X         "
        "X           -  X.X  - X.....X -        X.....X        -X...X            -            X...X-    X..X         "
        "XX          -  X.X  -X.......X-       X.......X       -X..X.X           -           X.X..X-    X..X         "
        "X.X         -  X.X  -XXXX.XXXX-       XXXX.XXXX       -X.X X.X          -          X.X X.X-    X..XXX       "
        "X..X        -  X.X  -   X.X   -          X.X          -XX   X.X         -         X.X   XX-    X..X..XXX    "
        "X...X       -  X.X  -   X.X   -    XX    X.X    XX    -      X.X        -        X.X      -    X..X..X..XX  "
        "X....X      -  X.X  -   X.X   -   X.X    X.X    X.X   -       X.X       -       X.X       -    X..X..X..X.X "
        "X.....X     -  X.X  -   X.X   -  X..X    X.X    X..X  -        X.X      -      X.X        -XXX X..X..X..X..X"
        "X......X    -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -         X.X   XX-XX   X.X         -X..XX........X..X"
        "X.......X   -  X.X  -   X.X   -X.....................X-          X.X X.X-X.X X.X          -X...X...........X"
        "X........X  -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -           X.X..X-X..X.X           - X..............X"
        "X.........X -XXX.XXX-   X.X   -  X..X    X.X    X..X  -            X...X-X...X            -  X.............X"
        "X..........X-X.....X-   X.X   -   X.X    X.X    X.X   -           X....X-X....X           -  X.............X"
        "X......XXXXX-XXXXXXX-   X.X   -    XX    X.X    XX    -          X.....X-X.....X          -   X............X"
        "X...X..X    ---------   X.X   -          X.X          -          XXXXXXX-XXXXXXX          -   X...........X "
        "X..X X..X   -       -XXXX.XXXX-       XXXX.XXXX       -------------------------------------    X..........X "
        "X.X  X..X   -       -X.......X-       X.......X       -    XX           XX    -           -    X..........X "
        "XX    X..X  -       - X.....X -        X.....X        -   X.X           X.X   -           -     X........X  "
        "      X..X          -  X...X  -         X...X         -  X..X           X..X  -           -     X........X  "
        "       XX           -   X.X   -          X.X          - X...XXXXXXXXXXXXX...X -           -     XXXXXXXXXX  "
        "------------        -    X    -           X           -X.....................X-           ------------------"
        "                    ----------------------------------- X...XXXXXXXXXXXXX...X -                             "
        "                                                      -  X..X           X..X  -                             "
        "                                                      -   X.X           X.X   -                             "
        "                                                      -    XX           XX    -                             "};

static const Float2 CursorTexData[][3] =
    {
        // Pos ........ Size ......... Offset ......
        {Float2(0, 3), Float2(12, 19), Float2(0, 0)},    // DRAW_CURSOR_ARROW
        {Float2(13, 0), Float2(7, 16), Float2(1, 8)},    // DRAW_CURSOR_TEXT_INPUT
        {Float2(31, 0), Float2(23, 23), Float2(11, 11)}, // DRAW_CURSOR_RESIZE_ALL
        {Float2(21, 0), Float2(9, 23), Float2(4, 11)},   // DRAW_CURSOR_RESIZE_NS
        {Float2(55, 18), Float2(23, 9), Float2(11, 4)},  // DRAW_CURSOR_RESIZE_EW
        {Float2(73, 0), Float2(17, 17), Float2(8, 8)},   // DRAW_CURSOR_RESIZE_NESW
        {Float2(55, 0), Float2(17, 17), Float2(8, 8)},   // DRAW_CURSOR_RESIZE_NWSE
        {Float2(91, 0), Float2(17, 22), Float2(5, 0)},   // DRAW_CURSOR_RESIZE_HAND
};

static const unsigned short* GetGlyphRangesDefault()
{
    static const WideChar ranges[] =
        {
            0x0020,
            0x00FF, // Basic Latin + Latin Supplement
            0,
        };
    return &ranges[0];
}

static const unsigned short* GetGlyphRangesKorean()
{
    static const WideChar ranges[] =
        {
            0x0020,
            0x00FF, // Basic Latin + Latin Supplement
            0x3131,
            0x3163, // Korean alphabets
            0xAC00,
            0xD79D, // Korean characters
            0,
        };
    return &ranges[0];
}

static void UnpackAccumulativeOffsetsIntoRanges(int base_codepoint, const short* accumulative_offsets, int accumulative_offsets_count, WideChar* out_ranges)
{
    for (int n = 0; n < accumulative_offsets_count; n++, out_ranges += 2)
    {
        out_ranges[0] = out_ranges[1] = (WideChar)(base_codepoint + accumulative_offsets[n]);
        base_codepoint += accumulative_offsets[n];
    }
    out_ranges[0] = 0;
}

static const unsigned short* GetGlyphRangesJapanese()
{
    // 1946 common ideograms code points for Japanese
    // Sourced from http://theinstructionlimit.com/common-kanji-character-ranges-for-xna-spritefont-rendering
    // FIXME: Source a list of the revised 2136 Joyo Kanji list from 2010 and rebuild this.
    static const short accumulative_offsets_from_0x4E00[] =
        {
            0,
            1,
            2,
            4,
            1,
            1,
            1,
            1,
            2,
            1,
            6,
            2,
            2,
            1,
            8,
            5,
            7,
            11,
            1,
            2,
            10,
            10,
            8,
            2,
            4,
            20,
            2,
            11,
            8,
            2,
            1,
            2,
            1,
            6,
            2,
            1,
            7,
            5,
            3,
            7,
            1,
            1,
            13,
            7,
            9,
            1,
            4,
            6,
            1,
            2,
            1,
            10,
            1,
            1,
            9,
            2,
            2,
            4,
            5,
            6,
            14,
            1,
            1,
            9,
            3,
            18,
            5,
            4,
            2,
            2,
            10,
            7,
            1,
            1,
            1,
            3,
            2,
            4,
            3,
            23,
            2,
            10,
            12,
            2,
            14,
            2,
            4,
            13,
            1,
            6,
            10,
            3,
            1,
            7,
            13,
            6,
            4,
            13,
            5,
            2,
            3,
            17,
            2,
            2,
            5,
            7,
            6,
            4,
            1,
            7,
            14,
            16,
            6,
            13,
            9,
            15,
            1,
            1,
            7,
            16,
            4,
            7,
            1,
            19,
            9,
            2,
            7,
            15,
            2,
            6,
            5,
            13,
            25,
            4,
            14,
            13,
            11,
            25,
            1,
            1,
            1,
            2,
            1,
            2,
            2,
            3,
            10,
            11,
            3,
            3,
            1,
            1,
            4,
            4,
            2,
            1,
            4,
            9,
            1,
            4,
            3,
            5,
            5,
            2,
            7,
            12,
            11,
            15,
            7,
            16,
            4,
            5,
            16,
            2,
            1,
            1,
            6,
            3,
            3,
            1,
            1,
            2,
            7,
            6,
            6,
            7,
            1,
            4,
            7,
            6,
            1,
            1,
            2,
            1,
            12,
            3,
            3,
            9,
            5,
            8,
            1,
            11,
            1,
            2,
            3,
            18,
            20,
            4,
            1,
            3,
            6,
            1,
            7,
            3,
            5,
            5,
            7,
            2,
            2,
            12,
            3,
            1,
            4,
            2,
            3,
            2,
            3,
            11,
            8,
            7,
            4,
            17,
            1,
            9,
            25,
            1,
            1,
            4,
            2,
            2,
            4,
            1,
            2,
            7,
            1,
            1,
            1,
            3,
            1,
            2,
            6,
            16,
            1,
            2,
            1,
            1,
            3,
            12,
            20,
            2,
            5,
            20,
            8,
            7,
            6,
            2,
            1,
            1,
            1,
            1,
            6,
            2,
            1,
            2,
            10,
            1,
            1,
            6,
            1,
            3,
            1,
            2,
            1,
            4,
            1,
            12,
            4,
            1,
            3,
            1,
            1,
            1,
            1,
            1,
            10,
            4,
            7,
            5,
            13,
            1,
            15,
            1,
            1,
            30,
            11,
            9,
            1,
            15,
            38,
            14,
            1,
            32,
            17,
            20,
            1,
            9,
            31,
            2,
            21,
            9,
            4,
            49,
            22,
            2,
            1,
            13,
            1,
            11,
            45,
            35,
            43,
            55,
            12,
            19,
            83,
            1,
            3,
            2,
            3,
            13,
            2,
            1,
            7,
            3,
            18,
            3,
            13,
            8,
            1,
            8,
            18,
            5,
            3,
            7,
            25,
            24,
            9,
            24,
            40,
            3,
            17,
            24,
            2,
            1,
            6,
            2,
            3,
            16,
            15,
            6,
            7,
            3,
            12,
            1,
            9,
            7,
            3,
            3,
            3,
            15,
            21,
            5,
            16,
            4,
            5,
            12,
            11,
            11,
            3,
            6,
            3,
            2,
            31,
            3,
            2,
            1,
            1,
            23,
            6,
            6,
            1,
            4,
            2,
            6,
            5,
            2,
            1,
            1,
            3,
            3,
            22,
            2,
            6,
            2,
            3,
            17,
            3,
            2,
            4,
            5,
            1,
            9,
            5,
            1,
            1,
            6,
            15,
            12,
            3,
            17,
            2,
            14,
            2,
            8,
            1,
            23,
            16,
            4,
            2,
            23,
            8,
            15,
            23,
            20,
            12,
            25,
            19,
            47,
            11,
            21,
            65,
            46,
            4,
            3,
            1,
            5,
            6,
            1,
            2,
            5,
            26,
            2,
            1,
            1,
            3,
            11,
            1,
            1,
            1,
            2,
            1,
            2,
            3,
            1,
            1,
            10,
            2,
            3,
            1,
            1,
            1,
            3,
            6,
            3,
            2,
            2,
            6,
            6,
            9,
            2,
            2,
            2,
            6,
            2,
            5,
            10,
            2,
            4,
            1,
            2,
            1,
            2,
            2,
            3,
            1,
            1,
            3,
            1,
            2,
            9,
            23,
            9,
            2,
            1,
            1,
            1,
            1,
            5,
            3,
            2,
            1,
            10,
            9,
            6,
            1,
            10,
            2,
            31,
            25,
            3,
            7,
            5,
            40,
            1,
            15,
            6,
            17,
            7,
            27,
            180,
            1,
            3,
            2,
            2,
            1,
            1,
            1,
            6,
            3,
            10,
            7,
            1,
            3,
            6,
            17,
            8,
            6,
            2,
            2,
            1,
            3,
            5,
            5,
            8,
            16,
            14,
            15,
            1,
            1,
            4,
            1,
            2,
            1,
            1,
            1,
            3,
            2,
            7,
            5,
            6,
            2,
            5,
            10,
            1,
            4,
            2,
            9,
            1,
            1,
            11,
            6,
            1,
            44,
            1,
            3,
            7,
            9,
            5,
            1,
            3,
            1,
            1,
            10,
            7,
            1,
            10,
            4,
            2,
            7,
            21,
            15,
            7,
            2,
            5,
            1,
            8,
            3,
            4,
            1,
            3,
            1,
            6,
            1,
            4,
            2,
            1,
            4,
            10,
            8,
            1,
            4,
            5,
            1,
            5,
            10,
            2,
            7,
            1,
            10,
            1,
            1,
            3,
            4,
            11,
            10,
            29,
            4,
            7,
            3,
            5,
            2,
            3,
            33,
            5,
            2,
            19,
            3,
            1,
            4,
            2,
            6,
            31,
            11,
            1,
            3,
            3,
            3,
            1,
            8,
            10,
            9,
            12,
            11,
            12,
            8,
            3,
            14,
            8,
            6,
            11,
            1,
            4,
            41,
            3,
            1,
            2,
            7,
            13,
            1,
            5,
            6,
            2,
            6,
            12,
            12,
            22,
            5,
            9,
            4,
            8,
            9,
            9,
            34,
            6,
            24,
            1,
            1,
            20,
            9,
            9,
            3,
            4,
            1,
            7,
            2,
            2,
            2,
            6,
            2,
            28,
            5,
            3,
            6,
            1,
            4,
            6,
            7,
            4,
            2,
            1,
            4,
            2,
            13,
            6,
            4,
            4,
            3,
            1,
            8,
            8,
            3,
            2,
            1,
            5,
            1,
            2,
            2,
            3,
            1,
            11,
            11,
            7,
            3,
            6,
            10,
            8,
            6,
            16,
            16,
            22,
            7,
            12,
            6,
            21,
            5,
            4,
            6,
            6,
            3,
            6,
            1,
            3,
            2,
            1,
            2,
            8,
            29,
            1,
            10,
            1,
            6,
            13,
            6,
            6,
            19,
            31,
            1,
            13,
            4,
            4,
            22,
            17,
            26,
            33,
            10,
            4,
            15,
            12,
            25,
            6,
            67,
            10,
            2,
            3,
            1,
            6,
            10,
            2,
            6,
            2,
            9,
            1,
            9,
            4,
            4,
            1,
            2,
            16,
            2,
            5,
            9,
            2,
            3,
            8,
            1,
            8,
            3,
            9,
            4,
            8,
            6,
            4,
            8,
            11,
            3,
            2,
            1,
            1,
            3,
            26,
            1,
            7,
            5,
            1,
            11,
            1,
            5,
            3,
            5,
            2,
            13,
            6,
            39,
            5,
            1,
            5,
            2,
            11,
            6,
            10,
            5,
            1,
            15,
            5,
            3,
            6,
            19,
            21,
            22,
            2,
            4,
            1,
            6,
            1,
            8,
            1,
            4,
            8,
            2,
            4,
            2,
            2,
            9,
            2,
            1,
            1,
            1,
            4,
            3,
            6,
            3,
            12,
            7,
            1,
            14,
            2,
            4,
            10,
            2,
            13,
            1,
            17,
            7,
            3,
            2,
            1,
            3,
            2,
            13,
            7,
            14,
            12,
            3,
            1,
            29,
            2,
            8,
            9,
            15,
            14,
            9,
            14,
            1,
            3,
            1,
            6,
            5,
            9,
            11,
            3,
            38,
            43,
            20,
            7,
            7,
            8,
            5,
            15,
            12,
            19,
            15,
            81,
            8,
            7,
            1,
            5,
            73,
            13,
            37,
            28,
            8,
            8,
            1,
            15,
            18,
            20,
            165,
            28,
            1,
            6,
            11,
            8,
            4,
            14,
            7,
            15,
            1,
            3,
            3,
            6,
            4,
            1,
            7,
            14,
            1,
            1,
            11,
            30,
            1,
            5,
            1,
            4,
            14,
            1,
            4,
            2,
            7,
            52,
            2,
            6,
            29,
            3,
            1,
            9,
            1,
            21,
            3,
            5,
            1,
            26,
            3,
            11,
            14,
            11,
            1,
            17,
            5,
            1,
            2,
            1,
            3,
            2,
            8,
            1,
            2,
            9,
            12,
            1,
            1,
            2,
            3,
            8,
            3,
            24,
            12,
            7,
            7,
            5,
            17,
            3,
            3,
            3,
            1,
            23,
            10,
            4,
            4,
            6,
            3,
            1,
            16,
            17,
            22,
            3,
            10,
            21,
            16,
            16,
            6,
            4,
            10,
            2,
            1,
            1,
            2,
            8,
            8,
            6,
            5,
            3,
            3,
            3,
            39,
            25,
            15,
            1,
            1,
            16,
            6,
            7,
            25,
            15,
            6,
            6,
            12,
            1,
            22,
            13,
            1,
            4,
            9,
            5,
            12,
            2,
            9,
            1,
            12,
            28,
            8,
            3,
            5,
            10,
            22,
            60,
            1,
            2,
            40,
            4,
            61,
            63,
            4,
            1,
            13,
            12,
            1,
            4,
            31,
            12,
            1,
            14,
            89,
            5,
            16,
            6,
            29,
            14,
            2,
            5,
            49,
            18,
            18,
            5,
            29,
            33,
            47,
            1,
            17,
            1,
            19,
            12,
            2,
            9,
            7,
            39,
            12,
            3,
            7,
            12,
            39,
            3,
            1,
            46,
            4,
            12,
            3,
            8,
            9,
            5,
            31,
            15,
            18,
            3,
            2,
            2,
            66,
            19,
            13,
            17,
            5,
            3,
            46,
            124,
            13,
            57,
            34,
            2,
            5,
            4,
            5,
            8,
            1,
            1,
            1,
            4,
            3,
            1,
            17,
            5,
            3,
            5,
            3,
            1,
            8,
            5,
            6,
            3,
            27,
            3,
            26,
            7,
            12,
            7,
            2,
            17,
            3,
            7,
            18,
            78,
            16,
            4,
            36,
            1,
            2,
            1,
            6,
            2,
            1,
            39,
            17,
            7,
            4,
            13,
            4,
            4,
            4,
            1,
            10,
            4,
            2,
            4,
            6,
            3,
            10,
            1,
            19,
            1,
            26,
            2,
            4,
            33,
            2,
            73,
            47,
            7,
            3,
            8,
            2,
            4,
            15,
            18,
            1,
            29,
            2,
            41,
            14,
            1,
            21,
            16,
            41,
            7,
            39,
            25,
            13,
            44,
            2,
            2,
            10,
            1,
            13,
            7,
            1,
            7,
            3,
            5,
            20,
            4,
            8,
            2,
            49,
            1,
            10,
            6,
            1,
            6,
            7,
            10,
            7,
            11,
            16,
            3,
            12,
            20,
            4,
            10,
            3,
            1,
            2,
            11,
            2,
            28,
            9,
            2,
            4,
            7,
            2,
            15,
            1,
            27,
            1,
            28,
            17,
            4,
            5,
            10,
            7,
            3,
            24,
            10,
            11,
            6,
            26,
            3,
            2,
            7,
            2,
            2,
            49,
            16,
            10,
            16,
            15,
            4,
            5,
            27,
            61,
            30,
            14,
            38,
            22,
            2,
            7,
            5,
            1,
            3,
            12,
            23,
            24,
            17,
            17,
            3,
            3,
            2,
            4,
            1,
            6,
            2,
            7,
            5,
            1,
            1,
            5,
            1,
            1,
            9,
            4,
            1,
            3,
            6,
            1,
            8,
            2,
            8,
            4,
            14,
            3,
            5,
            11,
            4,
            1,
            3,
            32,
            1,
            19,
            4,
            1,
            13,
            11,
            5,
            2,
            1,
            8,
            6,
            8,
            1,
            6,
            5,
            13,
            3,
            23,
            11,
            5,
            3,
            16,
            3,
            9,
            10,
            1,
            24,
            3,
            198,
            52,
            4,
            2,
            2,
            5,
            14,
            5,
            4,
            22,
            5,
            20,
            4,
            11,
            6,
            41,
            1,
            5,
            2,
            2,
            11,
            5,
            2,
            28,
            35,
            8,
            22,
            3,
            18,
            3,
            10,
            7,
            5,
            3,
            4,
            1,
            5,
            3,
            8,
            9,
            3,
            6,
            2,
            16,
            22,
            4,
            5,
            5,
            3,
            3,
            18,
            23,
            2,
            6,
            23,
            5,
            27,
            8,
            1,
            33,
            2,
            12,
            43,
            16,
            5,
            2,
            3,
            6,
            1,
            20,
            4,
            2,
            9,
            7,
            1,
            11,
            2,
            10,
            3,
            14,
            31,
            9,
            3,
            25,
            18,
            20,
            2,
            5,
            5,
            26,
            14,
            1,
            11,
            17,
            12,
            40,
            19,
            9,
            6,
            31,
            83,
            2,
            7,
            9,
            19,
            78,
            12,
            14,
            21,
            76,
            12,
            113,
            79,
            34,
            4,
            1,
            1,
            61,
            18,
            85,
            10,
            2,
            2,
            13,
            31,
            11,
            50,
            6,
            33,
            159,
            179,
            6,
            6,
            7,
            4,
            4,
            2,
            4,
            2,
            5,
            8,
            7,
            20,
            32,
            22,
            1,
            3,
            10,
            6,
            7,
            28,
            5,
            10,
            9,
            2,
            77,
            19,
            13,
            2,
            5,
            1,
            4,
            4,
            7,
            4,
            13,
            3,
            9,
            31,
            17,
            3,
            26,
            2,
            6,
            6,
            5,
            4,
            1,
            7,
            11,
            3,
            4,
            2,
            1,
            6,
            2,
            20,
            4,
            1,
            9,
            2,
            6,
            3,
            7,
            1,
            1,
            1,
            20,
            2,
            3,
            1,
            6,
            2,
            3,
            6,
            2,
            4,
            8,
            1,
            5,
            13,
            8,
            4,
            11,
            23,
            1,
            10,
            6,
            2,
            1,
            3,
            21,
            2,
            2,
            4,
            24,
            31,
            4,
            10,
            10,
            2,
            5,
            192,
            15,
            4,
            16,
            7,
            9,
            51,
            1,
            2,
            1,
            1,
            5,
            1,
            1,
            2,
            1,
            3,
            5,
            3,
            1,
            3,
            4,
            1,
            3,
            1,
            3,
            3,
            9,
            8,
            1,
            2,
            2,
            2,
            4,
            4,
            18,
            12,
            92,
            2,
            10,
            4,
            3,
            14,
            5,
            25,
            16,
            42,
            4,
            14,
            4,
            2,
            21,
            5,
            126,
            30,
            31,
            2,
            1,
            5,
            13,
            3,
            22,
            5,
            6,
            6,
            20,
            12,
            1,
            14,
            12,
            87,
            3,
            19,
            1,
            8,
            2,
            9,
            9,
            3,
            3,
            23,
            2,
            3,
            7,
            6,
            3,
            1,
            2,
            3,
            9,
            1,
            3,
            1,
            6,
            3,
            2,
            1,
            3,
            11,
            3,
            1,
            6,
            10,
            3,
            2,
            3,
            1,
            2,
            1,
            5,
            1,
            1,
            11,
            3,
            6,
            4,
            1,
            7,
            2,
            1,
            2,
            5,
            5,
            34,
            4,
            14,
            18,
            4,
            19,
            7,
            5,
            8,
            2,
            6,
            79,
            1,
            5,
            2,
            14,
            8,
            2,
            9,
            2,
            1,
            36,
            28,
            16,
            4,
            1,
            1,
            1,
            2,
            12,
            6,
            42,
            39,
            16,
            23,
            7,
            15,
            15,
            3,
            2,
            12,
            7,
            21,
            64,
            6,
            9,
            28,
            8,
            12,
            3,
            3,
            41,
            59,
            24,
            51,
            55,
            57,
            294,
            9,
            9,
            2,
            6,
            2,
            15,
            1,
            2,
            13,
            38,
            90,
            9,
            9,
            9,
            3,
            11,
            7,
            1,
            1,
            1,
            5,
            6,
            3,
            2,
            1,
            2,
            2,
            3,
            8,
            1,
            4,
            4,
            1,
            5,
            7,
            1,
            4,
            3,
            20,
            4,
            9,
            1,
            1,
            1,
            5,
            5,
            17,
            1,
            5,
            2,
            6,
            2,
            4,
            1,
            4,
            5,
            7,
            3,
            18,
            11,
            11,
            32,
            7,
            5,
            4,
            7,
            11,
            127,
            8,
            4,
            3,
            3,
            1,
            10,
            1,
            1,
            6,
            21,
            14,
            1,
            16,
            1,
            7,
            1,
            3,
            6,
            9,
            65,
            51,
            4,
            3,
            13,
            3,
            10,
            1,
            1,
            12,
            9,
            21,
            110,
            3,
            19,
            24,
            1,
            1,
            10,
            62,
            4,
            1,
            29,
            42,
            78,
            28,
            20,
            18,
            82,
            6,
            3,
            15,
            6,
            84,
            58,
            253,
            15,
            155,
            264,
            15,
            21,
            9,
            14,
            7,
            58,
            40,
            39,
        };
    static WideChar base_ranges[] = // not zero-terminated
        {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
            0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
            0x31F0, 0x31FF, // Katakana Phonetic Extensions
            0xFF00, 0xFFEF  // Half-width characters
        };
    static WideChar full_ranges[HK_ARRAY_SIZE(base_ranges) + HK_ARRAY_SIZE(accumulative_offsets_from_0x4E00) * 2 + 1] = {0};
    if (!full_ranges[0])
    {
        Platform::Memcpy(full_ranges, base_ranges, sizeof(base_ranges));
        UnpackAccumulativeOffsetsIntoRanges(0x4E00, accumulative_offsets_from_0x4E00, HK_ARRAY_SIZE(accumulative_offsets_from_0x4E00), full_ranges + HK_ARRAY_SIZE(base_ranges));
    }
    return &full_ranges[0];
}

static const unsigned short* GetGlyphRangesChineseFull()
{
    static const WideChar ranges[] =
        {
            0x0020,
            0x00FF, // Basic Latin + Latin Supplement
            0x2000,
            0x206F, // General Punctuation
            0x3000,
            0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
            0x31F0,
            0x31FF, // Katakana Phonetic Extensions
            0xFF00,
            0xFFEF, // Half-width characters
            0x4e00,
            0x9FAF, // CJK Ideograms
            0,
        };
    return &ranges[0];
}

static const unsigned short* GetGlyphRangesChineseSimplifiedCommon()
{
    // Store 2500 regularly used characters for Simplified Chinese.
    // Sourced from https://zh.wiktionary.org/wiki/%E9%99%84%E5%BD%95:%E7%8E%B0%E4%BB%A3%E6%B1%89%E8%AF%AD%E5%B8%B8%E7%94%A8%E5%AD%97%E8%A1%A8
    // This table covers 97.97% of all characters used during the month in July, 1987.
    static const short accumulative_offsets_from_0x4E00[] =
        {
            0, 1, 2, 4, 1, 1, 1, 1, 2, 1, 3, 2, 1, 2, 2, 1, 1, 1, 1, 1, 5, 2, 1, 2, 3, 3, 3, 2, 2, 4, 1, 1, 1, 2, 1, 5, 2, 3, 1, 2, 1, 2, 1, 1, 2, 1, 1, 2, 2, 1, 4, 1, 1, 1, 1, 5, 10, 1, 2, 19, 2, 1, 2, 1, 2, 1, 2, 1, 2,
            1, 5, 1, 6, 3, 2, 1, 2, 2, 1, 1, 1, 4, 8, 5, 1, 1, 4, 1, 1, 3, 1, 2, 1, 5, 1, 2, 1, 1, 1, 10, 1, 1, 5, 2, 4, 6, 1, 4, 2, 2, 2, 12, 2, 1, 1, 6, 1, 1, 1, 4, 1, 1, 4, 6, 5, 1, 4, 2, 2, 4, 10, 7, 1, 1, 4, 2, 4,
            2, 1, 4, 3, 6, 10, 12, 5, 7, 2, 14, 2, 9, 1, 1, 6, 7, 10, 4, 7, 13, 1, 5, 4, 8, 4, 1, 1, 2, 28, 5, 6, 1, 1, 5, 2, 5, 20, 2, 2, 9, 8, 11, 2, 9, 17, 1, 8, 6, 8, 27, 4, 6, 9, 20, 11, 27, 6, 68, 2, 2, 1, 1,
            1, 2, 1, 2, 2, 7, 6, 11, 3, 3, 1, 1, 3, 1, 2, 1, 1, 1, 1, 1, 3, 1, 1, 8, 3, 4, 1, 5, 7, 2, 1, 4, 4, 8, 4, 2, 1, 2, 1, 1, 4, 5, 6, 3, 6, 2, 12, 3, 1, 3, 9, 2, 4, 3, 4, 1, 5, 3, 3, 1, 3, 7, 1, 5, 1, 1, 1, 1, 2,
            3, 4, 5, 2, 3, 2, 6, 1, 1, 2, 1, 7, 1, 7, 3, 4, 5, 15, 2, 2, 1, 5, 3, 22, 19, 2, 1, 1, 1, 1, 2, 5, 1, 1, 1, 6, 1, 1, 12, 8, 2, 9, 18, 22, 4, 1, 1, 5, 1, 16, 1, 2, 7, 10, 15, 1, 1, 6, 2, 4, 1, 2, 4, 1, 6,
            1, 1, 3, 2, 4, 1, 6, 4, 5, 1, 2, 1, 1, 2, 1, 10, 3, 1, 3, 2, 1, 9, 3, 2, 5, 7, 2, 19, 4, 3, 6, 1, 1, 1, 1, 1, 4, 3, 2, 1, 1, 1, 2, 5, 3, 1, 1, 1, 2, 2, 1, 1, 2, 1, 1, 2, 1, 3, 1, 1, 1, 3, 7, 1, 4, 1, 1, 2, 1,
            1, 2, 1, 2, 4, 4, 3, 8, 1, 1, 1, 2, 1, 3, 5, 1, 3, 1, 3, 4, 6, 2, 2, 14, 4, 6, 6, 11, 9, 1, 15, 3, 1, 28, 5, 2, 5, 5, 3, 1, 3, 4, 5, 4, 6, 14, 3, 2, 3, 5, 21, 2, 7, 20, 10, 1, 2, 19, 2, 4, 28, 28, 2, 3,
            2, 1, 14, 4, 1, 26, 28, 42, 12, 40, 3, 52, 79, 5, 14, 17, 3, 2, 2, 11, 3, 4, 6, 3, 1, 8, 2, 23, 4, 5, 8, 10, 4, 2, 7, 3, 5, 1, 1, 6, 3, 1, 2, 2, 2, 5, 28, 1, 1, 7, 7, 20, 5, 3, 29, 3, 17, 26, 1, 8, 4,
            27, 3, 6, 11, 23, 5, 3, 4, 6, 13, 24, 16, 6, 5, 10, 25, 35, 7, 3, 2, 3, 3, 14, 3, 6, 2, 6, 1, 4, 2, 3, 8, 2, 1, 1, 3, 3, 3, 4, 1, 1, 13, 2, 2, 4, 5, 2, 1, 14, 14, 1, 2, 2, 1, 4, 5, 2, 3, 1, 14, 3, 12,
            3, 17, 2, 16, 5, 1, 2, 1, 8, 9, 3, 19, 4, 2, 2, 4, 17, 25, 21, 20, 28, 75, 1, 10, 29, 103, 4, 1, 2, 1, 1, 4, 2, 4, 1, 2, 3, 24, 2, 2, 2, 1, 1, 2, 1, 3, 8, 1, 1, 1, 2, 1, 1, 3, 1, 1, 1, 6, 1, 5, 3, 1, 1,
            1, 3, 4, 1, 1, 5, 2, 1, 5, 6, 13, 9, 16, 1, 1, 1, 1, 3, 2, 3, 2, 4, 5, 2, 5, 2, 2, 3, 7, 13, 7, 2, 2, 1, 1, 1, 1, 2, 3, 3, 2, 1, 6, 4, 9, 2, 1, 14, 2, 14, 2, 1, 18, 3, 4, 14, 4, 11, 41, 15, 23, 15, 23,
            176, 1, 3, 4, 1, 1, 1, 1, 5, 3, 1, 2, 3, 7, 3, 1, 1, 2, 1, 2, 4, 4, 6, 2, 4, 1, 9, 7, 1, 10, 5, 8, 16, 29, 1, 1, 2, 2, 3, 1, 3, 5, 2, 4, 5, 4, 1, 1, 2, 2, 3, 3, 7, 1, 6, 10, 1, 17, 1, 44, 4, 6, 2, 1, 1, 6,
            5, 4, 2, 10, 1, 6, 9, 2, 8, 1, 24, 1, 2, 13, 7, 8, 8, 2, 1, 4, 1, 3, 1, 3, 3, 5, 2, 5, 10, 9, 4, 9, 12, 2, 1, 6, 1, 10, 1, 1, 7, 7, 4, 10, 8, 3, 1, 13, 4, 3, 1, 6, 1, 3, 5, 2, 1, 2, 17, 16, 5, 2, 16, 6,
            1, 4, 2, 1, 3, 3, 6, 8, 5, 11, 11, 1, 3, 3, 2, 4, 6, 10, 9, 5, 7, 4, 7, 4, 7, 1, 1, 4, 2, 1, 3, 6, 8, 7, 1, 6, 11, 5, 5, 3, 24, 9, 4, 2, 7, 13, 5, 1, 8, 82, 16, 61, 1, 1, 1, 4, 2, 2, 16, 10, 3, 8, 1, 1,
            6, 4, 2, 1, 3, 1, 1, 1, 4, 3, 8, 4, 2, 2, 1, 1, 1, 1, 1, 6, 3, 5, 1, 1, 4, 6, 9, 2, 1, 1, 1, 2, 1, 7, 2, 1, 6, 1, 5, 4, 4, 3, 1, 8, 1, 3, 3, 1, 3, 2, 2, 2, 2, 3, 1, 6, 1, 2, 1, 2, 1, 3, 7, 1, 8, 2, 1, 2, 1, 5,
            2, 5, 3, 5, 10, 1, 2, 1, 1, 3, 2, 5, 11, 3, 9, 3, 5, 1, 1, 5, 9, 1, 2, 1, 5, 7, 9, 9, 8, 1, 3, 3, 3, 6, 8, 2, 3, 2, 1, 1, 32, 6, 1, 2, 15, 9, 3, 7, 13, 1, 3, 10, 13, 2, 14, 1, 13, 10, 2, 1, 3, 10, 4, 15,
            2, 15, 15, 10, 1, 3, 9, 6, 9, 32, 25, 26, 47, 7, 3, 2, 3, 1, 6, 3, 4, 3, 2, 8, 5, 4, 1, 9, 4, 2, 2, 19, 10, 6, 2, 3, 8, 1, 2, 2, 4, 2, 1, 9, 4, 4, 4, 6, 4, 8, 9, 2, 3, 1, 1, 1, 1, 3, 5, 5, 1, 3, 8, 4, 6,
            2, 1, 4, 12, 1, 5, 3, 7, 13, 2, 5, 8, 1, 6, 1, 2, 5, 14, 6, 1, 5, 2, 4, 8, 15, 5, 1, 23, 6, 62, 2, 10, 1, 1, 8, 1, 2, 2, 10, 4, 2, 2, 9, 2, 1, 1, 3, 2, 3, 1, 5, 3, 3, 2, 1, 3, 8, 1, 1, 1, 11, 3, 1, 1, 4,
            3, 7, 1, 14, 1, 2, 3, 12, 5, 2, 5, 1, 6, 7, 5, 7, 14, 11, 1, 3, 1, 8, 9, 12, 2, 1, 11, 8, 4, 4, 2, 6, 10, 9, 13, 1, 1, 3, 1, 5, 1, 3, 2, 4, 4, 1, 18, 2, 3, 14, 11, 4, 29, 4, 2, 7, 1, 3, 13, 9, 2, 2, 5,
            3, 5, 20, 7, 16, 8, 5, 72, 34, 6, 4, 22, 12, 12, 28, 45, 36, 9, 7, 39, 9, 191, 1, 1, 1, 4, 11, 8, 4, 9, 2, 3, 22, 1, 1, 1, 1, 4, 17, 1, 7, 7, 1, 11, 31, 10, 2, 4, 8, 2, 3, 2, 1, 4, 2, 16, 4, 32, 2,
            3, 19, 13, 4, 9, 1, 5, 2, 14, 8, 1, 1, 3, 6, 19, 6, 5, 1, 16, 6, 2, 10, 8, 5, 1, 2, 3, 1, 5, 5, 1, 11, 6, 6, 1, 3, 3, 2, 6, 3, 8, 1, 1, 4, 10, 7, 5, 7, 7, 5, 8, 9, 2, 1, 3, 4, 1, 1, 3, 1, 3, 3, 2, 6, 16,
            1, 4, 6, 3, 1, 10, 6, 1, 3, 15, 2, 9, 2, 10, 25, 13, 9, 16, 6, 2, 2, 10, 11, 4, 3, 9, 1, 2, 6, 6, 5, 4, 30, 40, 1, 10, 7, 12, 14, 33, 6, 3, 6, 7, 3, 1, 3, 1, 11, 14, 4, 9, 5, 12, 11, 49, 18, 51, 31,
            140, 31, 2, 2, 1, 5, 1, 8, 1, 10, 1, 4, 4, 3, 24, 1, 10, 1, 3, 6, 6, 16, 3, 4, 5, 2, 1, 4, 2, 57, 10, 6, 22, 2, 22, 3, 7, 22, 6, 10, 11, 36, 18, 16, 33, 36, 2, 5, 5, 1, 1, 1, 4, 10, 1, 4, 13, 2, 7,
            5, 2, 9, 3, 4, 1, 7, 43, 3, 7, 3, 9, 14, 7, 9, 1, 11, 1, 1, 3, 7, 4, 18, 13, 1, 14, 1, 3, 6, 10, 73, 2, 2, 30, 6, 1, 11, 18, 19, 13, 22, 3, 46, 42, 37, 89, 7, 3, 16, 34, 2, 2, 3, 9, 1, 7, 1, 1, 1, 2,
            2, 4, 10, 7, 3, 10, 3, 9, 5, 28, 9, 2, 6, 13, 7, 3, 1, 3, 10, 2, 7, 2, 11, 3, 6, 21, 54, 85, 2, 1, 4, 2, 2, 1, 39, 3, 21, 2, 2, 5, 1, 1, 1, 4, 1, 1, 3, 4, 15, 1, 3, 2, 4, 4, 2, 3, 8, 2, 20, 1, 8, 7, 13,
            4, 1, 26, 6, 2, 9, 34, 4, 21, 52, 10, 4, 4, 1, 5, 12, 2, 11, 1, 7, 2, 30, 12, 44, 2, 30, 1, 1, 3, 6, 16, 9, 17, 39, 82, 2, 2, 24, 7, 1, 7, 3, 16, 9, 14, 44, 2, 1, 2, 1, 2, 3, 5, 2, 4, 1, 6, 7, 5, 3,
            2, 6, 1, 11, 5, 11, 2, 1, 18, 19, 8, 1, 3, 24, 29, 2, 1, 3, 5, 2, 2, 1, 13, 6, 5, 1, 46, 11, 3, 5, 1, 1, 5, 8, 2, 10, 6, 12, 6, 3, 7, 11, 2, 4, 16, 13, 2, 5, 1, 1, 2, 2, 5, 2, 28, 5, 2, 23, 10, 8, 4,
            4, 22, 39, 95, 38, 8, 14, 9, 5, 1, 13, 5, 4, 3, 13, 12, 11, 1, 9, 1, 27, 37, 2, 5, 4, 4, 63, 211, 95, 2, 2, 2, 1, 3, 5, 2, 1, 1, 2, 2, 1, 1, 1, 3, 2, 4, 1, 2, 1, 1, 5, 2, 2, 1, 1, 2, 3, 1, 3, 1, 1, 1,
            3, 1, 4, 2, 1, 3, 6, 1, 1, 3, 7, 15, 5, 3, 2, 5, 3, 9, 11, 4, 2, 22, 1, 6, 3, 8, 7, 1, 4, 28, 4, 16, 3, 3, 25, 4, 4, 27, 27, 1, 4, 1, 2, 2, 7, 1, 3, 5, 2, 28, 8, 2, 14, 1, 8, 6, 16, 25, 3, 3, 3, 14, 3,
            3, 1, 1, 2, 1, 4, 6, 3, 8, 4, 1, 1, 1, 2, 3, 6, 10, 6, 2, 3, 18, 3, 2, 5, 5, 4, 3, 1, 5, 2, 5, 4, 23, 7, 6, 12, 6, 4, 17, 11, 9, 5, 1, 1, 10, 5, 12, 1, 1, 11, 26, 33, 7, 3, 6, 1, 17, 7, 1, 5, 12, 1, 11,
            2, 4, 1, 8, 14, 17, 23, 1, 2, 1, 7, 8, 16, 11, 9, 6, 5, 2, 6, 4, 16, 2, 8, 14, 1, 11, 8, 9, 1, 1, 1, 9, 25, 4, 11, 19, 7, 2, 15, 2, 12, 8, 52, 7, 5, 19, 2, 16, 4, 36, 8, 1, 16, 8, 24, 26, 4, 6, 2, 9,
            5, 4, 36, 3, 28, 12, 25, 15, 37, 27, 17, 12, 59, 38, 5, 32, 127, 1, 2, 9, 17, 14, 4, 1, 2, 1, 1, 8, 11, 50, 4, 14, 2, 19, 16, 4, 17, 5, 4, 5, 26, 12, 45, 2, 23, 45, 104, 30, 12, 8, 3, 10, 2, 2,
            3, 3, 1, 4, 20, 7, 2, 9, 6, 15, 2, 20, 1, 3, 16, 4, 11, 15, 6, 134, 2, 5, 59, 1, 2, 2, 2, 1, 9, 17, 3, 26, 137, 10, 211, 59, 1, 2, 4, 1, 4, 1, 1, 1, 2, 6, 2, 3, 1, 1, 2, 3, 2, 3, 1, 3, 4, 4, 2, 3, 3,
            1, 4, 3, 1, 7, 2, 2, 3, 1, 2, 1, 3, 3, 3, 2, 2, 3, 2, 1, 3, 14, 6, 1, 3, 2, 9, 6, 15, 27, 9, 34, 145, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 2, 2, 3, 1, 2, 1, 1, 1, 2, 3, 5, 8, 3, 5, 2, 4, 1, 3, 2, 2, 2, 12,
            4, 1, 1, 1, 10, 4, 5, 1, 20, 4, 16, 1, 15, 9, 5, 12, 2, 9, 2, 5, 4, 2, 26, 19, 7, 1, 26, 4, 30, 12, 15, 42, 1, 6, 8, 172, 1, 1, 4, 2, 1, 1, 11, 2, 2, 4, 2, 1, 2, 1, 10, 8, 1, 2, 1, 4, 5, 1, 2, 5, 1, 8,
            4, 1, 3, 4, 2, 1, 6, 2, 1, 3, 4, 1, 2, 1, 1, 1, 1, 12, 5, 7, 2, 4, 3, 1, 1, 1, 3, 3, 6, 1, 2, 2, 3, 3, 3, 2, 1, 2, 12, 14, 11, 6, 6, 4, 12, 2, 8, 1, 7, 10, 1, 35, 7, 4, 13, 15, 4, 3, 23, 21, 28, 52, 5,
            26, 5, 6, 1, 7, 10, 2, 7, 53, 3, 2, 1, 1, 1, 2, 163, 532, 1, 10, 11, 1, 3, 3, 4, 8, 2, 8, 6, 2, 2, 23, 22, 4, 2, 2, 4, 2, 1, 3, 1, 3, 3, 5, 9, 8, 2, 1, 2, 8, 1, 10, 2, 12, 21, 20, 15, 105, 2, 3, 1, 1,
            3, 2, 3, 1, 1, 2, 5, 1, 4, 15, 11, 19, 1, 1, 1, 1, 5, 4, 5, 1, 1, 2, 5, 3, 5, 12, 1, 2, 5, 1, 11, 1, 1, 15, 9, 1, 4, 5, 3, 26, 8, 2, 1, 3, 1, 1, 15, 19, 2, 12, 1, 2, 5, 2, 7, 2, 19, 2, 20, 6, 26, 7, 5,
            2, 2, 7, 34, 21, 13, 70, 2, 128, 1, 1, 2, 1, 1, 2, 1, 1, 3, 2, 2, 2, 15, 1, 4, 1, 3, 4, 42, 10, 6, 1, 49, 85, 8, 1, 2, 1, 1, 4, 4, 2, 3, 6, 1, 5, 7, 4, 3, 211, 4, 1, 2, 1, 2, 5, 1, 2, 4, 2, 2, 6, 5, 6,
            10, 3, 4, 48, 100, 6, 2, 16, 296, 5, 27, 387, 2, 2, 3, 7, 16, 8, 5, 38, 15, 39, 21, 9, 10, 3, 7, 59, 13, 27, 21, 47, 5, 21, 6};
    static WideChar base_ranges[] = // not zero-terminated
        {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
            0x2000, 0x206F, // General Punctuation
            0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
            0x31F0, 0x31FF, // Katakana Phonetic Extensions
            0xFF00, 0xFFEF  // Half-width characters
        };
    static WideChar full_ranges[HK_ARRAY_SIZE(base_ranges) + HK_ARRAY_SIZE(accumulative_offsets_from_0x4E00) * 2 + 1] = {0};
    if (!full_ranges[0])
    {
        Platform::Memcpy(full_ranges, base_ranges, sizeof(base_ranges));
        UnpackAccumulativeOffsetsIntoRanges(0x4E00, accumulative_offsets_from_0x4E00, HK_ARRAY_SIZE(accumulative_offsets_from_0x4E00), full_ranges + HK_ARRAY_SIZE(base_ranges));
    }
    return &full_ranges[0];
}

static const unsigned short* GetGlyphRangesCyrillic()
{
    static const WideChar ranges[] =
        {
            0x0020,
            0x00FF, // Basic Latin + Latin Supplement
            0x0400,
            0x052F, // Cyrillic + Cyrillic Supplement
            0x2DE0,
            0x2DFF, // Cyrillic Extended-A
            0xA640,
            0xA69F, // Cyrillic Extended-B
            0,
        };
    return &ranges[0];
}

static const unsigned short* GetGlyphRangesThai()
{
    static const WideChar ranges[] =
        {
            0x0020,
            0x00FF, // Basic Latin
            0x2010,
            0x205E, // Punctuations
            0x0E00,
            0x0E7F, // Thai
            0,
        };
    return &ranges[0];
}

static const unsigned short* GetGlyphRangesVietnamese()
{
    static const WideChar ranges[] =
        {
            0x0020,
            0x00FF, // Basic Latin
            0x0102,
            0x0103,
            0x0110,
            0x0111,
            0x0128,
            0x0129,
            0x0168,
            0x0169,
            0x01A0,
            0x01A1,
            0x01AF,
            0x01B0,
            0x1EA0,
            0x1EF9,
            0,
        };
    return &ranges[0];
}

// 2 value per range, values are inclusive, zero-terminated list
static const unsigned short* GetGlyphRange(EGlyphRange _GlyphRange)
{
    switch (_GlyphRange)
    {
        case GLYPH_RANGE_KOREAN:
            return GetGlyphRangesKorean();
        case GLYPH_RANGE_JAPANESE:
            return GetGlyphRangesJapanese();
        case GLYPH_RANGE_CHINESE_FULL:
            return GetGlyphRangesChineseFull();
        case GLYPH_RANGE_CHINESE_SIMPLIFIED_COMMON:
            return GetGlyphRangesChineseSimplifiedCommon();
        case GLYPH_RANGE_CYRILLIC:
            return GetGlyphRangesCyrillic();
        case GLYPH_RANGE_THAI:
            return GetGlyphRangesThai();
        case GLYPH_RANGE_VIETNAMESE:
            return GetGlyphRangesVietnamese();
        case GLYPH_RANGE_DEFAULT:
        default:
            break;
    }
    return GetGlyphRangesDefault();
}

HK_CLASS_META(AFont)

AFont::AFont()
{
}

AFont::~AFont()
{
    Purge();
}

void AFont::InitializeFromMemoryTTF(BlobRef Memory, SFontCreateInfo const* _pCreateInfo)
{
    Purge();

    SFontCreateInfo defaultCreateInfo;

    if (!_pCreateInfo)
    {
        _pCreateInfo = &defaultCreateInfo;

        defaultCreateInfo.SizePixels         = DefaultFontSize;
        defaultCreateInfo.GlyphRange         = GGlyphRange;
        defaultCreateInfo.OversampleH        = 3; // FIXME: 2 may be a better default?
        defaultCreateInfo.OversampleV        = 1;
        defaultCreateInfo.bPixelSnapH        = false;
        defaultCreateInfo.GlyphExtraSpacing  = Float2(0.0f, 0.0f);
        defaultCreateInfo.GlyphOffset        = Float2(0.0f, 0.0f);
        defaultCreateInfo.GlyphMinAdvanceX   = 0.0f;
        defaultCreateInfo.GlyphMaxAdvanceX   = Math::MaxValue<float>();
        defaultCreateInfo.RasterizerMultiply = 1.0f;
    }

    if (!Build(Memory.GetData(), Memory.Size(), _pCreateInfo))
    {
        return;
    }

    // Create atlas texture
    GEngine->GetRenderDevice()->CreateTexture(RenderCore::STextureDesc{}
                                                  .SetResolution(RenderCore::STextureResolution2D(TexWidth, TexHeight))
                                                  .SetFormat(TEXTURE_FORMAT_R8_UNORM)
                                                  .SetBindFlags(RenderCore::BIND_SHADER_RESOURCE)
                                                  .SetSwizzle(RenderCore::STextureSwizzle(RenderCore::TEXTURE_SWIZZLE_R,
                                                                                          RenderCore::TEXTURE_SWIZZLE_R,
                                                                                          RenderCore::TEXTURE_SWIZZLE_R,
                                                                                          RenderCore::TEXTURE_SWIZZLE_R)),
                                              &AtlasTexture);

    AtlasTexture->SetDebugName("Font Atlas Texture");
    AtlasTexture->Write(0, TexWidth * TexHeight, IsAligned(TexWidth, 4) ? 4 : 1, TexPixelsAlpha8);
}

void AFont::LoadInternalResource(AStringView _Path)
{
    if (!_Path.Icmp("/Default/Fonts/Default"))
    {

        // Load embedded ProggyClean.ttf

        // NOTE:
        // ProggyClean.ttf
        // Copyright (c) 2004, 2005 Tristan Grimmer
        // MIT license (see License.txt in http://www.upperbounds.net/download/ProggyClean.ttf.zip)
        // Download and more information at http://upperbounds.net

        SFontCreateInfo createInfo    = {};
        createInfo.FontNum            = 0;
        createInfo.SizePixels         = DefaultFontSize;
        createInfo.GlyphRange         = GGlyphRange;
        createInfo.OversampleH        = 1;
        createInfo.OversampleV        = 1;
        createInfo.bPixelSnapH        = true;
        createInfo.GlyphExtraSpacing  = Float2(0.0f, 0.0f);
        createInfo.GlyphOffset        = Float2(0.0f, 0.0f);
        createInfo.GlyphMinAdvanceX   = 0.0f;
        createInfo.GlyphMaxAdvanceX   = Math::MaxValue<float>();
        createInfo.RasterizerMultiply = 1.0f;

        AFile f = AFile::OpenRead("Fonts/ProggyClean.ttf", Runtime::GetEmbeddedResources());
        if (!f)
        {
            CriticalError("Failed to create default font\n");
        }

        InitializeFromMemoryTTF(BlobRef(f.GetHeapPtr(), f.SizeInBytes()), &createInfo);

        DrawOffset.Y = 1.0f;
        return;
    }

    LOG("Unknown internal font {}\n", _Path);

    LoadInternalResource("/Default/Fonts/Default");
}

bool AFont::LoadResource(IBinaryStreamReadInterface& Stream)
{
    Purge();

    AString text = Stream.AsString();

    SDocumentDeserializeInfo deserializeInfo;
    deserializeInfo.pDocumentData = text.CStr();
    deserializeInfo.bInsitu       = true;

    ADocument doc;
    doc.DeserializeFromString(deserializeInfo);

    ADocMember* fontMemb = doc.FindMember("Font");
    if (!fontMemb)
    {
        LOG("AFont::LoadResource: invalid font\n");
        return false;
    }

    auto fontFile = fontMemb->GetStringView();
    if (fontFile.IsEmpty())
    {
        LOG("AFont::LoadResource: invalid font\n");
        return false;
    }

    ABinaryResource* fontBinary = AResource::CreateFromFile<ABinaryResource>(fontFile);

    if (!fontBinary->GetSizeInBytes())
    {
        LOG("AFont::LoadResource: invalid font\n");
        return false;
    }

    SFontCreateInfo createInfo = {};
    createInfo.GlyphRange      = GGlyphRange;

    ADocMember* member;

    member             = doc.FindMember("FontNum");
    createInfo.FontNum = member ? Core::ParseInt32(member->GetStringView()) : 0;
    createInfo.FontNum = Math::Max(createInfo.FontNum, 0);

    member                = doc.FindMember("SizePixels");
    createInfo.SizePixels = member ? Core::ParseInt32(member->GetStringView()) : 18;
    createInfo.SizePixels = Math::Clamp(createInfo.SizePixels, 6, 80);

    member                 = doc.FindMember("OversampleH");
    createInfo.OversampleH = member ? Core::ParseInt32(member->GetStringView()) : 3; // FIXME: 2 may be a better default?
    createInfo.OversampleH = Math::Clamp(createInfo.OversampleH, 0, 10);

    member                 = doc.FindMember("OversampleV");
    createInfo.OversampleV = member ? Core::ParseInt32(member->GetStringView()) : 1;
    createInfo.OversampleV = Math::Clamp(createInfo.OversampleV, 0, 10);

    member                 = doc.FindMember("bPixelSnapH");
    createInfo.bPixelSnapH = member ? Core::ParseBool(member->GetStringView()) : false;

    member                         = doc.FindMember("GlyphExtraSpacingX");
    createInfo.GlyphExtraSpacing.X = member ? Core::ParseFloat(member->GetStringView()) : 0.0f;
    createInfo.GlyphExtraSpacing.X = Math::Clamp(createInfo.GlyphExtraSpacing.X, 0, 10);

    member                         = doc.FindMember("GlyphExtraSpacingY");
    createInfo.GlyphExtraSpacing.Y = member ? Core::ParseFloat(member->GetStringView()) : 0.0f;
    createInfo.GlyphExtraSpacing.Y = Math::Clamp(createInfo.GlyphExtraSpacing.Y, 0, 10);

    member                   = doc.FindMember("GlyphOffsetX");
    createInfo.GlyphOffset.X = member ? Core::ParseFloat(member->GetStringView()) : 0.0f;
    createInfo.GlyphOffset.X = Math::Clamp(createInfo.GlyphOffset.X, 0, 10);

    member                   = doc.FindMember("GlyphOffsetY");
    createInfo.GlyphOffset.Y = member ? Core::ParseFloat(member->GetStringView()) : 0.0f;
    createInfo.GlyphOffset.Y = Math::Clamp(createInfo.GlyphOffset.Y, 0, 10);

    member                      = doc.FindMember("GlyphMinAdvanceX");
    createInfo.GlyphMinAdvanceX = member ? Core::ParseFloat(member->GetStringView()) : 0.0f;
    createInfo.GlyphMinAdvanceX = Math::Max(createInfo.GlyphMinAdvanceX, 0.0f);

    member                      = doc.FindMember("GlyphMaxAdvanceX");
    createInfo.GlyphMaxAdvanceX = member ? Core::ParseFloat(member->GetStringView()) : Math::MaxValue<float>();
    createInfo.GlyphMaxAdvanceX = Math::Max(createInfo.GlyphMaxAdvanceX, createInfo.GlyphMinAdvanceX);

    member                        = doc.FindMember("RasterizerMultiply");
    createInfo.RasterizerMultiply = member ? Core::ParseFloat(member->GetStringView()) : 1.0f;
    createInfo.RasterizerMultiply = Math::Clamp(createInfo.RasterizerMultiply, 0.0f, 10.0f);

    InitializeFromMemoryTTF(BlobRef(fontBinary->GetBinaryData(), fontBinary->GetSizeInBytes()), &createInfo);

    return true;
}

void AFont::Purge()
{
    TexWidth         = 0;
    TexHeight        = 0;
    TexUvScale       = Float2(0.0f, 0.0f);
    TexUvWhitePixel  = Float2(0.0f, 0.0f);
    FallbackAdvanceX = 0.0f;
    FallbackGlyph    = NULL;
    Glyphs.Free();
    WideCharAdvanceX.Free();
    WideCharToGlyph.Free();
    CustomRects.Free();
    AtlasTexture = nullptr;
    Platform::GetHeapAllocator<HEAP_MISC>().Free(TexPixelsAlpha8);
    TexPixelsAlpha8 = nullptr;
}

Float2 AFont::CalcTextSizeA(float _Size, float _MaxWidth, float _WrapWidth, const char* _TextBegin, const char* _TextEnd, const char** _Remaining) const
{
    if (!IsValid())
    {
        return Float2::Zero();
    }

    if (!_TextEnd)
        _TextEnd = _TextBegin + strlen(_TextBegin); // FIXME-OPT: Need to avoid this.

    const float line_height = _Size;
    const float scale       = _Size / FontSize;

    Float2 text_size  = Float2(0, 0);
    float  line_width = 0.0f;

    const bool  word_wrap_enabled = (_WrapWidth > 0.0f);
    const char* word_wrap_eol     = NULL;

    const char* s = _TextBegin;
    while (s < _TextEnd)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
            {
                word_wrap_eol = CalcWordWrapPositionA(scale, s, _TextEnd, _WrapWidth - line_width);
                if (word_wrap_eol == s) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
                    word_wrap_eol++;    // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
            }

            if (s >= word_wrap_eol)
            {
                if (text_size.X < line_width)
                    text_size.X = line_width;
                text_size.Y += line_height;
                line_width    = 0.0f;
                word_wrap_eol = NULL;

                // Wrapping skips upcoming blanks
                while (s < _TextEnd)
                {
                    const char c = *s;
                    if (Core::CharIsBlank(c)) { s++; }
                    else if (c == '\n')
                    {
                        s++;
                        break;
                    }
                    else
                    {
                        break;
                    }
                }
                continue;
            }
        }

        // Decode and advance source
        const char* prev_s = s;
        WideChar   c      = (WideChar)*s;
        if (c < 0x80)
        {
            s += 1;
        }
        else
        {
            s += Core::WideCharDecodeUTF8(s, _TextEnd, c);
            if (c == 0) // Malformed UTF-8?
                break;
        }

        if (c < 32)
        {
            if (c == '\n')
            {
                text_size.X = Math::Max(text_size.X, line_width);
                text_size.Y += line_height;
                line_width = 0.0f;
                continue;
            }
            if (c == '\r')
                continue;
        }

        const float char_width = ((int)c < WideCharAdvanceX.Size() ? WideCharAdvanceX.ToPtr()[c] : FallbackAdvanceX) * scale;
        if (line_width + char_width >= _MaxWidth)
        {
            s = prev_s;
            break;
        }

        line_width += char_width;
    }

    if (text_size.X < line_width)
        text_size.X = line_width;

    if (line_width > 0 || text_size.Y == 0.0f)
        text_size.Y += line_height;

    if (_Remaining)
        *_Remaining = s;

    return text_size;
}

const char* AFont::CalcWordWrapPositionA(float _Scale, const char* _Text, const char* _TextEnd, float _WrapWidth) const
{
    if (!IsValid())
    {
        return _Text;
    }

    // Simple word-wrapping for English, not full-featured. Please submit failing cases!
    // FIXME: Much possible improvements (don't cut things like "word !", "word!!!" but cut within "word,,,,", more sensible support for punctuations, support for Unicode punctuations, etc.)

    // For references, possible wrap point marked with ^
    //  "aaa bbb, ccc,ddd. eee   fff. ggg!"
    //      ^    ^    ^   ^   ^__    ^    ^

    // List of hardcoded separators: .,;!?'"

    // Skip extra blanks after a line returns (that includes not counting them in width computation)
    // e.g. "Hello    world" --> "Hello" "World"

    // Cut words that cannot possibly fit within one line.
    // e.g.: "The tropical fish" with ~5 characters worth of width --> "The tr" "opical" "fish"

    float line_width  = 0.0f;
    float word_width  = 0.0f;
    float blank_width = 0.0f;
    _WrapWidth /= _Scale; // We work with unscaled widths to avoid scaling every characters

    const char* word_end      = _Text;
    const char* prev_word_end = NULL;
    bool        inside_word   = true;

    const char* s = _Text;
    while (s < _TextEnd)
    {
        WideChar   c = (WideChar)*s;
        const char* next_s;
        if (c < 0x80)
            next_s = s + 1;
        else
            next_s = s + Core::WideCharDecodeUTF8(s, _TextEnd, c);
        if (c == 0)
            break;

        if (c < 32)
        {
            if (c == '\n')
            {
                line_width = word_width = blank_width = 0.0f;
                inside_word                           = true;
                s                                     = next_s;
                continue;
            }
            if (c == '\r')
            {
                s = next_s;
                continue;
            }
        }

        const float char_width = ((int)c < WideCharAdvanceX.Size() ? WideCharAdvanceX.ToPtr()[c] : FallbackAdvanceX);
        if (Core::WideCharIsBlank(c))
        {
            if (inside_word)
            {
                line_width += blank_width;
                blank_width = 0.0f;
                word_end    = s;
            }
            blank_width += char_width;
            inside_word = false;
        }
        else
        {
            word_width += char_width;
            if (inside_word)
            {
                word_end = next_s;
            }
            else
            {
                prev_word_end = word_end;
                line_width += word_width + blank_width;
                word_width = blank_width = 0.0f;
            }

            // Allow wrapping after punctuation.
            inside_word = !(c == '.' || c == ',' || c == ';' || c == '!' || c == '?' || c == '\"');
        }

        // We ignore blank width at the end of the line (they can be skipped)
        if (line_width + word_width > _WrapWidth)
        {
            // Words that cannot possibly fit within an entire line will be cut anywhere.
            if (word_width < _WrapWidth)
                s = prev_word_end ? prev_word_end : word_end;
            break;
        }

        s = next_s;
    }

    return s;
}

WideChar const* AFont::CalcWordWrapPositionW(float _Scale, WideChar const* _Text, WideChar const* _TextEnd, float _WrapWidth) const
{
    if (!IsValid())
    {
        return _Text;
    }

    // Simple word-wrapping for English, not full-featured. Please submit failing cases!
    // FIXME: Much possible improvements (don't cut things like "word !", "word!!!" but cut within "word,,,,", more sensible support for punctuations, support for Unicode punctuations, etc.)

    // For references, possible wrap point marked with ^
    //  "aaa bbb, ccc,ddd. eee   fff. ggg!"
    //      ^    ^    ^   ^   ^__    ^    ^

    // List of hardcoded separators: .,;!?'"

    // Skip extra blanks after a line returns (that includes not counting them in width computation)
    // e.g. "Hello    world" --> "Hello" "World"

    // Cut words that cannot possibly fit within one line.
    // e.g.: "The tropical fish" with ~5 characters worth of width --> "The tr" "opical" "fish"

    float line_width  = 0.0f;
    float word_width  = 0.0f;
    float blank_width = 0.0f;
    _WrapWidth /= _Scale; // We work with unscaled widths to avoid scaling every characters

    WideChar const* word_end      = _Text;
    WideChar const* prev_word_end = NULL;
    bool             inside_word   = true;

    WideChar const* s = _Text;
    while (s < _TextEnd)
    {
        WideChar c = *s;

        if (c == 0)
            break;

        WideChar const* next_s = s + 1;

        if (c < 32)
        {
            if (c == '\n')
            {
                line_width = word_width = blank_width = 0.0f;
                inside_word                           = true;
                s                                     = next_s;
                continue;
            }
            if (c == '\r')
            {
                s = next_s;
                continue;
            }
        }

        const float char_width = ((int)c < WideCharAdvanceX.Size() ? WideCharAdvanceX.ToPtr()[c] : FallbackAdvanceX);
        if (Core::WideCharIsBlank(c))
        {
            if (inside_word)
            {
                line_width += blank_width;
                blank_width = 0.0f;
                word_end    = s;
            }
            blank_width += char_width;
            inside_word = false;
        }
        else
        {
            word_width += char_width;
            if (inside_word)
            {
                word_end = next_s;
            }
            else
            {
                prev_word_end = word_end;
                line_width += word_width + blank_width;
                word_width = blank_width = 0.0f;
            }

            // Allow wrapping after punctuation.
            inside_word = !(c == '.' || c == ',' || c == ';' || c == '!' || c == '?' || c == '\"');
        }

        // We ignore blank width at the end of the line (they can be skipped)
        if (line_width + word_width >= _WrapWidth)
        {
            // Words that cannot possibly fit within an entire line will be cut anywhere.
            if (word_width < _WrapWidth)
                s = prev_word_end ? prev_word_end : word_end;
            break;
        }

        s = next_s;
    }

    return s;
}

void AFont::SetDrawOffset(Float2 const& _Offset)
{
    DrawOffset = _Offset;
}

bool AFont::GetMouseCursorTexData(EDrawCursor cursor_type, Float2* out_offset, Float2* out_size, Float2 out_uv_border[2], Float2 out_uv_fill[2]) const
{
    if (cursor_type < DRAW_CURSOR_ARROW || cursor_type > DRAW_CURSOR_RESIZE_HAND)
        return false;

    SFontCustomRect const& r    = CustomRects[0];
    Float2                 pos  = CursorTexData[cursor_type][0] + Float2((float)r.X, (float)r.Y);
    Float2                 size = CursorTexData[cursor_type][1];
    *out_size                   = size;
    *out_offset                 = CursorTexData[cursor_type][2];
    out_uv_border[0]            = (pos)*TexUvScale;
    out_uv_border[1]            = (pos + size) * TexUvScale;
    pos.X += FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF + 1;
    out_uv_fill[0] = (pos)*TexUvScale;
    out_uv_fill[1] = (pos + size) * TexUvScale;
    return true;
}

void AFont::SetGlyphRanges(EGlyphRange _GlyphRange)
{
    GGlyphRange = _GlyphRange;
}

static void ImFontAtlasBuildMultiplyCalcLookupTable(unsigned char out_table[256], float in_brighten_factor)
{
    for (unsigned int i = 0; i < 256; i++)
    {
        unsigned int value = (unsigned int)(i * in_brighten_factor);
        out_table[i]       = value > 255 ? 255 : (value & 0xFF);
    }
}

static void ImFontAtlasBuildMultiplyRectAlpha8(const unsigned char table[256], unsigned char* pixels, int x, int y, int w, int h, int stride)
{
    unsigned char* data = pixels + x + y * stride;
    for (int j = h; j > 0; j--, data += stride)
        for (int i = 0; i < w; i++)
            data[i] = table[data[i]];
}

// Helper: ImBoolVector
// Store 1-bit per value. Note that Resize() currently clears the whole vector.
struct ImBoolVector
{
    TPodVector<int> Storage;
    ImBoolVector() {}
    void Resize(int sz)
    {
        Storage.ResizeInvalidate((sz + 31) >> 5);
        Storage.ZeroMem();
    }
    void Clear() { Storage.Clear(); }
    bool GetBit(int n) const
    {
        int off  = (n >> 5);
        int mask = 1 << (n & 31);
        return (Storage[off] & mask) != 0;
    }
    void SetBit(int n, bool v)
    {
        int off  = (n >> 5);
        int mask = 1 << (n & 31);
        if (v) Storage[off] |= mask;
        else
            Storage[off] &= ~mask;
    }
};

bool AFont::Build(const void* _SysMem, size_t _SizeInBytes, SFontCreateInfo const* _CreateInfo)
{
    HK_ASSERT(_SysMem != NULL && _SizeInBytes > 0);
    HK_ASSERT(_CreateInfo->SizePixels > 0.0f);

    SFontCreateInfo const& cfg = *_CreateInfo;
    FontSize                   = cfg.SizePixels;

    AddCustomRect(FONT_ATLAS_DEFAULT_TEX_DATA_ID, FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF * 2 + 1, FONT_ATLAS_DEFAULT_TEX_DATA_H);

    const int fontOffset = stbtt_GetFontOffsetForIndex((const unsigned char*)_SysMem, cfg.FontNum);
    if (fontOffset < 0)
    {
        LOG("AFont::Build: Invalid font\n");
        return false;
    }

    stbtt_fontinfo fontInfo = {};
    if (!stbtt_InitFont(&fontInfo, (const unsigned char*)_SysMem, fontOffset))
    {
        LOG("AFont::Build: Invalid font\n");
        return false;
    }

    WideChar const* glyphRanges = GetGlyphRange(cfg.GlyphRange);

    // Measure highest codepoints
    int glyphsHighest = 0;
    for (WideChar const* range = glyphRanges; range[0] && range[1]; range += 2)
    {
        glyphsHighest = Math::Max(glyphsHighest, (int)range[1]);
    }

    // 2. For every requested codepoint, check for their presence in the font data, and handle redundancy or overlaps between source fonts to avoid unused glyphs.
    int totalGlyphsCount = 0;

    ImBoolVector glyphsSet;
    glyphsSet.Resize(glyphsHighest + 1);

    for (WideChar const* range = glyphRanges; range[0] && range[1]; range += 2)
    {
        for (unsigned int codepoint = range[0]; codepoint <= range[1]; codepoint++)
        {
            if (glyphsSet.GetBit(codepoint))
            {
                LOG("Warning: duplicated glyph\n");
                continue;
            }

            if (!stbtt_FindGlyphIndex(&fontInfo, codepoint))
            { // It is actually in the font?
                continue;
            }

            // Add to avail set/counters
            glyphsSet.SetBit(codepoint, true);
            totalGlyphsCount++;
        }
    }

    if (totalGlyphsCount == 0)
    {
        LOG("AFont::Build: total glyph count = 0\n");
        return false;
    }

    // 3. Unpack our bit map into a flat list (we now have all the Unicode points that we know are requested _and_ available _and_ not overlapping another)
    TPodVector<int> glyphsList;
    glyphsList.Reserve(totalGlyphsCount);
    const int* it_begin = glyphsSet.Storage.begin();
    const int* it_end   = glyphsSet.Storage.end();
    for (const int* it = it_begin; it < it_end; it++)
        if (int entries_32 = *it)
            for (int bit_n = 0; bit_n < 32; bit_n++)
                if (entries_32 & (1u << bit_n))
                    glyphsList.Add((int)((it - it_begin) << 5) + bit_n);
    HK_ASSERT(glyphsList.Size() == totalGlyphsCount);

    // Allocate packing character data and flag packed characters buffer as non-packed (x0=y0=x1=y1=0)
    // (We technically don't need to zero-clear rects, but let's do it for the sake of sanity)

    // Rectangle to pack. We first fill in their size and the packer will give us their position.
    TPodVector<stbrp_rect> rects;
    rects.Resize(totalGlyphsCount);
    rects.ZeroMem();

    // Output glyphs
    TPodVector<stbtt_packedchar> packedChars;
    packedChars.Resize(totalGlyphsCount);
    packedChars.ZeroMem();

    // 4. Gather glyphs sizes so we can pack them in our virtual canvas.

    // Convert our ranges in the format stb_truetype wants
    stbtt_pack_range packRange                 = {}; // Hold the list of codepoints to pack (essentially points to Codepoints.Data)
    packRange.font_size                        = cfg.SizePixels;
    packRange.first_unicode_codepoint_in_range = 0;
    packRange.array_of_unicode_codepoints      = glyphsList.ToPtr();
    packRange.num_chars                        = glyphsList.Size();
    packRange.chardata_for_range               = packedChars.ToPtr();
    packRange.h_oversample                     = (unsigned char)cfg.OversampleH;
    packRange.v_oversample                     = (unsigned char)cfg.OversampleV;

    // Gather the sizes of all rectangles we will need to pack (this loop is based on stbtt_PackFontRangesGatherRects)
    const float scale   = (cfg.SizePixels > 0) ? stbtt_ScaleForPixelHeight(&fontInfo, cfg.SizePixels) : stbtt_ScaleForMappingEmToPixels(&fontInfo, -cfg.SizePixels);
    const int   padding = TexGlyphPadding;
    int         area    = 0;
    for (int glyph_i = 0; glyph_i < glyphsList.Size(); glyph_i++)
    {
        int       x0, y0, x1, y1;
        const int glyph_index_in_font = stbtt_FindGlyphIndex(&fontInfo, glyphsList[glyph_i]);
        HK_ASSERT(glyph_index_in_font != 0);
        stbtt_GetGlyphBitmapBoxSubpixel(&fontInfo, glyph_index_in_font, scale * cfg.OversampleH, scale * cfg.OversampleV, 0, 0, &x0, &y0, &x1, &y1);
        rects[glyph_i].w = (stbrp_coord)(x1 - x0 + padding + cfg.OversampleH - 1);
        rects[glyph_i].h = (stbrp_coord)(y1 - y0 + padding + cfg.OversampleV - 1);
        area += rects[glyph_i].w * rects[glyph_i].h;
    }

    // We need a width for the skyline algorithm, any width!
    // The exact width doesn't really matter much, but some API/GPU have texture size limitations and increasing width can decrease height.
    const int surface_sqrt = (int)std::sqrt((float)area) + 1;
    TexWidth               = (surface_sqrt >= 4096 * 0.7f) ? 4096 : (surface_sqrt >= 2048 * 0.7f) ? 2048 :
                      (surface_sqrt >= 1024 * 0.7f)                                               ? 1024 :
                                                                                                    512;
    TexHeight              = 0;

    // 5. Start packing
    // Pack our extra data rectangles first, so it will be on the upper-left corner of our texture (UV will have small values).
    const int          TEX_HEIGHT_MAX = 1024 * 32;
    stbtt_pack_context spc            = {};
    stbtt_PackBegin(&spc, NULL, TexWidth, TEX_HEIGHT_MAX, 0, TexGlyphPadding, NULL);

    stbrp_context* pack_context = (stbrp_context*)spc.pack_info;
    HK_ASSERT(pack_context != NULL);

    HK_ASSERT(CustomRects.Size() >= 1); // We expect at least the default custom rects to be registered, else something went wrong.

    TPodVector<stbrp_rect> pack_rects;
    pack_rects.Resize(CustomRects.Size());
    pack_rects.ZeroMem();
    for (int i = 0; i < CustomRects.Size(); i++)
    {
        pack_rects[i].w = CustomRects[i].Width;
        pack_rects[i].h = CustomRects[i].Height;
    }
    stbrp_pack_rects(pack_context, pack_rects.ToPtr(), pack_rects.Size());
    for (int i = 0; i < pack_rects.Size(); i++)
    {
        if (pack_rects[i].was_packed)
        {
            CustomRects[i].X = pack_rects[i].x;
            CustomRects[i].Y = pack_rects[i].y;
            HK_ASSERT(pack_rects[i].w == CustomRects[i].Width && pack_rects[i].h == CustomRects[i].Height);
            TexHeight = Math::Max(TexHeight, pack_rects[i].y + pack_rects[i].h);
        }
    }

    // 6. Pack each source font. No rendering yet, we are working with rectangles in an infinitely tall texture at this point.
    stbrp_pack_rects((stbrp_context*)spc.pack_info, rects.ToPtr(), totalGlyphsCount);

    // Extend texture height and mark missing glyphs as non-packed so we won't render them.
    // FIXME: We are not handling packing failure here (would happen if we got off TEX_HEIGHT_MAX or if a single if larger than TexWidth?)
    for (int glyph_i = 0; glyph_i < totalGlyphsCount; glyph_i++)
        if (rects[glyph_i].was_packed)
            TexHeight = Math::Max(TexHeight, rects[glyph_i].y + rects[glyph_i].h);

    // 7. Allocate texture
    TexHeight       = bTexNoPowerOfTwoHeight ? (TexHeight + 1) : Math::ToGreaterPowerOfTwo(TexHeight);
    TexUvScale      = Float2(1.0f / TexWidth, 1.0f / TexHeight);
    TexPixelsAlpha8 = (unsigned char*)Platform::GetHeapAllocator<HEAP_IMAGE>().Alloc(TexWidth * TexHeight, 16, MALLOC_ZERO);
    spc.pixels      = TexPixelsAlpha8;
    spc.height      = TexHeight;

    // 8. Render/rasterize font characters into the texture
    stbtt_PackFontRangesRenderIntoRects(&spc, &fontInfo, &packRange, 1, rects.ToPtr());

    // Apply multiply operator
    if (cfg.RasterizerMultiply != 1.0f)
    {
        unsigned char multiply_table[256];
        ImFontAtlasBuildMultiplyCalcLookupTable(multiply_table, cfg.RasterizerMultiply);
        stbrp_rect* r = &rects[0];
        for (int glyph_i = 0; glyph_i < totalGlyphsCount; glyph_i++, r++)
        {
            if (r->was_packed)
            {
                ImFontAtlasBuildMultiplyRectAlpha8(multiply_table, TexPixelsAlpha8, r->x, r->y, r->w, r->h, TexWidth);
            }
        }
    }

    // End packing
    stbtt_PackEnd(&spc);

    // 9. Setup AFont and glyphs for runtime
    const float font_scale = stbtt_ScaleForPixelHeight(&fontInfo, cfg.SizePixels);
    int         unscaled_ascent, unscaled_descent, unscaled_line_gap;
    stbtt_GetFontVMetrics(&fontInfo, &unscaled_ascent, &unscaled_descent, &unscaled_line_gap);

    const float ascent = Math::Floor(unscaled_ascent * font_scale + ((unscaled_ascent > 0.0f) ? +1 : -1));
    //const float descent = Math::Floor( unscaled_descent * font_scale + ((unscaled_descent > 0.0f) ? +1 : -1) );

    const float font_off_x = cfg.GlyphOffset.X;
    const float font_off_y = cfg.GlyphOffset.Y + Math::Round(ascent);

    for (int glyph_i = 0; glyph_i < totalGlyphsCount; glyph_i++)
    {
        const int               codepoint = glyphsList[glyph_i];
        const stbtt_packedchar& pc        = packedChars[glyph_i];

        const float char_advance_x_org = pc.xadvance;
        const float char_advance_x_mod = Math::Clamp(char_advance_x_org, cfg.GlyphMinAdvanceX, cfg.GlyphMaxAdvanceX);
        float       char_off_x         = font_off_x;
        if (char_advance_x_org != char_advance_x_mod)
        {
            char_off_x += cfg.bPixelSnapH ? Math::Floor((char_advance_x_mod - char_advance_x_org) * 0.5f) : (char_advance_x_mod - char_advance_x_org) * 0.5f;
        }

        // Register glyph
        stbtt_aligned_quad q;
        float              dummy_x = 0.0f, dummy_y = 0.0f;
        stbtt_GetPackedQuad(packedChars.ToPtr(), TexWidth, TexHeight, glyph_i, &dummy_x, &dummy_y, &q, 0);
        AddGlyph(cfg, (WideChar)codepoint, q.x0 + char_off_x, q.y0 + font_off_y, q.x1 + char_off_x, q.y1 + font_off_y, q.s0, q.t0, q.s1, q.t1, char_advance_x_mod);
    }

    // Render into our custom data block
    {
        SFontCustomRect& r = CustomRects[0];
        HK_ASSERT(r.Id == FONT_ATLAS_DEFAULT_TEX_DATA_ID);

        // Render/copy pixels
        HK_ASSERT(r.Width == FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF * 2 + 1 && r.Height == FONT_ATLAS_DEFAULT_TEX_DATA_H);
        for (int y = 0, n = 0; y < FONT_ATLAS_DEFAULT_TEX_DATA_H; y++)
        {
            for (int x = 0; x < FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF; x++, n++)
            {
                const int offset0        = (int)(r.X + x) + (int)(r.Y + y) * TexWidth;
                const int offset1        = offset0 + FONT_ATLAS_DEFAULT_TEX_DATA_W_HALF + 1;
                TexPixelsAlpha8[offset0] = FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS[n] == '.' ? 0xFF : 0x00;
                TexPixelsAlpha8[offset1] = FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS[n] == 'X' ? 0xFF : 0x00;
            }
        }

        TexUvWhitePixel = Float2((r.X + 0.5f) * TexUvScale.X, (r.Y + 0.5f) * TexUvScale.Y);
    }

    // Register custom rectangle glyphs
    for (int i = 0; i < CustomRects.Size(); i++)
    {
        SFontCustomRect const& r = CustomRects[i];
        if (r.Id >= 0x110000)
        {
            continue;
        }

        Float2 uv0 = Float2((float)r.X * TexUvScale.X, (float)r.Y * TexUvScale.Y);
        Float2 uv1 = Float2((float)(r.X + r.Width) * TexUvScale.X, (float)(r.Y + r.Height) * TexUvScale.Y);

        AddGlyph(cfg,
                 (WideChar)r.Id,
                 r.GlyphOffset.X, r.GlyphOffset.Y,
                 r.GlyphOffset.X + r.Width, r.GlyphOffset.Y + r.Height,
                 uv0.X, uv0.Y, uv1.X, uv1.Y,
                 r.GlyphAdvanceX);
    }

    HK_ASSERT(Glyphs.Size() < 0xFFFF); // -1 is reserved

    int widecharCount = 0;
    for (int i = 0; i != Glyphs.Size(); i++)
    {
        widecharCount = Math::Max(widecharCount, (int)Glyphs[i].Codepoint);
    }
    widecharCount = widecharCount + 1;

    WideCharAdvanceX.Resize(widecharCount);
    WideCharToGlyph.Resize(widecharCount);

    for (int n = 0; n < widecharCount; n++)
    {
        WideCharAdvanceX[n] = -1.0f;
        WideCharToGlyph[n]  = 0xffff;
    }

    for (int i = 0; i < Glyphs.Size(); i++)
    {
        int codepoint               = (int)Glyphs[i].Codepoint;
        WideCharAdvanceX[codepoint] = Glyphs[i].AdvanceX;
        WideCharToGlyph[codepoint]  = (unsigned short)i;

        // Ensure there is no TAB codepoint
        HK_ASSERT(codepoint != '\t');
    }

    // Create a glyph to handle TAB
    const int space = ' ';
    if (space < WideCharToGlyph.Size() && WideCharToGlyph[space] != 0xffff)
    {
        const int codepoint = '\t';
        if (codepoint < widecharCount)
        {
            SFontGlyph tabGlyph = Glyphs[WideCharToGlyph[space]];
            tabGlyph.Codepoint  = codepoint;
            tabGlyph.AdvanceX *= TabSize;
            Glyphs.Add(tabGlyph);
            WideCharAdvanceX[codepoint] = tabGlyph.AdvanceX;
            WideCharToGlyph[codepoint]  = (unsigned short)(Glyphs.Size() - 1);
        }
        else
        {
            LOG("AFont::Build: Warning: couldn't create TAB glyph\n");
        }
    }

    unsigned short fallbackGlyphNum = 0;
    if (FallbackChar < WideCharToGlyph.Size() && WideCharToGlyph[FallbackChar] != 0xffff)
    {
        fallbackGlyphNum = WideCharToGlyph[FallbackChar];
    }
    else
    {
        LOG("AFont::Build: Warning: fallback character not found\n");
    }

    FallbackGlyph    = &Glyphs[fallbackGlyphNum];
    FallbackAdvanceX = FallbackGlyph->AdvanceX;

    for (int i = 0; i < widecharCount; i++)
    {
        if (WideCharAdvanceX[i] < 0.0f)
        {
            WideCharAdvanceX[i] = FallbackAdvanceX;
        }
        if (WideCharToGlyph[i] == 0xffff)
        {
            WideCharToGlyph[i] = fallbackGlyphNum;
        }
    }

    return true;
}

// x0/y0/x1/y1 are offset from the character upper-left layout position, in pixels. Therefore x0/y0 are often fairly close to zero.
// Not to be mistaken with texture coordinates, which are held by u0/v0/u1/v1 in normalized format (0.0..1.0 on each texture axis).
void AFont::AddGlyph(SFontCreateInfo const& cfg, WideChar codepoint, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, float advance_x)
{
    Glyphs.Resize(Glyphs.Size() + 1);
    SFontGlyph& glyph = Glyphs.Last();
    glyph.Codepoint   = (WideChar)codepoint;
    glyph.X0          = x0;
    glyph.Y0          = y0;
    glyph.X1          = x1;
    glyph.Y1          = y1;
    glyph.U0          = u0;
    glyph.V0          = v0;
    glyph.U1          = u1;
    glyph.V1          = v1;
    glyph.AdvanceX    = advance_x + cfg.GlyphExtraSpacing.X; // Bake spacing into AdvanceX

    if (cfg.bPixelSnapH)
        glyph.AdvanceX = Math::Round(glyph.AdvanceX);
}

int AFont::AddCustomRect(unsigned int id, int width, int height)
{
    // Id needs to be >= 0x110000. Id >= 0x80000000 are reserved for internal use
    HK_ASSERT(id >= 0x110000);
    HK_ASSERT(width > 0 && width <= 0xFFFF);
    HK_ASSERT(height > 0 && height <= 0xFFFF);
    SFontCustomRect r;
    r.Id     = id;
    r.Width  = (unsigned short)width;
    r.Height = (unsigned short)height;
    r.X = r.Y       = 0xFFFF;
    r.GlyphAdvanceX = 0.0f;
    r.GlyphOffset   = Float2(0, 0);
    CustomRects.Add(r);
    return CustomRects.Size() - 1; // Return index
}
