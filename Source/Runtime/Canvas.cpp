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

#include "Canvas.h"
#include "PlayerController.h"
#include "HUD.h"
#include "Texture.h"
#include "ResourceManager.h"

#include <Platform/Utf8.h>
#include <Platform/Logger.h>

#include "nanovg/nanovg.h"
#include "nanovg/fontstash.h"

CanvasPaint& CanvasPaint::LinearGradient(float sx, float sy, float ex, float ey, Color4 const& icol, Color4 const& ocol)
{
    float dx, dy, d;
    const float large = 1e5;

    Platform::ZeroMem(this, sizeof(*this));

    // Calculate transform aligned to the line
    dx = ex - sx;
    dy = ey - sy;
    d = sqrtf(dx * dx + dy * dy);
    if (d > 0.0001f)
    {
        dx /= d;
        dy /= d;
    }
    else
    {
        dx = 0;
        dy = 1;
    }

    Xform[0] = dy;
    Xform[1] = -dx;
    Xform[2] = dx;
    Xform[3] = dy;
    Xform[4] = sx - dx * large;
    Xform[5] = sy - dy * large;

    Extent[0] = large;
    Extent[1] = large + d * 0.5f;

    Radius = 0.0f;

    Feather = Math::Max(1.0f, d);

    InnerColor = icol;
    OuterColor = ocol;

    return *this;
}

CanvasPaint& CanvasPaint::RadialGradient(float cx, float cy, float inr, float outr, Color4 const& icol, Color4 const& ocol)
{
    float r = (inr + outr) * 0.5f;
    float f = (outr - inr);

    Platform::ZeroMem(this, sizeof(*this));

    nvgTransformIdentity(Xform);
    Xform[4] = cx;
    Xform[5] = cy;

    Extent[0] = r;
    Extent[1] = r;

    Radius = r;

    Feather = Math::Max(1.0f, f);

    InnerColor = icol;
    OuterColor = ocol;

    return *this;
}

CanvasPaint& CanvasPaint::BoxGradient(float x, float y, float w, float h, float r, float f, Color4 const& icol, Color4 const& ocol)
{
    Platform::ZeroMem(this, sizeof(*this));

    nvgTransformIdentity(Xform);
    Xform[4] = x + w * 0.5f;
    Xform[5] = y + h * 0.5f;

    Extent[0] = w * 0.5f;
    Extent[1] = h * 0.5f;

    Radius = r;

    Feather = Math::Max(1.0f, f);

    InnerColor = icol;
    OuterColor = ocol;

    return *this;
}

CanvasPaint& CanvasPaint::ImagePattern(float x, float y, float w, float h, float angle, ATexture* texture, Color4 const& tintColor, CANVAS_IMAGE_FLAGS imageFlags)
{
    Platform::ZeroMem(this, sizeof(*this));

    if (angle != 0.0f)
        nvgTransformRotate(Xform, angle);
    else
        nvgTransformIdentity(Xform);

    Xform[4] = x;
    Xform[5] = y;

    Extent[0] = w;
    Extent[1] = h;

    pTexture = texture->GetGPUResource();
    ImageFlags = imageFlags;

    InnerColor = OuterColor = tintColor;

    return *this;
}

CanvasPaint& CanvasPaint::Solid(Color4 const& color)
{
    Platform::ZeroMem(this, sizeof(*this));

    nvgTransformIdentity(Xform);
    InnerColor = OuterColor = color;

    return *this;
}

CanvasTransform& CanvasTransform::SetIdentity()
{
    nvgTransformIdentity(Matrix);
    return *this;
}

CanvasTransform& CanvasTransform::Translate(float tx, float ty)
{
    nvgTransformTranslate(Matrix, tx, ty);
    return *this;
}

CanvasTransform& CanvasTransform::Scale(float sx, float sy)
{
    nvgTransformScale(Matrix, sx, sy);
    return *this;
}

CanvasTransform& CanvasTransform::Rotate(float a)
{
    nvgTransformRotate(Matrix, a);
    return *this;
}

CanvasTransform& CanvasTransform::SkewX(float a)
{
    nvgTransformSkewX(Matrix, a);
    return *this;
}

CanvasTransform& CanvasTransform::SkewY(float a)
{
    nvgTransformSkewY(Matrix, a);
    return *this;
}

CanvasTransform& CanvasTransform::operator*=(CanvasTransform const& rhs)
{
    nvgTransformMultiply(Matrix, rhs.Matrix);
    return *this;
}

CanvasTransform& CanvasTransform::Premultiply(CanvasTransform const& rhs)
{
    nvgTransformPremultiply(Matrix, rhs.Matrix);
    return *this;
}

CanvasTransform CanvasTransform::Inversed() const
{
    CanvasTransform inversed;
    nvgTransformInverse(inversed.Matrix, Matrix);
    return inversed;
}

Float2 CanvasTransform::TransformPoint(Float2 const& p) const
{
    Float2 result;
    nvgTransformPoint(&result.X, &result.Y, Matrix, p.X, p.Y);
    return result;
}

static void CopyMatrix3to4(float* pDest, const float* pSource)
{
    unsigned int i;
    for (i = 0; i < 3; i++)
    {
        memcpy(&pDest[i * 4], &pSource[i * 3], sizeof(float) * 3);
    }
}

static void xformToMat3x3(float* m3, float* t)
{
    m3[0] = t[0];
    m3[1] = t[1];
    m3[2] = 0.0f;
    m3[3] = t[2];
    m3[4] = t[3];
    m3[5] = 0.0f;
    m3[6] = t[4];
    m3[7] = t[5];
    m3[8] = 1.0f;
}

static HK_FORCEINLINE Color4 ConvertColor(NVGcolor const& c)
{
    return Color4(c.r, c.g, c.b, c.a);
}

static int GetVertexCount(const NVGpath* paths, int npaths)
{
    int i, count = 0;
    for (i = 0; i < npaths; i++)
    {
        count += paths[i].nfill;
        count += paths[i].nstroke;
    }
    return count;
}

static HK_FORCEINLINE void SetVertex(CanvasVertex* vtx, float x, float y, float u, float v)
{
    vtx->x = x;
    vtx->y = y;
    vtx->u = u;
    vtx->v = v;
}


