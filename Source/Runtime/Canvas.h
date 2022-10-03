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
#include "Font.h"

class ACameraComponent;
class ARenderingParameters;

/**
Paints

Supported four types of paints: linear gradient, box gradient, radial gradient and image pattern.
These can be used as paints for strokes and fills.
*/
struct CanvasPaint
{
    float              Xform[6];
    float              Extent[2];
    float              Radius{};
    float              Feather{};
    Color4             InnerColor;
    Color4             OuterColor;
    RenderCore::ITexture* pTexture{};
    CANVAS_IMAGE_FLAGS ImageFlags = CANVAS_IMAGE_DEFAULT;

    /** Creates and returns a linear gradient. Parameters (sx,sy)-(ex,ey) specify the start and end coordinates
    of the linear gradient, icol specifies the start color and ocol the end color.
    The gradient is transformed by the current transform when it is passed to FillPaint() or StrokePaint(). */
    CanvasPaint& LinearGradient(float sx, float sy, float ex, float ey, Color4 const& icol, Color4 const& ocol);

    /** Creates and returns a radial gradient. Parameters (cx,cy) specify the center, inr and outr specify
    the inner and outer radius of the gradient, icol specifies the start color and ocol the end color.
    The gradient is transformed by the current transform when it is passed to FillPaint() or StrokePaint(). */
    CanvasPaint& RadialGradient(float cx, float cy, float inr, float outr, Color4 const& icol, Color4 const& ocol);

    /** Creates and returns a box gradient. Box gradient is a feathered rounded rectangle, it is useful for rendering
    drop shadows or highlights for boxes. Parameters (x,y) define the top-left corner of the rectangle,
    (w,h) define the size of the rectangle, r defines the corner radius, and f feather. Feather defines how blurry
    the border of the rectangle is. Parameter icol specifies the inner color and ocol the outer color of the gradient.
    The gradient is transformed by the current transform when it is passed to FillPaint() or StrokePaint(). */
    CanvasPaint& BoxGradient(float x, float y, float w, float h, float r, float f, Color4 const& icol, Color4 const& ocol);

    /** Creates and returns an image patter. Parameters (ox,oy) specify the left-top location of the image pattern,
    (ex,ey) the size of one image, angle rotation around the top-left corner, texture is handle to the image to render.
    The gradient is transformed by the current transform when it is passed to FillPaint() or StrokePaint(). */
    CanvasPaint& ImagePattern(float cx, float cy, float w, float h, float angle, ATexture* texture, Color4 const& tintColor, CANVAS_IMAGE_FLAGS imageFlags = CANVAS_IMAGE_DEFAULT);
};

struct CanvasTransform
{
    float Matrix[6];

    CanvasTransform()
    {
        SetIdentity();
    }

    /**
    The parameters are interpreted as matrix as follows:
       [a c e]
       [b d f]
       [0 0 1]
    */
    CanvasTransform(float a, float b, float c, float d, float e, float f)
    {
        Matrix[0] = a;
        Matrix[1] = b;
        Matrix[2] = c;
        Matrix[3] = d;
        Matrix[4] = e;
        Matrix[5] = f;
    }

    CanvasTransform& SetIdentity();

    /** Sets the transform to translation matrix matrix. */
    CanvasTransform& Translate(float tx, float ty);

    /** Sets the transform to scale matrix. */
    CanvasTransform& Scale(float sx, float sy);

    /** Sets the transform to rotate matrix. Angle is specified in radians. */
    CanvasTransform& Rotate(float a);

    /** Sets the transform to skew-x matrix. Angle is specified in radians. */
    CanvasTransform& SkewX(float a);

    /** Sets the transform to skew-y matrix. Angle is specified in radians. */
    CanvasTransform& SkewY(float a);

    /** Sets the transform to the result of multiplication of two transforms, of A = A*B. */
    CanvasTransform& operator*=(CanvasTransform const& rhs);

    /** Sets the transform to the result of multiplication of two transforms, of A = B*A. */
    CanvasTransform& Premultiply(CanvasTransform const& rhs);

