/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include <Hork/Core/Containers/Vector.h>
#include <Hork/Renderer/RenderDefs.h>

#include <Hork/Runtime/ResourceManager/ResourceManager.h>

#include "Paint.h"
#include "Transform2D.h"

struct FONScontext;

HK_NAMESPACE_BEGIN

enum class CanvasPushFlag
{
    Keep,
    Reset
};

struct RoundingDesc
{
    RoundingDesc() = default;

    explicit RoundingDesc(float rounding) :
        RoundingTL(rounding),
        RoundingTR(rounding),
        RoundingBL(rounding),
        RoundingBR(rounding)
    {}

    RoundingDesc(float roundingTL, float roundingTR, float roundingBL, float roundingBR) :
        RoundingTL(roundingTL),
        RoundingTR(roundingTR),
        RoundingBL(roundingBL),
        RoundingBR(roundingBR)
    {}

    float RoundingTL = 0;
    float RoundingTR = 0;
    float RoundingBL = 0;
    float RoundingBR = 0;
};

struct DrawTextureDesc
{
    TextureHandle    TexHandle{};
    float            X{};
    float            Y{};
    float            W{};
    float            H{};
    RoundingDesc     Rounding;
    float            Angle{};
    Color4           TintColor = Color4(1.0f);
    Float2           UVOffset{0, 0};
    Float2           UVScale{1, 1};
    CANVAS_COMPOSITE Composite = CANVAS_COMPOSITE_SOURCE_OVER;
    bool             bTiledX : 1;
    bool             bTiledY : 1;
    bool             bFlipY : 1;
    bool             bAlphaPremultiplied : 1;
    bool             bNearestFilter : 1;

    DrawTextureDesc() :
        bTiledX(false),
        bTiledY(false),
        bFlipY(false),
        bAlphaPremultiplied(false),
        bNearestFilter(false)
    {}
};

enum CanvasLineCap : uint8_t
{
    Butt   = 0,
    Round  = 1,
    Square = 2
};

enum class CanvasLineJoin : uint8_t
{
    Miter = 0,
    Round = 1,
    Bevel = 2    
};

enum CanvasPathWinding : uint8_t
{
    CCW = 1, // Winding for solid shapes
    CW  = 2, // Winding for holes

    Solid = CCW,
    Hole  = CW,
};

enum TEXT_ALIGNMENT_FLAGS
{
    TEXT_ALIGNMENT_LEFT        = HK_BIT(0),
    TEXT_ALIGNMENT_HCENTER     = HK_BIT(1),
    TEXT_ALIGNMENT_RIGHT       = HK_BIT(2),
    TEXT_ALIGNMENT_TOP         = HK_BIT(3),
    TEXT_ALIGNMENT_VCENTER     = HK_BIT(4),
    TEXT_ALIGNMENT_BOTTOM      = HK_BIT(5),
    TEXT_ALIGNMENT_KEEP_SPACES = HK_BIT(6)
};

HK_FLAG_ENUM_OPERATORS(TEXT_ALIGNMENT_FLAGS)

enum class CanvasCursor
{
    Arrow,
    TextInput,
    ResizeAll,
    ResizeNS,
    ResizeEW,
    ResizeNESW,
    ResizeNWSE,
    Hand
};

struct VGPoint
{
    float                   x, y;
    float                   dx, dy;
    float                   len;
    float                   dmx, dmy;
    unsigned char           flags;
};

struct VGPath
{
    int                     First;
    int                     Count;
    bool                    bClosed;
    int                     NumBevel;
    CanvasVertex*           Fill;
    int                     NumFill;
    CanvasVertex*           Stroke;
    int                     NumStroke;
    int                     Winding;
    bool                    bConvex;
};

struct VGPathCache
{
    Vector<VGPoint>         Points;
    Vector<VGPath>          Paths;
    Vector<CanvasVertex>    Verts;
    float                   Bounds[4];
    float                   DistTol;

                            VGPathCache();

    void                    Clear();

    VGPath*                 AddPath();

    void                    AddPoint(float x, float y, int flags);

    CanvasVertex*           AllocVerts(int nverts);
};

struct TextMetrics
{
    float                   Ascender;
    float                   Descender;
    float                   LineHeight;
};

struct TextRow
{
    /// Pointer to the input text where the row starts.
    const char*             Start;

    /// Pointer to the input text where the row ends(one past the last character).
    const char*             End;

    /// Pointer to the beginning of the next row.
    const char*             Next;

    /// Logical width of the row.
    float                   Width;