static void ConvertPaint(CanvasUniforms* frag, NVGpaint* paint, NVGscissor* scissor, float width, float fringe, float strokeThr)
{
    float invxform[6], paintMat[9], scissorMat[9];

    memset(frag, 0, sizeof(*frag));

    frag->innerCol = ConvertColor(paint->innerColor);
    frag->outerCol = ConvertColor(paint->outerColor);

    if (scissor->extent[0] < -0.5f || scissor->extent[1] < -0.5f)
    {
        memset(scissorMat, 0, sizeof(scissorMat));
        frag->scissorExt[0] = 1.0f;
        frag->scissorExt[1] = 1.0f;
        frag->scissorScale[0] = 1.0f;
        frag->scissorScale[1] = 1.0f;
    }
    else
    {
        nvgTransformInverse(invxform, scissor->xform);
        xformToMat3x3(scissorMat, invxform);
        frag->scissorExt[0] = scissor->extent[0];
        frag->scissorExt[1] = scissor->extent[1];
        frag->scissorScale[0] = sqrtf(scissor->xform[0] * scissor->xform[0] + scissor->xform[2] * scissor->xform[2]) / fringe;
        frag->scissorScale[1] = sqrtf(scissor->xform[1] * scissor->xform[1] + scissor->xform[3] * scissor->xform[3]) / fringe;
    }
    CopyMatrix3to4(frag->scissorMat, scissorMat);

    frag->extent[0] = paint->extent[0];
    frag->extent[1] = paint->extent[1];

    frag->strokeMult = (width * 0.5f + fringe * 0.5f) / fringe;
    frag->strokeThr = strokeThr;

    if (paint->image != 0)
    {
        if ((paint->imageFlags & CANVAS_IMAGE_FLIPY) != 0)
        {
            float m1[6], m2[6];
            nvgTransformTranslate(m1, 0.0f, frag->extent[1] * 0.5f);
            nvgTransformMultiply(m1, paint->xform);
            nvgTransformScale(m2, 1.0f, -1.0f);
            nvgTransformMultiply(m2, m1);
            nvgTransformTranslate(m1, 0.0f, -frag->extent[1] * 0.5f);
            nvgTransformMultiply(m1, m2);
            nvgTransformInverse(invxform, m1);
        }
        else
        {
            nvgTransformInverse(invxform, paint->xform);
        }
        frag->type = CANVAS_SHADER_FILLIMG;

        frag->texType = (paint->imageFlags & CANVAS_IMAGE_PREMULTIPLIED) ? 0 : 1;
    }
    else
    {
        frag->type = CANVAS_SHADER_FILLGRAD;
        frag->radius = paint->radius;
        frag->feather = paint->feather;
        nvgTransformInverse(invxform, paint->xform);
    }

    xformToMat3x3(paintMat, invxform);
    CopyMatrix3to4(frag->paintMat, paintMat);
}

ACanvas::ACanvas() :
    m_bEdgeAntialias(true), // TODO: Set from config?
    m_bStencilStrokes(true) // TODO: Set from config?
{}

ACanvas::~ACanvas()
{
    if (m_Context)
        nvgDeleteInternal(m_Context);

    Platform::GetHeapAllocator<HEAP_MISC>().Free(m_DrawData.Paths);
    Platform::GetHeapAllocator<HEAP_MISC>().Free(m_DrawData.Vertices);
    Platform::GetHeapAllocator<HEAP_MISC>().Free(m_DrawData.Uniforms);
    Platform::GetHeapAllocator<HEAP_MISC>().Free(m_DrawData.DrawCommands);
}

AFont* ACanvas::GetDefaultFont()
{
    static TStaticResourceFinder<AFont> FontResource("/Root/fonts/RobotoMono/RobotoMono-Regular.ttf"s);
    return FontResource.GetObject();
}

void ACanvas::NewFrame(uint32_t width, uint32_t height)
{
    if (!m_Context)
    {
        m_FontStash = GetSharedInstance<AFontStash>();

        NVGparams params = {};
        params.renderFill = [](void* uptr, NVGpaint* paint, NVGcompositeOperation compositeOperation, NVGscissor* scissor, float fringe, const float* bounds, const NVGpath* paths, int npaths)
        {
            ((ACanvas*)uptr)->RenderFill(paint, CANVAS_COMPOSITE(compositeOperation), scissor, fringe, bounds, paths, npaths);
        };
        params.renderStroke = [](void* uptr, NVGpaint* paint, NVGcompositeOperation compositeOperation, NVGscissor* scissor, float fringe, float strokeWidth, const NVGpath* paths, int npaths)
        {
            ((ACanvas*)uptr)->RenderStroke(paint, CANVAS_COMPOSITE(compositeOperation), scissor, fringe, strokeWidth, paths, npaths);
        };
        params.renderTriangles = [](void* uptr, NVGpaint* paint, NVGcompositeOperation compositeOperation, NVGscissor* scissor, const NVGvertex* verts, int nverts, float fringe)
        {
            ((ACanvas*)uptr)->RenderTriangles(paint, CANVAS_COMPOSITE(compositeOperation), scissor, verts, nverts, fringe);
        };
        params.reallocateTexture = [](void* uptr) -> int
        {
            return (int)((ACanvas*)uptr)->m_FontStash->ReallocTexture();
        };
        params.updateFontTexture = [](void* uptr)
        {
            ((ACanvas*)uptr)->m_FontStash->UpdateTexture();
        };
        params.getFontTexture = [](void* uptr) -> void*
        {
            return ((ACanvas*)uptr)->m_FontStash->GetTexture();
        };

        params.userPtr = this;
        params.edgeAntiAlias = m_bEdgeAntialias;

        m_Context = nvgCreateInternal(&params, m_FontStash->GetImpl());
        if (!m_Context)
            CriticalError("ACanvas: failed to initialize\n");
    }

    m_FontStash->Cleanup();

    m_Width = width;
    m_Height = height;
    m_Viewports.Clear();

    ClearDrawData();

    float devicePixelRatio = GEngine->GetRetinaScale().X;
    nvgBeginFrame(m_Context, devicePixelRatio);

    // Set default font
    FontFace(nullptr);
}

void ACanvas::Push(CANVAS_PUSH_FLAG ResetFlag)
{
    nvgSave(m_Context, ResetFlag);
}

