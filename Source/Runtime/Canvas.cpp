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

#include "Canvas.h"
#include "PlayerController.h"
#include "HUD.h"
#include "Texture.h"
#include "ResourceManager.h"

#include <Platform/Utf8.h>
#include <Platform/Logger.h>

ACanvas::ACanvas()
    : DrawList( &DrawListSharedData )
{
    DrawList._Data = &DrawListSharedData;
}


ACanvas::~ACanvas()
{
    DrawList.ClearFreeMemory();
    Viewports.Free();
}

AFont * ACanvas::GetDefaultFont()
{
    static TStaticResourceFinder< AFont > FontResource( _CTS( "/Root/fonts/RobotoMono-Regular18.font"  ) );
    return FontResource.GetObject();
}

void ACanvas::Begin( int _Width, int _Height ) {
    AN_ASSERT( FontStack.IsEmpty() );

    Width = _Width;
    Height = _Height;
    DrawList.Clear();
    Viewports.Clear();

    DrawListSharedData.ClipRectFullscreen.x = 0;
    DrawListSharedData.ClipRectFullscreen.y = 0;
    DrawListSharedData.ClipRectFullscreen.z = _Width;
    DrawListSharedData.ClipRectFullscreen.w = _Height;

    PushFont( GetDefaultFont() );
    PushClipRectFullScreen();
}

void ACanvas::End() {
    PopClipRect();
    PopFont();
    if ( DrawList.CmdBuffer.Size > 0 ) {
        if ( DrawList.CmdBuffer.back().ElemCount == 0 ) {
            DrawList.CmdBuffer.pop_back();
        }
    }
}

void ACanvas::PushClipRect( Float2 const & _Mins, Float2 const & _Maxs, bool _IntersectWithCurrentClipRect ) {
    DrawList.PushClipRect( _Mins, _Maxs, _IntersectWithCurrentClipRect );
}

void ACanvas::PushClipRectFullScreen() {
    DrawList.PushClipRect( Float2(0, 0), Float2(Width, Height) );
}

void ACanvas::PopClipRect() {
    DrawList.PopClipRect();
}

void ACanvas::PushBlendingState( EColorBlending _Blending ) {
    DrawList.PushBlendingState( HUD_DRAW_CMD_ALPHA | ( _Blending << 8 ) );
}

void ACanvas::PopBlendingState() {
    DrawList.PopBlendingState();
}

void ACanvas::SetCurrentFont( AFont const * _Font ) {
    if ( _Font ) {
        DrawListSharedData.TexUvWhitePixel = _Font->GetUVWhitePixel();
        DrawListSharedData.FontSize = _Font->GetFontSize();
        DrawListSharedData.Font = _Font;
    } else {
        DrawListSharedData.TexUvWhitePixel = Float2::Zero();
        DrawListSharedData.FontSize = 13;
        DrawListSharedData.Font = nullptr;
    }
}

void ACanvas::PushFont( AFont const * _Font ) {
    SetCurrentFont( _Font );
    FontStack.Append( _Font );
    DrawList.PushTextureID( _Font->GetTexture()->GetGPUResource() );
}

void ACanvas::PopFont() {
    if ( FontStack.IsEmpty() ) {
        GLogger.Printf( "ACanvas::PopFont: stack was corrupted\n" );
        return;
    }
    DrawList.PopTextureID();
    FontStack.RemoveLast();
    SetCurrentFont( FontStack.IsEmpty() ? nullptr : FontStack.Last() );
}

void ACanvas::DrawLine( Float2 const & a, Float2 const & b, Color4 const & col, float thickness ) {
    DrawList.AddLine( a, b, col.GetDWord(), thickness );
}

void ACanvas::DrawRect( Float2 const & a, Float2 const & b, Color4 const & col, float rounding, int _RoundingCorners, float thickness ) {
    DrawList.AddRect( a, b, col.GetDWord(), rounding, _RoundingCorners, thickness );
}

void ACanvas::DrawRectFilled( Float2 const & a, Float2 const & b, Color4 const & col, float rounding, int _RoundingCorners ) {
    DrawList.AddRectFilled( a, b, col.GetDWord(), rounding, _RoundingCorners );
}

void ACanvas::DrawRectFilledMultiColor( Float2 const & a, Float2 const & b, Color4 const & col_upr_left, Color4 const & col_upr_right, Color4 const & col_bot_right, Color4 const & col_bot_left ) {
    DrawList.AddRectFilledMultiColor( a, b, col_upr_left.GetDWord(), col_upr_right.GetDWord(), col_bot_right.GetDWord(), col_bot_left.GetDWord() );
}

