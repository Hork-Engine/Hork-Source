/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

/*

NOTE. Canvas is based on the nanovg API originally written by Mikko Mononen.
See ThridParty.md for details.

*/

#include "Canvas.h"

#include <Engine/Core/Logger.h>
#include <Engine/Core/ConsoleVar.h>

#include <Engine/GameApplication/GameApplication.h>

#include <nanovg/fontstash.h>

HK_NAMESPACE_BEGIN

static ConsoleVar vg_EdgeAntialias("vg_EdgeAntialias"_s, "1"_s);
static ConsoleVar vg_StencilStrokes("vg_StencilStrokes"_s, "1"_s);

constexpr float NVG_KAPPA90 = 0.5522847493f; // Length proportional to radius of a cubic bezier handle for 90deg arcs.

#define NVG_INIT_STATES        32
#define NVG_INIT_COMMANDS_SIZE 256
#define NVG_INIT_POINTS_SIZE   128
#define NVG_INIT_PATHS_SIZE    16
#define NVG_INIT_VERTS_SIZE    256

enum VGCommand
{
    VG_MOVETO   = 0,
    VG_LINETO   = 1,
    VG_BEZIERTO = 2,
    VG_CLOSE    = 3,
    VG_WINDING  = 4,
};

enum VGPointFlags
{
    VG_PT_CORNER     = 0x01,
    VG_PT_LEFT       = 0x02,
    VG_PT_BEVEL      = 0x04,
    VG_PR_INNERBEVEL = 0x08,
};

static int   nvg__mini(int a, int b) { return a < b ? a : b; }
static int   nvg__maxi(int a, int b) { return a > b ? a : b; }
static int   nvg__clampi(int a, int mn, int mx) { return a < mn ? mn : (a > mx ? mx : a); }
static float nvg__minf(float a, float b) { return a < b ? a : b; }
static float nvg__maxf(float a, float b) { return a > b ? a : b; }
static float nvg__signf(float a) { return a >= 0.0f ? 1.0f : -1.0f; }
static float nvg__clampf(float a, float mn, float mx) { return a < mn ? mn : (a > mx ? mx : a); }
static float nvg__cross(float dx0, float dy0, float dx1, float dy1) { return dx1 * dy0 - dx0 * dy1; }

static float nvg__normalize(float* x, float* y)
{
    float d = std::sqrt((*x) * (*x) + (*y) * (*y));
    if (d > 1e-6f)
    {
        float id = 1.0f / d;
        *x *= id;
        *y *= id;
    }
    return d;
}

static int nvg__ptEquals(float x1, float y1, float x2, float y2, float tol)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy < tol * tol;
}

static void nvg__isectRects(float* dst,
                            float  ax,
                            float  ay,
                            float  aw,
                            float  ah,
                            float  bx,
                            float  by,
                            float  bw,
                            float  bh)
{
    float minx = nvg__maxf(ax, bx);
    float miny = nvg__maxf(ay, by);
    float maxx = nvg__minf(ax + aw, bx + bw);
    float maxy = nvg__minf(ay + ah, by + bh);
    dst[0]     = minx;
    dst[1]     = miny;
    dst[2]     = nvg__maxf(0.0f, maxx - minx);
    dst[3]     = nvg__maxf(0.0f, maxy - miny);
}

static float nvg__distPtSeg(float x, float y, float px, float py, float qx, float qy)
{
    float pqx, pqy, dx, dy, d, t;
    pqx = qx - px;
    pqy = qy - py;
    dx  = x - px;
    dy  = y - py;
    d   = pqx * pqx + pqy * pqy;
    t   = pqx * dx + pqy * dy;
    if (d > 0) t /= d;
    if (t < 0) t = 0;
    else if (t > 1)
        t = 1;
    dx = px + t * pqx - x;
    dy = py + t * pqy - y;
    return dx * dx + dy * dy;
}

static float nvg__triarea2(float ax, float ay, float bx, float by, float cx, float cy)
{
    float abx = bx - ax;
    float aby = by - ay;
    float acx = cx - ax;
    float acy = cy - ay;
    return acx * aby - abx * acy;
}

static float nvg__polyArea(VGPoint* pts, int npts)
{
    int   i;
    float area = 0;
    for (i = 2; i < npts; i++)
    {
        VGPoint* a = &pts[0];
        VGPoint* b = &pts[i - 1];
        VGPoint* c = &pts[i];
        area += nvg__triarea2(a->x, a->y, b->x, b->y, c->x, c->y);
    }
    return area * 0.5f;
}

static void nvg__polyReverse(VGPoint* pts, int npts)
{
    VGPoint tmp;
    int     i = 0, j = npts - 1;
    while (i < j)
    {
        tmp    = pts[i];
        pts[i] = pts[j];
        pts[j] = tmp;
        i++;
        j--;
    }
}

static float nvg__getAverageScale(Transform2D const& t)
{
    float sx = std::sqrt(t[0][0] * t[0][0] + t[1][0] * t[1][0]);
    float sy = std::sqrt(t[0][1] * t[0][1] + t[1][1] * t[1][1]);
    return (sx + sy) * 0.5f;
}

static int nvg__curveDivs(float r, float arc, float tol)
{
    float da = std::acos(r / (r + tol)) * 2.0f;
    return nvg__maxi(2, (int)std::ceil(arc / da));
}

static void nvg__chooseBevel(int bevel, VGPoint* p0, VGPoint* p1, float w, float* x0, float* y0, float* x1, float* y1)
{
    if (bevel)
    {
        *x0 = p1->x + p0->dy * w;
        *y0 = p1->y - p0->dx * w;
        *x1 = p1->x + p1->dy * w;
        *y1 = p1->y - p1->dx * w;
    }
    else
    {
        *x0 = p1->x + p1->dmx * w;
        *y0 = p1->y + p1->dmy * w;
        *x1 = p1->x + p1->dmx * w;
        *y1 = p1->y + p1->dmy * w;
    }
}

static HK_FORCEINLINE void nvg__vset(CanvasVertex* vtx, float x, float y, float u, float v)
{
    vtx->x = x;
    vtx->y = y;
    vtx->u = u;
    vtx->v = v;
}

static CanvasVertex* nvg__roundJoin(CanvasVertex* dst, VGPoint* p0, VGPoint* p1, float lw, float rw, float lu, float ru, int ncap, float fringe)
{
    int   i, n;
    float dlx0 = p0->dy;
    float dly0 = -p0->dx;
    float dlx1 = p1->dy;
    float dly1 = -p1->dx;
    HK_UNUSED(fringe);

    if (p1->flags & VG_PT_LEFT)
    {
        float lx0, ly0, lx1, ly1, a0, a1;
        nvg__chooseBevel(p1->flags & VG_PR_INNERBEVEL, p0, p1, lw, &lx0, &ly0, &lx1, &ly1);
        a0 = std::atan2(-dly0, -dlx0);
        a1 = std::atan2(-dly1, -dlx1);
        if (a1 > a0) a1 -= Math::_PI * 2;

        nvg__vset(dst, lx0, ly0, lu, 1);
        dst++;
        nvg__vset(dst, p1->x - dlx0 * rw, p1->y - dly0 * rw, ru, 1);
        dst++;

        n = nvg__clampi((int)std::ceil(((a0 - a1) / Math::_PI) * ncap), 2, ncap);
        for (i = 0; i < n; i++)
        {
            float u  = i / (float)(n - 1);
            float a  = a0 + u * (a1 - a0);
            float rx = p1->x + std::cos(a) * rw;
            float ry = p1->y + std::sin(a) * rw;
            nvg__vset(dst, p1->x, p1->y, 0.5f, 1);
            dst++;
            nvg__vset(dst, rx, ry, ru, 1);
            dst++;
        }

        nvg__vset(dst, lx1, ly1, lu, 1);
        dst++;
        nvg__vset(dst, p1->x - dlx1 * rw, p1->y - dly1 * rw, ru, 1);
        dst++;
    }
    else
    {
        float rx0, ry0, rx1, ry1, a0, a1;
        nvg__chooseBevel(p1->flags & VG_PR_INNERBEVEL, p0, p1, -rw, &rx0, &ry0, &rx1, &ry1);
        a0 = std::atan2(dly0, dlx0);
        a1 = std::atan2(dly1, dlx1);
        if (a1 < a0) a1 += Math::_PI * 2;

        nvg__vset(dst, p1->x + dlx0 * rw, p1->y + dly0 * rw, lu, 1);
        dst++;
        nvg__vset(dst, rx0, ry0, ru, 1);
        dst++;

        n = nvg__clampi((int)std::ceil(((a1 - a0) / Math::_PI) * ncap), 2, ncap);
        for (i = 0; i < n; i++)
        {
            float u  = i / (float)(n - 1);
            float a  = a0 + u * (a1 - a0);
            float lx = p1->x + std::cos(a) * lw;
            float ly = p1->y + std::sin(a) * lw;
            nvg__vset(dst, lx, ly, lu, 1);
            dst++;
            nvg__vset(dst, p1->x, p1->y, 0.5f, 1);
            dst++;
        }

        nvg__vset(dst, p1->x + dlx1 * rw, p1->y + dly1 * rw, lu, 1);
        dst++;
        nvg__vset(dst, rx1, ry1, ru, 1);
        dst++;
    }
    return dst;
}

static CanvasVertex* nvg__bevelJoin(CanvasVertex* dst, VGPoint* p0, VGPoint* p1, float lw, float rw, float lu, float ru, float fringe)
{
    float rx0, ry0, rx1, ry1;
    float lx0, ly0, lx1, ly1;
    float dlx0 = p0->dy;
    float dly0 = -p0->dx;
    float dlx1 = p1->dy;
    float dly1 = -p1->dx;
    HK_UNUSED(fringe);

    if (p1->flags & VG_PT_LEFT)
    {
        nvg__chooseBevel(p1->flags & VG_PR_INNERBEVEL, p0, p1, lw, &lx0, &ly0, &lx1, &ly1);

        nvg__vset(dst, lx0, ly0, lu, 1);
        dst++;
        nvg__vset(dst, p1->x - dlx0 * rw, p1->y - dly0 * rw, ru, 1);
        dst++;

        if (p1->flags & VG_PT_BEVEL)
        {
            nvg__vset(dst, lx0, ly0, lu, 1);
            dst++;
            nvg__vset(dst, p1->x - dlx0 * rw, p1->y - dly0 * rw, ru, 1);
            dst++;

            nvg__vset(dst, lx1, ly1, lu, 1);
            dst++;
            nvg__vset(dst, p1->x - dlx1 * rw, p1->y - dly1 * rw, ru, 1);
            dst++;
        }
        else
        {
            rx0 = p1->x - p1->dmx * rw;
            ry0 = p1->y - p1->dmy * rw;

            nvg__vset(dst, p1->x, p1->y, 0.5f, 1);
            dst++;
            nvg__vset(dst, p1->x - dlx0 * rw, p1->y - dly0 * rw, ru, 1);
            dst++;

            nvg__vset(dst, rx0, ry0, ru, 1);
            dst++;
            nvg__vset(dst, rx0, ry0, ru, 1);
            dst++;

            nvg__vset(dst, p1->x, p1->y, 0.5f, 1);
            dst++;
            nvg__vset(dst, p1->x - dlx1 * rw, p1->y - dly1 * rw, ru, 1);
            dst++;
        }

        nvg__vset(dst, lx1, ly1, lu, 1);
        dst++;
        nvg__vset(dst, p1->x - dlx1 * rw, p1->y - dly1 * rw, ru, 1);
        dst++;
    }
    else
    {
        nvg__chooseBevel(p1->flags & VG_PR_INNERBEVEL, p0, p1, -rw, &rx0, &ry0, &rx1, &ry1);

        nvg__vset(dst, p1->x + dlx0 * lw, p1->y + dly0 * lw, lu, 1);
        dst++;
        nvg__vset(dst, rx0, ry0, ru, 1);
        dst++;

        if (p1->flags & VG_PT_BEVEL)
        {
            nvg__vset(dst, p1->x + dlx0 * lw, p1->y + dly0 * lw, lu, 1);
            dst++;
            nvg__vset(dst, rx0, ry0, ru, 1);
            dst++;

            nvg__vset(dst, p1->x + dlx1 * lw, p1->y + dly1 * lw, lu, 1);
            dst++;
            nvg__vset(dst, rx1, ry1, ru, 1);
            dst++;
        }
        else
        {
            lx0 = p1->x + p1->dmx * lw;
            ly0 = p1->y + p1->dmy * lw;

            nvg__vset(dst, p1->x + dlx0 * lw, p1->y + dly0 * lw, lu, 1);
            dst++;
            nvg__vset(dst, p1->x, p1->y, 0.5f, 1);
            dst++;

            nvg__vset(dst, lx0, ly0, lu, 1);
            dst++;
            nvg__vset(dst, lx0, ly0, lu, 1);
            dst++;

            nvg__vset(dst, p1->x + dlx1 * lw, p1->y + dly1 * lw, lu, 1);
            dst++;
            nvg__vset(dst, p1->x, p1->y, 0.5f, 1);
            dst++;
        }

        nvg__vset(dst, p1->x + dlx1 * lw, p1->y + dly1 * lw, lu, 1);
        dst++;
        nvg__vset(dst, rx1, ry1, ru, 1);
        dst++;
    }

    return dst;
}