void ACanvas::Pop()
{
    nvgRestore(m_Context);
}

void ACanvas::Reset()
{
    nvgReset(m_Context);
}

void ACanvas::DrawLine(Float2 const& p0, Float2 const& p1, Color4 const& color, float thickness)
{
    if (thickness <= 0)
        return;

    BeginPath();
    MoveTo(p0);
    LineTo(p1);
    StrokeColor(color);
    StrokeWidth(thickness);
    Stroke();
}

void ACanvas::DrawRect(Float2 const& mins, Float2 const& maxs, Color4 const& color, float thickness, RoundingDesc const& rounding)
{
    if (thickness <= 0)
        return;

    BeginPath();
    RoundedRectVarying(mins.X, mins.Y, maxs.X - mins.X, maxs.Y - mins.Y,
                       rounding.RoundingTL,
                       rounding.RoundingTR,
                       rounding.RoundingBR,
                       rounding.RoundingBL);

    StrokeColor(color);
    StrokeWidth(thickness);
    Stroke();
}

void ACanvas::DrawRectFilled(Float2 const& mins, Float2 const& maxs, Color4 const& color, RoundingDesc const& rounding)
{
    BeginPath();
    RoundedRectVarying(mins.X, mins.Y, maxs.X - mins.X, maxs.Y - mins.Y,
                       rounding.RoundingTL,
                       rounding.RoundingTR,
                       rounding.RoundingBR,
                       rounding.RoundingBL);

    FillColor(color);
    Fill();
}

void ACanvas::DrawTriangle(Float2 const& p0, Float2 const& p1, Float2 const& p2, Color4 const& color, float thickness)
{
    if (thickness <= 0)
        return;

    BeginPath();
    MoveTo(p0);
    LineTo(p1);
    LineTo(p2);
    ClosePath();
    StrokeColor(color);
    StrokeWidth(thickness);
    Stroke();
}

void ACanvas::DrawTriangleFilled(Float2 const& p0, Float2 const& p1, Float2 const& p2, Color4 const& color)
{
    BeginPath();
    MoveTo(p0);
    LineTo(p1);
    LineTo(p2);
    FillColor(color);
    Fill();
}

void ACanvas::DrawCircle(Float2 const& center, float radius, Color4 const& color, float thickness)
{
    if (thickness <= 0)
        return;

    BeginPath();
    Circle(center, radius);
    StrokeColor(color);
    StrokeWidth(thickness);
    Stroke();
}

void ACanvas::DrawCircleFilled(Float2 const& center, float radius, Color4 const& color)
{
    BeginPath();
    Circle(center, radius);
    FillColor(color);
    Fill();
}

void ACanvas::DrawTextUTF8(FontStyle const& style, Float2 const& pos, Color4 const& color, AStringView text, bool bShadow)
{
    if (bShadow)
    {
        FontStyle shadowStyle = style;
        shadowStyle.FontBlur  = 1;
        FillColor(Color4(0, 0, 0, color.A));
        Text(shadowStyle, pos.X + 2, pos.Y + 2, HALIGNMENT_LEFT, text);
    }

    FillColor(color);
    Text(style, pos.X, pos.Y, HALIGNMENT_LEFT, text);
}

void ACanvas::DrawTextWrapUTF8(FontStyle const& style, Float2 const& pos, Color4 const& color, AStringView text, float wrapWidth, bool bShadow)
{
    if (bShadow)
    {
        FontStyle shadowStyle = style;
        shadowStyle.FontBlur  = 1;
        FillColor(Color4(0, 0, 0, color.A));
        TextBox(shadowStyle, pos.X + 2, pos.Y + 2, wrapWidth, HALIGNMENT_LEFT, text);
    }

    FillColor(color);
    TextBox(style, pos.X, pos.Y, wrapWidth, HALIGNMENT_LEFT, text);
}

void ACanvas::DrawTextWChar(FontStyle const& style, Float2 const& pos, Color4 const& color, AWideStringView text, bool bShadow)
{
    TVector<char> str(text.Size() * 4 + 1); // In worst case WideChar transforms to 4 bytes,
                                            // one additional byte is reserved for trailing '\0'

    Core::WideStrEncodeUTF8(str.ToPtr(), str.Size(), text.Begin(), text.End());

    DrawTextUTF8(style, pos, color, AStringView(str.ToPtr(), str.Size()), bShadow);
}

void ACanvas::DrawTextWrapWChar(FontStyle const& style, Float2 const& pos, Color4 const& color, AWideStringView text, float wrapWidth, bool bShadow)
{
    TVector<char> str(text.Size() * 4 + 1); // In worst case WideChar transforms to 4 bytes,
                                            // one additional byte is reserved for trailing '\0'

    Core::WideStrEncodeUTF8(str.ToPtr(), str.Size(), text.Begin(), text.End());

    DrawTextWrapUTF8(style, pos, color, AStringView(str.ToPtr(), str.Size()), wrapWidth, bShadow);
}

void ACanvas::DrawChar(FontStyle const& style, char ch, float x, float y, Color4 const& color)
{
    FillColor(color);
    Text(style, x, y, HALIGNMENT_LEFT, AStringView(&ch, 1));
}

void ACanvas::DrawWChar(FontStyle const& style, WideChar ch, float x, float y, Color4 const& color)
{
    char buf[4];
    int n = Core::WideCharEncodeUTF8(buf, sizeof(buf), ch);

    FillColor(color);
    Text(style, x, y, HALIGNMENT_LEFT, AStringView(buf, n));
}

void ACanvas::DrawCharUTF8(FontStyle const& style, const char* ch, float x, float y, Color4 const& color)
{
    if (!ch)
        return;

    FillColor(color);
    Text(style, x, y, HALIGNMENT_LEFT, AStringView(ch, Core::UTF8CharSizeInBytes(ch)));
}