    /// Actual bounds of the row. Logical with and bounds can differ because of kerning and some parts over extending.
    float                   MinX, MaxX;

    StringView              GetStringView() const { return StringView(Start, End); }
};

struct TextRowW
{
    /// Pointer to the input text where the row starts.
    const WideChar*         Start;

    /// Pointer to the input text where the row ends(one past the last character).
    const WideChar*         End;

    /// Pointer to the beginning of the next row.
    const WideChar*         Next;

    /// Logical width of the row.
    float                   Width;

    /// Actual bounds of the row. Logical width and bounds can differ because of kerning and some parts over extending.
    float                   MinX, MaxX;

    WideStringView          GetStringView() const { return WideStringView(Start, End); }
};

struct FontStyle
{
    float                   FontSize{14};

    /// Font blur allows you to create simple text effects such as drop shadows.
    float                   FontBlur{0};

    /// Letter spacing.
    float                   LetterSpacing{0};

    /// Proportional line height. The line height is specified as multiple of font size.
    float                   LineHeight{1};
};

using FontHandle = uint16_t;

inline constexpr int MAX_FONTS = 32;

class Canvas final : public Noncopyable
{
public:
                            Canvas();
                            ~Canvas();

    void                    SetDefaultFont(StringView path);

    void                    SetRetinaScale(float retinaScale);

    /// Begin drawing a new frame
    void                    NewFrame();

    void                    Finish(class StreamedMemoryGPU* streamMemory);

    /// Clears drawing data.
    void                    ClearDrawData();

    CanvasDrawData const*   GetDrawData() const;

    //
    // State Handling
    //
    // Canvas contains state which represents how paths will be rendered.
    // The state contains transform, fill and stroke styles, text and font styles,
    // and scissor clipping.

    /// Pushes and saves the current render state into a state stack.
    /// A matching Pop() must be used to restore the state.
    void                    Push(CanvasPushFlag resetFlag = CanvasPushFlag::Keep);

    /// Pops and restores current render state.
    void                    Pop();

    /// Resets current render state to default values.Does not affect the render state stack.
    void                    Reset();

    //
    // Scissoring
    //
    // Scissoring allows you to clip the rendering into a rectangle. This is useful for various
    // user interface cases like rendering a text edit or a timeline.

    /// Sets the current scissor rectangle.
    /// The scissor rectangle is transformed by the current transform.    
    void                    Scissor(Float2 const& mins, Float2 const& maxs);

    /// Intersects current scissor rectangle with the specified rectangle.
    /// The scissor rectangle is transformed by the current transform.
    /// Note: in case the rotation of previous scissor rect differs from
    /// the current one, the intersection will be done between the specified
    /// rectangle and the previous scissor rectangle transformed in the current
    /// transform space. The resulting shape is always rectangle.
    void                    IntersectScissor(Float2 const& mins, Float2 const& maxs);

    void                    GetIntersectedScissor(float x, float y, float w, float h, float* px, float* py, float* pw, float* ph);

    void                    GetIntersectedScissor(Float2 const& mins, Float2 const& maxs, Float2& resultMins, Float2& resultMaxs);

    /// Reset and disables scissoring.
    void                    ResetScissor();

    //
    // Composite operation
    //

    /// Sets the composite operation.
    /// The composite operations are modeled after HTML Canvas API.
    /// The colors in the blending state have premultiplied alpha.
    /// Returns previous composite operation.
    CANVAS_COMPOSITE        CompositeOperation(CANVAS_COMPOSITE op);

    //
    // Render styles
    //
    // Fill and stroke render style can be either a solid color or a paint which is a gradient or a pattern.
    // Solid color is simply defined as a color value, different kinds of paints can be created
    // using LinearGradient(), BoxGradient(), RadialGradient() and ImagePattern().
    //
    // Current render style can be saved and restored using Push() and Pop().

    /// Sets whether to draw antialias for Stroke() and Fill(). It's enabled by default.
    bool                    ShapeAntiAlias(bool bEnabled);

    /// Sets current stroke style to a solid color.
    void                    StrokeColor(Color4 const& color);

    /// Sets current stroke style to a paint, which can be a one of the gradients or a pattern.
    void                    StrokePaint(CanvasPaint const& paint);

    /// Sets current fill style to a solid color.
    void                    FillColor(Color4 const& color);

    /// Sets current fill style to a paint, which can be a one of the gradients or a pattern.
    void                    FillPaint(CanvasPaint const& paint);

