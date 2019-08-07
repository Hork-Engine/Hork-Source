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

#include <Engine/World/Public/Canvas.h>
#include <Engine/World/Public/Actors/PlayerController.h>
#include <Engine/World/Public/Actors/HUD.h>

#include <Engine/Resource/Public/Texture.h>

#include <Engine/Core/Public/Utf8.h>

void FCanvas::Initialize() {
    DrawList._Data = &DrawListSharedData;
}

void FCanvas::Deinitialize() {
    DrawList.ClearFreeMemory();
    Viewports.Free();
}

void FCanvas::Begin( ImFont * _DefaultFont, int _Width, int _Height ) {
    AN_Assert( FontStack.IsEmpty() );

    Width = _Width;
    Height = _Height;
    DrawList.Clear();
    Viewports.Clear();

    DrawListSharedData.ClipRectFullscreen.x = 0;
    DrawListSharedData.ClipRectFullscreen.y = 0;
    DrawListSharedData.ClipRectFullscreen.z = _Width;
    DrawListSharedData.ClipRectFullscreen.w = _Height;

    PushFont( _DefaultFont );
    PushClipRectFullScreen();
}

void FCanvas::End() {
    PopClipRect();
    PopFont();
    if ( DrawList.CmdBuffer.Size > 0 ) {
        if ( DrawList.CmdBuffer.back().ElemCount == 0 ) {
            DrawList.CmdBuffer.pop_back();
        }
    }
}

void FCanvas::PushClipRect( Float2 const & _Mins, Float2 const & _Maxs, bool _IntersectWithCurrentClipRect ) {
    DrawList.PushClipRect( _Mins, _Maxs, _IntersectWithCurrentClipRect );
}

void FCanvas::PushClipRectFullScreen() {
    DrawList.PushClipRect( Float2(0, 0), Float2(Width, Height) );
}

void FCanvas::PopClipRect() {
    DrawList.PopClipRect();
}

void FCanvas::PushBlendingState( EColorBlending _Blending ) {
    DrawList.PushBlendingState( CANVAS_DRAW_CMD_ALPHA | ( _Blending << 8 ) );
}

void FCanvas::PopBlendingState() {
    DrawList.PopBlendingState();
}

void FCanvas::SetCurrentFont( ImFont * _Font ) {
    if ( _Font && _Font->IsLoaded() && _Font->Scale > 0.0f ) {
        ImFontAtlas * atlas = _Font->ContainerAtlas;
        DrawListSharedData.TexUvWhitePixel = atlas->TexUvWhitePixel;
        DrawListSharedData.FontSize = _Font->FontSize * _Font->Scale;
    } else {
        DrawListSharedData.TexUvWhitePixel = Float2(0.0f);
        DrawListSharedData.FontSize = 16;
    }
    DrawListSharedData.Font = _Font;
}

void FCanvas::PushFont( ImFont * _Font ) {
    SetCurrentFont( _Font );
    FontStack.Append( _Font );
    DrawList.PushTextureID( _Font->ContainerAtlas->TexID );
}

void FCanvas::PopFont() {
    if ( FontStack.IsEmpty() ) {
        GLogger.Printf( "FCanvas::PopFont: stack was corrupted\n" );
        return;
    }
    DrawList.PopTextureID();
    FontStack.RemoveLast();
    SetCurrentFont( FontStack.IsEmpty() ? nullptr : FontStack.Last() );
}

void FCanvas::DrawLine( Float2 const & a, Float2 const & b, uint32_t col, float thickness ) {
    DrawList.AddLine( a, b, col, thickness );
}

void FCanvas::DrawRect( Float2 const & a, Float2 const & b, uint32_t col, float rounding, int rounding_corners_flags, float thickness ) {
    DrawList.AddRect( a, b, col, rounding, rounding_corners_flags, thickness );
}

void FCanvas::DrawRectFilled( Float2 const & a, Float2 const & b, uint32_t col, float rounding, int rounding_corners_flags ) {
    DrawList.AddRectFilled( a, b, col, rounding, rounding_corners_flags );
}

void FCanvas::DrawRectFilledMultiColor( Float2 const & a, Float2 const & b, uint32_t col_upr_left, uint32_t col_upr_right, uint32_t col_bot_right, uint32_t col_bot_left ) {
    DrawList.AddRectFilledMultiColor( a, b, col_upr_left, col_upr_right, col_bot_right, col_bot_left );
}