void ACanvas::DrawTexture(DrawTextureDesc const& desc)
{
    if (desc.W < 1.0f || desc.H < 1.0f)
        return;

    if (desc.Composite == CANVAS_COMPOSITE_SOURCE_OVER && desc.TintColor.IsTransparent())
        return;

    float clipX, clipY, clipW, clipH;
    nvgGetIntersectedScissor(m_Context, desc.X, desc.Y, desc.W, desc.H, &clipX, &clipY, &clipW, &clipH);

    if (clipW < 1.0f || clipH < 1.0f)
        return;

    CANVAS_IMAGE_FLAGS imageFlags = CANVAS_IMAGE_DEFAULT;
    if (desc.bTiledX)
        imageFlags |= CANVAS_IMAGE_REPEATX;
    if (desc.bTiledX)
        imageFlags |= CANVAS_IMAGE_REPEATY;
    if (desc.bFlipY)
        imageFlags |= CANVAS_IMAGE_FLIPY;
    if (desc.bAlphaPremultiplied)
        imageFlags |= CANVAS_IMAGE_PREMULTIPLIED;
    if (desc.bNearestFilter)
        imageFlags |= CANVAS_IMAGE_NEAREST;

    CANVAS_COMPOSITE currentComposite = CompositeOperation(desc.Composite);

    CanvasPaint paint;
    paint.ImagePattern(desc.X + desc.UVOffset.X,
                       desc.Y + desc.UVOffset.Y,
                       desc.W * desc.UVScale.X,
                       desc.H * desc.UVScale.Y,
                       desc.Angle,
                       desc.pTexture,
                       desc.TintColor,
                       imageFlags);
    BeginPath();
    RoundedRectVarying(desc.X, desc.Y, desc.W, desc.H,
                       desc.Rounding.RoundingTL,
                       desc.Rounding.RoundingTR,
                       desc.Rounding.RoundingBR,
                       desc.Rounding.RoundingBL);
    FillPaint(paint);
    Fill();

    CompositeOperation(currentComposite);
}

void ACanvas::DrawViewport(DrawViewportDesc const& desc)
{
    if (!desc.pCamera)
        return;

    if (!desc.pRenderingParams)
        return;

    if (desc.W < 1.0f || desc.H < 1.0f)
        return;

    if (desc.TextureResolutionX < 1 || desc.TextureResolutionY < 1)
        return;

    if (desc.Composite == CANVAS_COMPOSITE_SOURCE_OVER && desc.TintColor.IsTransparent())
        return;

    float clipX, clipY, clipW, clipH;
    nvgGetIntersectedScissor(m_Context, desc.X, desc.Y, desc.W, desc.H, &clipX, &clipY, &clipW, &clipH);

    if (clipW < 1.0f || clipH < 1.0f)
        return;

    CANVAS_COMPOSITE currentComposite = CompositeOperation(desc.Composite);

    CanvasPaint paint;

    if (desc.Angle != 0.0f)
        nvgTransformRotate(paint.Xform, desc.Angle);
    else
        nvgTransformIdentity(paint.Xform);
    paint.Xform[4] = desc.X;
    paint.Xform[5] = desc.Y;
    paint.Extent[0] = desc.W;
    paint.Extent[1] = desc.H;
    paint.pTexture = (RenderCore::ITexture*)(size_t)(m_Viewports.Size() + 1);
    paint.ImageFlags = _CANVAS_IMAGE_VIEWPORT_INDEX | CANVAS_IMAGE_FLIPY | CANVAS_IMAGE_NEAREST;
    paint.InnerColor = desc.TintColor;
    paint.OuterColor = desc.TintColor;

    BeginPath();
    RoundedRectVarying(desc.X, desc.Y, desc.W, desc.H,
                       desc.Rounding.RoundingTL,
                       desc.Rounding.RoundingTR,
                       desc.Rounding.RoundingBR,
                       desc.Rounding.RoundingBL);
    FillPaint(paint);
    //FillColor(Color4::Orange());
    Fill();

    CompositeOperation(currentComposite);

    SViewport& viewport = m_Viewports.Add();
    viewport.X               = desc.X;
    viewport.Y               = desc.Y;
    viewport.Width           = desc.TextureResolutionX;
    viewport.Height          = desc.TextureResolutionY;
    viewport.Camera          = desc.pCamera;
    viewport.RenderingParams = desc.pRenderingParams;
}

void ACanvas::DrawPolyline(Float2 const* points, int numPoints, Color4 const& color, bool bClosed, float thickness)
{
    if (numPoints <= 0)
        return;

    if (thickness <= 0)
        return;

    BeginPath();
    MoveTo(points[0]);
    for (int i = 1; i < numPoints; ++i)
    {
        LineTo(points[i]);
    }

    if (bClosed)
        ClosePath();

    StrokeColor(color);
    StrokeWidth(thickness);
    Stroke();
}

void ACanvas::DrawPolyFilled(Float2 const* points, int numPoints, Color4 const& color)
{
    if (numPoints <= 0)
        return;

    BeginPath();
    MoveTo(points[0]);
    for (int i = 1; i < numPoints; ++i)
    {
        LineTo(points[i]);
    }

    FillColor(color);
    Fill();
}

void ACanvas::DrawBezierCurve(Float2 const& pos0, Float2 const& cp0, Float2 const& cp1, Float2 const& pos1, Color4 const& color, float thickness)
{
    if (thickness <= 0)
        return;

    BeginPath();
    MoveTo(pos0);
    BezierTo(cp0.X, cp0.Y, cp1.X, cp1.Y, pos1.X, pos1.Y);
    StrokeColor(color);
    StrokeWidth(thickness);
    Stroke();
}

HK_FORCEINLINE RenderCore::ITexture* ConvertTexture(NVGpaint* paint)
{
    return (RenderCore::ITexture*)paint->image;
}