    /// Sets the miter limit of the stroke style.
    /// Miter limit controls when a sharp corner is beveled.
    void                    MiterLimit(float limit);

    /// Sets the stroke width of the stroke style.
    void                    StrokeWidth(float size);

    /// Sets how the end of the line(cap) is drawn,
    /// Can be one of: CanvasLineCap::Butt (default), CanvasLineCap::Round, CanvasLineCap::Square.
    void                    LineCap(CanvasLineCap cap);

    /// Sets how sharp path corners are drawn.
    /// Can be one of CanvasLineJoin::Miter (default), CanvasLineJoin::Round, CanvasLineJoin::Bevel.
    void                    LineJoin(CanvasLineJoin join);

    /// Sets the transparency applied to all rendered shapes.
    /// Already transparent paths will get proportionally more transparent as well.
    void                    GlobalAlpha(float alpha);

    //
    // Transforms
    //
    // The paths, gradients, patterns and scissor region are transformed by an transformation
    // matrix at the time when they are passed to the API.
    // The current transformation matrix is a affine matrix:
    //   [sx kx tx]
    //   [ky sy ty]
    //   [ 0  0  1]
    // Where: sx,sy define scaling, kx,ky skewing, and tx,ty translation.
    // The last row is assumed to be 0,0,1 and is not stored.
    //
    // Apart from ResetTransform(), each transformation function first creates
    // specific transformation matrix and pre-multiplies the current transformation by it.
    //
    // Current coordinate system (transformation) can be saved and restored using Push() and Pop().

    /// Resets current transform to a identity matrix.
    void                    ResetTransform();

    /// Premultiplies current coordinate system by specified matrix.
    void                    Transform(Transform2D const& transform);

    /// Translates current coordinate system.
    void                    Translate(float x, float y);

    /// Rotates current coordinate system. Angle is specified in radians.
    void                    Rotate(float angle);

    /// Skews the current coordinate system along X axis. Angle is specified in radians.
    void                    SkewX(float angle);

    /// Skews the current coordinate system along Y axis. Angle is specified in radians.
    void                    SkewY(float angle);

    /// Scales the current coordinate system.
    void                    Scale(float x, float y);

    /// Current coordinate system (transformation)
    Transform2D const&      CurrentTransform();

    //
    // Paths
    //
    // Drawing a new shape starts with BeginPath(), it clears all the currently defined paths.
    // Then you define one or more paths and sub-paths which describe the shape. The are functions
    // to draw common shapes like rectangles and circles, and lower level step-by-step functions,
    // which allow to define a path curve by curve.
    //
    // Canvas uses even-odd fill rule to draw the shapes. Solid shapes should have counter clockwise
    // winding and holes should have counter clockwise order. To specify winding of a path you can
    // call PathWinding(). This is useful especially for the common shapes, which are drawn CCW.
    //
    // Finally you can fill the path using current fill style by calling Fill(), and stroke it
    // with current stroke style by calling Stroke().
    //
    // The curve segments and sub-paths are transformed by the current transform.

    /// Clears the current path and sub-paths.
    void                    BeginPath();

    /// Starts new sub-path with specified point as first point.
    void                    MoveTo(float x, float y);
    void                    MoveTo(Float2 const& p);

    /// Adds line segment from the last point in the path to the specified point.
    void                    LineTo(float x, float y);
    void                    LineTo(Float2 const& p);