void ACanvas::DrawQuad( Float2 const & a, Float2 const & b, Float2 const & c, Float2 const & d, Color4 const & col, float thickness ) {
    DrawList.AddQuad( a, b, c, d, col.GetDWord(), thickness );
}

void ACanvas::DrawQuadFilled( Float2 const & a, Float2 const & b, Float2 const & c, Float2 const & d, Color4 const & col ) {
    DrawList.AddQuadFilled( a, b, c, d, col.GetDWord() );
}

void ACanvas::DrawTriangle( Float2 const & a, Float2 const & b, Float2 const & c, Color4 const & col, float thickness ) {
    DrawList.AddTriangle( a, b, c, col.GetDWord(), thickness );
}

void ACanvas::DrawTriangleFilled( Float2 const & a, Float2 const & b, Float2 const & c, Color4 const & col ) {
    DrawList.AddTriangleFilled( a, b, c, col.GetDWord() );
}

void ACanvas::DrawCircle( Float2 const & centre, float radius, Color4 const & col, int num_segments, float thickness ) {
    DrawList.AddCircle( centre, radius, col.GetDWord(), num_segments, thickness );
}

void ACanvas::DrawCircleFilled( Float2 const & centre, float radius, Color4 const & col, int num_segments ) {
    DrawList.AddCircleFilled( centre, radius, col.GetDWord(), num_segments );
}

void ACanvas::DrawTextUTF8( Float2 const & pos, Color4 const & col, const char* _TextBegin, const char* _TextEnd, bool bShadow ) {
    DrawTextUTF8( DrawListSharedData.FontSize, pos, col, _TextBegin, _TextEnd, 0, nullptr, bShadow );
}

void ACanvas::DrawTextUTF8( float _FontSize, Float2 const & _Pos, Color4 const & _Color, const char* _TextBegin, const char* _TextEnd, float _WrapWidth, Float4 const * _CPUFineClipRect, bool bShadow ) {
    if ( bShadow ) {
        _DrawTextUTF8( _FontSize, _Pos + Float2( 1, 1 ), Color4::Black(), _TextBegin, _TextEnd, _WrapWidth, _CPUFineClipRect );
    }
    _DrawTextUTF8( _FontSize, _Pos, _Color, _TextBegin, _TextEnd, _WrapWidth, _CPUFineClipRect );
}