void ACanvas::RenderFill(NVGpaint* paint, CANVAS_COMPOSITE composite, NVGscissor* scissor, float fringe, const float* bounds, const NVGpath* paths, int npaths)
{
    CanvasDrawCmd* drawCommand = AllocDrawCommand();
    CanvasVertex* quad;
    CanvasUniforms* frag;

    drawCommand->Type = CANVAS_DRAW_COMMAND_FILL;
    drawCommand->Composite = composite;
    drawCommand->VertexCount = 4;
    drawCommand->FirstPath = AllocPaths(npaths);
    drawCommand->PathCount = npaths;
    drawCommand->pTexture = ConvertTexture(paint);
    drawCommand->TextureFlags = CANVAS_IMAGE_FLAGS(paint->imageFlags);

    if (npaths == 1 && paths[0].convex)
    {
        drawCommand->Type = CANVAS_DRAW_COMMAND_CONVEXFILL;
        drawCommand->VertexCount = 0; // Bounding box fill quad not needed for convex fill
    }

    // Allocate vertices for all the paths.
    int offset = AllocVerts(GetVertexCount(paths, npaths) + drawCommand->VertexCount);

    for (int i = 0; i < npaths; i++)
    {
        CanvasPath* copy = &m_DrawData.Paths[drawCommand->FirstPath + i];
        const NVGpath* path = &paths[i];
        memset(copy, 0, sizeof(CanvasPath));
        if (path->nfill > 0)
        {
            copy->FillOffset = offset;
            copy->FillCount = path->nfill;
            memcpy(&m_DrawData.Vertices[offset], path->fill, sizeof(NVGvertex) * path->nfill);
            offset += path->nfill;
        }
        if (path->nstroke > 0)
        {
            copy->StrokeOffset = offset;
            copy->StrokeCount = path->nstroke;
            memcpy(&m_DrawData.Vertices[offset], path->stroke, sizeof(NVGvertex) * path->nstroke);
            offset += path->nstroke;
        }
    }

    if (drawCommand->Type == CANVAS_DRAW_COMMAND_FILL)
    {
        // Quad
        drawCommand->FirstVertex = offset;
        quad = &m_DrawData.Vertices[drawCommand->FirstVertex];
        SetVertex(&quad[0], bounds[2], bounds[3], 0.5f, 1.0f);
        SetVertex(&quad[1], bounds[2], bounds[1], 0.5f, 1.0f);
        SetVertex(&quad[2], bounds[0], bounds[3], 0.5f, 1.0f);
        SetVertex(&quad[3], bounds[0], bounds[1], 0.5f, 1.0f);

        drawCommand->UniformOffset = AllocUniforms(2);
        // Simple shader for stencil
        frag = GetUniformPtr(drawCommand->UniformOffset);
        memset(frag, 0, sizeof(*frag));
        frag->strokeThr = -1.0f;
        frag->type = CANVAS_SHADER_SIMPLE;
        // Fill shader
        ConvertPaint(GetUniformPtr(drawCommand->UniformOffset + sizeof(CanvasUniforms)), paint, scissor, fringe, fringe, -1.0f);
    }
    else
    {
        drawCommand->UniformOffset = AllocUniforms(1);
        // Fill shader
        ConvertPaint(GetUniformPtr(drawCommand->UniformOffset), paint, scissor, fringe, fringe, -1.0f);
    }
}

void ACanvas::RenderStroke(NVGpaint* paint, CANVAS_COMPOSITE composite, NVGscissor* scissor, float fringe, float strokeWidth, const NVGpath* paths, int npaths)
{
    CanvasDrawCmd* drawCommand = AllocDrawCommand();

    drawCommand->Type = CANVAS_DRAW_COMMAND_STROKE;
    drawCommand->Composite = composite;
    drawCommand->FirstPath = AllocPaths(npaths);
    drawCommand->PathCount = npaths;
    drawCommand->pTexture = ConvertTexture(paint);
    drawCommand->TextureFlags = CANVAS_IMAGE_FLAGS(paint->imageFlags);

    // Allocate vertices for all the paths.
    int offset = AllocVerts(GetVertexCount(paths, npaths));

    for (int i = 0; i < npaths; i++)
    {
        CanvasPath* copy = &m_DrawData.Paths[drawCommand->FirstPath + i];
        const NVGpath* path = &paths[i];
        memset(copy, 0, sizeof(CanvasPath));
        if (path->nstroke)
        {
            copy->StrokeOffset = offset;
            copy->StrokeCount = path->nstroke;
            memcpy(&m_DrawData.Vertices[offset], path->stroke, sizeof(NVGvertex) * path->nstroke);
            offset += path->nstroke;
        }
    }

    if (m_bStencilStrokes)
    {
        drawCommand->Type = CANVAS_DRAW_COMMAND_STENCIL_STROKE;

        // Fill shader
        drawCommand->UniformOffset = AllocUniforms(2);

        ConvertPaint(GetUniformPtr(drawCommand->UniformOffset), paint, scissor, strokeWidth, fringe, -1.0f);
        ConvertPaint(GetUniformPtr(drawCommand->UniformOffset + sizeof(CanvasUniforms)), paint, scissor, strokeWidth, fringe, 1.0f - 0.5f / 255.0f);
    }
    else
    {
        // Fill shader
        drawCommand->UniformOffset = AllocUniforms(1);
        ConvertPaint(GetUniformPtr(drawCommand->UniformOffset), paint, scissor, strokeWidth, fringe, -1.0f);
    }
}

void ACanvas::RenderTriangles(NVGpaint* paint, CANVAS_COMPOSITE composite, NVGscissor* scissor, const NVGvertex* verts, int nverts, float fringe)
{
    CanvasDrawCmd* drawCommand = AllocDrawCommand();
    CanvasUniforms* frag;

    drawCommand->Type = CANVAS_DRAW_COMMAND_TRIANGLES;
    drawCommand->Composite = composite;
    drawCommand->pTexture = ConvertTexture(paint);
    drawCommand->TextureFlags = CANVAS_IMAGE_FLAGS(paint->imageFlags);

    // Allocate vertices for all the paths.
    drawCommand->FirstVertex = AllocVerts(nverts);
    drawCommand->VertexCount = nverts;

    memcpy(&m_DrawData.Vertices[drawCommand->FirstVertex], verts, sizeof(NVGvertex) * nverts);

    // Fill shader
    drawCommand->UniformOffset = AllocUniforms(1);
    frag = GetUniformPtr(drawCommand->UniformOffset);
    ConvertPaint(frag, paint, scissor, 1.0f, fringe, -1.0f);
    frag->type = CANVAS_SHADER_IMAGE;
}

CanvasDrawCmd* ACanvas::AllocDrawCommand()
{
    if (m_DrawData.NumDrawCommands + 1 > m_DrawData.MaxDrawCommands)
    {
        int MaxDrawCommands = std::max(m_DrawData.NumDrawCommands + 1, 128) + m_DrawData.MaxDrawCommands / 2; // 1.5x Overallocate
        m_DrawData.DrawCommands = (CanvasDrawCmd*)Platform::GetHeapAllocator<HEAP_MISC>().Realloc(m_DrawData.DrawCommands, sizeof(CanvasDrawCmd) * MaxDrawCommands);
        m_DrawData.MaxDrawCommands = MaxDrawCommands;
    }
    CanvasDrawCmd* cmd = &m_DrawData.DrawCommands[m_DrawData.NumDrawCommands++];
    memset(cmd, 0, sizeof(CanvasDrawCmd));
    return cmd;
}