    /// Adds cubic bezier segment from last point in the path via two control points to the specified point.
    void                    BezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y);

    /// Adds quadratic bezier segment from last point in the path via a control point to the specified point.
    void                    QuadTo(float cx, float cy, float x, float y);

    /// Adds an arc segment at the corner defined by the last path point, and two specified points.
    void                    ArcTo(float x1, float y1, float x2, float y2, float radius);

    /// Closes current sub-path with a line segment.
    void                    ClosePath();

    /// Sets the current sub-path winding, see CanvasPathWinding.
    void                    PathWinding(CanvasPathWinding winding);

    /// Creates new circle arc shaped sub-path. The arc center is at cx,cy, the arc radius is r,
    /// and the arc is drawn from angle a0 to a1, and swept in direction dir (CanvasPathWinding::CCW, or CanvasPathWinding::CW).
    /// Angles are specified in radians.
    void                    Arc(float cx, float cy, float r, float a0, float a1, CanvasPathWinding dir);

    /// Creates new rectangle shaped sub-path.
    void                    Rect(float x, float y, float w, float h);

    /// Creates new rounded rectangle shaped sub-path.
    void                    RoundedRect(float x, float y, float w, float h, float r);

    /// Creates new rounded rectangle shaped sub-path with varying radii for each corner.
    void                    RoundedRectVarying(float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft);

    /// Creates new ellipse shaped sub-path.
    void                    Ellipse(Float2 const& center, float rx, float ry);

    /// Creates new circle shaped sub-path.
    void                    Circle(Float2 const& center, float r);

    /// Fills the current path with current fill style.
    void                    Fill();

    /// Fills the current path with current stroke style.
    void                    Stroke();

    //
    // Text
    //

    bool                    AddFont(FontHandle faceIndex, StringView font);
    void                    RemoveFont(FontHandle faceIndex);

    FontHandle              FindFont(StringView font) const;

    bool                    IsValidFont(FontHandle faceIndex) const;

    /// Sets the font face. If the font is not loaded by the resource manager, the default fallback font will be used.
    void                    FontFace(FontHandle faceIndex);

    /// Sets the font face. If the font is not loaded by the resource manager, the default fallback font will be used.
    void                    FontFace(StringView font);

    /// Draws text string at specified location.
    float                   Text(FontStyle const& style, float x, float y, TEXT_ALIGNMENT_FLAGS flags, StringView string);
    float                   Text(FontStyle const& style, float x, float y, TEXT_ALIGNMENT_FLAGS flags, WideStringView string);

    /// Draws multi-line text string at specified box.
    void                    TextBox(FontStyle const& style, Float2 const& mins, Float2 const& maxs, TEXT_ALIGNMENT_FLAGS flags, bool bWrap, StringView text);
    void                    TextBox(FontStyle const& style, Float2 const& mins, Float2 const& maxs, TEXT_ALIGNMENT_FLAGS flags, bool bWrap, WideStringView text);

    //
    // Text metrics
    //

    /// Returns the vertical metrics based on the current text style.
    void                    GetTextMetrics(FontStyle const& fontStyle, TextMetrics& metrics);

    float                   GetCharAdvance(FontStyle const& fontStyle, WideChar ch);

    /// Measures the size of specified multi-text string
    Float2                  GetTextBoxSize(FontStyle const& fontStyle, float breakRowWidth, StringView text, bool bKeepSpaces = false);
    Float2                  GetTextBoxSize(FontStyle const& fontStyle, float breakRowWidth, WideStringView text, bool bKeepSpaces = false);

    /// Breaks the specified text into lines.
    /// White space is stripped at the beginning of the rows, the text is split at word boundaries or when new-line characters are encountered.
    /// Words longer than the max width are slit at nearest character (i.e. no hyphenation).
    int                     TextBreakLines(FontStyle const& fontStyle, StringView text, float breakRowWidth, TextRow* rows, int maxRows, bool bKeepSpaces = false);
    int                     TextBreakLines(FontStyle const& fontStyle, WideStringView text, float breakRowWidth, TextRowW* rows, int maxRows, bool bKeepSpaces = false);

    int                     TextLineCount(FontStyle const& fontStyle, StringView text, float breakRowWidth, bool bKeepSpaces = false);
    int                     TextLineCount(FontStyle const& fontStyle, WideStringView text, float breakRowWidth, bool bKeepSpaces = false);

    //
    // Utilites
    //

    void                    DrawLine(Float2 const& p0, Float2 const& p1, Color4 const& color, float thickness = 1.0f);

    void                    DrawRect(Float2 const& mins, Float2 const& maxs, Color4 const& color, float thickness = 1.0f, RoundingDesc const& rounding = {});
    void                    DrawRectFilled(Float2 const& mins, Float2 const& maxs, Color4 const& color, RoundingDesc const& rounding = {});

    void                    DrawTriangle(Float2 const& p0, Float2 const& p1, Float2 const& p2, Color4 const& color, float thickness = 1.0f);
    void                    DrawTriangleFilled(Float2 const& p0, Float2 const& p1, Float2 const& p2, Color4 const& color);

    void                    DrawCircle(Float2 const& center, float radius, Color4 const& color, float thickness = 1.0f);
    void                    DrawCircleFilled(Float2 const& center, float radius, Color4 const& color);

    void                    DrawPolyline(Float2 const* points, int numPoints, Color4 const& color, bool bClosed, float thickness = 1.0f);
    void                    DrawPolyFilled(Float2 const* points, int numPoints, Color4 const& color);
    void                    DrawBezierCurve(Float2 const& pos0, Float2 const& cp0, Float2 const& cp1, Float2 const& pos1, Color4 const& color, float thickness = 1.0f);

    void                    DrawText(FontStyle const& style, Float2 const& pos, Color4 const& color, StringView Text, bool bShadow = false);

    // Texture
    void                    DrawTexture(DrawTextureDesc const& desc);

    // Material (TODO)
    //void                  DrawMaterial(DrawMaterialDesc const& desc);

    void                    DrawCursor(CanvasCursor cursor, Float2 const& position, Color4 const& fillColor = Color4::sWhite(), Color4 const& borderColor = Color4::sBlack(), bool bShadow = true);