static CanvasVertex* nvg__buttCapStart(CanvasVertex* dst, VGPoint* p, float dx, float dy, float w, float d, float aa, float u0, float u1)
{
    float px  = p->x - dx * d;
    float py  = p->y - dy * d;
    float dlx = dy;
    float dly = -dx;
    nvg__vset(dst, px + dlx * w - dx * aa, py + dly * w - dy * aa, u0, 0);
    dst++;
    nvg__vset(dst, px - dlx * w - dx * aa, py - dly * w - dy * aa, u1, 0);
    dst++;
    nvg__vset(dst, px + dlx * w, py + dly * w, u0, 1);
    dst++;
    nvg__vset(dst, px - dlx * w, py - dly * w, u1, 1);
    dst++;
    return dst;
}

static CanvasVertex* nvg__buttCapEnd(CanvasVertex* dst, VGPoint* p, float dx, float dy, float w, float d, float aa, float u0, float u1)
{
    float px  = p->x + dx * d;
    float py  = p->y + dy * d;
    float dlx = dy;
    float dly = -dx;
    nvg__vset(dst, px + dlx * w, py + dly * w, u0, 1);
    dst++;
    nvg__vset(dst, px - dlx * w, py - dly * w, u1, 1);
    dst++;
    nvg__vset(dst, px + dlx * w + dx * aa, py + dly * w + dy * aa, u0, 0);
    dst++;
    nvg__vset(dst, px - dlx * w + dx * aa, py - dly * w + dy * aa, u1, 0);
    dst++;
    return dst;
}

static CanvasVertex* nvg__roundCapStart(CanvasVertex* dst, VGPoint* p, float dx, float dy, float w, int ncap, float aa, float u0, float u1)
{
    int   i;
    float px  = p->x;
    float py  = p->y;
    float dlx = dy;
    float dly = -dx;
    HK_UNUSED(aa);
    for (i = 0; i < ncap; i++)
    {
        float a  = i / (float)(ncap - 1) * Math::_PI;
        float ax = std::cos(a) * w, ay = std::sin(a) * w;
        nvg__vset(dst, px - dlx * ax - dx * ay, py - dly * ax - dy * ay, u0, 1);
        dst++;
        nvg__vset(dst, px, py, 0.5f, 1);
        dst++;
    }
    nvg__vset(dst, px + dlx * w, py + dly * w, u0, 1);
    dst++;
    nvg__vset(dst, px - dlx * w, py - dly * w, u1, 1);
    dst++;
    return dst;
}

static CanvasVertex* nvg__roundCapEnd(CanvasVertex* dst, VGPoint* p, float dx, float dy, float w, int ncap, float aa, float u0, float u1)
{
    int   i;
    float px  = p->x;
    float py  = p->y;
    float dlx = dy;
    float dly = -dx;
    HK_UNUSED(aa);
    nvg__vset(dst, px + dlx * w, py + dly * w, u0, 1);
    dst++;
    nvg__vset(dst, px - dlx * w, py - dly * w, u1, 1);
    dst++;
    for (i = 0; i < ncap; i++)
    {
        float a  = i / (float)(ncap - 1) * Math::_PI;
        float ax = std::cos(a) * w, ay = std::sin(a) * w;
        nvg__vset(dst, px, py, 0.5f, 1);
        dst++;
        nvg__vset(dst, px - dlx * ax + dx * ay, py - dly * ax + dy * ay, u0, 1);
        dst++;
    }
    return dst;
}

static int GetVertexCount(Vector<VGPath> const& paths)
{
    int count = 0;
    for (VGPath const& path : paths)
    {
        count += path.NumFill;
        count += path.NumStroke;
    }
    return count;
}

VGPathCache::VGPathCache()
{
    Core::ZeroMem(Bounds, sizeof(Bounds));

    Points.Reserve(NVG_INIT_POINTS_SIZE);
    Paths.Reserve(NVG_INIT_PATHS_SIZE);
    Verts.Reserve(NVG_INIT_VERTS_SIZE);

    DistTol = 0;
}

void VGPathCache::Clear()
{
    Points.Clear();
    Paths.Clear();
    Verts.Clear();
}

VGPath* VGPathCache::AddPath()
{
    VGPath& path = Paths.Add();
    Core::ZeroMem(&path, sizeof(path));
    path.First   = Points.Size();
    path.Winding = CANVAS_PATH_WINDING_CCW;
    return &path;
}

void VGPathCache::AddPoint(float x, float y, int flags)
{
    if (Paths.IsEmpty())
        return;

    VGPath& path = Paths.Last();

    if (path.Count > 0 && !Points.IsEmpty())
    {
        VGPoint& point = Points.Last();

        if (nvg__ptEquals(point.x, point.y, x, y, DistTol))
        {
            point.flags |= flags;
            return;
        }
    }

    VGPoint& point = Points.Add();

    Core::ZeroMem(&point, sizeof(point));
    point.x     = x;
    point.y     = y;
    point.flags = (unsigned char)flags;

    path.Count++;
}

CanvasVertex* VGPathCache::AllocVerts(int nverts)
{
    if (nverts > Verts.Capacity())
    {
        // Round up to prevent allocations when things change just slightly.
        int cverts = (nverts + 0xff) & ~0xff;

        Verts.Reserve(cverts);
    }

    Verts.Resize(nverts);

    return Verts.ToPtr();
}

Canvas::Canvas() :
    m_bEdgeAntialias(true), // TODO: Set from config?
    m_bStencilStrokes(true) // TODO: Set from config?
{
    m_FontStash = GetSharedInstance<FontStash>();

    m_States.Reserve(NVG_INIT_STATES);

    m_Commands.Reserve(NVG_INIT_COMMANDS_SIZE);
}

Canvas::~Canvas()
{
    Core::GetHeapAllocator<HEAP_MISC>().Free(m_DrawData.Paths);
    Core::GetHeapAllocator<HEAP_MISC>().Free(m_DrawData.Vertices);
    Core::GetHeapAllocator<HEAP_MISC>().Free(m_DrawData.Uniforms);
    Core::GetHeapAllocator<HEAP_MISC>().Free(m_DrawData.DrawCommands);
}

void Canvas::NewFrame()
{
    ClearDrawData();

    m_bEdgeAntialias  = vg_EdgeAntialias.GetBool();
    m_bStencilStrokes = vg_StencilStrokes.GetBool();

    m_NumStates = 0;
    Push(CANVAS_PUSH_FLAG_RESET);

    m_DevicePxRatio = GameApplication::GetRetinaScale().X;

    m_TessTol     = 0.25f / m_DevicePxRatio;
    m_DistTol     = 0.01f / m_DevicePxRatio;
    m_FringeWidth = 1.0f / m_DevicePxRatio;

    m_PathCache.DistTol = m_DistTol;

    m_DrawCallCount  = 0;
    m_FillTriCount   = 0;
    m_StrokeTriCount = 0;
    m_TextTriCount   = 0;

    // Set default font
    FontFace(FontHandle{});
}

void Canvas::Push(CANVAS_PUSH_FLAG ResetFlag)
{
    bool bNeedReset = false;

    if (m_NumStates >= m_States.Size())
    {
        m_States.Add();
        bNeedReset = true;
    }

    if (m_NumStates > 0 && ResetFlag == CANVAS_PUSH_FLAG_KEEP)
    {
        Core::Memcpy(&m_States[m_NumStates], &m_States[m_NumStates - 1], sizeof(VGState));
        bNeedReset = false;
    }

    m_NumStates++;
    if (ResetFlag == CANVAS_PUSH_FLAG_RESET || bNeedReset)
        Reset();
}

void Canvas::Pop()
{
    if (m_NumStates <= 1)
        return;
    m_NumStates--;
}

Canvas::VGState* Canvas::GetState()
{
    return &m_States[m_NumStates - 1];
}

void Canvas::Reset()
{
    VGState* state = GetState();

    state->Fill.Solid(Color4(1, 1, 1, 1));
    state->Stroke.Solid(Color4(0, 0, 0, 1));
    state->CompositeOperation = CANVAS_COMPOSITE_SOURCE_OVER;
    state->bShapeAntiAlias    = true;
    state->StrokeWidth        = 1.0f;
    state->MiterLimit         = 10.0f;
    state->LineCap            = CANVAS_LINE_CAP_BUTT;
    state->LineJoin           = CANVAS_LINE_JOIN_MITER;
    state->Alpha              = 1.0f;

    state->Xform.SetIdentity();

    state->Scissor.Xform.Clear();
    state->Scissor.Extent[0] = -1.0f;
    state->Scissor.Extent[1] = -1.0f;

    state->Font = {};
}

void Canvas::DrawLine(Float2 const& p0, Float2 const& p1, Color4 const& color, float thickness)
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

void Canvas::DrawRect(Float2 const& mins, Float2 const& maxs, Color4 const& color, float thickness, RoundingDesc const& rounding)
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

void Canvas::DrawRectFilled(Float2 const& mins, Float2 const& maxs, Color4 const& color, RoundingDesc const& rounding)
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

void Canvas::DrawTriangle(Float2 const& p0, Float2 const& p1, Float2 const& p2, Color4 const& color, float thickness)
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

void Canvas::DrawTriangleFilled(Float2 const& p0, Float2 const& p1, Float2 const& p2, Color4 const& color)
{
    BeginPath();
    MoveTo(p0);
    LineTo(p1);
    LineTo(p2);
    FillColor(color);
    Fill();
}

void Canvas::DrawCircle(Float2 const& center, float radius, Color4 const& color, float thickness)
{
    if (thickness <= 0)
        return;

    BeginPath();
    Circle(center, radius);
    StrokeColor(color);
    StrokeWidth(thickness);
    Stroke();
}

void Canvas::DrawCircleFilled(Float2 const& center, float radius, Color4 const& color)
{
    BeginPath();
    Circle(center, radius);
    FillColor(color);
    Fill();
}

void Canvas::DrawText(FontStyle const& style, Float2 const& pos, Color4 const& color, StringView text, bool bShadow)
{
    if (bShadow)
    {
        FontStyle shadowStyle = style;
        shadowStyle.FontBlur  = 1;
        FillColor(Color4(0, 0, 0, color.A));
        Text(shadowStyle, pos.X + 2, pos.Y + 2, TEXT_ALIGNMENT_LEFT, text);
    }

    FillColor(color);
    Text(style, pos.X, pos.Y, TEXT_ALIGNMENT_LEFT, text);
}