    /** Returns inversed transform. */
    CanvasTransform Inversed() const;

    /** Transform a point */
    Float2 TransformPoint(Float2 const& p) const;
};

struct GlyphPosition
{
    /** Position of the glyph in the input string. */
    const char* Str;

    /** The x - coordinate of the logical glyph position. */
    float X;

    /** The bounds of the glyph shape. */
    float MinX, MaxX;
};

struct TextRow
{
    /** Pointer to the input text where the row starts. */
    const char* Start;

    /** Pointer to the input text where the row ends(one past the last character). */
    const char* End;

    /** Pointer to the beginning of the next row. */
    const char* Next;

    /** Logical width of the row. */
    float Width;

    /** Actual bounds of the row.Logical with and bounds can differ because of kerning and some parts over extending. */
    float MinX, MaxX;

    AStringView GetStringView() const { return AStringView(Start, End); }
};


struct NVGcontext;
struct NVGpaint;
struct NVGscissor;
struct NVGvertex;
struct NVGpath;

enum CANVAS_SAVE_FLAG
{
    CANVAS_SAVE_KEEP,
    CANVAS_SAVE_RESET
};

struct SViewport
{
    ACameraComponent*     Camera;
    ARenderingParameters* RenderingParams;
    int                   X;
    int                   Y;
    uint32_t              Width;
    uint32_t              Height;
};

using AViewportList = TSmallVector<SViewport, 2>;

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
    ATexture*     pTexture = 0;
    float         X        = 0;
    float         Y        = 0;
    float         W        = 0;
    float         H        = 0;
    RoundingDesc  Rounding;
    float         Angle     = 0;
    Color4        TintColor = Color4(1.0f);
    Float2        UVOffset{0, 0};
    Float2        UVScale{1, 1};
    CANVAS_COMPOSITE Composite = CANVAS_COMPOSITE_SOURCE_OVER;
    bool          bTiledX : 1;
    bool          bTiledY : 1;
    bool          bFlipY : 1;
    bool          bAlphaPremultiplied : 1;
    bool          bNearestFilter : 1;

    DrawTextureDesc() :
        bTiledX(false),
        bTiledY(false),
        bFlipY(false),
        bAlphaPremultiplied(false),
        bNearestFilter(false)
    {}
};

struct DrawViewportDesc
{
    ACameraComponent*     pCamera          = 0;
    ARenderingParameters* pRenderingParams = 0;
    float                 X                = 0;
    float                 Y                = 0;
    float                 W                = 0;
    float                 H                = 0;
    RoundingDesc          Rounding;
    float                 Angle            = 0;
    Color4                TintColor        = Color4::White();
    CANVAS_COMPOSITE      Composite        = CANVAS_COMPOSITE_SOURCE_OVER;
};

enum CANVAS_LINE_CAP : uint8_t
{
    CANVAS_LINE_CAP_BUTT   = 0,
    CANVAS_LINE_CAP_ROUND  = 1,
    CANVAS_LINE_CAP_SQUARE = 2
};

enum CANVAS_LINE_JOIN : uint8_t
{
    CANVAS_LINE_JOIN_MITER = 0,
    CANVAS_LINE_JOIN_ROUND = 1,
    CANVAS_LINE_JOIN_BEVEL = 2    
};

enum CANVAS_PATH_WINDING : uint8_t
{
    CANVAS_PATH_WINDING_CCW = 1, // Winding for solid shapes
    CANVAS_PATH_WINDING_CW  = 2, // Winding for holes

    CANVAS_PATH_WINDING_SOLID = CANVAS_PATH_WINDING_CCW,
    CANVAS_PATH_WINDING_HOLE  = CANVAS_PATH_WINDING_CW,
};