int ACanvas::AllocPaths(int n)
{
    if (m_DrawData.NumPaths + n > m_DrawData.MaxPaths)
    {
        int MaxPaths = std::max(m_DrawData.NumPaths + n, 128) + m_DrawData.MaxPaths / 2; // 1.5x Overallocate
        m_DrawData.Paths = (CanvasPath*)Platform::GetHeapAllocator<HEAP_MISC>().Realloc(m_DrawData.Paths, sizeof(CanvasPath) * MaxPaths);
        m_DrawData.MaxPaths = MaxPaths;
    }
    int ret = m_DrawData.NumPaths;
    m_DrawData.NumPaths += n;
    return ret;
}

int ACanvas::AllocVerts(int n)
{
    if (m_DrawData.VertexCount + n > m_DrawData.MaxVerts)
    {
        int MaxVerts = std::max(m_DrawData.VertexCount + n, 4096) + m_DrawData.MaxVerts / 2; // 1.5x Overallocate
        m_DrawData.Vertices = (CanvasVertex*)Platform::GetHeapAllocator<HEAP_MISC>().Realloc(m_DrawData.Vertices, sizeof(CanvasVertex) * MaxVerts);
        m_DrawData.MaxVerts = MaxVerts;
    }
    int ret = m_DrawData.VertexCount;
    m_DrawData.VertexCount += n;
    return ret;
}

int ACanvas::AllocUniforms(int n)
{
    int ret = 0, structSize = sizeof(CanvasUniforms);
    if (m_DrawData.UniformCount + n > m_DrawData.MaxUniforms)
    {
        int MaxUniforms = std::max(m_DrawData.UniformCount + n, 128) + m_DrawData.MaxUniforms / 2; // 1.5x Overallocate
        m_DrawData.Uniforms = (unsigned char*)Platform::GetHeapAllocator<HEAP_MISC>().Realloc(m_DrawData.Uniforms, structSize * MaxUniforms);
        m_DrawData.MaxUniforms = MaxUniforms;
    }
    ret = m_DrawData.UniformCount * structSize;
    m_DrawData.UniformCount += n;
    return ret;
}

CanvasUniforms* ACanvas::GetUniformPtr(int i)
{
    return (CanvasUniforms*)&m_DrawData.Uniforms[i];
}

void ACanvas::ClearDrawData()
{
    m_DrawData.VertexCount = 0;
    m_DrawData.NumPaths = 0;
    m_DrawData.NumDrawCommands = 0;
    m_DrawData.UniformCount = 0;
}

CANVAS_COMPOSITE ACanvas::CompositeOperation(CANVAS_COMPOSITE op)
{
    return CANVAS_COMPOSITE(nvgGlobalCompositeOperation(m_Context, op));
}

bool ACanvas::ShapeAntiAlias(bool bEnabled)
{
    return !!nvgShapeAntiAlias(m_Context, bEnabled);
}

void ACanvas::StrokeColor(Color4 const& color)
{
    nvgStrokeColor(m_Context, *reinterpret_cast<NVGcolor const*>(&color));
}

void ACanvas::StrokePaint(CanvasPaint const& paint)
{
    nvgStrokePaint(m_Context, *reinterpret_cast<NVGpaint const*>(&paint));
}

void ACanvas::FillColor(Color4 const& color)
{
    nvgFillColor(m_Context, *reinterpret_cast<NVGcolor const*>(&color));
}

void ACanvas::FillPaint(CanvasPaint const& paint)
{
    nvgFillPaint(m_Context, *reinterpret_cast<NVGpaint const*>(&paint));
}

void ACanvas::MiterLimit(float limit)
{
    nvgMiterLimit(m_Context, limit);
}

void ACanvas::StrokeWidth(float size)
{
    nvgStrokeWidth(m_Context, size);
}

void ACanvas::LineCap(CANVAS_LINE_CAP cap)
{
    constexpr int lut[] = {NVG_BUTT, NVG_ROUND, NVG_SQUARE};
    nvgLineCap(m_Context, lut[cap]);
}

void ACanvas::LineJoin(CANVAS_LINE_JOIN join)
{
    constexpr int lut[] = {NVG_MITER, NVG_ROUND, NVG_BEVEL};
    nvgLineJoin(m_Context, lut[join]);
}

void ACanvas::GlobalAlpha(float alpha)
{
    nvgGlobalAlpha(m_Context, alpha);
}

void ACanvas::ResetTransform()
{
    nvgResetTransform(m_Context);
}

void ACanvas::Transform(CanvasTransform const& transform)
{
    nvgTransform(m_Context, transform.Matrix[0], transform.Matrix[1], transform.Matrix[2], transform.Matrix[3], transform.Matrix[4], transform.Matrix[5]);
}

void ACanvas::Translate(float x, float y)
{
    nvgTranslate(m_Context, x, y);
}

void ACanvas::Rotate(float angle)
{
    nvgRotate(m_Context, angle);
}

void ACanvas::SkewX(float angle)
{
    nvgSkewX(m_Context, angle);
}

void ACanvas::SkewY(float angle)
{
    nvgSkewY(m_Context, angle);
}

void ACanvas::Scale(float x, float y)
{
    nvgScale(m_Context, x, y);
}

CanvasTransform ACanvas::CurrentTransform()
{
    CanvasTransform transform;
    nvgCurrentTransform(m_Context, transform.Matrix);
    return transform;
}

void ACanvas::Scissor(Float2 const& mins, Float2 const& maxs)
{
    nvgScissor(m_Context, mins.X, mins.Y, maxs.X - mins.X, maxs.Y - mins.Y);
}

void ACanvas::IntersectScissor(Float2 const& mins, Float2 const& maxs)
{
    nvgIntersectScissor(m_Context, mins.X, mins.Y, maxs.X - mins.X, maxs.Y - mins.Y);
}

void ACanvas::ResetScissor()
{
    nvgResetScissor(m_Context);
}

