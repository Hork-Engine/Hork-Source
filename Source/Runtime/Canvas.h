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

#pragma once

#include <Containers/Vector.h>
#include <Renderer/RenderDefs.h>
#include "FontAtlas.h"
#include "imdrawlist.h"

class ACameraComponent;
class ARenderingParameters;
class ATexture;
class AMaterialInstance;

struct SViewport
{
    ACameraComponent*     Camera;
    ARenderingParameters* RenderingParams;
    int                   X;
    int                   Y;
    int                   Width;
    int                   Height;
    int                   ScaledWidth;
    int                   ScaledHeight;
};

enum EDrawCornerFlags
{
    CORNER_ROUND_TOP_LEFT     = 1 << 0,
    CORNER_ROUND_TOP_RIGHT    = 1 << 1,
    CORNER_ROUND_BOTTOM_LEFT  = 1 << 2,
    CORNER_ROUND_BOTTOM_RIGHT = 1 << 3,
    CORNER_ROUND_TOP          = CORNER_ROUND_TOP_LEFT | CORNER_ROUND_TOP_RIGHT,
    CORNER_ROUND_BOTTOM       = CORNER_ROUND_BOTTOM_LEFT | CORNER_ROUND_BOTTOM_RIGHT,
    CORNER_ROUND_LEFT         = CORNER_ROUND_TOP_LEFT | CORNER_ROUND_BOTTOM_LEFT,
    CORNER_ROUND_RIGHT        = CORNER_ROUND_TOP_RIGHT | CORNER_ROUND_BOTTOM_RIGHT,
    CORNER_ROUND_ALL          = ~0,
    CORNER_ROUND_NONE         = 0
};

class ACanvas
{
    HK_FORBID_COPY(ACanvas)

public:
    ACanvas();
    virtual ~ACanvas();

    int GetWidth() const { return Width; }
    int GetHeight() const { return Height; }

    void Begin(int _Width, int _Height);
    void End();

    // Clipping
    void PushClipRect(Float2 const& _Mins, Float2 const& _Maxs, bool _IntersectWithCurrentClipRect = false);
    void PushClipRectFullScreen();
    void PopClipRect();

    Float2 const& GetClipMins() const { return *(Float2*)&DrawList._ClipRectStack.back().x; }
    Float2 const& GetClipMaxs() const { return *(Float2*)&DrawList._ClipRectStack.back().z; }

    // Primitive blending
    void PushBlendingState(EColorBlending _Blending);
    void PopBlendingState();

    // Font
    void PushFont(AFont const* _Font);
    void PopFont();

    static AFont* GetDefaultFont();
    AFont const*  GetCurrentFont() { return FontStack.Last(); }

    // Primitives
    void DrawLine(Float2 const& a, Float2 const& b, Color4 const& col, float thickness = 1.0f);
    void DrawRect(Float2 const& a, Float2 const& b, Color4 const& col, float rounding = 0.0f, int _RoundingCorners = CORNER_ROUND_ALL, float thickness = 1.0f);
    void DrawRectFilled(Float2 const& a, Float2 const& b, Color4 const& col, float rounding = 0.0f, int _RoundingCorners = CORNER_ROUND_ALL);
    void DrawRectFilledMultiColor(Float2 const& a, Float2 const& b, Color4 const& col_upr_left, Color4 const& col_upr_right, Color4 const& col_bot_right, Color4 const& col_bot_left);
    void DrawQuad(Float2 const& a, Float2 const& b, Float2 const& c, Float2 const& d, Color4 const& col, float thickness = 1.0f);
    void DrawQuadFilled(Float2 const& a, Float2 const& b, Float2 const& c, Float2 const& d, Color4 const& col);
    void DrawTriangle(Float2 const& a, Float2 const& b, Float2 const& c, Color4 const& col, float thickness = 1.0f);
    void DrawTriangleFilled(Float2 const& a, Float2 const& b, Float2 const& c, Color4 const& col);
    void DrawCircle(Float2 const& centre, float radius, Color4 const& col, int num_segments = 12, float thickness = 1.0f);
    void DrawCircleFilled(Float2 const& centre, float radius, Color4 const& col, int num_segments = 12);
    void DrawPolyline(Float2 const* points, int num_points, Color4 const& col, bool closed, float thickness);
    void DrawConvexPolyFilled(Float2 const* points, int num_points, Color4 const& col); // Note: Anti-aliased filling requires points to be in clockwise order.
    void DrawBezierCurve(Float2 const& pos0, Float2 const& cp0, Float2 const& cp1, Float2 const& pos1, Color4 const& col, float thickness, int num_segments = 0);

    // Text
    void DrawTextUTF8(Float2 const& _Pos, Color4 const& _Color, AStringView Text, bool bShadow = false);
    void DrawTextUTF8(float _FontSize, Float2 const& _Pos, Color4 const& _Color, AStringView Text, float _WrapWidth = 0.0f, Float4 const* _CPUFineClipRect = nullptr, bool bShadow = false);
    void DrawTextWChar(Float2 const& _Pos, Color4 const& _Color, WideChar const* _TextBegin, WideChar const* _TextEnd = nullptr, bool bShadow = false);
    void DrawTextWChar(float _FontSize, Float2 const& _Pos, Color4 const& _Color, WideChar const* _TextBegin, WideChar const* _TextEnd = nullptr, float _WrapWidth = 0.0f, Float4 const* _CPUFineClipRect = nullptr, bool bShadow = false);
    void DrawChar(char _Ch, int _X, int _Y, float _Scale, Color4 const& _Color);
    void DrawWChar(WideChar _Ch, int _X, int _Y, float _Scale, Color4 const& _Color);
    void DrawCharUTF8(const char* _Ch, int _X, int _Y, float _Scale, Color4 const& _Color);