void FCanvas::DrawQuad( Float2 const & a, Float2 const & b, Float2 const & c, Float2 const & d, uint32_t col, float thickness ) {
    DrawList.AddQuad( a, b, c, d, col, thickness );
}

void FCanvas::DrawQuadFilled( Float2 const & a, Float2 const & b, Float2 const & c, Float2 const & d, uint32_t col ) {
    DrawList.AddQuadFilled( a, b, c, d, col );
}

void FCanvas::DrawTriangle( Float2 const & a, Float2 const & b, Float2 const & c, uint32_t col, float thickness ) {
    DrawList.AddTriangle( a, b, c, col, thickness );
}

void FCanvas::DrawTriangleFilled( Float2 const & a, Float2 const & b, Float2 const & c, uint32_t col ) {
    DrawList.AddTriangleFilled( a, b, c, col );
}

void FCanvas::DrawCircle( Float2 const & centre, float radius, uint32_t col, int num_segments, float thickness ) {
    DrawList.AddCircle( centre, radius, col, num_segments, thickness );
}

void FCanvas::DrawCircleFilled( Float2 const & centre, float radius, uint32_t col, int num_segments ) {
    DrawList.AddCircleFilled( centre, radius, col, num_segments );
}

void FCanvas::DrawTextUTF8( Float2 const & pos, uint32_t col, const char* _TextBegin, const char* _TextEnd ) {
    DrawTextUTF8( GetCurrentFont(), DrawListSharedData.FontSize, pos, col, _TextBegin, _TextEnd );
}