void ACanvas::BeginPath()
{
    nvgBeginPath(m_Context);
}

void ACanvas::MoveTo(float x, float y)
{
    nvgMoveTo(m_Context, x, y);
}

void ACanvas::MoveTo(Float2 const& p)
{
    nvgMoveTo(m_Context, p.X, p.Y);
}

void ACanvas::LineTo(float x, float y)
{
    nvgLineTo(m_Context, x, y);
}

void ACanvas::LineTo(Float2 const& p)
{
    nvgLineTo(m_Context, p.X, p.Y);
}

void ACanvas::BezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y)
{
    nvgBezierTo(m_Context, c1x, c1y, c2x, c2y, x, y);
}

void ACanvas::QuadTo(float cx, float cy, float x, float y)
{
    nvgQuadTo(m_Context, cx, cy, x, y);
}

void ACanvas::ArcTo(float x1, float y1, float x2, float y2, float radius)
{
    nvgArcTo(m_Context, x1, y1, x2, y2, radius);
}

void ACanvas::ClosePath()
{
    nvgClosePath(m_Context);
}

void ACanvas::PathWinding(CANVAS_PATH_WINDING winding)
{
    nvgPathWinding(m_Context, winding);
}

void ACanvas::Arc(float cx, float cy, float r, float a0, float a1, CANVAS_PATH_WINDING dir)
{
    nvgArc(m_Context, cx, cy, r, a0, a1, dir);
}

void ACanvas::Rect(float x, float y, float w, float h)
{
    nvgRect(m_Context, x, y, w, h);
}

void ACanvas::RoundedRect(float x, float y, float w, float h, float r)
{
    nvgRoundedRect(m_Context, x, y, w, h, r);
}

void ACanvas::RoundedRectVarying(float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft)
{
    nvgRoundedRectVarying(m_Context, x, y, w, h, radTopLeft, radTopRight, radBottomRight, radBottomLeft);
}

void ACanvas::Ellipse(Float2 const& center, float rx, float ry)
{
    nvgEllipse(m_Context, center.X, center.Y, rx, ry);
}

void ACanvas::Circle(Float2 const& center, float r)
{
    nvgCircle(m_Context, center.X, center.Y, r);
}

void ACanvas::Fill()
{
    nvgFill(m_Context);
}

void ACanvas::Stroke()
{
    nvgStroke(m_Context);
}


#if 0

void ACanvas::TextAlign(CANVAS_TEXT_ALIGN align)
{
    if ((align & 7) == 0)
        align |= CANVAS_TEXT_ALIGN_LEFT; // Default, align text horizontally to left.

    if ((align & 0x78) == 0)
        align |= CANVAS_TEXT_ALIGN_BASELINE; // Default, align text vertically to baseline.

    nvgTextAlign(m_Context, align);
}
#endif

void ACanvas::FontFace(AFont* font)
{
    if (font)
    {
        nvgFontFaceId(m_Context, font->GetId(), font);
    }
    else
    {
        // Set default font
        font = GetDefaultFont();
        nvgFontFaceId(m_Context, font->GetId(), font);
    }
}

void ACanvas::FontFace(AStringView font)
{
    FontFace(FindResource<AFont>(font));
}

float ACanvas::Text(FontStyle const& fontStyle, float x, float y, HALIGNMENT HAlignment, AStringView string)
{
    nvgFontSize(m_Context, fontStyle.FontSize);
    nvgFontBlur(m_Context, fontStyle.FontBlur);
    nvgTextLetterSpacing(m_Context, fontStyle.LetterSpacing);
    nvgTextLineHeight(m_Context, fontStyle.LineHeight);

    int align = CANVAS_TEXT_ALIGN_TOP;
    switch (HAlignment)
    {
        case HALIGNMENT_LEFT:
            align |= CANVAS_TEXT_ALIGN_LEFT;
            break;
        case HALIGNMENT_CENTER:
            align |= CANVAS_TEXT_ALIGN_CENTER;
            break;
        case HALIGNMENT_RIGHT:
            align |= CANVAS_TEXT_ALIGN_RIGHT;
            break;
    }

    nvgTextAlign(m_Context, align);

    return nvgText(m_Context, x, y, string.Begin(), string.End());
}

void ACanvas::TextBox(FontStyle const& fontStyle, float x, float y, float breakRowWidth, HALIGNMENT HAlignment, AStringView text)
{
    AFont* font = (AFont*)nvgGetFontFace(m_Context);
    if (!font)
        return;

    static TextRow rows[128];
    int        nrows    = 0;

    TextMetrics metrics;
    font->GetTextMetrics(fontStyle, metrics);
    float lineh = metrics.LineHeight * fontStyle.LineHeight;

    while ((nrows = font->TextBreakLines(fontStyle, text, breakRowWidth, rows, 2)) > 0)
    {
        for (int i = 0; i < nrows; i++)
        {
            TextRow* row = &rows[i];

            float cx = x;

            switch (HAlignment)
            {
                case HALIGNMENT_LEFT:
                    break;
                case HALIGNMENT_CENTER:
                    cx += breakRowWidth * 0.5f - row->Width * 0.5f;
                    break;
                case HALIGNMENT_RIGHT:
                    cx += breakRowWidth - row->Width;
                    break;
            }

            Text(fontStyle, cx, y, HALIGNMENT_LEFT, row->GetStringView());

            y += lineh;
        }
        text = AStringView(rows[nrows - 1].Next, text.End());
    }
}