private:
    struct VGScissor
    {
        Transform2D         Xform;
        float               Extent[2];
    };

    struct VGState
    {
        CANVAS_COMPOSITE    CompositeOperation;
        bool                bShapeAntiAlias;
        CanvasPaint         Fill;
        CanvasPaint         Stroke;
        float               StrokeWidth;
        float               MiterLimit;
        CanvasLineJoin      LineJoin;
        CanvasLineCap       LineCap;
        float               Alpha;
        Transform2D         Xform;
        VGScissor           Scissor;
        FontHandle          FontFace;
    };

    void                    FlattenPaths();
    void                    TesselateBezier(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int level, int type);
    void                    CalculateJoins(float w, CanvasLineJoin lineJoin, float miterLimit);
    int                     ExpandStroke(float w, float fringe, CanvasLineCap lineCap, CanvasLineJoin lineJoin, float miterLimit);
    int                     ExpandFill(float w, CanvasLineJoin lineJoin, float miterLimit);

    void                    ConvertPaint(CanvasUniforms* frag, CanvasPaint* paint, VGScissor const& scissor, float width, float fringe, float strokeThr);

    void                    RenderFill(CanvasPaint* paint, CANVAS_COMPOSITE composite, VGScissor const& scissor, float fringe, const float* bounds);

    void                    RenderStroke(CanvasPaint* paint, CANVAS_COMPOSITE composite, VGScissor const& scissor, float fringe, float strokeWidth);

    void                    RenderTriangles(CanvasPaint* paint, CANVAS_COMPOSITE composite, VGScissor const& scissor, const CanvasVertex* verts, int nverts, float fringe);

    void                    RenderText(CanvasVertex* verts, int nverts);

    void                    AppendCommands(float const* vals, int nvals);

    CanvasDrawCmd*          AllocDrawCommand();

    int                     AllocPaths(int n);

    int                     AllocVerts(int n);

    int                     AllocUniforms(int n);

    VGState*                GetState();

    CanvasUniforms*         GetUniformPtr(int i);

    void                    CreateCursorMap();

    RHI::ITexture*          GetTexture(CanvasPaint const* paint);

    // Returns the current font ID. If the font is invalid, returns the default font ID.
    int                     CurrentFontID();

    void                    PrepareFontAtlas();
    bool                    ReallocFontAtlas();
    void                    UpdateFontAtlas();

    Vector<VGState>         m_States;
    int                     m_NumStates{};
    CanvasDrawData          m_DrawData;
    Vector<float>           m_Commands;
    Float2                  m_CommandPos;
    VGPathCache             m_PathCache;
    float                   m_TessTol{};
    float                   m_DistTol{};
    float                   m_FringeWidth{};
    float                   m_DevicePxRatio{};
    int                     m_DrawCallCount{};
    int                     m_FillTriCount{};
    int                     m_StrokeTriCount{};
    int                     m_TextTriCount{};
    TextureHandle           m_CursorMap;
    uint32_t                m_CursorMapWidth{};
    uint32_t                m_CursorMapHeight{};
    mutable bool            m_UpdateFontTexture{};

    // Flag indicating if geoemtry based anti-aliasing is used (may not be needed when using MSAA).
    bool                    m_EdgeAntialias{};

    // Flag indicating if strokes should be drawn using stencil buffer. The rendering will be a little
    // slower, but path overlaps (i.e. self-intersecting or sharp turns) will be drawn just once.
    bool                    m_StencilStrokes{};

    struct FontData
    {
        String      Name;
        int         ID = -1;
        HeapBlob    Blob;
    };

    Vector<FontData>        m_Fonts;

    enum
    {
        MAX_FONT_IMAGES = 4,
        MAX_FONTIMAGE_SIZE = 2048,
        INITIAL_FONTIMAGE_SIZE = 512
    };
    FONScontext*            m_FontStash{};
    Ref<RHI::ITexture>      m_FontImages[MAX_FONT_IMAGES];
    int                     m_FontImageIdx{};
};

HK_NAMESPACE_END