void ACanvas::_DrawTextUTF8( float _FontSize, Float2 const & _Pos, Color4 const & _Color, const char* _TextBegin, const char* _TextEnd, float _WrapWidth, Float4 const * _CPUFineClipRect ) {
    if ( _Color.IsTransparent() ) {
        return;
    }

    if ( !_TextEnd ) {
        _TextEnd = _TextBegin + strlen(_TextBegin);
    }

    if ( _TextBegin == _TextEnd ) {
        return;
    }

    AFont const * font = GetCurrentFont();

    if ( !font->IsValid() ) {
        return;
    }

    uint32_t color = _Color.GetDWord();

    AN_ASSERT( const_cast< AFont * >( font )->GetTexture()->GetGPUResource() == DrawList._TextureIdStack.back() );

    Float4 clipRect = DrawList._ClipRectStack.back();
    if ( _CPUFineClipRect ) {
        clipRect.X = Math::Max( clipRect.X, _CPUFineClipRect->X );
        clipRect.Y = Math::Max( clipRect.Y, _CPUFineClipRect->Y );
        clipRect.Z = Math::Min( clipRect.Z, _CPUFineClipRect->Z );
        clipRect.W = Math::Min( clipRect.W, _CPUFineClipRect->W );
    }

    //font->RenderText( &DrawList, _FontSize, _Pos, _Color, clipRect, _TextBegin, _TextEnd, _WrapWidth, _CPUFineClipRect != NULL );

    Float2 const & fontOffset = font->GetDrawOffset();

    // Align to be pixel perfect
    Float2 pos;
    pos.X = (float)(int)_Pos.X + fontOffset.X;
    pos.Y = (float)(int)_Pos.Y + fontOffset.Y;
    float x = pos.X;
    float y = pos.Y;
    if (y > clipRect.W)
        return;

    const float scale = _FontSize / font->GetFontSize();
    const float lineHeight = _FontSize;
    const bool bWordWrap = (_WrapWidth > 0.0f);
    const char* wordWrapEOL = NULL;

    // Fast-forward to first visible line
    const char* s = _TextBegin;
    if ( y + lineHeight < clipRect.Y && !bWordWrap ) {
        while ( y + lineHeight < clipRect.Y && s < _TextEnd ) {
            s = (const char*)memchr(s, '\n', _TextEnd - s);
            s = s ? s + 1 : _TextEnd;
            y += lineHeight;
        }
    }

    // For large text, scan for the last visible line in order to avoid over-reserving in the call to PrimReserve()
    // Note that very large horizontal line will still be affected by the issue (e.g. a one megabyte string buffer without a newline will likely crash atm)
    if ( _TextEnd - s > 10000 && !bWordWrap ) {
        const char* s_end = s;
        float y_end = y;
        while ( y_end < clipRect.W && s_end < _TextEnd ) {
            s_end = (const char*)memchr(s_end, '\n', _TextEnd - s_end);
            s_end = s_end ? s_end + 1 : _TextEnd;
            y_end += lineHeight;
        }
        _TextEnd = s_end;
    }
    if ( s == _TextEnd ) {
        return;
    }

    // Reserve vertices for remaining worse case (over-reserving is useful and easily amortized)
    const int MaxVertices = (int)(_TextEnd - s) * 4;
    const int MaxIndices = (int)(_TextEnd - s) * 6;
    const int ReservedIndicesCount = DrawList.IdxBuffer.Size + MaxIndices;
    DrawList.PrimReserve( MaxIndices, MaxVertices );

    ImDrawVert * pVertices = DrawList._VtxWritePtr;
    ImDrawIdx * pIndices = DrawList._IdxWritePtr;
    unsigned int firstVertex = DrawList._VtxCurrentIdx;

    while ( s < _TextEnd ) {
        if ( bWordWrap ) {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if ( !wordWrapEOL ) {
                wordWrapEOL = font->CalcWordWrapPositionA(scale, s, _TextEnd, _WrapWidth - (x - pos.X));
                if ( wordWrapEOL == s ) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
                    wordWrapEOL++;      // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
            }

            if ( s >= wordWrapEOL ) {
                x = pos.X;
                y += lineHeight;
                wordWrapEOL = NULL;

                // Wrapping skips upcoming blanks
                while ( s < _TextEnd ) {
                    const char c = *s;
                    if ( Core::CharIsBlank(c) ) {
                        s++;
                    } else if (c == '\n') {
                        s++;
                        break;
                    } else {
                        break;
                    }
                }
                continue;
            }
        }

        // Decode and advance source
        SWideChar c = (SWideChar)*s;
        if ( c < 0x80 ) {
            s += 1;
        } else {
            s += Core::WideCharDecodeUTF8( s, _TextEnd, c );
            if ( c == 0 ) // Malformed UTF-8?
                break;
        }

        if ( c < 32 ) {
            if ( c == '\n' ) {
                x = pos.X;
                y += lineHeight;
                if ( y > clipRect.W )
                    break; // break out of main loop
                continue;
            }
            if ( c == '\r' )
                continue;
        }

        SFontGlyph const * glyph = font->GetGlyph( c );
        float charWidth = glyph->AdvanceX * scale;

        // Arbitrarily assume that both space and tabs are empty glyphs as an optimization
        if ( c != ' ' && c != '\t' ) {
            // We don't do a second finer clipping test on the Y axis as we've already skipped anything before clipRect.Y and exit once we pass clipRect.W
            float x1 = x + glyph->X0 * scale;
            float x2 = x + glyph->X1 * scale;
            float y1 = y + glyph->Y0 * scale;
            float y2 = y + glyph->Y1 * scale;
            if ( x1 <= clipRect.Z && x2 >= clipRect.X ) {
                // Render a character
                float u1 = glyph->U0;
                float v1 = glyph->V0;
                float u2 = glyph->U1;
                float v2 = glyph->V1;

                // CPU side clipping used to fit text in their frame when the frame is too small. Only does clipping for axis aligned quads.
                if ( _CPUFineClipRect ) {
                    if ( x1 < clipRect.X ) {
                        u1 = u1 + (1.0f - (x2 - clipRect.X) / (x2 - x1)) * (u2 - u1);
                        x1 = clipRect.X;
                    }
                    if ( y1 < clipRect.Y ) {
                        v1 = v1 + (1.0f - (y2 - clipRect.Y) / (y2 - y1)) * (v2 - v1);
                        y1 = clipRect.Y;
                    }
                    if ( x2 > clipRect.Z ) {
                        u2 = u1 + ((clipRect.Z - x1) / (x2 - x1)) * (u2 - u1);
                        x2 = clipRect.Z;
                    }
                    if ( y2 > clipRect.W ) {
                        v2 = v1 + ((clipRect.W - y1) / (y2 - y1)) * (v2 - v1);
                        y2 = clipRect.W;
                    }
                    if ( y1 >= y2 ) {
                        x += charWidth;
                        continue;
                    }
                }

                pIndices[0] = firstVertex;
                pIndices[1] = firstVertex+1;
                pIndices[2] = firstVertex+2;
                pIndices[3] = firstVertex;
                pIndices[4] = firstVertex+2;
                pIndices[5] = firstVertex+3;
                pVertices[0].pos.x = x1;
                pVertices[0].pos.y = y1;
                pVertices[0].col = color;
                pVertices[0].uv.x = u1;
                pVertices[0].uv.y = v1;
                pVertices[1].pos.x = x2;
                pVertices[1].pos.y = y1;
                pVertices[1].col = color;
                pVertices[1].uv.x = u2;
                pVertices[1].uv.y = v1;
                pVertices[2].pos.x = x2;
                pVertices[2].pos.y = y2;
                pVertices[2].col = color;
                pVertices[2].uv.x = u2;
                pVertices[2].uv.y = v2;
                pVertices[3].pos.x = x1;
                pVertices[3].pos.y = y2;
                pVertices[3].col = color;
                pVertices[3].uv.x = u1;
                pVertices[3].uv.y = v2;
                pVertices += 4;
                firstVertex += 4;
                pIndices += 6;
            }
        }

        x += charWidth;
    }

    // Give back unused vertices
    DrawList.VtxBuffer.resize((int)(pVertices - DrawList.VtxBuffer.Data));
    DrawList.IdxBuffer.resize((int)(pIndices - DrawList.IdxBuffer.Data));
    DrawList.CmdBuffer[DrawList.CmdBuffer.Size-1].ElemCount -= (ReservedIndicesCount - DrawList.IdxBuffer.Size);
    DrawList._VtxWritePtr = pVertices;
    DrawList._IdxWritePtr = pIndices;
    DrawList._VtxCurrentIdx = (unsigned int)DrawList.VtxBuffer.Size;
}