enum CANVAS_TEXT_ALIGN : uint8_t
{
    // Horizontal align
    CANVAS_TEXT_ALIGN_LEFT     = 1 << 0, // Default, align text horizontally to left.
    CANVAS_TEXT_ALIGN_CENTER   = 1 << 1, // Align text horizontally to center.
    CANVAS_TEXT_ALIGN_RIGHT    = 1 << 2, // Align text horizontally to right.
    // Vertical align
    CANVAS_TEXT_ALIGN_TOP      = 1 << 3, // Align text vertically to top.
    CANVAS_TEXT_ALIGN_MIDDLE   = 1 << 4, // Align text vertically to middle.
    CANVAS_TEXT_ALIGN_BOTTOM   = 1 << 5, // Align text vertically to bottom.
    CANVAS_TEXT_ALIGN_BASELINE = 1 << 6, // Default, align text vertically to baseline.
};

HK_FLAG_ENUM_OPERATORS(CANVAS_TEXT_ALIGN)

enum DRAW_CURSOR
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

class ACanvas
{
    HK_FORBID_COPY(ACanvas)

public:
    static AFont* GetDefaultFont();

    ACanvas();
    virtual ~ACanvas();

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }

    /** Begin drawing a new frame */
    void NewFrame(uint32_t width, uint32_t height);

    /** Clears drawing data. */
    void ClearDrawData();

    CanvasDrawData const* GetDrawData() const { return &m_DrawData; }

    AViewportList const& GetViewports() { return m_Viewports; }

    //
    // State Handling
    //
    // Canvas contains state which represents how paths will be rendered.
    // The state contains transform, fill and stroke styles, text and font styles,
    // and scissor clipping.

    /** Pushes and saves the current render state into a state stack.
    A matching Pop() must be used to restore the state. */
    void Push(CANVAS_SAVE_FLAG ResetFlag = CANVAS_SAVE_KEEP);

    /** Pops and restores current render state. */
    void Pop();

    /** Resets current render state to default values.Does not affect the render state stack. */
    void Reset();

    //
    // Scissoring
    //
    // Scissoring allows you to clip the rendering into a rectangle. This is useful for various
    // user interface cases like rendering a text edit or a timeline.

    /**
    Sets the current scissor rectangle.
    The scissor rectangle is transformed by the current transform.    
    */
    void Scissor(Float2 const& mins, Float2 const& maxs);

    /**
    Intersects current scissor rectangle with the specified rectangle.
    The scissor rectangle is transformed by the current transform.
    Note: in case the rotation of previous scissor rect differs from
    the current one, the intersection will be done between the specified
    rectangle and the previous scissor rectangle transformed in the current
    transform space. The resulting shape is always rectangle.
    */
    void IntersectScissor(Float2 const& mins, Float2 const& maxs);

    /** Reset and disables scissoring. */
    void ResetScissor();

    //
    // Composite operation
    //

    /**
    Sets the composite operation.
    The composite operations are modeled after HTML Canvas API.
    The colors in the blending state have premultiplied alpha.
    Returns previous composite operation.
    */
    CANVAS_COMPOSITE CompositeOperation(CANVAS_COMPOSITE op);

    //
    // Render styles
    //
    // Fill and stroke render style can be either a solid color or a paint which is a gradient or a pattern.
    // Solid color is simply defined as a color value, different kinds of paints can be created
    // using LinearGradient(), BoxGradient(), RadialGradient() and ImagePattern().
    //
    // Current render style can be saved and restored using Push() and Pop().

    /** Sets whether to draw antialias for Stroke() and Fill(). It's enabled by default. */
    void ShapeAntiAlias(bool bEnabled);

    /** Sets current stroke style to a solid color. */
    void StrokeColor(Color4 const& color);

    /** Sets current stroke style to a paint, which can be a one of the gradients or a pattern. */
    void StrokePaint(CanvasPaint const& paint);

    /** Sets current fill style to a solid color. */
    void FillColor(Color4 const& color);

    /** Sets current fill style to a paint, which can be a one of the gradients or a pattern. */
    void FillPaint(CanvasPaint const& paint);

    /** Sets the miter limit of the stroke style.
    Miter limit controls when a sharp corner is beveled. */
    void MiterLimit(float limit);

    /** Sets the stroke width of the stroke style. */
    void StrokeWidth(float size);

    /** Sets how the end of the line(cap) is drawn,
    Can be one of: CANVAS_LINE_CAP_BUTT (default), CANVAS_LINE_CAP_ROUND, CANVAS_LINE_CAP_SQUARE. */
    void LineCap(CANVAS_LINE_CAP cap);

    /** Sets how sharp path corners are drawn.
    Can be one of CANVAS_LINE_JOIN_MITER (default), CANVAS_LINE_JOIN_ROUND, CANVAS_LINE_JOIN_BEVEL. */
    void LineJoin(CANVAS_LINE_JOIN join);

    /** Sets the transparency applied to all rendered shapes.
    Already transparent paths will get proportionally more transparent as well. */
    void GlobalAlpha(float alpha);

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

    /** Resets current transform to a identity matrix. */
    void ResetTransform();

    /**
    Premultiplies current coordinate system by specified matrix.
    The parameters are interpreted as matrix as follows:
       [a c e]
       [b d f]
       [0 0 1]
    */
    void Transform(CanvasTransform const& transform);

    /** Translates current coordinate system. */
    void Translate(float x, float y);

    /** Rotates current coordinate system. Angle is specified in radians. */
    void Rotate(float angle);

    /** Skews the current coordinate system along X axis. Angle is specified in radians. */
    void SkewX(float angle);

    /** Skews the current coordinate system along Y axis. Angle is specified in radians. */
    void SkewY(float angle);

    /** Scales the current coordinate system. */
    void Scale(float x, float y);

    /** Stores the top part (a-f) of the current transformation matrix in to the specified buffer.
       [a c e]
       [b d f]
       [0 0 1] */
    CanvasTransform CurrentTransform();

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

    /** Clears the current path and sub-paths. */
    void BeginPath();

    /** Starts new sub-path with specified point as first point. */
    void MoveTo(float x, float y);
    void MoveTo(Float2 const& p);

    /** Adds line segment from the last point in the path to the specified point. */
    void LineTo(float x, float y);
    void LineTo(Float2 const& p);

    /** Adds cubic bezier segment from last point in the path via two control points to the specified point. */
    void BezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y);

    /** Adds quadratic bezier segment from last point in the path via a control point to the specified point. */
    void QuadTo(float cx, float cy, float x, float y);

    /** Adds an arc segment at the corner defined by the last path point, and two specified points. */
    void ArcTo(float x1, float y1, float x2, float y2, float radius);

    /** Closes current sub-path with a line segment. */
    void ClosePath();

    /** Sets the current sub-path winding, see CANVAS_PATH_WINDING. */
    void PathWinding(CANVAS_PATH_WINDING winding);

    /** Creates new circle arc shaped sub-path. The arc center is at cx,cy, the arc radius is r,
    and the arc is drawn from angle a0 to a1, and swept in direction dir (CANVAS_PATH_WINDING_CCW, or CANVAS_PATH_WINDING_CW).
    Angles are specified in radians. */
    void Arc(float cx, float cy, float r, float a0, float a1, CANVAS_PATH_WINDING dir);

    /** Creates new rectangle shaped sub-path. */
    void Rect(float x, float y, float w, float h);

    /** Creates new rounded rectangle shaped sub-path. */
    void RoundedRect(float x, float y, float w, float h, float r);

    /** Creates new rounded rectangle shaped sub-path with varying radii for each corner. */
    void RoundedRectVarying(float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft);

    /** Creates new ellipse shaped sub-path. */
    void Ellipse(Float2 const& center, float rx, float ry);

    /** Creates new circle shaped sub-path. */
    void Circle(Float2 const& center, float r);

    /** Fills the current path with current fill style. */
    void Fill();

    /** Fills the current path with current stroke style. */
    void Stroke();

    //
    // Text
    //
    // Canvas allows you to and use the font to render text.
    //
    // The appearance of the text can be defined by setting the current text style
    // and by specifying the fill color. Common text and font settings such as
    // font size, letter spacing and text align are supported. Font blur allows you
    // to create simple text effects such as drop shadows.
    //
    // At render time the font face can be set based on the font handles or name.
    //
    // Font measure functions return values in local space, the calculations are
    // carried in the same resolution as the final rendering. This is done because
    // the text glyph positions are snapped to the nearest pixels sharp rendering.
    //
    // The local space means that values are not rotated or scale as per the current
    // transformation. For example if you set font size to 12, which would mean that
    // line height is 16, then regardless of the current scaling and rotation, the
    // returned line height is always 16. Some measures may vary because of the scaling
    // since aforementioned pixel snapping.
    //
    // While this may sound a little odd, the setup allows you to always render the
    // same way regardless of scaling. I.e. following works regardless of scaling:
    //
    //		const char* txt = "Text me up.";
    //		TextBounds(x,y, txt, NULL, bounds);
    //		BeginPath();
    //		RoundedRect(bounds[0],bounds[1], bounds[2]-bounds[0], bounds[3]-bounds[1]);
    //		Fill();
    //
    // Note: currently only solid color fill is supported for text.

    /** Sets the font face based on specified id of current text style. */
    void FontFace(AFont* font);

    /** Sets the font face based on specified name of current text style. */
    void FontFace(AStringView font);

    /** Sets the font size of current text style. */
    void FontSize(float size);

    /** Sets the blur of current text style. */
    void FontBlur(float blur);

    /** Sets the letter spacing of current text style. */
    void TextLetterSpacing(float spacing);

    /** Sets the proportional line height of current text style. The line height is specified as multiple of font size. */
    void TextLineHeight(float lineHeight);

    /** Sets the text align of current text style, see CANVAS_TEXT_ALIGN for options. */
    void TextAlign(CANVAS_TEXT_ALIGN align);

    /** Draws text string at specified location. If end is specified only the sub-string up to the end is drawn. */
    float Text(float x, float y, AStringView string);

    /** Draws multi-line text string at specified location wrapped at the specified width. If end is specified only the sub-string up to the end is drawn.
    White space is stripped at the beginning of the rows, the text is split at word boundaries or when new-line characters are encountered.
    Words longer than the max width are slit at nearest character (i.e. no hyphenation). */
    void TextBox(float x, float y, float breakRowWidth, AStringView string);

    /** Measures the specified text string (see TextBounds structure).
    Returns horizontal advance of the text (i.e.where the next character should drawn).
    Measured values are returned in local coordinate space. */
    float GetTextBounds(float x, float y, AStringView string, TextBounds& bounds);

    /** Returns horizontal advance of the text (i.e.where the next character should drawn). */
    float GetTextAdvance(float x, float y, AStringView string);

    /** Measures the specified multi-text string. The bounds value are [xmin,ymin, xmax,ymax]
    Measured values are returned in local coordinate space. */
    void GetTextBoxBounds(float x, float y, float breakRowWidth, AStringView string, TextBounds& bounds);

    /** Calculates the glyph x positions of the specified text. If end is specified only the sub-string will be used.
    Measured values are returned in local coordinate space. */
    int GetTextGlyphPositions(float x, float y, AStringView string, GlyphPosition* positions, int maxPositions);

    /** Returns the vertical metrics based on the current text style.
    Measured values are returned in local coordinate space. */
    void GetTextMetrics(TextMetrics& metrics);

    /** Breaks the specified text into lines. If end is specified only the sub-string will be used.
    White space is stripped at the beginning of the rows, the text is split at word boundaries or when new-line characters are encountered.
    Words longer than the max width are slit at nearest character (i.e. no hyphenation). */
    int TextBreakLines(AStringView string, float breakRowWidth, TextRow* rows, int maxRows);

    //
    // Utilites
    //

    void DrawLine(Float2 const& p0, Float2 const& p1, Color4 const& color, float thickness = 1.0f);

    void DrawRect(Float2 const& mins, Float2 const& maxs, Color4 const& color, float thickness = 1.0f, RoundingDesc const& rounding = {});
    void DrawRectFilled(Float2 const& mins, Float2 const& maxs, Color4 const& color, RoundingDesc const& rounding = {});
    void DrawRectFilledGradient(Float2 const& mins, Float2 const& maxs, Color4 const& c0, Color4 const& c1, RoundingDesc const& rounding = {});

    void DrawTriangle(Float2 const& p0, Float2 const& p1, Float2 const& p2, Color4 const& color, float thickness = 1.0f);
    void DrawTriangleFilled(Float2 const& p0, Float2 const& p1, Float2 const& p2, Color4 const& color);

    void DrawCircle(Float2 const& center, float radius, Color4 const& color, float thickness = 1.0f);
    void DrawCircleFilled(Float2 const& center, float radius, Color4 const& color);

    void DrawPolyline(Float2 const* points, int numPoints, Color4 const& color, bool bClosed, float thickness = 1.0f);
    void DrawPolyFilled(Float2 const* points, int numPoints, Color4 const& color);
    void DrawBezierCurve(Float2 const& pos0, Float2 const& cp0, Float2 const& cp1, Float2 const& pos1, Color4 const& color, float thickness = 1.0f);

    void DrawTextUTF8(Float2 const& pos, Color4 const& color, AStringView Text, bool bShadow = false);
    void DrawTextWrapUTF8(Float2 const& pos, Color4 const& color, AStringView Text, float wrapWidth, bool bShadow = false);
    void DrawChar(char ch, float x, float y, Color4 const& color);
    void DrawCharUTF8(const char* ch, float x, float y, Color4 const& color);

    // TODO: Remove
    HK_DEPRECATED void DrawTextWChar(Float2 const& pos, Color4 const& color, AWideStringView text, bool bShadow = false);
    HK_DEPRECATED void DrawTextWrapWChar(Float2 const& pos, Color4 const& color, AWideStringView text, float wrapWidth, bool bShadow = false);
    HK_DEPRECATED void DrawWChar(WideChar ch, float x, float y, Color4 const& color);

    // Texture
    void DrawTexture(DrawTextureDesc const& desc);

    // Material (TODO)
    //void DrawMaterial(DrawMaterialDesc const& desc);

    // Viewport
    void DrawViewport(DrawViewportDesc const& desc);

    void DrawCursor(DRAW_CURSOR cursor, Float2 const& position, Color4 const& fillColor = Color4::White(), Color4 const& borderColor = Color4::Black(), bool bShadow = true);