void FCanvas::DrawTextUTF8( ImFont const * _Font, float _FontSize, Float2 const & _Pos, uint32_t _Color, const char* _TextBegin, const char* _TextEnd, float _WrapWidth, Float4 const * _CPUFineClipRect ) {

    AN_Assert( _Font && _FontSize > 0.0f );

    if ( (_Color & IM_COL32_A_MASK) == 0 ) {
        return;
    }

    if ( !_TextEnd ) {
        _TextEnd = _TextBegin + strlen(_TextBegin);
    }

    if ( _TextBegin == _TextEnd ) {
        return;
    }

    AN_Assert( _Font->ContainerAtlas->TexID == DrawList._TextureIdStack.back() );

    Float4 clipRect = DrawList._ClipRectStack.back();
    if ( _CPUFineClipRect ) {
        clipRect.X = FMath::Max( clipRect.X, _CPUFineClipRect->X );
        clipRect.Y = FMath::Max( clipRect.Y, _CPUFineClipRect->Y );
        clipRect.Z = FMath::Min( clipRect.Z, _CPUFineClipRect->Z );
        clipRect.W = FMath::Min( clipRect.W, _CPUFineClipRect->W );
    }

    //_Font->RenderText( &DrawList, _FontSize, _Pos, _Color, clipRect, _TextBegin, _TextEnd, _WrapWidth, _CPUFineClipRect != NULL );

    // Align to be pixel perfect
    Float2 pos;
    pos.X = (float)(int)_Pos.X + _Font->DisplayOffset.x;
    pos.Y = (float)(int)_Pos.Y + _Font->DisplayOffset.y;
    float x = pos.X;
    float y = pos.Y;
    if (y > clipRect.W)
        return;

    const float scale = _FontSize / _Font->FontSize;
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
                wordWrapEOL = _Font->CalcWordWrapPositionA(scale, s, _TextEnd, _WrapWidth - (x - pos.X));
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
                    if ( ImCharIsBlankA(c) ) {
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
        unsigned int c = (unsigned int)*s;
        if ( c < 0x80 ) {
            s += 1;
        } else {
            s += ImTextCharFromUtf8( &c, s, _TextEnd );
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
            if (c == '\r')
                continue;
        }

        float charWidth = 0.0f;
        if ( const ImFontGlyph * glyph = _Font->FindGlyph( (FWideChar)c ) ) {
            charWidth = glyph->AdvanceX * scale;

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
                    pVertices[0].col = _Color;
                    pVertices[0].uv.x = u1;
                    pVertices[0].uv.y = v1;
                    pVertices[1].pos.x = x2;
                    pVertices[1].pos.y = y1;
                    pVertices[1].col = _Color;
                    pVertices[1].uv.x = u2;
                    pVertices[1].uv.y = v1;
                    pVertices[2].pos.x = x2;
                    pVertices[2].pos.y = y2;
                    pVertices[2].col = _Color;
                    pVertices[2].uv.x = u2;
                    pVertices[2].uv.y = v2;
                    pVertices[3].pos.x = x1;
                    pVertices[3].pos.y = y2;
                    pVertices[3].col = _Color;
                    pVertices[3].uv.x = u1;
                    pVertices[3].uv.y = v2;
                    pVertices += 4;
                    firstVertex += 4;
                    pIndices += 6;
                }
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

void FCanvas::DrawChar( ImFont const * _Font, char _Ch, int _X, int _Y, float _Scale, uint32_t _Color ) {
    DrawWChar( _Font, _Ch, _X, _Y, _Scale, _Color );
}

void FCanvas::DrawWChar( ImFont const * _Font, FWideChar _Ch, int _X, int _Y, float _Scale, uint32_t _Color ) {
    if ( (_Color & IM_COL32_A_MASK) == 0 ) {
        return;
    }

    const ImFontGlyph * glyph = _Font->FindGlyph( _Ch );
    if ( glyph ) {
        const Float2 a( _X + glyph->X0 * _Scale + _Font->DisplayOffset.x, _Y + glyph->Y0 * _Scale + _Font->DisplayOffset.y );
        const Float2 b( _X + glyph->X1 * _Scale + _Font->DisplayOffset.x, _Y + glyph->Y1 * _Scale + _Font->DisplayOffset.y );

        DrawList.PrimReserve( 6, 4 );
        DrawList.PrimRectUV( a, b, Float2( glyph->U0, glyph->V0 ), Float2( glyph->U1, glyph->V1 ), _Color );
    }
}

void FCanvas::DrawCharUTF8( ImFont const * _Font, const char * _Ch, int _X, int _Y, float _Scale, uint32_t _Color ) {
    if ( (_Color & IM_COL32_A_MASK) == 0 ) {
        return;
    }

    FWideChar ch;

    if ( !FCore::DecodeUTF8WChar( _Ch, ch ) ) {
        return;
    }

    DrawWChar( _Font, ch, _X, _Y, _Scale, _Color );
}

void FCanvas::DrawTexture( FTexture * _Texture, int _X, int _Y, int _W, int _H, Float2 const & _UV0, Float2 const & _UV1, uint32_t _Color, EColorBlending _Blending, ESamplerType _SamplerType ) {
    DrawList.AddImage( _Texture->GetRenderProxy(), ImVec2(_X,_Y), ImVec2(_X+_W,_Y+_H), _UV0, _UV1, _Color, CANVAS_DRAW_CMD_TEXTURE | ( _Blending << 8 ) | ( _SamplerType << 16 ) );
}

void FCanvas::DrawTextureQuad( FTexture * _Texture, int _X0, int _Y0, int _X1, int _Y1, int _X2, int _Y2, int _X3, int _Y3, Float2 const & _UV0, Float2 const & _UV1, Float2 const & _UV2, Float2 const & _UV3, uint32_t _Color, EColorBlending _Blending, ESamplerType _SamplerType ) {
    DrawList.AddImageQuad( _Texture, ImVec2(_X0,_Y0), ImVec2(_X1,_Y1), ImVec2(_X2,_Y2), ImVec2(_X3,_Y3), _UV0, _UV1, _UV2, _UV3, _Color, CANVAS_DRAW_CMD_TEXTURE | ( _Blending << 8 ) | ( _SamplerType << 16 ) );
}

void FCanvas::DrawTextureRounded( FTexture * _Texture, int _X, int _Y, int _W, int _H, Float2 const & _UV0, Float2 const & _UV1, uint32_t _Color, float _Rounding, int _RoundingCorners, EColorBlending _Blending, ESamplerType _SamplerType ) {
    DrawList.AddImageRounded( _Texture, ImVec2(_X,_Y), ImVec2(_X+_W,_Y+_H), _UV0, _UV1, _Color, _Rounding, _RoundingCorners, CANVAS_DRAW_CMD_TEXTURE | ( _Blending << 8 ) | ( _SamplerType << 16 ) );
}

void FCanvas::DrawMaterial( FMaterialInstance * _MaterialInstance, int _X, int _Y, int _W, int _H, Float2 const & _UV0, Float2 const & _UV1, uint32_t _Color ) {
    DrawList.AddImage( _MaterialInstance, ImVec2(_X,_Y), ImVec2(_X+_W,_Y+_H), _UV0, _UV1, _Color, CANVAS_DRAW_CMD_MATERIAL );
}

void FCanvas::DrawMaterialQuad( FMaterialInstance * _MaterialInstance, int _X0, int _Y0, int _X1, int _Y1, int _X2, int _Y2, int _X3, int _Y3, Float2 const & _UV0, Float2 const & _UV1, Float2 const & _UV2, Float2 const & _UV3, uint32_t _Color ) {
    DrawList.AddImageQuad( _MaterialInstance, ImVec2(_X0,_Y0), ImVec2(_X1,_Y1), ImVec2(_X2,_Y2), ImVec2(_X3,_Y3), _UV0, _UV1, _UV2, _UV3, _Color, CANVAS_DRAW_CMD_MATERIAL );
}

void FCanvas::DrawMaterialRounded( FMaterialInstance * _MaterialInstance, int _X, int _Y, int _W, int _H, Float2 const & _UV0, Float2 const & _UV1, uint32_t _Color, float _Rounding, int _RoundingCorners ) {
    DrawList.AddImageRounded( _MaterialInstance, ImVec2(_X,_Y), ImVec2(_X+_W,_Y+_H), _UV0, _UV1, _Color, _Rounding, _RoundingCorners, CANVAS_DRAW_CMD_MATERIAL );
}

void FCanvas::DrawViewport( FPlayerController * _PlayerController, int _X, int _Y, int _W, int _H, uint32_t _Color, EColorBlending _Blending ) {
    if ( (_Color & IM_COL32_A_MASK) == 0 ) {
        return;
    }

    Float2 const & clipMin = GetClipMins();
    Float2 const & clipMax = GetClipMaxs();

    if ( _X > clipMax.X
         || _Y > clipMax.Y
         || _X + _W < clipMin.X
         || _Y + _H < clipMin.Y ) {
        // Perfrom viewport clipping
        return;
    }

    Float2 a(_X,_Y);
    Float2 b(_X+_W,_Y+_H);

    DrawList.AddImage( (void*)(size_t)(Viewports.Size()+1), a, b, a, a, _Color, CANVAS_DRAW_CMD_VIEWPORT | ( _Blending << 8 ) );

    FViewport & viewport = Viewports.Append();
    viewport.X = _X;
    viewport.Y = _Y;
    viewport.Width = _W;
    viewport.Height = _H;
    viewport.PlayerController = _PlayerController;

    FHUD * hud = _PlayerController->GetHUD();
    if ( hud ) {

        PushClipRect( a, b, true );

        hud->Draw( this, _X, _Y, _W, _H );

        PopClipRect();
    }
}

void FCanvas::DrawPolyline( Float2 const * points, int num_points, uint32_t col, bool closed, float thickness ) {
    DrawList.AddPolyline( reinterpret_cast< ImVec2 const * >( points ), num_points, col, closed, thickness );
}

void FCanvas::DrawConvexPolyFilled( Float2 const * points, int num_points, uint32_t col ) {
    DrawList.AddConvexPolyFilled( reinterpret_cast< ImVec2 const * >( points ), num_points, col );
}

void FCanvas::DrawBezierCurve( Float2 const & pos0, Float2 const & cp0, Float2 const & cp1, Float2 const & pos1, uint32_t col, float thickness, int num_segments ) {
    DrawList.AddBezierCurve( pos0, cp0, cp1, pos1, col, thickness, num_segments );
}

//// Stateful path API, add points then finish with PathFillConvex() or PathStroke()
//void PathClear() { _Path.Size = 0; }
//void PathLineTo(Float2 const & pos) { _Path.push_back(pos); }
//void PathLineToMergeDuplicate(Float2 const & pos) { if (_Path.Size == 0 || memcmp(&_Path.Data[_Path.Size-1], &pos, 8) != 0) _Path.push_back(pos); }
//void PathFillConvex(uint32_t col) { AddConvexPolyFilled(_Path.Data, _Path.Size, col); _Path.Size = 0; }  // Note: Anti-aliased filling requires points to be in clockwise order.
//void PathStroke(uint32_t col, bool closed, float thickness ) { AddPolyline(_Path.Data, _Path.Size, col, closed, thickness); _Path.Size = 0; }
//void PathArcTo(Float2 const & centre, float radius, float a_min, float a_max, int num_segments = 10);
//void PathArcToFast(Float2 const & centre, float radius, int a_min_of_12, int a_max_of_12);                                            // Use precomputed angles for a 12 steps circle
//void PathBezierCurveTo(Float2 const & p1, Float2 const & p2, Float2 const & p3, int num_segments = 0);
//void PathRect(Float2 const & rect_min, Float2 const & rect_max, float rounding, int rounding_corners_flags = ImDrawCornerFlags_All);