void ACanvas::DrawTextWChar( Float2 const & pos, Color4 const & col, SWideChar const * _TextBegin, SWideChar const * _TextEnd, bool bShadow ) {
    DrawTextWChar( DrawListSharedData.FontSize, pos, col, _TextBegin, _TextEnd, 0, nullptr, bShadow );
}

void ACanvas::DrawTextWChar( float _FontSize, Float2 const & _Pos, Color4 const & _Color, SWideChar const * _TextBegin, SWideChar const * _TextEnd, float _WrapWidth, Float4 const * _CPUFineClipRect, bool bShadow ) {
    if ( bShadow ) {
        _DrawTextWChar( _FontSize, _Pos + Float2(1,1), Color4::Black(), _TextBegin, _TextEnd, _WrapWidth, _CPUFineClipRect );
    }
    _DrawTextWChar( _FontSize, _Pos, _Color, _TextBegin, _TextEnd, _WrapWidth, _CPUFineClipRect );
}

void ACanvas::_DrawTextWChar( float _FontSize, Float2 const & _Pos, Color4 const & _Color, SWideChar const * _TextBegin, SWideChar const * _TextEnd, float _WrapWidth, Float4 const * _CPUFineClipRect ) {
    if ( _Color.IsTransparent() ) {
        return;
    }

    if ( !_TextEnd ) {
        _TextEnd = _TextBegin + Core::WideStrLength( _TextBegin );
    }

    if ( _TextBegin == _TextEnd ) {
        return;
    }

    AFont const * font = GetCurrentFont();

    AN_ASSERT( font && _FontSize > 0.0f );

    if ( !font->IsValid() ) {
        return;
    }

    uint32_t color = _Color.GetDWord();

    AN_ASSERT( const_cast< AFont * >( font )->GetTexture()->GetGPUResource() == DrawList._TextureIdStack.back() );

    Float4 clipRect = DrawList._ClipRectStack.back();
    if ( _CPUFineClipRect ) {
        clipRect.X = Math::Max( clipRect.X, _CPUFineClipRect->X );
        clipRect.Y = Math::Max( clipRect.Y, _CPUFineClipRect->Y );
        clipRect.Z = Math::Min( clipRect.Z, _CPUFineClipRect->Z );
        clipRect.W = Math::Min( clipRect.W, _CPUFineClipRect->W );
    }

    //font->RenderText( &DrawList, _FontSize, _Pos, _Color, clipRect, _TextBegin, _TextEnd, _WrapWidth, _CPUFineClipRect != NULL );

    Float2 const & fontOffset = font->GetDrawOffset();

    // Align to be pixel perfect
    Float2 pos;
    pos.X = ( float )( int )_Pos.X + fontOffset.X;
    pos.Y = ( float )( int )_Pos.Y + fontOffset.Y;
    float x = pos.X;
    float y = pos.Y;
    if ( y > clipRect.W )
        return;

    const float scale = _FontSize / font->GetFontSize();
    const float lineHeight = _FontSize;
    const bool bWordWrap = ( _WrapWidth > 0.0f );
    SWideChar const * wordWrapEOL = NULL;

    // Fast-forward to first visible line
    SWideChar const * s = _TextBegin;
    if ( y + lineHeight < clipRect.Y && !bWordWrap ) {
        while ( y + lineHeight < clipRect.Y && s < _TextEnd ) {
            s = ( SWideChar const * )memchr( s, '\n', _TextEnd - s );
            s = s ? s + 1 : _TextEnd;
            y += lineHeight;
        }
    }

    // For large text, scan for the last visible line in order to avoid over-reserving in the call to PrimReserve()
    // Note that very large horizontal line will still be affected by the issue (e.g. a one megabyte string buffer without a newline will likely crash atm)
    if ( _TextEnd - s > 10000 && !bWordWrap ) {
        SWideChar const * s_end = s;
        float y_end = y;
        while ( y_end < clipRect.W && s_end < _TextEnd ) {
            s_end = ( SWideChar const * )memchr( s_end, '\n', _TextEnd - s_end );
            s_end = s_end ? s_end + 1 : _TextEnd;
            y_end += lineHeight;
        }
        _TextEnd = s_end;
    }
    if ( s == _TextEnd ) {
        return;
    }

    // Reserve vertices for remaining worse case (over-reserving is useful and easily amortized)
    const int MaxVertices = ( int )( _TextEnd - s ) * 4;
    const int MaxIndices = ( int )( _TextEnd - s ) * 6;
    const int ReservedIndicesCount = DrawList.IdxBuffer.Size + MaxIndices;
    DrawList.PrimReserve( MaxIndices, MaxVertices );

    ImDrawVert * pVertices = DrawList._VtxWritePtr;
    ImDrawIdx * pIndices = DrawList._IdxWritePtr;
    unsigned int firstVertex = DrawList._VtxCurrentIdx;

    while ( s < _TextEnd ) {
        if ( bWordWrap ) {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if ( !wordWrapEOL ) {
                wordWrapEOL = font->CalcWordWrapPositionW( scale, s, _TextEnd, _WrapWidth - ( x - pos.X ) );
                if ( wordWrapEOL == s ) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
                    wordWrapEOL++;      // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
            }

            if ( s >= wordWrapEOL ) {
                x = pos.X;
                y += lineHeight;
                wordWrapEOL = NULL;

                // Wrapping skips upcoming blanks
                while ( s < _TextEnd ) {
                    const char c = *s;
                    if ( Core::CharIsBlank( c ) ) {
                        s++;
                    } else if ( c == '\n' ) {
                        s++;
                        break;
                    } else {
                        break;
                    }
                }
                continue;
            }
        }

        // Decode and advance source
        //unsigned int c = ( unsigned int )*s;
        //if ( c < 0x80 ) {
        //    s += 1;
        //} else {
        //    s += ImTextCharFromUtf8( &c, s, _TextEnd );
        //    if ( c == 0 ) // Malformed UTF-8?
        //        break;
        //}
        SWideChar c = *s;
        s++;

        if ( c < 32 ) {
            if ( c == '\n' ) {
                x = pos.X;
                y += lineHeight;
                if ( y > clipRect.W )
                    break; // break out of main loop
                continue;
            }
            if ( c == '\r' )
                continue;
        }

        SFontGlyph const * glyph = font->GetGlyph( c );
        float charWidth = glyph->AdvanceX * scale;

        // Arbitrarily assume that both space and tabs are empty glyphs as an optimization
        if ( c != ' ' && c != '\t' ) {
            // We don't do a second finer clipping test on the Y axis as we've already skipped anything before clipRect.Y and exit once we pass clipRect.W
            float x1 = x + glyph->X0 * scale;
            float x2 = x + glyph->X1 * scale;
            float y1 = y + glyph->Y0 * scale;
            float y2 = y + glyph->Y1 * scale;
            if ( x1 <= clipRect.Z && x2 >= clipRect.X ) {
                // Render a character
                float u1 = glyph->U0;
                float v1 = glyph->V0;
                float u2 = glyph->U1;
                float v2 = glyph->V1;

                // CPU side clipping used to fit text in their frame when the frame is too small. Only does clipping for axis aligned quads.
                if ( _CPUFineClipRect ) {
                    if ( x1 < clipRect.X ) {
                        u1 = u1 + (1.0f - (x2 - clipRect.X) / (x2 - x1)) * (u2 - u1);
                        x1 = clipRect.X;
                    }
                    if ( y1 < clipRect.Y ) {
                        v1 = v1 + (1.0f - (y2 - clipRect.Y) / (y2 - y1)) * (v2 - v1);
                        y1 = clipRect.Y;
                    }
                    if ( x2 > clipRect.Z ) {
                        u2 = u1 + ((clipRect.Z - x1) / (x2 - x1)) * (u2 - u1);
                        x2 = clipRect.Z;
                    }
                    if ( y2 > clipRect.W ) {
                        v2 = v1 + ((clipRect.W - y1) / (y2 - y1)) * (v2 - v1);
                        y2 = clipRect.W;
                    }
                    if ( y1 >= y2 ) {
                        x += charWidth;
                        continue;
                    }
                }

                pIndices[0] = firstVertex;
                pIndices[1] = firstVertex + 1;
                pIndices[2] = firstVertex + 2;
                pIndices[3] = firstVertex;
                pIndices[4] = firstVertex + 2;
                pIndices[5] = firstVertex + 3;
                pVertices[0].pos.x = x1;
                pVertices[0].pos.y = y1;
                pVertices[0].col = color;
                pVertices[0].uv.x = u1;
                pVertices[0].uv.y = v1;
                pVertices[1].pos.x = x2;
                pVertices[1].pos.y = y1;
                pVertices[1].col = color;
                pVertices[1].uv.x = u2;
                pVertices[1].uv.y = v1;
                pVertices[2].pos.x = x2;
                pVertices[2].pos.y = y2;
                pVertices[2].col = color;
                pVertices[2].uv.x = u2;
                pVertices[2].uv.y = v2;
                pVertices[3].pos.x = x1;
                pVertices[3].pos.y = y2;
                pVertices[3].col = color;
                pVertices[3].uv.x = u1;
                pVertices[3].uv.y = v2;
                pVertices += 4;
                firstVertex += 4;
                pIndices += 6;
            }
        }

        x += charWidth;
    }

    // Give back unused vertices
    DrawList.VtxBuffer.resize( ( int )( pVertices - DrawList.VtxBuffer.Data ) );
    DrawList.IdxBuffer.resize( ( int )( pIndices - DrawList.IdxBuffer.Data ) );
    DrawList.CmdBuffer[ DrawList.CmdBuffer.Size - 1 ].ElemCount -= ( ReservedIndicesCount - DrawList.IdxBuffer.Size );
    DrawList._VtxWritePtr = pVertices;
    DrawList._IdxWritePtr = pIndices;
    DrawList._VtxCurrentIdx = ( unsigned int )DrawList.VtxBuffer.Size;
}