void ACanvas::TextBox(FontStyle const& fontStyle, Float2 const& mins, Float2 const& maxs, HALIGNMENT HAlignment, VALIGNMENT VAlignment, bool bWrap, AStringView text)
{
    AFont* font = (AFont*)nvgGetFontFace(m_Context);
    if (!font)
        return;

    TextMetrics metrics;
    font->GetTextMetrics(fontStyle, metrics);

    float lineHeight    = metrics.LineHeight * fontStyle.LineHeight;
    float x             = mins.X;
    float y             = mins.Y;
    float boxWidth      = maxs.X - mins.X;
    float boxHeight     = maxs.Y - mins.Y;
    float breakRowWidth = bWrap ? boxWidth : Math::MaxValue<float>();

    float yOffset = 0;

    static TextRow rows[128];
    int            nrows;

    if (VAlignment == VALIGNMENT_CENTER || VAlignment == VALIGNMENT_BOTTOM)
    {
        AStringView str = text;

        while ((nrows = font->TextBreakLines(fontStyle, str, breakRowWidth, rows, HK_ARRAY_SIZE(rows))) > 0)
        {
            yOffset += nrows * lineHeight;

            str = AStringView(rows[nrows - 1].Next, str.End());
        }

        yOffset = boxHeight - yOffset;

        if (VAlignment == VALIGNMENT_CENTER)
        {
            yOffset *= 0.5f;
        }
    }

    AStringView str = text;

    y += yOffset;

    while ((nrows = font->TextBreakLines(fontStyle, str, breakRowWidth, rows, HK_ARRAY_SIZE(rows))) > 0)
    {
        for (int i = 0; i < nrows; i++)
        {
            TextRow* row = &rows[i];

            float cx = x;

            switch (HAlignment)
            {
                case HALIGNMENT_LEFT:
                    break;
                case HALIGNMENT_CENTER:
                    cx += boxWidth * 0.5f - row->Width * 0.5f;
                    break;
                case HALIGNMENT_RIGHT:
                    cx += boxWidth - row->Width;
                    break;
            }

            if (y >= maxs.Y)
                return;

            if (y + lineHeight >= mins.Y)
            {
                Text(fontStyle, cx, y, HALIGNMENT_LEFT, row->GetStringView());
            }

            y += lineHeight;
        }

        str = AStringView(rows[nrows - 1].Next, str.End());
    }
}

// Cursor map from Dear ImGui:
// A work of art lies ahead! (. = white layer, X = black layer, others are blank)
// The white texels on the top left are the ones we'll use everywhere to render filled shapes.
const int CURSOR_MAP_HALF_WIDTH = 108;
const int CURSOR_MAP_HEIGHT = 27;
static const char CursorMap[CURSOR_MAP_HALF_WIDTH * CURSOR_MAP_HEIGHT + 1] =
    {
        "            -XXXXXXX-    X    -           X           -XXXXXXX          -          XXXXXXX-     XX          "
        "            -X.....X-   X.X   -          X.X          -X.....X          -          X.....X-    X..X         "
        "            -XXX.XXX-  X...X  -         X...X         -X....X           -           X....X-    X..X         "
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
        // Pos ........  Size .........  Offset ......
        {Float2(0, 3),   Float2(12, 19), Float2(0, 0)},   // DRAW_CURSOR_ARROW
        {Float2(13, 0),  Float2(7, 16),  Float2(1, 8)},   // DRAW_CURSOR_TEXT_INPUT
        {Float2(31, 0),  Float2(23, 23), Float2(11, 11)}, // DRAW_CURSOR_RESIZE_ALL
        {Float2(21, 0),  Float2(9, 23),  Float2(4, 11)},  // DRAW_CURSOR_RESIZE_NS
        {Float2(55, 18), Float2(23, 9),  Float2(11, 4)},  // DRAW_CURSOR_RESIZE_EW
        {Float2(73, 0),  Float2(17, 17), Float2(8, 8)},   // DRAW_CURSOR_RESIZE_NESW
        {Float2(55, 0),  Float2(17, 17), Float2(8, 8)},   // DRAW_CURSOR_RESIZE_NWSE
        {Float2(91, 0),  Float2(17, 22), Float2(5, 0)},   // DRAW_CURSOR_RESIZE_HAND
};

static ATexture* CreateCursorMap()
{
    int w = CURSOR_MAP_HALF_WIDTH * 2 + 1;
    int h = CURSOR_MAP_HEIGHT;

    ARawImage image(w, h, RAW_IMAGE_FORMAT_R8, Float4(0.0f));

    uint8_t* data = (uint8_t*)image.GetData();

    for (int y = 0, n = 0; y < CURSOR_MAP_HEIGHT; y++)
    {
        for (int x = 0; x < CURSOR_MAP_HALF_WIDTH; x++, n++)
        {
            const int offset0 = y * w + x;
            const int offset1 = offset0 + CURSOR_MAP_HALF_WIDTH + 1;
            data[offset0] = CursorMap[n] == '.' ? 0xFF : 0x00;
            data[offset1] = CursorMap[n] == 'X' ? 0xFF : 0x00;
        }
    }

    return ATexture::CreateFromImage(CreateImage(image, nullptr));
}

static void GetMouseCursorData(DRAW_CURSOR cursor, Float2& offset, Float2& size, Float2& uvfill, Float2& uvborder)
{
    HK_ASSERT(cursor >= DRAW_CURSOR_ARROW && cursor <= DRAW_CURSOR_RESIZE_HAND);

    Float2 pos = CursorTexData[cursor][0];
    size = CursorTexData[cursor][1];
    offset = CursorTexData[cursor][2];
    uvfill = pos;
    pos.X += CURSOR_MAP_HALF_WIDTH + 1;
    uvborder = pos;
}

void ACanvas::DrawCursor(DRAW_CURSOR cursor, Float2 const& position, Color4 const& fillColor, Color4 const& borderColor, bool bShadow)
{
    Float2 offset, size, uvfill, uvborder;

    GetMouseCursorData(cursor, offset, size, uvfill, uvborder);

    Float2 p = position.Floor() - offset;

    if (!m_Cursors)
        m_Cursors = CreateCursorMap();

    DrawTextureDesc desc;
    desc.pTexture = m_Cursors;
    desc.W = size.X;
    desc.H = size.Y;
    desc.UVScale.X = 1.0f / desc.W * m_Cursors->GetDimensionX();
    desc.UVScale.Y = 1.0f / desc.H * m_Cursors->GetDimensionY();

    desc.Y = p.Y;

    if (bShadow)
    {
        const Color4 shadowColor = Color4(0, 0, 0, 0.3f);

        desc.TintColor = shadowColor;
        desc.UVOffset = -uvborder;

        desc.X = p.X + 1;
        DrawTexture(desc);

        desc.X = p.X + 2;
        DrawTexture(desc);
    }

    desc.X = p.X;

    desc.TintColor = borderColor;
    desc.UVOffset = -uvborder;
    DrawTexture(desc);

    desc.TintColor = fillColor;
    desc.UVOffset = -uvfill;
    DrawTexture(desc);
}