    // Texture
    void DrawTexture(ATexture* _Texture, int _X, int _Y, int _W, int _H, Float2 const& _UV0 = Float2(0, 0), Float2 const& _UV1 = Float2(1, 1), Color4 const& _Color = Color4(1.0F), EColorBlending _Blending = COLOR_BLENDING_ALPHA, EHUDSamplerType _SamplerType = HUD_SAMPLER_TILED_LINEAR);
    void DrawTextureQuad(ATexture* _Texture, int _X0, int _Y0, int _X1, int _Y1, int _X2, int _Y2, int _X3, int _Y3, Float2 const& _UV0 = Float2(0, 0), Float2 const& _UV1 = Float2(1, 0), Float2 const& _UV2 = Float2(1, 1), Float2 const& _UV3 = Float2(0, 1), Color4 const& _Color = Color4(1.0F), EColorBlending _Blending = COLOR_BLENDING_ALPHA, EHUDSamplerType _SamplerType = HUD_SAMPLER_TILED_LINEAR);
    void DrawTextureRounded(ATexture* _Texture, int _X, int _Y, int _W, int _H, Float2 const& _UV0, Float2 const& _UV1, Color4 const& _Color, float _Rounding, int _RoundingCorners = CORNER_ROUND_ALL, EColorBlending _Blending = COLOR_BLENDING_ALPHA, EHUDSamplerType _SamplerType = HUD_SAMPLER_TILED_LINEAR);

    // Material
    void DrawMaterial(AMaterialInstance* _MaterialInstance, int _X, int _Y, int _W, int _H, Float2 const& _UV0 = Float2(0, 0), Float2 const& _UV1 = Float2(1, 1), Color4 const& _Color = Color4::White());
    void DrawMaterialQuad(AMaterialInstance* _MaterialInstance, int _X0, int _Y0, int _X1, int _Y1, int _X2, int _Y2, int _X3, int _Y3, Float2 const& _UV0 = Float2(0, 0), Float2 const& _UV1 = Float2(1, 0), Float2 const& _UV2 = Float2(1, 1), Float2 const& _UV3 = Float2(0, 1), Color4 const& _Color = Color4::White());
    void DrawMaterialRounded(AMaterialInstance* _MaterialInstance, int _X, int _Y, int _W, int _H, Float2 const& _UV0, Float2 const& _UV1, Color4 const& _Color, float _Rounding, int _RoundingCorners = CORNER_ROUND_ALL);

    // Viewport
    void DrawViewport(ACameraComponent* _Camera, ARenderingParameters* _RP, int _X, int _Y, int _W, int _H, Color4 const& _Color = Color4::White(), float _Rounding = 0.0f, int _RoundingCorners = CORNER_ROUND_ALL, EColorBlending _Blending = COLOR_BLENDING_DISABLED);

    // Cursor
    void DrawCursor(EDrawCursor _Cursor, Float2 const& _Position, Color4 const& _Color, Color4 const& _BorderColor, Color4 const& _ShadowColor, const float _Scale = 1.0f);

    TPodVector<SViewport> const& GetViewports() { return Viewports; }

    ImDrawList const& GetDrawList() const { return DrawList; }

    //    // Stateful path API, add points then finish with PathFillConvex() or PathStroke()
    //    void PathClear() { _Path.Size = 0; }
    //    void PathLineTo(Float2 const & pos) { _Path.push_back(pos); }
    //    void PathLineToMergeDuplicate(Float2 const & pos) { if (_Path.Size == 0 || memcmp(&_Path.Data[_Path.Size-1], &pos, 8) != 0) _Path.push_back(pos); }
    //    void PathFillConvex(Color4 const & col) { AddConvexPolyFilled(_Path.Data, _Path.Size, col); _Path.Size = 0; }  // Note: Anti-aliased filling requires points to be in clockwise order.
    //    void PathStroke(Color4 const & col, bool closed, float thickness = 1.0f) { AddPolyline(_Path.Data, _Path.Size, col, closed, thickness); _Path.Size = 0; }
    //    void PathArcTo(Float2 const & centre, float radius, float a_min, float a_max, int num_segments = 10);
    //    void PathArcToFast(Float2 const & centre, float radius, int a_min_of_12, int a_max_of_12);                                            // Use precomputed angles for a 12 steps circle
    //    void PathBezierCurveTo(Float2 const & p1, Float2 const & p2, Float2 const & p3, int num_segments = 0);
    //    void PathRect(Float2 const & rect_min, Float2 const & rect_max, float rounding = 0.0f, int rounding_corners_flags = ImDrawCornerFlags_All);

private:
    void SetCurrentFont(AFont const* font);
    void _DrawTextUTF8(float _FontSize, Float2 const& _Pos, Color4 const& _Color, AStringView Text, float _WrapWidth, Float4 const* _CPUFineClipRect);
    void _DrawTextWChar(float _FontSize, Float2 const& _Pos, Color4 const& _Color, WideChar const* _TextBegin, WideChar const* _TextEnd, float _WrapWidth, Float4 const* _CPUFineClipRect);

    int Width  = 0;
    int Height = 0;

    TPodVector<SViewport> Viewports;

    ImDrawListSharedData     DrawListSharedData;
    ImDrawList               DrawList;
    TPodVector<AFont const*> FontStack;
};