void ACanvas::DrawChar( char _Ch, int _X, int _Y, float _Scale, Color4 const & _Color ) {
    DrawWChar( _Ch, _X, _Y, _Scale, _Color );
}

void ACanvas::DrawWChar( SWideChar _Ch, int _X, int _Y, float _Scale, Color4 const & _Color ) {
    if ( _Color.IsTransparent() ) {
        return;
    }

    AFont const * font = GetCurrentFont();

    if ( !font->IsValid() ) {
        return;
    }

    SFontGlyph const * glyph = font->GetGlyph( _Ch );

    Float2 const & fontOffset = font->GetDrawOffset();

    const Float2 a( _X + glyph->X0 * _Scale + fontOffset.X, _Y + glyph->Y0 * _Scale + fontOffset.Y );
    const Float2 b( _X + glyph->X1 * _Scale + fontOffset.X, _Y + glyph->Y1 * _Scale + fontOffset.Y );

    DrawList.PrimReserve( 6, 4 );
    DrawList.PrimRectUV( a, b, Float2( glyph->U0, glyph->V0 ), Float2( glyph->U1, glyph->V1 ), _Color.GetDWord() );
}

void ACanvas::DrawCharUTF8( const char * _Ch, int _X, int _Y, float _Scale, Color4 const & _Color ) {
    if ( _Color.IsTransparent() ) {
        return;
    }

    SWideChar ch;

    if ( !Core::WideCharDecodeUTF8( _Ch, ch ) ) {
        return;
    }

    DrawWChar( ch, _X, _Y, _Scale, _Color );
}