private:
    void RenderFill(NVGpaint* paint, CANVAS_COMPOSITE composite, NVGscissor* scissor, float fringe, const float* bounds, const NVGpath* paths, int npaths);

    void RenderStroke(NVGpaint* paint, CANVAS_COMPOSITE composite, NVGscissor* scissor, float fringe, float strokeWidth, const NVGpath* paths, int npaths);

    void RenderTriangles(NVGpaint* paint, CANVAS_COMPOSITE composite, NVGscissor* scissor, const NVGvertex* verts, int nverts, float fringe);

    CanvasDrawCmd* AllocDrawCommand();

    int AllocPaths(int n);

    int AllocVerts(int n);

    int AllocUniforms(int n);

    CanvasUniforms* GetUniformPtr(int i);

    uint32_t         m_Width{};
    uint32_t         m_Height{};
    NVGcontext*      m_Context{};
    CanvasDrawData   m_DrawData;
    TRef<AFontStash> m_FontStash;
    AViewportList    m_Viewports;

    // Flag indicating if geoemtry based anti-aliasing is used (may not be needed when using MSAA).
    bool             m_bEdgeAntialias;

    // Flag indicating if strokes should be drawn using stencil buffer. The rendering will be a little
    // slower, but path overlaps (i.e. self-intersecting or sharp turns) will be drawn just once.
    bool             m_bStencilStrokes;

    TRef<ATexture>   m_Cursors;
};