void Canvas::DrawTexture(DrawTextureDesc const& desc)
{
    if (desc.W < 1.0f || desc.H < 1.0f)
        return;

    if (desc.Composite == CANVAS_COMPOSITE_SOURCE_OVER && desc.TintColor.IsTransparent())
        return;

    float clipX, clipY, clipW, clipH;
    GetIntersectedScissor(desc.X, desc.Y, desc.W, desc.H, &clipX, &clipY, &clipW, &clipH);

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
    paint.ImagePattern(Float2(desc.X + desc.UVOffset.X, desc.Y + desc.UVOffset.Y),
                       desc.W * desc.UVScale.X,
                       desc.H * desc.UVScale.Y,
                       desc.Angle,
                       desc.TexHandle,
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

void Canvas::DrawPolyline(Float2 const* points, int numPoints, Color4 const& color, bool bClosed, float thickness)
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

void Canvas::DrawPolyFilled(Float2 const* points, int numPoints, Color4 const& color)
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

void Canvas::DrawBezierCurve(Float2 const& pos0, Float2 const& cp0, Float2 const& cp1, Float2 const& pos1, Color4 const& color, float thickness)
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

void Canvas::ConvertPaint(CanvasUniforms* frag, CanvasPaint* paint, VGScissor const& scissor, float width, float fringe, float strokeThr)
{
    Transform2D invxform;

    frag->InnerColor = paint->InnerColor;
    frag->OuterColor = paint->OuterColor;

    if (scissor.Extent[0] < -0.5f || scissor.Extent[1] < -0.5f)
    {
        frag->ScissorMat.Clear();

        frag->ScissorExt[0]   = 1.0f;
        frag->ScissorExt[1]   = 1.0f;
        frag->ScissorScale[0] = 1.0f;
        frag->ScissorScale[1] = 1.0f;
    }
    else
    {
        frag->ScissorMat = scissor.Xform.Inversed().ToMatrix3x4();

        frag->ScissorExt[0]   = scissor.Extent[0];
        frag->ScissorExt[1]   = scissor.Extent[1];
        frag->ScissorScale[0] = std::sqrt(scissor.Xform[0][0] * scissor.Xform[0][0] + scissor.Xform[1][0] * scissor.Xform[1][0]) / fringe;
        frag->ScissorScale[1] = std::sqrt(scissor.Xform[0][1] * scissor.Xform[0][1] + scissor.Xform[1][1] * scissor.Xform[1][1]) / fringe;
    }

    frag->Extent[0] = paint->Extent[0];
    frag->Extent[1] = paint->Extent[1];

    frag->StrokeMult = (width * 0.5f + fringe * 0.5f) / fringe;
    frag->StrokeThr  = strokeThr;

    if (paint->TexHandle)
    {
        frag->Type    = CANVAS_SHADER_FILLIMG;
        frag->TexType = (paint->ImageFlags & CANVAS_IMAGE_PREMULTIPLIED) ? 0 : 1;
        frag->Radius  = 0;
        frag->Feather = 0;

        if ((paint->ImageFlags & CANVAS_IMAGE_FLIPY) != 0)
            invxform = (Transform2D::Translation({0.0f, -frag->Extent[1] * 0.5f}) * Transform2D::Scaling({1.0f, -1.0f}) * Transform2D::Translation({0.0f, frag->Extent[1] * 0.5f}) * paint->Xform).Inversed();
        else
            invxform = paint->Xform.Inversed();
    }
    else
    {
        frag->Type    = CANVAS_SHADER_FILLGRAD;
        frag->TexType = 0;
        frag->Radius  = paint->Radius;
        frag->Feather = paint->Feather;

        invxform = paint->Xform.Inversed();
    }

    frag->PaintMat = invxform.ToMatrix3x4();
}

RenderCore::ITexture* Canvas::GetTexture(CanvasPaint const* paint)
{
    auto* textureResource = GameApplication::GetResourceManager().TryGet(paint->TexHandle);
    if (!textureResource)
    {
        return nullptr;
    }

    return textureResource->GetTextureGPU();
}

void Canvas::RenderFill(CanvasPaint* paint, CANVAS_COMPOSITE composite, VGScissor const& scissor, float fringe, const float* bounds)
{
    int npaths = m_PathCache.Paths.Size();

    CanvasDrawCmd*  drawCommand = AllocDrawCommand();
    CanvasVertex*   quad;
    CanvasUniforms* frag;

    drawCommand->Type         = CANVAS_DRAW_COMMAND_FILL;
    drawCommand->Composite    = composite;
    drawCommand->VertexCount  = 4;
    drawCommand->FirstPath    = AllocPaths(npaths);
    drawCommand->PathCount    = npaths;
    drawCommand->Texture      = GetTexture(paint);
    drawCommand->TextureFlags = paint->ImageFlags;

    if (npaths == 1 && m_PathCache.Paths[0].bConvex)
    {
        drawCommand->Type        = CANVAS_DRAW_COMMAND_CONVEXFILL;
        drawCommand->VertexCount = 0; // Bounding box fill quad not needed for convex fill
    }

    // Allocate vertices for all the paths.
    int offset = AllocVerts(GetVertexCount(m_PathCache.Paths) + drawCommand->VertexCount);

    int pathNum = 0;
    for (VGPath const& path : m_PathCache.Paths)
    {
        CanvasPath* copy = &m_DrawData.Paths[drawCommand->FirstPath + pathNum];
        Core::ZeroMem(copy, sizeof(CanvasPath));
        if (path.NumFill > 0)
        {
            copy->FillOffset = offset;
            copy->FillCount  = path.NumFill;
            Core::Memcpy(&m_DrawData.Vertices[offset], path.Fill, sizeof(CanvasVertex) * path.NumFill);
            offset += path.NumFill;
        }
        if (path.NumStroke > 0)
        {
            copy->StrokeOffset = offset;
            copy->StrokeCount  = path.NumStroke;
            Core::Memcpy(&m_DrawData.Vertices[offset], path.Stroke, sizeof(CanvasVertex) * path.NumStroke);
            offset += path.NumStroke;
        }
        pathNum++;
    }

    if (drawCommand->Type == CANVAS_DRAW_COMMAND_FILL)
    {
        // Quad
        drawCommand->FirstVertex = offset;

        quad = &m_DrawData.Vertices[drawCommand->FirstVertex];
        nvg__vset(&quad[0], bounds[2], bounds[3], 0.5f, 1.0f);
        nvg__vset(&quad[1], bounds[2], bounds[1], 0.5f, 1.0f);
        nvg__vset(&quad[2], bounds[0], bounds[3], 0.5f, 1.0f);
        nvg__vset(&quad[3], bounds[0], bounds[1], 0.5f, 1.0f);

        drawCommand->UniformOffset = AllocUniforms(2);

        // Simple shader for stencil
        frag = GetUniformPtr(drawCommand->UniformOffset);
        Core::ZeroMem(frag, sizeof(*frag));
        frag->StrokeThr = -1.0f;
        frag->Type      = CANVAS_SHADER_SIMPLE;

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

void Canvas::RenderStroke(CanvasPaint* paint, CANVAS_COMPOSITE composite, VGScissor const& scissor, float fringe, float strokeWidth)
{
    int npaths = m_PathCache.Paths.Size();

    CanvasDrawCmd* drawCommand = AllocDrawCommand();

    drawCommand->Type         = CANVAS_DRAW_COMMAND_STROKE;
    drawCommand->Composite    = composite;
    drawCommand->FirstPath    = AllocPaths(npaths);
    drawCommand->PathCount    = npaths;
    drawCommand->Texture      = GetTexture(paint);
    drawCommand->TextureFlags = paint->ImageFlags;

    // Allocate vertices for all the paths.
    int offset = AllocVerts(GetVertexCount(m_PathCache.Paths));

    int pathNum = 0;
    for (VGPath const& path : m_PathCache.Paths)
    {
        CanvasPath* copy = &m_DrawData.Paths[drawCommand->FirstPath + pathNum];
        Core::ZeroMem(copy, sizeof(CanvasPath));
        if (path.NumStroke)
        {
            copy->StrokeOffset = offset;
            copy->StrokeCount  = path.NumStroke;
            Core::Memcpy(&m_DrawData.Vertices[offset], path.Stroke, sizeof(CanvasVertex) * path.NumStroke);
            offset += path.NumStroke;
        }
        pathNum++;
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

void Canvas::RenderTriangles(CanvasPaint* paint, CANVAS_COMPOSITE composite, VGScissor const& scissor, const CanvasVertex* verts, int nverts, float fringe)
{
    CanvasDrawCmd*  drawCommand = AllocDrawCommand();
    CanvasUniforms* frag;

    drawCommand->Type         = CANVAS_DRAW_COMMAND_TRIANGLES;
    drawCommand->Composite    = composite;
    drawCommand->Texture      = GetTexture(paint);
    drawCommand->TextureFlags = paint->ImageFlags;

    // Allocate vertices for all the paths.
    drawCommand->FirstVertex = AllocVerts(nverts);
    drawCommand->VertexCount = nverts;

    Core::Memcpy(&m_DrawData.Vertices[drawCommand->FirstVertex], verts, sizeof(CanvasVertex) * nverts);

    // Fill shader
    drawCommand->UniformOffset = AllocUniforms(1);

    frag = GetUniformPtr(drawCommand->UniformOffset);
    ConvertPaint(frag, paint, scissor, 1.0f, fringe, -1.0f);
    frag->Type = CANVAS_SHADER_IMAGE;
}

CanvasDrawCmd* Canvas::AllocDrawCommand()
{
    if (m_DrawData.NumDrawCommands + 1 > m_DrawData.MaxDrawCommands)
    {
        int MaxDrawCommands        = std::max(m_DrawData.NumDrawCommands + 1, 128) + m_DrawData.MaxDrawCommands / 2; // 1.5x Overallocate
        m_DrawData.DrawCommands    = (CanvasDrawCmd*)Core::GetHeapAllocator<HEAP_MISC>().Realloc(m_DrawData.DrawCommands, sizeof(CanvasDrawCmd) * MaxDrawCommands);
        m_DrawData.MaxDrawCommands = MaxDrawCommands;
    }
    CanvasDrawCmd* cmd = &m_DrawData.DrawCommands[m_DrawData.NumDrawCommands++];
    Core::ZeroMem(cmd, sizeof(CanvasDrawCmd));
    return cmd;
}

int Canvas::AllocPaths(int n)
{
    if (m_DrawData.NumPaths + n > m_DrawData.MaxPaths)
    {
        int MaxPaths        = std::max(m_DrawData.NumPaths + n, 128) + m_DrawData.MaxPaths / 2; // 1.5x Overallocate
        m_DrawData.Paths    = (CanvasPath*)Core::GetHeapAllocator<HEAP_MISC>().Realloc(m_DrawData.Paths, sizeof(CanvasPath) * MaxPaths);
        m_DrawData.MaxPaths = MaxPaths;
    }
    int ret = m_DrawData.NumPaths;
    m_DrawData.NumPaths += n;
    return ret;
}

int Canvas::AllocVerts(int n)
{
    if (m_DrawData.VertexCount + n > m_DrawData.MaxVerts)
    {
        int MaxVerts        = std::max(m_DrawData.VertexCount + n, 4096) + m_DrawData.MaxVerts / 2; // 1.5x Overallocate
        m_DrawData.Vertices = (CanvasVertex*)Core::GetHeapAllocator<HEAP_MISC>().Realloc(m_DrawData.Vertices, sizeof(CanvasVertex) * MaxVerts);
        m_DrawData.MaxVerts = MaxVerts;
    }
    int ret = m_DrawData.VertexCount;
    m_DrawData.VertexCount += n;
    return ret;
}

int Canvas::AllocUniforms(int n)
{
    int ret = 0, structSize = sizeof(CanvasUniforms);
    if (m_DrawData.UniformCount + n > m_DrawData.MaxUniforms)
    {
        int MaxUniforms        = std::max(m_DrawData.UniformCount + n, 128) + m_DrawData.MaxUniforms / 2; // 1.5x Overallocate
        m_DrawData.Uniforms    = (unsigned char*)Core::GetHeapAllocator<HEAP_MISC>().Realloc(m_DrawData.Uniforms, structSize * MaxUniforms);
        m_DrawData.MaxUniforms = MaxUniforms;
    }
    ret = m_DrawData.UniformCount * structSize;
    m_DrawData.UniformCount += n;
    return ret;
}

CanvasUniforms* Canvas::GetUniformPtr(int i)
{
    return (CanvasUniforms*)&m_DrawData.Uniforms[i];
}

void Canvas::ClearDrawData()
{
    m_DrawData.VertexCount     = 0;
    m_DrawData.NumPaths        = 0;
    m_DrawData.NumDrawCommands = 0;
    m_DrawData.UniformCount    = 0;
}

CANVAS_COMPOSITE Canvas::CompositeOperation(CANVAS_COMPOSITE op)
{
    VGState* state = GetState();

    CANVAS_COMPOSITE old      = state->CompositeOperation;
    state->CompositeOperation = op;
    return (CANVAS_COMPOSITE)old;
}

bool Canvas::ShapeAntiAlias(bool bEnabled)
{
    VGState* state = GetState();

    bool old               = state->bShapeAntiAlias;
    state->bShapeAntiAlias = bEnabled;

    return old;
}

void Canvas::StrokeColor(Color4 const& color)
{
    VGState* state = GetState();

    state->Stroke.Solid(color);
}

void Canvas::StrokePaint(CanvasPaint const& paint)
{
    VGState* state = GetState();

    state->Stroke = paint;
    state->Stroke.Xform *= state->Xform;
}

void Canvas::FillColor(Color4 const& color)
{
    VGState* state = GetState();

    state->Fill.Solid(color);
}

void Canvas::FillPaint(CanvasPaint const& paint)
{
    VGState* state = GetState();

    state->Fill = paint;
    state->Fill.Xform *= state->Xform;
}

void Canvas::MiterLimit(float limit)
{
    VGState* state = GetState();

    state->MiterLimit = limit;
}

void Canvas::StrokeWidth(float size)
{
    VGState* state = GetState();

    state->StrokeWidth = size;
}

void Canvas::LineCap(CANVAS_LINE_CAP cap)
{
    VGState* state = GetState();

    state->LineCap = cap;
}

void Canvas::LineJoin(CANVAS_LINE_JOIN join)
{
    VGState* state = GetState();

    state->LineJoin = join;
}

void Canvas::GlobalAlpha(float alpha)
{
    VGState* state = GetState();

    state->Alpha = alpha;
}

void Canvas::ResetTransform()
{
    VGState* state = GetState();

    state->Xform.SetIdentity();
}

void Canvas::Transform(Transform2D const& transform)
{
    VGState* state = GetState();

    state->Xform = transform * state->Xform;
}

void Canvas::Translate(float x, float y)
{
    VGState* state = GetState();

    state->Xform = Transform2D::Translation({x, y}) * state->Xform;
}

void Canvas::Rotate(float angle)
{
    VGState* state = GetState();

    state->Xform = Transform2D::Rotation(angle) * state->Xform;
}

void Canvas::SkewX(float angle)
{
    VGState* state = GetState();

    state->Xform = Transform2D::SkewX(angle) * state->Xform;
}

void Canvas::SkewY(float angle)
{
    VGState* state = GetState();

    state->Xform = Transform2D::SkewY(angle) * state->Xform;
}

void Canvas::Scale(float x, float y)
{
    VGState* state = GetState();

    state->Xform = Transform2D::Scaling({x, y}) * state->Xform;
}

Transform2D const& Canvas::CurrentTransform()
{
    VGState* state = GetState();

    return state->Xform;
}

void Canvas::Scissor(Float2 const& mins, Float2 const& maxs)
{
    VGState* state = GetState();

    float w = nvg__maxf(0.0f, maxs.X - mins.X);
    float h = nvg__maxf(0.0f, maxs.Y - mins.Y);

    state->Scissor.Xform.SetIdentity();
    state->Scissor.Xform[2][0] = mins.X + w * 0.5f;
    state->Scissor.Xform[2][1] = mins.Y + h * 0.5f;
    state->Scissor.Xform *= state->Xform;

    state->Scissor.Extent[0] = w * 0.5f;
    state->Scissor.Extent[1] = h * 0.5f;
}

void Canvas::IntersectScissor(Float2 const& mins, Float2 const& maxs)
{
    VGState* state = GetState();
    float    rect[4];
    float    ex, ey, tex, tey;

    // If no previous scissor has been set, set the scissor as current scissor.
    if (state->Scissor.Extent[0] < 0)
    {
        Scissor(mins, maxs);
        return;
    }

    // Transform the current scissor rect into current transform space.
    // If there is difference in rotation, this will be approximation.

    ex = state->Scissor.Extent[0];
    ey = state->Scissor.Extent[1];

    Transform2D pxform = state->Scissor.Xform * state->Xform.Inversed();

    tex = ex * Math::Abs(pxform[0][0]) + ey * Math::Abs(pxform[1][0]);
    tey = ex * Math::Abs(pxform[0][1]) + ey * Math::Abs(pxform[1][1]);

    // Intersect rects.
    float w = maxs.X - mins.X;
    float h = maxs.Y - mins.Y;
    nvg__isectRects(rect, pxform[2][0] - tex, pxform[2][1] - tey, tex * 2, tey * 2, mins.X, mins.Y, w, h);

    Scissor({rect[0], rect[1]}, {rect[0] + rect[2], rect[1] + rect[3]});
}

void Canvas::ResetScissor()
{
    VGState* state = GetState();

    state->Scissor.Xform.Clear();

    state->Scissor.Extent[0] = -1.0f;
    state->Scissor.Extent[1] = -1.0f;
}

void Canvas::GetIntersectedScissor(Float2 const& mins, Float2 const& maxs, Float2& resultMins, Float2& resultMaxs)
{
    GetIntersectedScissor(mins.X, mins.Y, maxs.X - mins.X, maxs.Y - mins.Y, &resultMins.X, &resultMins.Y, &resultMaxs.X, &resultMaxs.Y);
    resultMaxs += resultMins;
}

void Canvas::GetIntersectedScissor(float x, float y, float w, float h, float* px, float* py, float* pw, float* ph)
{
    VGState* state = GetState();
    float    rect[4];
    float    ex, ey, tex, tey;

    // If no previous scissor has been set, set the scissor as current scissor.
    if (state->Scissor.Extent[0] < 0)
    {
        *px = x;
        *py = y;
        *pw = w;
        *ph = h;
        return;
    }

    // Transform the current scissor rect into current transform space.
    // If there is difference in rotation, this will be approximation.

    ex = state->Scissor.Extent[0];
    ey = state->Scissor.Extent[1];

    Transform2D pxform = state->Scissor.Xform * state->Xform.Inversed();

    tex = ex * Math::Abs(pxform[0][0]) + ey * Math::Abs(pxform[1][0]);
    tey = ex * Math::Abs(pxform[0][1]) + ey * Math::Abs(pxform[1][1]);

    // Intersect rects.
    nvg__isectRects(rect, pxform[2][0] - tex, pxform[2][1] - tey, tex * 2, tey * 2, x, y, w, h);

    *px = rect[0];
    *py = rect[1];
    *pw = rect[2];
    *ph = rect[3];
}

void Canvas::BeginPath()
{
    m_Commands.Clear();
    m_PathCache.Clear();
}

void Canvas::MoveTo(float x, float y)
{
    const float vals[] = {VG_MOVETO, x, y};
    AppendCommands(vals, HK_ARRAY_SIZE(vals));
}

void Canvas::MoveTo(Float2 const& p)
{
    const float vals[] = {VG_MOVETO, p.X, p.Y};
    AppendCommands(vals, HK_ARRAY_SIZE(vals));
}

void Canvas::LineTo(float x, float y)
{
    const float vals[] = {VG_LINETO, x, y};
    AppendCommands(vals, HK_ARRAY_SIZE(vals));
}

void Canvas::LineTo(Float2 const& p)
{
    const float vals[] = {VG_LINETO, p.X, p.Y};
    AppendCommands(vals, HK_ARRAY_SIZE(vals));
}

void Canvas::BezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y)
{
    const float vals[] = {VG_BEZIERTO, c1x, c1y, c2x, c2y, x, y};
    AppendCommands(vals, HK_ARRAY_SIZE(vals));
}

void Canvas::QuadTo(float cx, float cy, float x, float y)
{
    float x0 = m_CommandPos.X;
    float y0 = m_CommandPos.Y;

    const float vals[] = {VG_BEZIERTO,
                          x0 + 2.0f / 3.0f * (cx - x0), y0 + 2.0f / 3.0f * (cy - y0),
                          x + 2.0f / 3.0f * (cx - x), y + 2.0f / 3.0f * (cy - y),
                          x, y};
    AppendCommands(vals, HK_ARRAY_SIZE(vals));
}

void Canvas::ArcTo(float x1, float y1, float x2, float y2, float radius)
{
    float x0 = m_CommandPos.X;
    float y0 = m_CommandPos.Y;

    float dx0, dy0, dx1, dy1, a, d, cx, cy, a0, a1;

    CANVAS_PATH_WINDING dir;

    if (m_Commands.IsEmpty())
    {
        return;
    }

    // Handle degenerate cases.
    if (nvg__ptEquals(x0, y0, x1, y1, m_DistTol) ||
        nvg__ptEquals(x1, y1, x2, y2, m_DistTol) ||
        nvg__distPtSeg(x1, y1, x0, y0, x2, y2) < m_DistTol * m_DistTol ||
        radius < m_DistTol)
    {
        LineTo(x1, y1);
        return;
    }

    // Calculate tangential circle to lines (x0,y0)-(x1,y1) and (x1,y1)-(x2,y2).
    dx0 = x0 - x1;
    dy0 = y0 - y1;
    dx1 = x2 - x1;
    dy1 = y2 - y1;
    nvg__normalize(&dx0, &dy0);
    nvg__normalize(&dx1, &dy1);
    a = std::acos(dx0 * dx1 + dy0 * dy1);
    d = radius / std::tan(a / 2.0f);

    if (d > 10000.0f)
    {
        LineTo(x1, y1);
        return;
    }

    if (nvg__cross(dx0, dy0, dx1, dy1) > 0.0f)
    {
        cx  = x1 + dx0 * d + dy0 * radius;
        cy  = y1 + dy0 * d + -dx0 * radius;
        a0  = std::atan2(dx0, -dy0);
        a1  = std::atan2(-dx1, dy1);
        dir = CANVAS_PATH_WINDING_CW;
    }
    else
    {
        cx  = x1 + dx0 * d + -dy0 * radius;
        cy  = y1 + dy0 * d + dx0 * radius;
        a0  = std::atan2(-dx0, dy0);
        a1  = std::atan2(dx1, -dy1);
        dir = CANVAS_PATH_WINDING_CCW;
    }

    Arc(cx, cy, radius, a0, a1, dir);
}

void Canvas::ClosePath()
{
    const float vals[] = {VG_CLOSE};
    AppendCommands(vals, HK_ARRAY_SIZE(vals));
}

void Canvas::PathWinding(CANVAS_PATH_WINDING winding)
{
    const float vals[] = {VG_WINDING, (float)winding};
    AppendCommands(vals, HK_ARRAY_SIZE(vals));
}

void Canvas::Arc(float cx, float cy, float r, float a0, float a1, CANVAS_PATH_WINDING dir)
{
    float a = 0, da = 0, hda = 0, kappa = 0;
    float dx = 0, dy = 0, x = 0, y = 0, tanx = 0, tany = 0;
    float px = 0, py = 0, ptanx = 0, ptany = 0;
    float vals[3 + 5 * 7 + 100];
    int   i, ndivs, nvals;
    int   move = !m_Commands.IsEmpty() ? VG_LINETO : VG_MOVETO;

    // Clamp angles
    da = a1 - a0;
    if (dir == CANVAS_PATH_WINDING_CW)
    {
        if (Math::Abs(da) >= Math::_PI * 2)
        {
            da = Math::_PI * 2;
        }
        else
        {
            while (da < 0.0f) da += Math::_PI * 2;
        }
    }
    else
    {
        if (Math::Abs(da) >= Math::_PI * 2)
        {
            da = -Math::_PI * 2;
        }
        else
        {
            while (da > 0.0f) da -= Math::_PI * 2;
        }
    }

    // Split arc into max 90 degree segments.
    ndivs = nvg__maxi(1, nvg__mini((int)(Math::Abs(da) / (Math::_PI * 0.5f) + 0.5f), 5));
    hda   = (da / (float)ndivs) / 2.0f;
    kappa = Math::Abs(4.0f / 3.0f * (1.0f - std::cos(hda)) / std::sin(hda));

    if (dir == CANVAS_PATH_WINDING_CCW)
        kappa = -kappa;

    nvals = 0;
    for (i = 0; i <= ndivs; i++)
    {
        a    = a0 + da * (i / (float)ndivs);
        dx   = std::cos(a);
        dy   = std::sin(a);
        x    = cx + dx * r;
        y    = cy + dy * r;
        tanx = -dy * r * kappa;
        tany = dx * r * kappa;

        if (i == 0)
        {
            vals[nvals++] = (float)move;
            vals[nvals++] = x;
            vals[nvals++] = y;
        }
        else
        {
            vals[nvals++] = VG_BEZIERTO;
            vals[nvals++] = px + ptanx;
            vals[nvals++] = py + ptany;
            vals[nvals++] = x - tanx;
            vals[nvals++] = y - tany;
            vals[nvals++] = x;
            vals[nvals++] = y;
        }
        px    = x;
        py    = y;
        ptanx = tanx;
        ptany = tany;
    }

    AppendCommands(vals, nvals);
}

void Canvas::Rect(float x, float y, float w, float h)
{
    const float vals[] = {
        VG_MOVETO, x, y,
        VG_LINETO, x, y + h,
        VG_LINETO, x + w, y + h,
        VG_LINETO, x + w, y,
        VG_CLOSE};
    AppendCommands(vals, HK_ARRAY_SIZE(vals));
}

void Canvas::RoundedRect(float x, float y, float w, float h, float r)
{
    RoundedRectVarying(x, y, w, h, r, r, r, r);
}

void Canvas::RoundedRectVarying(float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft)
{
    if (radTopLeft < 0.1f && radTopRight < 0.1f && radBottomRight < 0.1f && radBottomLeft < 0.1f)
    {
        Rect(x, y, w, h);
        return;
    }
    else
    {
        const float halfw = Math::Abs(w) * 0.5f;
        const float halfh = Math::Abs(h) * 0.5f;
        const float rxBL = nvg__minf(radBottomLeft, halfw) * nvg__signf(w), ryBL = nvg__minf(radBottomLeft, halfh) * nvg__signf(h);
        const float rxBR = nvg__minf(radBottomRight, halfw) * nvg__signf(w), ryBR = nvg__minf(radBottomRight, halfh) * nvg__signf(h);
        const float rxTR = nvg__minf(radTopRight, halfw) * nvg__signf(w), ryTR = nvg__minf(radTopRight, halfh) * nvg__signf(h);
        const float rxTL = nvg__minf(radTopLeft, halfw) * nvg__signf(w), ryTL = nvg__minf(radTopLeft, halfh) * nvg__signf(h);
        const float vals[] = {
            VG_MOVETO, x, y + ryTL,
            VG_LINETO, x, y + h - ryBL,
            VG_BEZIERTO, x, y + h - ryBL * (1 - NVG_KAPPA90), x + rxBL * (1 - NVG_KAPPA90), y + h, x + rxBL, y + h,
            VG_LINETO, x + w - rxBR, y + h,
            VG_BEZIERTO, x + w - rxBR * (1 - NVG_KAPPA90), y + h, x + w, y + h - ryBR * (1 - NVG_KAPPA90), x + w, y + h - ryBR,
            VG_LINETO, x + w, y + ryTR,
            VG_BEZIERTO, x + w, y + ryTR * (1 - NVG_KAPPA90), x + w - rxTR * (1 - NVG_KAPPA90), y, x + w - rxTR, y,
            VG_LINETO, x + rxTL, y,
            VG_BEZIERTO, x + rxTL * (1 - NVG_KAPPA90), y, x, y + ryTL * (1 - NVG_KAPPA90), x, y + ryTL,
            VG_CLOSE};
        AppendCommands(vals, HK_ARRAY_SIZE(vals));
    }
}

void Canvas::Ellipse(Float2 const& center, float rx, float ry)
{
    float cx = center.X;
    float cy = center.Y;

    const float vals[] = {
        VG_MOVETO, cx - rx, cy,
        VG_BEZIERTO, cx - rx, cy + ry * NVG_KAPPA90, cx - rx * NVG_KAPPA90, cy + ry, cx, cy + ry,
        VG_BEZIERTO, cx + rx * NVG_KAPPA90, cy + ry, cx + rx, cy + ry * NVG_KAPPA90, cx + rx, cy,
        VG_BEZIERTO, cx + rx, cy - ry * NVG_KAPPA90, cx + rx * NVG_KAPPA90, cy - ry, cx, cy - ry,
        VG_BEZIERTO, cx - rx * NVG_KAPPA90, cy - ry, cx - rx, cy - ry * NVG_KAPPA90, cx - rx, cy,
        VG_CLOSE};
    AppendCommands(vals, HK_ARRAY_SIZE(vals));
}

void Canvas::Circle(Float2 const& center, float r)
{
    Ellipse(center, r, r);
}

void Canvas::FlattenPaths()
{
    if (m_PathCache.Paths.Size() > 0)
        return;

    // Flatten
    {
        VGPath* path = nullptr;
        float*  p;

        for (int i = 0, count = m_Commands.Size(); i < count;)
        {
            int cmd = (int)m_Commands[i];
            switch (cmd)
            {
                case VG_MOVETO:
                    path = m_PathCache.AddPath();
                    p    = &m_Commands[i + 1];
                    m_PathCache.AddPoint(p[0], p[1], VG_PT_CORNER);
                    i += 3;
                    break;
                case VG_LINETO:
                    p = &m_Commands[i + 1];
                    m_PathCache.AddPoint(p[0], p[1], VG_PT_CORNER);
                    i += 3;
                    break;
                case VG_BEZIERTO:
                    if (!m_PathCache.Points.IsEmpty())
                    {
                        VGPoint& last = m_PathCache.Points.Last();

                        float* cp1 = &m_Commands[i + 1];
                        float* cp2 = &m_Commands[i + 3];
                        p          = &m_Commands[i + 5];
                        TesselateBezier(last.x, last.y, cp1[0], cp1[1], cp2[0], cp2[1], p[0], p[1], 0, VG_PT_CORNER);
                    }
                    i += 7;
                    break;
                case VG_CLOSE:
                    if (path)
                        path->bClosed = true;
                    i++;
                    break;
                case VG_WINDING:
                    if (path)
                        path->Winding = (int)m_Commands[i + 1];
                    i += 2;
                    break;
                default:
                    i++;
            }
        }
    }

    m_PathCache.Bounds[0] = m_PathCache.Bounds[1] = 1e6f;
    m_PathCache.Bounds[2] = m_PathCache.Bounds[3] = -1e6f;

    // Calculate the direction and length of line segments.
    for (VGPath& path : m_PathCache.Paths)
    {
        VGPoint* pts = &m_PathCache.Points[path.First];

        // If the first and last points are the same, remove the last, mark as closed path.
        VGPoint* p0 = &pts[path.Count - 1];
        VGPoint* p1 = &pts[0];
        if (nvg__ptEquals(p0->x, p0->y, p1->x, p1->y, m_DistTol))
        {
            path.Count--;
            p0           = &pts[path.Count - 1];
            path.bClosed = true;
        }

        // Enforce winding.
        if (path.Count > 2)
        {
            float area = nvg__polyArea(pts, path.Count);
            if (path.Winding == CANVAS_PATH_WINDING_CCW && area < 0.0f)
                nvg__polyReverse(pts, path.Count);
            if (path.Winding == CANVAS_PATH_WINDING_CW && area > 0.0f)
                nvg__polyReverse(pts, path.Count);
        }

        for (int i = 0; i < path.Count; i++)
        {
            // Calculate segment direction and length
            p0->dx  = p1->x - p0->x;
            p0->dy  = p1->y - p0->y;
            p0->len = nvg__normalize(&p0->dx, &p0->dy);
            // Update bounds
            m_PathCache.Bounds[0] = nvg__minf(m_PathCache.Bounds[0], p0->x);
            m_PathCache.Bounds[1] = nvg__minf(m_PathCache.Bounds[1], p0->y);
            m_PathCache.Bounds[2] = nvg__maxf(m_PathCache.Bounds[2], p0->x);
            m_PathCache.Bounds[3] = nvg__maxf(m_PathCache.Bounds[3], p0->y);
            // Advance
            p0 = p1++;
        }
    }
}

void Canvas::Fill()
{
    VGState*    state     = GetState();
    CanvasPaint fillPaint = state->Fill;

    FlattenPaths();
    if (m_bEdgeAntialias && state->bShapeAntiAlias && state->CompositeOperation != CANVAS_COMPOSITE_COPY)
        ExpandFill(m_FringeWidth, CANVAS_LINE_JOIN_MITER, 2.4f);
    else
        ExpandFill(0.0f, CANVAS_LINE_JOIN_MITER, 2.4f);

    // Apply global alpha
    fillPaint.InnerColor.A *= state->Alpha;
    fillPaint.OuterColor.A *= state->Alpha;

    RenderFill(&fillPaint, state->CompositeOperation, state->Scissor, m_FringeWidth, m_PathCache.Bounds);

    // Count triangles
    for (VGPath& path : m_PathCache.Paths)
    {
        m_FillTriCount += path.NumFill - 2;
        m_FillTriCount += path.NumStroke - 2;
        m_DrawCallCount += 2;
    }
}

void Canvas::Stroke()
{
    VGState*    state       = GetState();
    float       scale       = nvg__getAverageScale(state->Xform);
    float       strokeWidth = nvg__clampf(state->StrokeWidth * scale, 0.0f, 200.0f);
    CanvasPaint strokePaint = state->Stroke;

    if (strokeWidth < m_FringeWidth)
    {
        // If the stroke width is less than pixel size, use alpha to emulate coverage.
        // Since coverage is area, scale by alpha*alpha.
        float alpha = nvg__clampf(strokeWidth / m_FringeWidth, 0.0f, 1.0f);
        strokePaint.InnerColor.A *= alpha * alpha;
        strokePaint.OuterColor.A *= alpha * alpha;
        strokeWidth = m_FringeWidth;
    }

    // Apply global alpha
    strokePaint.InnerColor.A *= state->Alpha;
    strokePaint.OuterColor.A *= state->Alpha;

    FlattenPaths();

    if (m_bEdgeAntialias && state->bShapeAntiAlias && state->CompositeOperation != CANVAS_COMPOSITE_COPY)
        ExpandStroke(strokeWidth * 0.5f, m_FringeWidth, state->LineCap, state->LineJoin, state->MiterLimit);
    else
        ExpandStroke(strokeWidth * 0.5f, 0.0f, state->LineCap, state->LineJoin, state->MiterLimit);

    RenderStroke(&strokePaint, state->CompositeOperation, state->Scissor, m_FringeWidth, strokeWidth);

    // Count triangles
    for (VGPath& path : m_PathCache.Paths)
    {
        m_StrokeTriCount += path.NumStroke - 2;
        m_DrawCallCount++;
    }
}

void Canvas::FontFace(FontHandle font)
{
    VGState* state = GetState();

    state->Font = font;
}

void Canvas::FontFace(StringView font)
{
    FontFace(GameApplication::GetResourceManager().GetResource<FontResource>(font));
}

float Canvas::Text(FontStyle const& style, float x, float y, TEXT_ALIGNMENT_FLAGS flags, StringView string)
{
    enum CANVAS_TEXT_ALIGN : uint8_t
    {
        // Horizontal align
        CANVAS_TEXT_ALIGN_LEFT   = 1 << 0, // Default, align text horizontally to left.
        CANVAS_TEXT_ALIGN_CENTER = 1 << 1, // Align text horizontally to center.
        CANVAS_TEXT_ALIGN_RIGHT  = 1 << 2, // Align text horizontally to right.
        // Vertical align
        CANVAS_TEXT_ALIGN_TOP      = 1 << 3, // Align text vertically to top.
        CANVAS_TEXT_ALIGN_MIDDLE   = 1 << 4, // Align text vertically to middle.
        CANVAS_TEXT_ALIGN_BOTTOM   = 1 << 5, // Align text vertically to bottom.
        CANVAS_TEXT_ALIGN_BASELINE = 1 << 6, // Default, align text vertically to baseline.
    };

    int align = CANVAS_TEXT_ALIGN_TOP;
    if (flags & TEXT_ALIGNMENT_LEFT)
        align |= CANVAS_TEXT_ALIGN_LEFT;
    else if (flags & TEXT_ALIGNMENT_HCENTER)
        align |= CANVAS_TEXT_ALIGN_CENTER;
    else if (flags & TEXT_ALIGNMENT_RIGHT)
        align |= CANVAS_TEXT_ALIGN_RIGHT;
    else
        align |= CANVAS_TEXT_ALIGN_LEFT;

    VGState* state = GetState();

    FONStextIter  iter, prevIter;
    FONSquad      q;
    CanvasVertex* verts;
    float         scale    = /*nvg__getFontScale(state) * */ m_DevicePxRatio;
    float         invscale = 1.0f / scale;
    int           cverts   = 0;
    int           nverts   = 0;

    FontResource* font = CurrentFont();

    auto fs = m_FontStash->GetImpl();

    fonsSetSize(fs, style.FontSize * scale);
    fonsSetSpacing(fs, style.LetterSpacing * scale);
    fonsSetBlur(fs, style.FontBlur * scale);
    fonsSetAlign(fs, align);
    fonsSetFont(fs, font->GetId());

    cverts = Math::Max(2, (int)(string.Size())) * 6; // conservative estimate.
    verts  = m_PathCache.AllocVerts(cverts);
    if (verts == NULL) return x;

    float minx, maxx, miny, maxy;

    if (state->Scissor.Extent[0] < 0.0f)
    {
        minx = -Math::MaxValue<float>();
        maxx = Math::MaxValue<float>();
        miny = -Math::MaxValue<float>();
        maxy = Math::MaxValue<float>();
    }
    else
    {
        minx = state->Scissor.Xform[2][0] - state->Scissor.Extent[0];
        maxx = state->Scissor.Xform[2][0] + state->Scissor.Extent[0];
        miny = state->Scissor.Xform[2][1] - state->Scissor.Extent[1];
        maxy = state->Scissor.Xform[2][1] + state->Scissor.Extent[1];
    }

    fonsTextIterInit(fs, &iter, x * scale, y * scale, string.Begin(), string.End(), FONS_GLYPH_BITMAP_REQUIRED);
    prevIter = iter;
    while (fonsTextIterNext(fs, &iter, &q))
    {
        if (iter.prevGlyphIndex == -1)
        { // can not retrieve glyph?
            if (nverts != 0)
            {
                RenderText(verts, nverts);
                nverts = 0;
            }
            if (!m_FontStash->ReallocTexture())
                break; // no memory :(
            iter = prevIter;
            fonsTextIterNext(fs, &iter, &q); // try again
            if (iter.prevGlyphIndex == -1)   // still can not find glyph?
                break;
        }
        prevIter = iter;

        float x0 = q.x0 * invscale;
        float x1 = q.x1 * invscale;
        float y0 = q.y0 * invscale;
        float y1 = q.y1 * invscale;

        // Transform corners.
        Float2 c0 = state->Xform * Float2(x0, y0);
        Float2 c1 = state->Xform * Float2(x1, y0);
        Float2 c2 = state->Xform * Float2(x1, y1);
        Float2 c3 = state->Xform * Float2(x0, y1);

        // Clip by scissor
        if (c1.X < minx || c0.X >= maxx)
            continue;
        if (c2.Y < miny || c0.Y >= maxy)
            continue;

        // Create triangles
        if (nverts + 6 <= cverts)
        {
            nvg__vset(&verts[nverts], c0.X, c0.Y, q.s0, q.t0);
            nverts++;
            nvg__vset(&verts[nverts], c2.X, c2.Y, q.s1, q.t1);
            nverts++;
            nvg__vset(&verts[nverts], c1.X, c1.Y, q.s1, q.t0);
            nverts++;

            nvg__vset(&verts[nverts], c0.X, c0.Y, q.s0, q.t0);
            nverts++;
            nvg__vset(&verts[nverts], c3.X, c3.Y, q.s0, q.t1);
            nverts++;
            nvg__vset(&verts[nverts], c2.X, c2.Y, q.s1, q.t1);
            nverts++;
        }
    }

    m_bUpdateFontTexture = true;

    RenderText(verts, nverts);

    return iter.nextx / scale;
}

float Canvas::Text(FontStyle const& style, float x, float y, TEXT_ALIGNMENT_FLAGS flags, WideStringView string)
{
    enum CANVAS_TEXT_ALIGN : uint8_t
    {
        // Horizontal align
        CANVAS_TEXT_ALIGN_LEFT   = 1 << 0, // Default, align text horizontally to left.
        CANVAS_TEXT_ALIGN_CENTER = 1 << 1, // Align text horizontally to center.
        CANVAS_TEXT_ALIGN_RIGHT  = 1 << 2, // Align text horizontally to right.
        // Vertical align
        CANVAS_TEXT_ALIGN_TOP      = 1 << 3, // Align text vertically to top.
        CANVAS_TEXT_ALIGN_MIDDLE   = 1 << 4, // Align text vertically to middle.
        CANVAS_TEXT_ALIGN_BOTTOM   = 1 << 5, // Align text vertically to bottom.
        CANVAS_TEXT_ALIGN_BASELINE = 1 << 6, // Default, align text vertically to baseline.
    };

    int align = CANVAS_TEXT_ALIGN_TOP;
    if (flags & TEXT_ALIGNMENT_LEFT)
        align |= CANVAS_TEXT_ALIGN_LEFT;
    else if (flags & TEXT_ALIGNMENT_HCENTER)
        align |= CANVAS_TEXT_ALIGN_CENTER;
    else if (flags & TEXT_ALIGNMENT_RIGHT)
        align |= CANVAS_TEXT_ALIGN_RIGHT;
    else
        align |= CANVAS_TEXT_ALIGN_LEFT;

    VGState* state = GetState();

    FONStextIter  iter, prevIter;
    FONSquad      q;
    CanvasVertex* verts;
    float         scale    = /*nvg__getFontScale(state) * */ m_DevicePxRatio;
    float         invscale = 1.0f / scale;
    int           cverts   = 0;
    int           nverts   = 0;

    FontResource* font = CurrentFont();

    auto fs = m_FontStash->GetImpl();

    fonsSetSize(fs, style.FontSize * scale);
    fonsSetSpacing(fs, style.LetterSpacing * scale);
    fonsSetBlur(fs, style.FontBlur * scale);
    fonsSetAlign(fs, align);
    fonsSetFont(fs, font->GetId());

    cverts = Math::Max(2, (int)string.Size()) * 6; // conservative estimate.
    verts  = m_PathCache.AllocVerts(cverts);
    if (verts == NULL) return x;

    float minx, maxx, miny, maxy;

    if (state->Scissor.Extent[0] < 0.0f)
    {
        minx = -Math::MaxValue<float>();
        maxx = Math::MaxValue<float>();
        miny = -Math::MaxValue<float>();
        maxy = Math::MaxValue<float>();
    }
    else
    {
        minx = state->Scissor.Xform[2][0] - state->Scissor.Extent[0];
        maxx = state->Scissor.Xform[2][0] + state->Scissor.Extent[0];
        miny = state->Scissor.Xform[2][1] - state->Scissor.Extent[1];
        maxy = state->Scissor.Xform[2][1] + state->Scissor.Extent[1];
    }

    fonsTextIterInitW(fs, &iter, x * scale, y * scale, string.Begin(), string.End(), FONS_GLYPH_BITMAP_REQUIRED);
    prevIter = iter;
    while (fonsTextIterNextW(fs, &iter, &q))
    {
        if (iter.prevGlyphIndex == -1)
        { // can not retrieve glyph?
            if (nverts != 0)
            {
                RenderText(verts, nverts);
                nverts = 0;
            }
            if (!m_FontStash->ReallocTexture())
                break; // no memory :(
            iter = prevIter;
            fonsTextIterNextW(fs, &iter, &q); // try again
            if (iter.prevGlyphIndex == -1)    // still can not find glyph?
                break;
        }
        prevIter = iter;

        float x0 = q.x0 * invscale;
        float x1 = q.x1 * invscale;
        float y0 = q.y0 * invscale;
        float y1 = q.y1 * invscale;

        // Transform corners.
        Float2 c0 = state->Xform * Float2(x0, y0);
        Float2 c1 = state->Xform * Float2(x1, y0);
        Float2 c2 = state->Xform * Float2(x1, y1);
        Float2 c3 = state->Xform * Float2(x0, y1);

        // Clip by scissor
        if (c1.X < minx || c0.X >= maxx)
            continue;
        if (c2.Y < miny || c0.Y >= maxy)
            continue;

        // Create triangles
        if (nverts + 6 <= cverts)
        {
            nvg__vset(&verts[nverts], c0.X, c0.Y, q.s0, q.t0);
            nverts++;
            nvg__vset(&verts[nverts], c2.X, c2.Y, q.s1, q.t1);
            nverts++;
            nvg__vset(&verts[nverts], c1.X, c1.Y, q.s1, q.t0);
            nverts++;

            nvg__vset(&verts[nverts], c0.X, c0.Y, q.s0, q.t0);
            nverts++;
            nvg__vset(&verts[nverts], c3.X, c3.Y, q.s0, q.t1);
            nverts++;
            nvg__vset(&verts[nverts], c2.X, c2.Y, q.s1, q.t1);
            nverts++;
        }
    }

    m_bUpdateFontTexture = true;

    //Push();
    //ResetScissor();

    RenderText(verts, nverts);

    //Pop();

    return iter.nextx / scale;
}

FontResource* Canvas::CurrentFont()
{
    VGState* state = GetState();

    FontResource* resource = GameApplication::GetResourceManager().TryGet(state->Font);

    return resource ? resource : GameApplication::GetDefaultFont();
}

void Canvas::TextBox(FontStyle const& style, Float2 const& mins, Float2 const& maxs, TEXT_ALIGNMENT_FLAGS flags, bool bWrap, StringView text)
{
    if (text.IsEmpty())
        return;

    FontResource* font = CurrentFont();

    TextMetrics metrics;
    font->GetTextMetrics(style, metrics);

    Float2 clipMins, clipMaxs;
    GetIntersectedScissor(mins, maxs, clipMins, clipMaxs);

    float lineHeight    = metrics.LineHeight;
    float x             = mins.X;
    float y             = mins.Y;
    float boxWidth      = maxs.X - mins.X;
    float boxHeight     = maxs.Y - mins.Y;
    float breakRowWidth = bWrap ? boxWidth : Math::MaxValue<float>();

    static TextRow rows[128];
    int            nrows;

    if ((flags & TEXT_ALIGNMENT_VCENTER) || (flags & TEXT_ALIGNMENT_BOTTOM))
    {
        nrows = font->TextLineCount(style, text, breakRowWidth);

        float yOffset = boxHeight - nrows * lineHeight;

        if (flags & TEXT_ALIGNMENT_VCENTER)
        {
            yOffset *= 0.5f;
        }

        y += yOffset;
    }

    StringView str = text;

    bool bKeepSpaces = (flags & TEXT_ALIGNMENT_KEEP_SPACES);

    while ((nrows = font->TextBreakLines(style, str, breakRowWidth, rows, HK_ARRAY_SIZE(rows), bKeepSpaces)) > 0)
    {
        for (int i = 0; i < nrows; i++)
        {
            TextRow* row = &rows[i];

            float cx = x;

            if (flags & TEXT_ALIGNMENT_HCENTER)
            {
                cx += boxWidth * 0.5f - row->Width * 0.5f;
            }
            else if (flags & TEXT_ALIGNMENT_RIGHT)
            {
                cx += boxWidth - row->Width;
            }

            if (y + lineHeight < clipMins.Y)
            {
                y += lineHeight;
                continue;
            }

            if (y >= clipMaxs.Y)
                return;

            Text(style, cx, y, TEXT_ALIGNMENT_LEFT, row->GetStringView());

            y += lineHeight;
        }

        str = StringView(rows[nrows - 1].Next, str.End());
    }
}

void Canvas::TextBox(FontStyle const& style, Float2 const& mins, Float2 const& maxs, TEXT_ALIGNMENT_FLAGS flags, bool bWrap, WideStringView text)
{
    if (text.IsEmpty())
        return;

    FontResource* font = CurrentFont();

    TextMetrics metrics;
    font->GetTextMetrics(style, metrics);

    Float2 clipMins, clipMaxs;
    GetIntersectedScissor(mins, maxs, clipMins, clipMaxs);

    float lineHeight    = metrics.LineHeight;
    float x             = mins.X;
    float y             = mins.Y;
    float boxWidth      = maxs.X - mins.X;
    float boxHeight     = maxs.Y - mins.Y;
    float breakRowWidth = bWrap ? boxWidth : Math::MaxValue<float>();

    static TextRowW rows[128];
    int             nrows;

    if ((flags & TEXT_ALIGNMENT_VCENTER) || (flags & TEXT_ALIGNMENT_BOTTOM))
    {
        nrows = font->TextLineCount(style, text, breakRowWidth);

        float yOffset = boxHeight - nrows * lineHeight;

        if (flags & TEXT_ALIGNMENT_VCENTER)
        {
            yOffset *= 0.5f;
        }

        y += yOffset;
    }

    WideStringView str = text;

    bool bKeepSpaces = (flags & TEXT_ALIGNMENT_KEEP_SPACES);

    while ((nrows = font->TextBreakLines(style, str, breakRowWidth, rows, HK_ARRAY_SIZE(rows), bKeepSpaces)) > 0)
    {
        for (int i = 0; i < nrows; i++)
        {
            TextRowW* row = &rows[i];

            float cx = x;

            if (flags & TEXT_ALIGNMENT_HCENTER)
            {
                cx += boxWidth * 0.5f - row->Width * 0.5f;
            }
            else if (flags & TEXT_ALIGNMENT_RIGHT)
            {
                cx += boxWidth - row->Width;
            }

            if (y + lineHeight < clipMins.Y)
            {
                y += lineHeight;
                continue;
            }

            if (y >= clipMaxs.Y)
                return;

            Text(style, cx, y, TEXT_ALIGNMENT_LEFT, row->GetStringView());

            y += lineHeight;
        }

        str = WideStringView(rows[nrows - 1].Next, str.End());
    }
}

// Cursor map from Dear ImGui:
// A work of art lies ahead! (. = white layer, X = black layer, others are blank)
// The white texels on the top left are the ones we'll use everywhere to render filled shapes.
const int         CURSOR_MAP_HALF_WIDTH = 108;
const int         CURSOR_MAP_HEIGHT     = 27;
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
        {Float2(0, 3), Float2(12, 19), Float2(0, 0)},    // DRAW_CURSOR_ARROW
        {Float2(13, 0), Float2(7, 16), Float2(1, 8)},    // DRAW_CURSOR_TEXT_INPUT
        {Float2(31, 0), Float2(23, 23), Float2(11, 11)}, // DRAW_CURSOR_RESIZE_ALL
        {Float2(21, 0), Float2(9, 23), Float2(4, 11)},   // DRAW_CURSOR_RESIZE_NS
        {Float2(55, 18), Float2(23, 9), Float2(11, 4)},  // DRAW_CURSOR_RESIZE_EW
        {Float2(73, 0), Float2(17, 17), Float2(8, 8)},   // DRAW_CURSOR_RESIZE_NESW
        {Float2(55, 0), Float2(17, 17), Float2(8, 8)},   // DRAW_CURSOR_RESIZE_NWSE
        {Float2(91, 0), Float2(17, 22), Float2(5, 0)},   // DRAW_CURSOR_RESIZE_HAND
};

void Canvas::CreateCursorMap()
{
    int w = CURSOR_MAP_HALF_WIDTH * 2 + 1;
    int h = CURSOR_MAP_HEIGHT;

    RawImage image(w, h, RAW_IMAGE_FORMAT_R8, Float4(0.0f));

    uint8_t* data = (uint8_t*)image.GetData();

    for (int y = 0, n = 0; y < CURSOR_MAP_HEIGHT; y++)
    {
        for (int x = 0; x < CURSOR_MAP_HALF_WIDTH; x++, n++)
        {
            const int offset0 = y * w + x;
            const int offset1 = offset0 + CURSOR_MAP_HALF_WIDTH + 1;
            data[offset0]     = CursorMap[n] == '.' ? 0xFF : 0x00;
            data[offset1]     = CursorMap[n] == 'X' ? 0xFF : 0x00;
        }
    }

    UniqueRef<TextureResource> cursorMap = MakeUnique<TextureResource>(CreateImage(image, nullptr));
    cursorMap->Upload();

    m_CursorMap = GameApplication::GetResourceManager().CreateResourceWithData("internal_cursor_map", std::move(cursorMap));
    m_CursorMapWidth = w;
    m_CursorMapHeight = h;
}

static void GetMouseCursorData(DRAW_CURSOR cursor, Float2& offset, Float2& size, Float2& uvfill, Float2& uvborder)
{
    HK_ASSERT(cursor >= DRAW_CURSOR_ARROW && cursor <= DRAW_CURSOR_RESIZE_HAND);

    Float2 pos = CursorTexData[cursor][0];
    size       = CursorTexData[cursor][1];
    offset     = CursorTexData[cursor][2];
    uvfill     = pos;
    pos.X += CURSOR_MAP_HALF_WIDTH + 1;
    uvborder = pos;
}

void Canvas::DrawCursor(DRAW_CURSOR cursor, Float2 const& position, Color4 const& fillColor, Color4 const& borderColor, bool bShadow)
{
    Float2 offset, size, uvfill, uvborder;

    GetMouseCursorData(cursor, offset, size, uvfill, uvborder);

    Float2 p = position.Floor() - offset;

    if (!m_CursorMap)
        CreateCursorMap();

    DrawTextureDesc desc;
    desc.TexHandle = m_CursorMap;
    desc.W         = size.X;
    desc.H         = size.Y;
    desc.UVScale.X = 1.0f / desc.W * m_CursorMapWidth;
    desc.UVScale.Y = 1.0f / desc.H * m_CursorMapHeight;

    desc.Y = p.Y;

    if (bShadow)
    {
        const Color4 shadowColor = Color4(0, 0, 0, 0.3f);

        desc.TintColor = shadowColor;
        desc.UVOffset  = -uvborder;

        desc.X = p.X + 1;
        DrawTexture(desc);

        desc.X = p.X + 2;
        DrawTexture(desc);
    }

    desc.X = p.X;

    desc.TintColor = borderColor;
    desc.UVOffset  = -uvborder;
    DrawTexture(desc);

    desc.TintColor = fillColor;
    desc.UVOffset  = -uvfill;
    DrawTexture(desc);
}

void Canvas::RenderText(CanvasVertex* verts, int nverts)
{
    VGState* state = GetState();

    CanvasPaint paint = state->Fill;

    // Apply global alpha
    paint.InnerColor.A *= state->Alpha;
    paint.OuterColor.A *= state->Alpha;

    #if 0
    RenderTriangles(&paint, state->CompositeOperation, state->Scissor, verts, nverts, m_FringeWidth);
    #else

    // Set Dummy texture for ConvertPaint
    paint.TexHandle = TextureHandle(ResourceID(RESOURCE_TEXTURE, 1));

    CanvasDrawCmd*  drawCommand = AllocDrawCommand();
    CanvasUniforms* frag;

    drawCommand->Type         = CANVAS_DRAW_COMMAND_TRIANGLES;
    drawCommand->Composite    = state->CompositeOperation;
    drawCommand->Texture      = m_FontStash->GetTexture();
    drawCommand->TextureFlags = paint.ImageFlags;

    // Allocate vertices for all the paths.
    drawCommand->FirstVertex = AllocVerts(nverts);
    drawCommand->VertexCount = nverts;

    Core::Memcpy(&m_DrawData.Vertices[drawCommand->FirstVertex], verts, sizeof(CanvasVertex) * nverts);

    // Fill shader
    drawCommand->UniformOffset = AllocUniforms(1);

    frag = GetUniformPtr(drawCommand->UniformOffset);
    ConvertPaint(frag, &paint, state->Scissor, 1.0f, m_FringeWidth, -1.0f);
    frag->Type = CANVAS_SHADER_IMAGE;

    #endif

    m_DrawCallCount++;
    m_TextTriCount += nverts / 3;
}

void Canvas::AppendCommands(float const* vals, int nvals)
{
    VGState* state = GetState();
    int      i;

    int firstCommand = m_Commands.Size();
    m_Commands.Resize(firstCommand + nvals);

    float* commands = &m_Commands[firstCommand];
    Core::Memcpy(commands, vals, nvals * sizeof(float));

    if ((int)commands[0] != VG_CLOSE && (int)commands[0] != VG_WINDING)
    {
        m_CommandPos.X = commands[nvals - 2];
        m_CommandPos.Y = commands[nvals - 1];
    }

    // transform commands
    i = 0;
    while (i < nvals)
    {
        int     cmd  = (int)commands[i];
        Float2* vecs = (Float2*)&commands[i + 1];
        switch (cmd)
        {
            case VG_MOVETO:
                vecs[0] = state->Xform * vecs[0];
                i += 3;
                break;
            case VG_LINETO:
                vecs[0] = state->Xform * vecs[0];
                i += 3;
                break;
            case VG_BEZIERTO:
                vecs[0] = state->Xform * vecs[0];
                vecs[1] = state->Xform * vecs[1];
                vecs[2] = state->Xform * vecs[2];
                i += 7;
                break;
            case VG_CLOSE:
                i++;
                break;
            case VG_WINDING:
                i += 2;
                break;
            default:
                i++;
        }
    }
}

void Canvas::TesselateBezier(float x1,
                              float y1,
                              float x2,
                              float y2,
                              float x3,
                              float y3,
                              float x4,
                              float y4,
                              int   level,
                              int   type)
{
    float x12, y12, x23, y23, x34, y34, x123, y123, x234, y234, x1234, y1234;
    float dx, dy, d2, d3;

    if (level > 10)
        return;

    x12  = (x1 + x2) * 0.5f;
    y12  = (y1 + y2) * 0.5f;
    x23  = (x2 + x3) * 0.5f;
    y23  = (y2 + y3) * 0.5f;
    x34  = (x3 + x4) * 0.5f;
    y34  = (y3 + y4) * 0.5f;
    x123 = (x12 + x23) * 0.5f;
    y123 = (y12 + y23) * 0.5f;

    dx = x4 - x1;
    dy = y4 - y1;
    d2 = Math::Abs(((x2 - x4) * dy - (y2 - y4) * dx));
    d3 = Math::Abs(((x3 - x4) * dy - (y3 - y4) * dx));

    if ((d2 + d3) * (d2 + d3) < m_TessTol * (dx * dx + dy * dy))
    {
        m_PathCache.AddPoint(x4, y4, type);
        return;
    }

    /*	if (Math::Abs(x1+x3-x2-x2) + Math::Abs(y1+y3-y2-y2) + Math::Abs(x2+x4-x3-x3) + Math::Abs(y2+y4-y3-y3) < m_TessTol) {
		m_PathCache.AddPoint(x4, y4, type);
		return;
	}*/

    x234  = (x23 + x34) * 0.5f;
    y234  = (y23 + y34) * 0.5f;
    x1234 = (x123 + x234) * 0.5f;
    y1234 = (y123 + y234) * 0.5f;

    TesselateBezier(x1, y1, x12, y12, x123, y123, x1234, y1234, level + 1, 0);
    TesselateBezier(x1234, y1234, x234, y234, x34, y34, x4, y4, level + 1, type);
}

void Canvas::CalculateJoins(float w, CANVAS_LINE_JOIN lineJoin, float miterLimit)
{
    int   j;
    float iw = 0.0f;

    if (w > 0.0f) iw = 1.0f / w;

    // Calculate which joins needs extra vertices to append, and gather vertex count.
    for (VGPath& path : m_PathCache.Paths)
    {
        VGPoint* pts   = &m_PathCache.Points[path.First];
        VGPoint* p0    = &pts[path.Count - 1];
        VGPoint* p1    = &pts[0];
        int      nleft = 0;

        path.NumBevel = 0;

        for (j = 0; j < path.Count; j++)
        {
            float dlx0, dly0, dlx1, dly1, dmr2, cross, limit;
            dlx0 = p0->dy;
            dly0 = -p0->dx;
            dlx1 = p1->dy;
            dly1 = -p1->dx;
            // Calculate extrusions
            p1->dmx = (dlx0 + dlx1) * 0.5f;
            p1->dmy = (dly0 + dly1) * 0.5f;
            dmr2    = p1->dmx * p1->dmx + p1->dmy * p1->dmy;
            if (dmr2 > 0.000001f)
            {
                float scale = 1.0f / dmr2;
                if (scale > 600.0f)
                {
                    scale = 600.0f;
                }
                p1->dmx *= scale;
                p1->dmy *= scale;
            }

            // Clear flags, but keep the corner.
            p1->flags = (p1->flags & VG_PT_CORNER) ? VG_PT_CORNER : 0;

            // Keep track of left turns.
            cross = p1->dx * p0->dy - p0->dx * p1->dy;
            if (cross > 0.0f)
            {
                nleft++;
                p1->flags |= VG_PT_LEFT;
            }

            // Calculate if we should use bevel or miter for inner join.
            limit = nvg__maxf(1.01f, nvg__minf(p0->len, p1->len) * iw);
            if ((dmr2 * limit * limit) < 1.0f)
                p1->flags |= VG_PR_INNERBEVEL;

            // Check to see if the corner needs to be beveled.
            if (p1->flags & VG_PT_CORNER)
            {
                if ((dmr2 * miterLimit * miterLimit) < 1.0f || lineJoin == CANVAS_LINE_JOIN_BEVEL || lineJoin == CANVAS_LINE_JOIN_ROUND)
                {
                    p1->flags |= VG_PT_BEVEL;
                }
            }

            if ((p1->flags & (VG_PT_BEVEL | VG_PR_INNERBEVEL)) != 0)
                path.NumBevel++;

            p0 = p1++;
        }

        path.bConvex = (nleft == path.Count);
    }
}


int Canvas::ExpandStroke(float w, float fringe, CANVAS_LINE_CAP lineCap, CANVAS_LINE_JOIN lineJoin, float miterLimit)
{
    CanvasVertex* verts;
    CanvasVertex* dst;
    int           cverts, j;
    float         aa = fringe; //m_FringeWidth;
    float         u0 = 0.0f, u1 = 1.0f;
    int           ncap = nvg__curveDivs(w, Math::_PI, m_TessTol); // Calculate divisions per half circle.

    w += aa * 0.5f;

    // Disable the gradient used for antialiasing when antialiasing is not used.
    if (aa == 0.0f)
    {
        u0 = 0.5f;
        u1 = 0.5f;
    }

    CalculateJoins(w, lineJoin, miterLimit);

    // Calculate max vertex usage.
    cverts = 0;
    for (VGPath& path : m_PathCache.Paths)
    {
        bool loop = path.bClosed;
        if (lineJoin == CANVAS_LINE_JOIN_ROUND)
            cverts += (path.Count + path.NumBevel * (ncap + 2) + 1) * 2; // plus one for loop
        else
            cverts += (path.Count + path.NumBevel * 5 + 1) * 2; // plus one for loop
        if (!loop)
        {
            // space for caps
            if (lineCap == CANVAS_LINE_CAP_ROUND)
            {
                cverts += (ncap * 2 + 2) * 2;
            }
            else
            {
                cverts += (3 + 3) * 2;
            }
        }
    }

    verts = m_PathCache.AllocVerts(cverts);
    if (verts == NULL) return 0;

    for (VGPath& path : m_PathCache.Paths)
    {
        VGPoint* pts = &m_PathCache.Points[path.First];
        VGPoint* p0;
        VGPoint* p1;
        int      s, e;
        bool     loop;
        float    dx, dy;

        path.Fill    = 0;
        path.NumFill = 0;

        // Calculate fringe or stroke
        loop        = path.bClosed;
        dst         = verts;
        path.Stroke = dst;

        if (loop)
        {
            // Looping
            p0 = &pts[path.Count - 1];
            p1 = &pts[0];
            s  = 0;
            e  = path.Count;
        }
        else
        {
            // Add cap
            p0 = &pts[0];
            p1 = &pts[1];
            s  = 1;
            e  = path.Count - 1;
        }

        if (!loop)
        {
            // Add cap
            dx = p1->x - p0->x;
            dy = p1->y - p0->y;
            nvg__normalize(&dx, &dy);
            if (lineCap == CANVAS_LINE_CAP_BUTT)
                dst = nvg__buttCapStart(dst, p0, dx, dy, w, -aa * 0.5f, aa, u0, u1);
            else if (lineCap == CANVAS_LINE_CAP_BUTT || lineCap == CANVAS_LINE_CAP_SQUARE)
                dst = nvg__buttCapStart(dst, p0, dx, dy, w, w - aa, aa, u0, u1);
            else if (lineCap == CANVAS_LINE_CAP_ROUND)
                dst = nvg__roundCapStart(dst, p0, dx, dy, w, ncap, aa, u0, u1);
        }

        for (j = s; j < e; ++j)
        {
            if ((p1->flags & (VG_PT_BEVEL | VG_PR_INNERBEVEL)) != 0)
            {
                if (lineJoin == CANVAS_LINE_JOIN_ROUND)
                {
                    dst = nvg__roundJoin(dst, p0, p1, w, w, u0, u1, ncap, aa);
                }
                else
                {
                    dst = nvg__bevelJoin(dst, p0, p1, w, w, u0, u1, aa);
                }
            }
            else
            {
                nvg__vset(dst, p1->x + (p1->dmx * w), p1->y + (p1->dmy * w), u0, 1);
                dst++;
                nvg__vset(dst, p1->x - (p1->dmx * w), p1->y - (p1->dmy * w), u1, 1);
                dst++;
            }
            p0 = p1++;
        }

        if (loop)
        {
            // Loop it
            nvg__vset(dst, verts[0].x, verts[0].y, u0, 1);
            dst++;
            nvg__vset(dst, verts[1].x, verts[1].y, u1, 1);
            dst++;
        }
        else
        {
            // Add cap
            dx = p1->x - p0->x;
            dy = p1->y - p0->y;
            nvg__normalize(&dx, &dy);
            if (lineCap == CANVAS_LINE_CAP_BUTT)
                dst = nvg__buttCapEnd(dst, p1, dx, dy, w, -aa * 0.5f, aa, u0, u1);
            else if (lineCap == CANVAS_LINE_CAP_BUTT || lineCap == CANVAS_LINE_CAP_SQUARE)
                dst = nvg__buttCapEnd(dst, p1, dx, dy, w, w - aa, aa, u0, u1);
            else if (lineCap == CANVAS_LINE_CAP_ROUND)
                dst = nvg__roundCapEnd(dst, p1, dx, dy, w, ncap, aa, u0, u1);
        }

        path.NumStroke = (int)(dst - verts);

        verts = dst;
    }

    return 1;
}

int Canvas::ExpandFill(float w, CANVAS_LINE_JOIN lineJoin, float miterLimit)
{
    CanvasVertex* verts;
    CanvasVertex* dst;
    int           cverts, j;
    bool          bConvex;
    float         aa     = m_FringeWidth;
    int           fringe = w > 0.0f;

    CalculateJoins(w, lineJoin, miterLimit);

    // Calculate max vertex usage.
    cverts = 0;
    for (VGPath& path : m_PathCache.Paths)
    {
        cverts += path.Count + path.NumBevel + 1;
        if (fringe)
            cverts += (path.Count + path.NumBevel * 5 + 1) * 2; // plus one for loop
    }

    verts = m_PathCache.AllocVerts(cverts);
    if (verts == NULL) return 0;

    bConvex = (m_PathCache.Paths.Size() == 1) && m_PathCache.Paths[0].bConvex;

    for (VGPath& path : m_PathCache.Paths)
    {
        VGPoint* pts = &m_PathCache.Points[path.First];
        VGPoint* p0;
        VGPoint* p1;
        float    rw, lw, woff;
        float    ru, lu;

        // Calculate shape vertices.
        woff      = 0.5f * aa;
        dst       = verts;
        path.Fill = dst;

        if (fringe)
        {
            // Looping
            p0 = &pts[path.Count - 1];
            p1 = &pts[0];
            for (j = 0; j < path.Count; ++j)
            {
                if (p1->flags & VG_PT_BEVEL)
                {
                    float dlx0 = p0->dy;
                    float dly0 = -p0->dx;
                    float dlx1 = p1->dy;
                    float dly1 = -p1->dx;
                    if (p1->flags & VG_PT_LEFT)
                    {
                        float lx = p1->x + p1->dmx * woff;
                        float ly = p1->y + p1->dmy * woff;
                        nvg__vset(dst, lx, ly, 0.5f, 1);
                        dst++;
                    }
                    else
                    {
                        float lx0 = p1->x + dlx0 * woff;
                        float ly0 = p1->y + dly0 * woff;
                        float lx1 = p1->x + dlx1 * woff;
                        float ly1 = p1->y + dly1 * woff;
                        nvg__vset(dst, lx0, ly0, 0.5f, 1);
                        dst++;
                        nvg__vset(dst, lx1, ly1, 0.5f, 1);
                        dst++;
                    }
                }
                else
                {
                    nvg__vset(dst, p1->x + (p1->dmx * woff), p1->y + (p1->dmy * woff), 0.5f, 1);
                    dst++;
                }
                p0 = p1++;
            }
        }
        else
        {
            for (j = 0; j < path.Count; ++j)
            {
                nvg__vset(dst, pts[j].x, pts[j].y, 0.5f, 1);
                dst++;
            }
        }

        path.NumFill = (int)(dst - verts);
        verts        = dst;

        // Calculate fringe
        if (fringe)
        {
            lw          = w + woff;
            rw          = w - woff;
            lu          = 0;
            ru          = 1;
            dst         = verts;
            path.Stroke = dst;

            // Create only half a fringe for convex shapes so that
            // the shape can be rendered without stenciling.
            if (bConvex)
            {
                lw = woff; // This should generate the same vertex as fill inset above.
                lu = 0.5f; // Set outline fade at middle.
            }

            // Looping
            p0 = &pts[path.Count - 1];
            p1 = &pts[0];

            for (j = 0; j < path.Count; ++j)
            {
                if ((p1->flags & (VG_PT_BEVEL | VG_PR_INNERBEVEL)) != 0)
                {
                    dst = nvg__bevelJoin(dst, p0, p1, lw, rw, lu, ru, m_FringeWidth);
                }
                else
                {
                    nvg__vset(dst, p1->x + (p1->dmx * lw), p1->y + (p1->dmy * lw), lu, 1);
                    dst++;
                    nvg__vset(dst, p1->x - (p1->dmx * rw), p1->y - (p1->dmy * rw), ru, 1);
                    dst++;
                }
                p0 = p1++;
            }

            // Loop it
            nvg__vset(dst, verts[0].x, verts[0].y, lu, 1);
            dst++;
            nvg__vset(dst, verts[1].x, verts[1].y, ru, 1);
            dst++;

            path.NumStroke = (int)(dst - verts);
            verts          = dst;
        }
        else
        {
            path.Stroke    = NULL;
            path.NumStroke = 0;
        }
    }

    return 1;
}

CanvasDrawData const* Canvas::GetDrawData() const
{
    if (m_bUpdateFontTexture)
    {
        m_bUpdateFontTexture = false;
        m_FontStash->UpdateTexture();
    }
    return &m_DrawData;
}

HK_NAMESPACE_END