void ACanvas::DrawTexture( ATexture * _Texture, int _X, int _Y, int _W, int _H, Float2 const & _UV0, Float2 const & _UV1, Color4 const & _Color, EColorBlending _Blending, EHUDSamplerType _SamplerType ) {
    DrawList.AddImage( _Texture->GetGPUResource(), ImVec2(_X,_Y), ImVec2(_X+_W,_Y+_H), _UV0, _UV1, _Color.GetDWord(), HUD_DRAW_CMD_TEXTURE | ( _Blending << 8 ) | ( _SamplerType << 16 ) );
}

void ACanvas::DrawTextureQuad( ATexture * _Texture, int _X0, int _Y0, int _X1, int _Y1, int _X2, int _Y2, int _X3, int _Y3, Float2 const & _UV0, Float2 const & _UV1, Float2 const & _UV2, Float2 const & _UV3, Color4 const & _Color, EColorBlending _Blending, EHUDSamplerType _SamplerType ) {
    DrawList.AddImageQuad( _Texture->GetGPUResource(), ImVec2(_X0,_Y0), ImVec2(_X1,_Y1), ImVec2(_X2,_Y2), ImVec2(_X3,_Y3), _UV0, _UV1, _UV2, _UV3, _Color.GetDWord(), HUD_DRAW_CMD_TEXTURE | ( _Blending << 8 ) | ( _SamplerType << 16 ) );
}

void ACanvas::DrawTextureRounded( ATexture * _Texture, int _X, int _Y, int _W, int _H, Float2 const & _UV0, Float2 const & _UV1, Color4 const & _Color, float _Rounding, int _RoundingCorners, EColorBlending _Blending, EHUDSamplerType _SamplerType ) {
    DrawList.AddImageRounded( _Texture->GetGPUResource(), ImVec2(_X,_Y), ImVec2(_X+_W,_Y+_H), _UV0, _UV1, _Color.GetDWord(), _Rounding, _RoundingCorners, HUD_DRAW_CMD_TEXTURE | ( _Blending << 8 ) | ( _SamplerType << 16 ) );
}

void ACanvas::DrawMaterial( AMaterialInstance * _MaterialInstance, int _X, int _Y, int _W, int _H, Float2 const & _UV0, Float2 const & _UV1, Color4 const & _Color ) {
    DrawList.AddImage( _MaterialInstance, ImVec2(_X,_Y), ImVec2(_X+_W,_Y+_H), _UV0, _UV1, _Color.GetDWord(), HUD_DRAW_CMD_MATERIAL );
}

void ACanvas::DrawMaterialQuad( AMaterialInstance * _MaterialInstance, int _X0, int _Y0, int _X1, int _Y1, int _X2, int _Y2, int _X3, int _Y3, Float2 const & _UV0, Float2 const & _UV1, Float2 const & _UV2, Float2 const & _UV3, Color4 const & _Color ) {
    DrawList.AddImageQuad( _MaterialInstance, ImVec2(_X0,_Y0), ImVec2(_X1,_Y1), ImVec2(_X2,_Y2), ImVec2(_X3,_Y3), _UV0, _UV1, _UV2, _UV3, _Color.GetDWord(), HUD_DRAW_CMD_MATERIAL );
}

void ACanvas::DrawMaterialRounded( AMaterialInstance * _MaterialInstance, int _X, int _Y, int _W, int _H, Float2 const & _UV0, Float2 const & _UV1, Color4 const & _Color, float _Rounding, int _RoundingCorners ) {
    DrawList.AddImageRounded( _MaterialInstance, ImVec2(_X,_Y), ImVec2(_X+_W,_Y+_H), _UV0, _UV1, _Color.GetDWord(), _Rounding, _RoundingCorners, HUD_DRAW_CMD_MATERIAL );
}

void ACanvas::DrawViewport( ACameraComponent * _Camera, ARenderingParameters * _RP, int _X, int _Y, int _W, int _H, Color4 const & _Color, float _Rounding, int _RoundingCorners, EColorBlending _Blending ) {
    if ( !_Camera ) {
        return;
    }

    if ( !_RP ) {
        return;
    }

    if ( _Color.IsTransparent() ) {
        return;
    }

    Float2 const & clipMin = GetClipMins();
    Float2 const & clipMax = GetClipMaxs();

    if ( _X > clipMax.X
         || _Y > clipMax.Y
         || _X + _W < clipMin.X
         || _Y + _H < clipMin.Y ) {
        // Perform viewport clipping
        return;
    }

    Float2 a(_X,_Y);
    Float2 b(_X+_W,_Y+_H);

    //DrawList.AddImageRounded( (void*)(size_t)(Viewports.Size()+1), a, b, a, a, _Color.GetDWord(), _Rounding, _RoundingCorners, HUD_DRAW_CMD_VIEWPORT | ( _Blending << 8 ) );

    DrawList.AddImageRounded( (void*)(size_t)(Viewports.Size()+1), a, b, Float2(0.0f), Float2(1.0f), _Color.GetDWord(), _Rounding, _RoundingCorners, HUD_DRAW_CMD_VIEWPORT | (_Blending << 8) );

    SViewport & viewport = Viewports.Append();
    viewport.X = _X;
    viewport.Y = _Y;
    viewport.Width = _W;
    viewport.Height = _H;
    viewport.Camera = _Camera;
    viewport.RenderingParams = _RP;
}

void ACanvas::DrawCursor( EDrawCursor _Cursor, Float2 const & _Position, Color4 const & _Color, Color4 const & _BorderColor, Color4 const & _ShadowColor, const float _Scale ) {
    AFont const * font = DrawList._Data->Font;
    Float2 offset, size, uv[ 4 ];

    if ( font->GetMouseCursorTexData( _Cursor, &offset, &size, &uv[ 0 ], &uv[ 2 ] ) ) {
        Float2 pos = _Position.Floor() - offset;
        RenderCore::ITexture * textureId = font->GetTexture()->GetGPUResource();
        const uint32_t shadow = _ShadowColor.GetDWord();
        DrawList.PushClipRectFullScreen();
        DrawList.AddImage( textureId, pos + Float2( 1, 0 )*_Scale, pos + Float2( 1, 0 )*_Scale + size*_Scale, uv[ 2 ], uv[ 3 ], shadow );
        DrawList.AddImage( textureId, pos + Float2( 2, 0 )*_Scale, pos + Float2( 2, 0 )*_Scale + size*_Scale, uv[ 2 ], uv[ 3 ], shadow );
        DrawList.AddImage( textureId, pos, pos + size*_Scale, uv[ 2 ], uv[ 3 ], _BorderColor.GetDWord() );
        DrawList.AddImage( textureId, pos, pos + size*_Scale, uv[ 0 ], uv[ 1 ], _Color.GetDWord() );
        DrawList.PopClipRect();
    }
}

void ACanvas::DrawPolyline( Float2 const * points, int num_points, Color4 const & col, bool closed, float thickness ) {
    DrawList.AddPolyline( reinterpret_cast< ImVec2 const * >( points ), num_points, col.GetDWord(), closed, thickness );
}

void ACanvas::DrawConvexPolyFilled( Float2 const * points, int num_points, Color4 const & col ) {
    DrawList.AddConvexPolyFilled( reinterpret_cast< ImVec2 const * >( points ), num_points, col.GetDWord() );
}

void ACanvas::DrawBezierCurve( Float2 const & pos0, Float2 const & cp0, Float2 const & cp1, Float2 const & pos1, Color4 const & col, float thickness, int num_segments ) {
    DrawList.AddBezierCurve( pos0, cp0, cp1, pos1, col.GetDWord(), thickness, num_segments );
}

//// Stateful path API, add points then finish with PathFillConvex() or PathStroke()
//void PathClear() { _Path.Size = 0; }
//void PathLineTo(Float2 const & pos) { _Path.push_back(pos); }
//void PathLineToMergeDuplicate(Float2 const & pos) { if (_Path.Size == 0 || memcmp(&_Path.Data[_Path.Size-1], &pos, 8) != 0) _Path.push_back(pos); }
//void PathFillConvex(Color4 const & col) { AddConvexPolyFilled(_Path.Data, _Path.Size, col); _Path.Size = 0; }  // Note: Anti-aliased filling requires points to be in clockwise order.
//void PathStroke(Color4 const & col, bool closed, float thickness ) { AddPolyline(_Path.Data, _Path.Size, col, closed, thickness); _Path.Size = 0; }
//void PathArcTo(Float2 const & centre, float radius, float a_min, float a_max, int num_segments = 10);
//void PathArcToFast(Float2 const & centre, float radius, int a_min_of_12, int a_max_of_12);                                            // Use precomputed angles for a 12 steps circle
//void PathBezierCurveTo(Float2 const & p1, Float2 const & p2, Float2 const & p3, int num_segments = 0);
//void PathRect(Float2 const & rect_min, Float2 const & rect_max, float rounding, int rounding_corners_flags = ImDrawCornerFlags_All);
