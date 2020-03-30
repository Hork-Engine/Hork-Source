/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <World/Public/imdrawlist.h>

static inline ImVec2 operator*( const ImVec2& lhs, const float rhs ) { return ImVec2( lhs.x*rhs, lhs.y*rhs ); }
static inline ImVec2 operator/( const ImVec2& lhs, const float rhs ) { return ImVec2( lhs.x/rhs, lhs.y/rhs ); }
static inline ImVec2 operator+( const ImVec2& lhs, const ImVec2& rhs ) { return ImVec2( lhs.x+rhs.x, lhs.y+rhs.y ); }
static inline ImVec2 operator-( const ImVec2& lhs, const ImVec2& rhs ) { return ImVec2( lhs.x-rhs.x, lhs.y-rhs.y ); }
static inline ImVec2 operator*( const ImVec2& lhs, const ImVec2& rhs ) { return ImVec2( lhs.x*rhs.x, lhs.y*rhs.y ); }
static inline ImVec2 operator/( const ImVec2& lhs, const ImVec2& rhs ) { return ImVec2( lhs.x/rhs.x, lhs.y/rhs.y ); }
static inline ImVec2& operator+=( ImVec2& lhs, const ImVec2& rhs ) { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
static inline ImVec2& operator-=( ImVec2& lhs, const ImVec2& rhs ) { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
static inline ImVec2& operator*=( ImVec2& lhs, const float rhs ) { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
static inline ImVec2& operator/=( ImVec2& lhs, const float rhs ) { lhs.x /= rhs; lhs.y /= rhs; return lhs; }
static inline ImVec4 operator+( const ImVec4& lhs, const ImVec4& rhs ) { return ImVec4( lhs.x+rhs.x, lhs.y+rhs.y, lhs.z+rhs.z, lhs.w+rhs.w ); }
static inline ImVec4 operator-( const ImVec4& lhs, const ImVec4& rhs ) { return ImVec4( lhs.x-rhs.x, lhs.y-rhs.y, lhs.z-rhs.z, lhs.w-rhs.w ); }
static inline ImVec4 operator*( const ImVec4& lhs, const ImVec4& rhs ) { return ImVec4( lhs.x*rhs.x, lhs.y*rhs.y, lhs.z*rhs.z, lhs.w*rhs.w ); }


#define IM_PI               3.14159265358979323846f
#define IM_FLOOR(_VAL)      ((float)(int)(_VAL))                                    // ImFloor() is not inlined in MSVC debug builds
#define IM_ROUND(_VAL)      ((float)(int)((_VAL) + 0.5f))                           //
#define ImSqrt(X)           sqrtf(X)
#define ImFabs(X)           fabsf(X)
#define ImCeil(X)           ceilf(X)
#define ImFloorStd(X)       floorf(X)
#define ImFmod(X, Y)        fmodf((X), (Y))
static inline float  ImPow( float x, float y ) { return powf( x, y ); }          // DragBehaviorT/SliderBehaviorT uses ImPow with either float/double and need the precision
static inline double ImPow( double x, double y ) { return pow( x, y ); }
#define ImCos(X)            cosf(X)
#define ImSin(X)            sinf(X)
#define ImAcos(X)           acosf(X)
template<typename T> static inline T ImMin( T lhs, T rhs ) { return lhs < rhs ? lhs : rhs; }
template<typename T> static inline T ImMax( T lhs, T rhs ) { return lhs >= rhs ? lhs : rhs; }
static inline ImVec2 ImMin( const ImVec2& lhs, const ImVec2& rhs ) { return ImVec2( lhs.x < rhs.x ? lhs.x : rhs.x, lhs.y < rhs.y ? lhs.y : rhs.y ); }
static inline ImVec2 ImMax( const ImVec2& lhs, const ImVec2& rhs ) { return ImVec2( lhs.x >= rhs.x ? lhs.x : rhs.x, lhs.y >= rhs.y ? lhs.y : rhs.y ); }
template<typename T> static inline T ImClamp( T v, T mn, T mx ) { return (v < mn) ? mn : (v > mx) ? mx : v; }
static inline ImVec2 ImClamp( const ImVec2& v, const ImVec2& mn, ImVec2 mx ) { return ImVec2( (v.x < mn.x) ? mn.x : (v.x > mx.x) ? mx.x : v.x, (v.y < mn.y) ? mn.y : (v.y > mx.y) ? mx.y : v.y ); }
static inline ImVec2 ImMul( const ImVec2& lhs, const ImVec2& rhs ) { return ImVec2( lhs.x * rhs.x, lhs.y * rhs.y ); }
template<typename T> static inline T ImLerp( T a, T b, float t ) { return (T)(a + (b - a) * t); }
static inline ImVec2 ImLerp( const ImVec2& a, const ImVec2& b, float t ) { return ImVec2( a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t ); }
static inline ImVec2 ImLerp( const ImVec2& a, const ImVec2& b, const ImVec2& t ) { return ImVec2( a.x + (b.x - a.x) * t.x, a.y + (b.y - a.y) * t.y ); }
static inline ImVec4 ImLerp( const ImVec4& a, const ImVec4& b, float t ) { return ImVec4( a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t ); }
static inline float  ImLengthSqr( const ImVec2& lhs ) { return lhs.x*lhs.x + lhs.y*lhs.y; }
static inline float  ImLengthSqr( const ImVec4& lhs ) { return lhs.x*lhs.x + lhs.y*lhs.y + lhs.z*lhs.z + lhs.w*lhs.w; }
static inline float  ImDot( const ImVec2& a, const ImVec2& b ) { return a.x * b.x + a.y * b.y; }
// Helper function to calculate a circle's segment count given its radius and a "maximum error" value.
#define IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_MIN                     12
#define IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_MAX                     512
#define IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC(_RAD,_MAXERROR)    ImClamp((int)((IM_PI * 2.0f) / ImAcos((_RAD - _MAXERROR) / _RAD)), IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_MIN, IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_MAX)

struct ImNewDummy {};
inline void* operator new(size_t, ImNewDummy, void* ptr) { return ptr; }
inline void  operator delete(void*, ImNewDummy, void*) {} // This is only required so we can use the symmetrical new()
#define IM_PLACEMENT_NEW(_PTR)              new(ImNewDummy(), _PTR)
#define IM_NEW(_TYPE)                       new(ImNewDummy(), IM_ALLOC(sizeof(_TYPE))) _TYPE
template<typename T> void IM_DELETE( T* p ) { if ( p ) { p->~T(); IM_FREE( p ); } }

#define IM_COL32_A_MASK     0xFF000000

ImDrawListSharedData::ImDrawListSharedData()
{
    Font = NULL;
    FontSize = 0.0f;
    CurveTessellationTol = 0.0f;
    CircleSegmentMaxError = 0.0f;
    ClipRectFullscreen = ImVec4( -8192.0f, -8192.0f, +8192.0f, +8192.0f );
    InitialFlags = ImDrawListFlags_None;

    // Lookup tables
    for ( int i = 0; i < AN_ARRAY_SIZE( CircleVtx12 ); i++ )
    {
        const float a = ((float)i * 2 * IM_PI) / (float)AN_ARRAY_SIZE( CircleVtx12 );
        CircleVtx12[i] = ImVec2( ImCos( a ), ImSin( a ) );
    }
    memset( CircleSegmentCounts, 0, sizeof( CircleSegmentCounts ) ); // This will be set by SetCircleSegmentMaxError()
}

void ImDrawListSharedData::SetCircleSegmentMaxError( float max_error )
{
    if ( CircleSegmentMaxError == max_error )
        return;
    CircleSegmentMaxError = max_error;
    for ( int i = 0; i < AN_ARRAY_SIZE( CircleSegmentCounts ); i++ )
    {
        const float radius = i + 1.0f;
        const int segment_count = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC( radius, CircleSegmentMaxError );
        CircleSegmentCounts[i] = (uint8_t)ImMin( segment_count, 255 );
    }
}

void ImDrawList::Clear()
{
    CmdBuffer.resize( 0 );
    IdxBuffer.resize( 0 );
    VtxBuffer.resize( 0 );
    Flags = _Data ? _Data->InitialFlags : ImDrawListFlags_None;
    _VtxCurrentOffset = 0;
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.resize( 0 );
    _TextureIdStack.resize( 0 );
    _BlendingStack.resize( 0 );
    _Path.resize( 0 );
    _Splitter.Clear();
}

void ImDrawList::ClearFreeMemory()
{
    CmdBuffer.clear();
    IdxBuffer.clear();
    VtxBuffer.clear();
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.clear();
    _TextureIdStack.clear();
    _BlendingStack.clear();
    _Path.clear();
    _Splitter.ClearFreeMemory();
}

ImDrawList* ImDrawList::CloneOutput() const
{
    ImDrawList* dst = IM_NEW( ImDrawList( _Data ) );
    dst->CmdBuffer = CmdBuffer;
    dst->IdxBuffer = IdxBuffer;
    dst->VtxBuffer = VtxBuffer;
    dst->Flags = Flags;
    return dst;
}

// Using macros because C++ is a terrible language, we want guaranteed inline, no code in header, and no overhead in Debug builds
#define GetCurrentClipRect()    (_ClipRectStack.Size ? _ClipRectStack.Data[_ClipRectStack.Size-1]  : _Data->ClipRectFullscreen)
#define GetCurrentTextureId()   (_TextureIdStack.Size ? _TextureIdStack.Data[_TextureIdStack.Size-1] : (ImTextureID)NULL)
#define GetCurrentBlending()    (_BlendingStack.Size ? _BlendingStack.Data[_BlendingStack.Size-1] : 0)

void ImDrawList::AddDrawCmd()
{
    ImDrawCmd draw_cmd;
    draw_cmd.ClipRect = GetCurrentClipRect();
    draw_cmd.TextureId = GetCurrentTextureId();
    draw_cmd.VtxOffset = _VtxCurrentOffset;
    draw_cmd.IdxOffset = IdxBuffer.Size;
    draw_cmd.BlendingState = GetCurrentBlending();

    AN_ASSERT( draw_cmd.ClipRect.x <= draw_cmd.ClipRect.z && draw_cmd.ClipRect.y <= draw_cmd.ClipRect.w );
    CmdBuffer.push_back( draw_cmd );
}

void ImDrawList::AddCallback( ImDrawCallback callback, void* callback_data )
{
    ImDrawCmd* current_cmd = CmdBuffer.Size ? &CmdBuffer.back() : NULL;
    if ( !current_cmd || current_cmd->ElemCount != 0 || current_cmd->UserCallback != NULL )
    {
        AddDrawCmd();
        current_cmd = &CmdBuffer.back();
    }
    current_cmd->UserCallback = callback;
    current_cmd->UserCallbackData = callback_data;

    AddDrawCmd(); // Force a new command after us (see comment below)
}

// Our scheme may appears a bit unusual, basically we want the most-common calls AddLine AddRect etc. to not have to perform any check so we always have a command ready in the stack.
// The cost of figuring out if a new command has to be added or if we can merge is paid in those Update** functions only.
void ImDrawList::UpdateClipRect()
{
    // If current command is used with different settings we need to add a new command
    const ImVec4 curr_clip_rect = GetCurrentClipRect();
    ImDrawCmd* curr_cmd = CmdBuffer.Size > 0 ? &CmdBuffer.Data[CmdBuffer.Size-1] : NULL;
    if ( !curr_cmd || (curr_cmd->ElemCount != 0 && memcmp( &curr_cmd->ClipRect, &curr_clip_rect, sizeof( ImVec4 ) ) != 0) || curr_cmd->UserCallback != NULL )
    {
        AddDrawCmd();
        return;
    }

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = CmdBuffer.Size > 1 ? curr_cmd - 1 : NULL;
    if ( curr_cmd->ElemCount == 0 && prev_cmd && memcmp( &prev_cmd->ClipRect, &curr_clip_rect, sizeof( ImVec4 ) ) == 0 && prev_cmd->TextureId == GetCurrentTextureId() && prev_cmd->BlendingState == GetCurrentBlending() && prev_cmd->UserCallback == NULL )
        CmdBuffer.pop_back();
    else
        curr_cmd->ClipRect = curr_clip_rect;
}

void ImDrawList::UpdateBlendingState()
{
    // If current command is used with different settings we need to add a new command
    const int curr_blending = GetCurrentBlending();
    ImDrawCmd* curr_cmd = CmdBuffer.Size ? &CmdBuffer.back() : NULL;
    if ( !curr_cmd || (curr_cmd->ElemCount != 0 && curr_cmd->BlendingState != curr_blending) || curr_cmd->UserCallback != NULL )
    {
        AddDrawCmd();
        return;
    }

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = CmdBuffer.Size > 1 ? curr_cmd - 1 : NULL;
    if ( curr_cmd->ElemCount == 0 && prev_cmd && prev_cmd->TextureId == GetCurrentTextureId() && prev_cmd->BlendingState == curr_blending && memcmp( &prev_cmd->ClipRect, &GetCurrentClipRect(), sizeof( ImVec4 ) ) == 0 && prev_cmd->UserCallback == NULL )
        CmdBuffer.pop_back();
    else
        curr_cmd->BlendingState = curr_blending;
}

void ImDrawList::UpdateTextureID()
{
    // If current command is used with different settings we need to add a new command
    const ImTextureID curr_texture_id = GetCurrentTextureId();
    ImDrawCmd* curr_cmd = CmdBuffer.Size ? &CmdBuffer.back() : NULL;
    if ( !curr_cmd || (curr_cmd->ElemCount != 0 && curr_cmd->TextureId != curr_texture_id) || curr_cmd->UserCallback != NULL )
    {
        AddDrawCmd();
        return;
    }

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = CmdBuffer.Size > 1 ? curr_cmd - 1 : NULL;
    if ( curr_cmd->ElemCount == 0 && prev_cmd && prev_cmd->TextureId == curr_texture_id && prev_cmd->BlendingState == GetCurrentBlending() && memcmp( &prev_cmd->ClipRect, &GetCurrentClipRect(), sizeof( ImVec4 ) ) == 0 && prev_cmd->UserCallback == NULL )
        CmdBuffer.pop_back();
    else
        curr_cmd->TextureId = curr_texture_id;
}

#undef GetCurrentClipRect
#undef GetCurrentTextureId

// Render-level scissoring. This is passed down to your render function but not used for CPU-side coarse clipping. Prefer using higher-level ImGui::PushClipRect() to affect logic (hit-testing and widget culling)
void ImDrawList::PushClipRect( ImVec2 cr_min, ImVec2 cr_max, bool intersect_with_current_clip_rect )
{
    ImVec4 cr( cr_min.x, cr_min.y, cr_max.x, cr_max.y );
    if ( intersect_with_current_clip_rect && _ClipRectStack.Size )
    {
        ImVec4 current = _ClipRectStack.Data[_ClipRectStack.Size-1];
        if ( cr.x < current.x ) cr.x = current.x;
        if ( cr.y < current.y ) cr.y = current.y;
        if ( cr.z > current.z ) cr.z = current.z;
        if ( cr.w > current.w ) cr.w = current.w;
    }
    cr.z = ImMax( cr.x, cr.z );
    cr.w = ImMax( cr.y, cr.w );

    _ClipRectStack.push_back( cr );
    UpdateClipRect();
}

void ImDrawList::PushClipRectFullScreen()
{
    PushClipRect( ImVec2( _Data->ClipRectFullscreen.x, _Data->ClipRectFullscreen.y ), ImVec2( _Data->ClipRectFullscreen.z, _Data->ClipRectFullscreen.w ) );
}

void ImDrawList::PopClipRect()
{
    AN_ASSERT( _ClipRectStack.Size > 0 );
    _ClipRectStack.pop_back();
    UpdateClipRect();
}

void ImDrawList::PushBlendingState( int _Blending )
{
    _BlendingStack.push_back( _Blending );
    UpdateBlendingState();
}

void ImDrawList::PopBlendingState()
{
    AN_ASSERT( _BlendingStack.Size > 0 );
    _BlendingStack.pop_back();
    UpdateBlendingState();
}

void ImDrawList::PushTextureID( ImTextureID texture_id )
{
    _TextureIdStack.push_back( texture_id );
    UpdateTextureID();
}

void ImDrawList::PopTextureID()
{
    AN_ASSERT( _TextureIdStack.Size > 0 );
    _TextureIdStack.pop_back();
    UpdateTextureID();
}

// Reserve space for a number of vertices and indices.
// You must finish filling your reserved data before calling PrimReserve() again, as it may reallocate or
// submit the intermediate results. PrimUnreserve() can be used to release unused allocations.
void ImDrawList::PrimReserve( int idx_count, int vtx_count )
{
    // Large mesh support (when enabled)
    AN_ASSERT( idx_count >= 0 && vtx_count >= 0 );
    if ( sizeof( ImDrawIdx ) == 2 && (_VtxCurrentIdx + vtx_count >= (1 << 16)) )
    {
        _VtxCurrentOffset = VtxBuffer.Size;
        _VtxCurrentIdx = 0;
        AddDrawCmd();
    }

    ImDrawCmd& draw_cmd = CmdBuffer.Data[CmdBuffer.Size - 1];
    draw_cmd.ElemCount += idx_count;

    int vtx_buffer_old_size = VtxBuffer.Size;
    VtxBuffer.resize( vtx_buffer_old_size + vtx_count );
    _VtxWritePtr = VtxBuffer.Data + vtx_buffer_old_size;

    int idx_buffer_old_size = IdxBuffer.Size;
    IdxBuffer.resize( idx_buffer_old_size + idx_count );
    _IdxWritePtr = IdxBuffer.Data + idx_buffer_old_size;
}

// Release the a number of reserved vertices/indices from the end of the last reservation made with PrimReserve().
void ImDrawList::PrimUnreserve( int idx_count, int vtx_count )
{
    AN_ASSERT( idx_count >= 0 && vtx_count >= 0 );

    ImDrawCmd& draw_cmd = CmdBuffer.Data[CmdBuffer.Size - 1];
    draw_cmd.ElemCount -= idx_count;
    VtxBuffer.shrink( VtxBuffer.Size - vtx_count );
    IdxBuffer.shrink( IdxBuffer.Size - idx_count );
}

// Fully unrolled with inline call to keep our debug builds decently fast.
void ImDrawList::PrimRect( const ImVec2& a, const ImVec2& c, uint32_t col )
{
    ImVec2 b( c.x, a.y ), d( a.x, c.y ), uv( _Data->TexUvWhitePixel );
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::PrimRectUV( const ImVec2& a, const ImVec2& c, const ImVec2& uv_a, const ImVec2& uv_c, uint32_t col )
{
    ImVec2 b( c.x, a.y ), d( a.x, c.y ), uv_b( uv_c.x, uv_a.y ), uv_d( uv_a.x, uv_c.y );
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv_a; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv_b; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv_c; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv_d; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::PrimQuadUV( const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, const ImVec2& uv_a, const ImVec2& uv_b, const ImVec2& uv_c, const ImVec2& uv_d, uint32_t col )
{
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv_a; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv_b; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv_c; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv_d; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

// On AddPolyline() and AddConvexPolyFilled() we intentionally avoid using ImVec2 and superflous function calls to optimize debug/non-inlined builds.
// Those macros expects l-values.
#define IM_NORMALIZE2F_OVER_ZERO(VX,VY)     { float d2 = VX*VX + VY*VY; if (d2 > 0.0f) { float inv_len = 1.0f / ImSqrt(d2); VX *= inv_len; VY *= inv_len; } }
#define IM_FIXNORMAL2F(VX,VY)               { float d2 = VX*VX + VY*VY; if (d2 < 0.5f) d2 = 0.5f; float inv_lensq = 1.0f / d2; VX *= inv_lensq; VY *= inv_lensq; }

// TODO: Thickness anti-aliased lines cap are missing their AA fringe.
// We avoid using the ImVec2 math operators here to reduce cost to a minimum for debug/non-inlined builds.
void ImDrawList::AddPolyline( const ImVec2* points, const int points_count, uint32_t col, bool closed, float thickness )
{
    if ( points_count < 2 )
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;

    int count = points_count;
    if ( !closed )
        count = points_count-1;

    const bool thick_line = thickness > 1.0f;
    if ( Flags & ImDrawListFlags_AntiAliasedLines )
    {
        // Anti-aliased stroke
        const float AA_SIZE = 1.0f;
        const uint32_t col_trans = col & ~IM_COL32_A_MASK;

        const int idx_count = thick_line ? count*18 : count*12;
        const int vtx_count = thick_line ? points_count*4 : points_count*3;
        PrimReserve( idx_count, vtx_count );

        // Temporary buffer
        ImVec2* temp_normals = (ImVec2*)StackAlloc( points_count * (thick_line ? 5 : 3) * sizeof( ImVec2 ) ); //-V630
        ImVec2* temp_points = temp_normals + points_count;

        for ( int i1 = 0; i1 < count; i1++ )
        {
            const int i2 = (i1+1) == points_count ? 0 : i1+1;
            float dx = points[i2].x - points[i1].x;
            float dy = points[i2].y - points[i1].y;
            IM_NORMALIZE2F_OVER_ZERO( dx, dy );
            temp_normals[i1].x = dy;
            temp_normals[i1].y = -dx;
        }
        if ( !closed )
            temp_normals[points_count-1] = temp_normals[points_count-2];

        if ( !thick_line )
        {
            if ( !closed )
            {
                temp_points[0] = points[0] + temp_normals[0] * AA_SIZE;
                temp_points[1] = points[0] - temp_normals[0] * AA_SIZE;
                temp_points[(points_count-1)*2+0] = points[points_count-1] + temp_normals[points_count-1] * AA_SIZE;
                temp_points[(points_count-1)*2+1] = points[points_count-1] - temp_normals[points_count-1] * AA_SIZE;
            }

            // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
            unsigned int idx1 = _VtxCurrentIdx;
            for ( int i1 = 0; i1 < count; i1++ )
            {
                const int i2 = (i1+1) == points_count ? 0 : i1+1;
                unsigned int idx2 = (i1+1) == points_count ? _VtxCurrentIdx : idx1+3;

                // Average normals
                float dm_x = (temp_normals[i1].x + temp_normals[i2].x) * 0.5f;
                float dm_y = (temp_normals[i1].y + temp_normals[i2].y) * 0.5f;
                IM_FIXNORMAL2F( dm_x, dm_y )
                    dm_x *= AA_SIZE;
                dm_y *= AA_SIZE;

                // Add temporary vertexes
                ImVec2* out_vtx = &temp_points[i2*2];
                out_vtx[0].x = points[i2].x + dm_x;
                out_vtx[0].y = points[i2].y + dm_y;
                out_vtx[1].x = points[i2].x - dm_x;
                out_vtx[1].y = points[i2].y - dm_y;

                // Add indexes
                _IdxWritePtr[0] = (ImDrawIdx)(idx2+0); _IdxWritePtr[1] = (ImDrawIdx)(idx1+0); _IdxWritePtr[2] = (ImDrawIdx)(idx1+2);
                _IdxWritePtr[3] = (ImDrawIdx)(idx1+2); _IdxWritePtr[4] = (ImDrawIdx)(idx2+2); _IdxWritePtr[5] = (ImDrawIdx)(idx2+0);
                _IdxWritePtr[6] = (ImDrawIdx)(idx2+1); _IdxWritePtr[7] = (ImDrawIdx)(idx1+1); _IdxWritePtr[8] = (ImDrawIdx)(idx1+0);
                _IdxWritePtr[9] = (ImDrawIdx)(idx1+0); _IdxWritePtr[10] = (ImDrawIdx)(idx2+0); _IdxWritePtr[11] = (ImDrawIdx)(idx2+1);
                _IdxWritePtr += 12;

                idx1 = idx2;
            }

            // Add vertexes
            for ( int i = 0; i < points_count; i++ )
            {
                _VtxWritePtr[0].pos = points[i];          _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
                _VtxWritePtr[1].pos = temp_points[i*2+0]; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;
                _VtxWritePtr[2].pos = temp_points[i*2+1]; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col_trans;
                _VtxWritePtr += 3;
            }
        } else
        {
            const float half_inner_thickness = (thickness - AA_SIZE) * 0.5f;
            if ( !closed )
            {
                temp_points[0] = points[0] + temp_normals[0] * (half_inner_thickness + AA_SIZE);
                temp_points[1] = points[0] + temp_normals[0] * (half_inner_thickness);
                temp_points[2] = points[0] - temp_normals[0] * (half_inner_thickness);
                temp_points[3] = points[0] - temp_normals[0] * (half_inner_thickness + AA_SIZE);
                temp_points[(points_count-1)*4+0] = points[points_count-1] + temp_normals[points_count-1] * (half_inner_thickness + AA_SIZE);
                temp_points[(points_count-1)*4+1] = points[points_count-1] + temp_normals[points_count-1] * (half_inner_thickness);
                temp_points[(points_count-1)*4+2] = points[points_count-1] - temp_normals[points_count-1] * (half_inner_thickness);
                temp_points[(points_count-1)*4+3] = points[points_count-1] - temp_normals[points_count-1] * (half_inner_thickness + AA_SIZE);
            }

            // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
            unsigned int idx1 = _VtxCurrentIdx;
            for ( int i1 = 0; i1 < count; i1++ )
            {
                const int i2 = (i1+1) == points_count ? 0 : i1+1;
                unsigned int idx2 = (i1+1) == points_count ? _VtxCurrentIdx : idx1+4;

                // Average normals
                float dm_x = (temp_normals[i1].x + temp_normals[i2].x) * 0.5f;
                float dm_y = (temp_normals[i1].y + temp_normals[i2].y) * 0.5f;
                IM_FIXNORMAL2F( dm_x, dm_y );
                float dm_out_x = dm_x * (half_inner_thickness + AA_SIZE);
                float dm_out_y = dm_y * (half_inner_thickness + AA_SIZE);
                float dm_in_x = dm_x * half_inner_thickness;
                float dm_in_y = dm_y * half_inner_thickness;

                // Add temporary vertexes
                ImVec2* out_vtx = &temp_points[i2*4];
                out_vtx[0].x = points[i2].x + dm_out_x;
                out_vtx[0].y = points[i2].y + dm_out_y;
                out_vtx[1].x = points[i2].x + dm_in_x;
                out_vtx[1].y = points[i2].y + dm_in_y;
                out_vtx[2].x = points[i2].x - dm_in_x;
                out_vtx[2].y = points[i2].y - dm_in_y;
                out_vtx[3].x = points[i2].x - dm_out_x;
                out_vtx[3].y = points[i2].y - dm_out_y;

                // Add indexes
                _IdxWritePtr[0] = (ImDrawIdx)(idx2+1); _IdxWritePtr[1] = (ImDrawIdx)(idx1+1); _IdxWritePtr[2] = (ImDrawIdx)(idx1+2);
                _IdxWritePtr[3] = (ImDrawIdx)(idx1+2); _IdxWritePtr[4] = (ImDrawIdx)(idx2+2); _IdxWritePtr[5] = (ImDrawIdx)(idx2+1);
                _IdxWritePtr[6] = (ImDrawIdx)(idx2+1); _IdxWritePtr[7] = (ImDrawIdx)(idx1+1); _IdxWritePtr[8] = (ImDrawIdx)(idx1+0);
                _IdxWritePtr[9] = (ImDrawIdx)(idx1+0); _IdxWritePtr[10] = (ImDrawIdx)(idx2+0); _IdxWritePtr[11] = (ImDrawIdx)(idx2+1);
                _IdxWritePtr[12] = (ImDrawIdx)(idx2+2); _IdxWritePtr[13] = (ImDrawIdx)(idx1+2); _IdxWritePtr[14] = (ImDrawIdx)(idx1+3);
                _IdxWritePtr[15] = (ImDrawIdx)(idx1+3); _IdxWritePtr[16] = (ImDrawIdx)(idx2+3); _IdxWritePtr[17] = (ImDrawIdx)(idx2+2);
                _IdxWritePtr += 18;

                idx1 = idx2;
            }

            // Add vertexes
            for ( int i = 0; i < points_count; i++ )
            {
                _VtxWritePtr[0].pos = temp_points[i*4+0]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col_trans;
                _VtxWritePtr[1].pos = temp_points[i*4+1]; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col;
                _VtxWritePtr[2].pos = temp_points[i*4+2]; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col;
                _VtxWritePtr[3].pos = temp_points[i*4+3]; _VtxWritePtr[3].uv = uv; _VtxWritePtr[3].col = col_trans;
                _VtxWritePtr += 4;
            }
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    } else
    {
        // Non Anti-aliased Stroke
        const int idx_count = count*6;
        const int vtx_count = count*4;      // FIXME-OPT: Not sharing edges
        PrimReserve( idx_count, vtx_count );

        for ( int i1 = 0; i1 < count; i1++ )
        {
            const int i2 = (i1+1) == points_count ? 0 : i1+1;
            const ImVec2& p1 = points[i1];
            const ImVec2& p2 = points[i2];

            float dx = p2.x - p1.x;
            float dy = p2.y - p1.y;
            IM_NORMALIZE2F_OVER_ZERO( dx, dy );
            dx *= (thickness * 0.5f);
            dy *= (thickness * 0.5f);

            _VtxWritePtr[0].pos.x = p1.x + dy; _VtxWritePtr[0].pos.y = p1.y - dx; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr[1].pos.x = p2.x + dy; _VtxWritePtr[1].pos.y = p2.y - dx; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col;
            _VtxWritePtr[2].pos.x = p2.x - dy; _VtxWritePtr[2].pos.y = p2.y + dx; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col;
            _VtxWritePtr[3].pos.x = p1.x - dy; _VtxWritePtr[3].pos.y = p1.y + dx; _VtxWritePtr[3].uv = uv; _VtxWritePtr[3].col = col;
            _VtxWritePtr += 4;

            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx+1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx+2);
            _IdxWritePtr[3] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[4] = (ImDrawIdx)(_VtxCurrentIdx+2); _IdxWritePtr[5] = (ImDrawIdx)(_VtxCurrentIdx+3);
            _IdxWritePtr += 6;
            _VtxCurrentIdx += 4;
        }
    }
}

// We intentionally avoid using ImVec2 and its math operators here to reduce cost to a minimum for debug/non-inlined builds.
void ImDrawList::AddConvexPolyFilled( const ImVec2* points, const int points_count, uint32_t col )
{
    if ( points_count < 3 )
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;

    if ( Flags & ImDrawListFlags_AntiAliasedFill )
    {
        // Anti-aliased Fill
        const float AA_SIZE = 1.0f;
        const uint32_t col_trans = col & ~IM_COL32_A_MASK;
        const int idx_count = (points_count-2)*3 + points_count*6;
        const int vtx_count = (points_count*2);
        PrimReserve( idx_count, vtx_count );

        // Add indexes for fill
        unsigned int vtx_inner_idx = _VtxCurrentIdx;
        unsigned int vtx_outer_idx = _VtxCurrentIdx+1;
        for ( int i = 2; i < points_count; i++ )
        {
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx+((i-1)<<1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_inner_idx+(i<<1));
            _IdxWritePtr += 3;
        }

        // Compute normals
        ImVec2* temp_normals = (ImVec2*)StackAlloc( points_count * sizeof( ImVec2 ) ); //-V630
        for ( int i0 = points_count-1, i1 = 0; i1 < points_count; i0 = i1++ )
        {
            const ImVec2& p0 = points[i0];
            const ImVec2& p1 = points[i1];
            float dx = p1.x - p0.x;
            float dy = p1.y - p0.y;
            IM_NORMALIZE2F_OVER_ZERO( dx, dy );
            temp_normals[i0].x = dy;
            temp_normals[i0].y = -dx;
        }

        for ( int i0 = points_count-1, i1 = 0; i1 < points_count; i0 = i1++ )
        {
            // Average normals
            const ImVec2& n0 = temp_normals[i0];
            const ImVec2& n1 = temp_normals[i1];
            float dm_x = (n0.x + n1.x) * 0.5f;
            float dm_y = (n0.y + n1.y) * 0.5f;
            IM_FIXNORMAL2F( dm_x, dm_y );
            dm_x *= AA_SIZE * 0.5f;
            dm_y *= AA_SIZE * 0.5f;

            // Add vertices
            _VtxWritePtr[0].pos.x = (points[i1].x - dm_x); _VtxWritePtr[0].pos.y = (points[i1].y - dm_y); _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;        // Inner
            _VtxWritePtr[1].pos.x = (points[i1].x + dm_x); _VtxWritePtr[1].pos.y = (points[i1].y + dm_y); _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;  // Outer
            _VtxWritePtr += 2;

            // Add indexes for fringes
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx+(i1<<1)); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx+(i0<<1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_outer_idx+(i0<<1));
            _IdxWritePtr[3] = (ImDrawIdx)(vtx_outer_idx+(i0<<1)); _IdxWritePtr[4] = (ImDrawIdx)(vtx_outer_idx+(i1<<1)); _IdxWritePtr[5] = (ImDrawIdx)(vtx_inner_idx+(i1<<1));
            _IdxWritePtr += 6;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    } else
    {
        // Non Anti-aliased Fill
        const int idx_count = (points_count-2)*3;
        const int vtx_count = points_count;
        PrimReserve( idx_count, vtx_count );
        for ( int i = 0; i < vtx_count; i++ )
        {
            _VtxWritePtr[0].pos = points[i]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr++;
        }
        for ( int i = 2; i < points_count; i++ )
        {
            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx+i-1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx+i);
            _IdxWritePtr += 3;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
}

void ImDrawList::PathArcToFast( const ImVec2& center, float radius, int a_min_of_12, int a_max_of_12 )
{
    if ( radius == 0.0f || a_min_of_12 > a_max_of_12 )
    {
        _Path.push_back( center );
        return;
    }
    _Path.reserve( _Path.Size + (a_max_of_12 - a_min_of_12 + 1) );
    for ( int a = a_min_of_12; a <= a_max_of_12; a++ )
    {
        const ImVec2& c = _Data->CircleVtx12[a % AN_ARRAY_SIZE( _Data->CircleVtx12 )];
        _Path.push_back( ImVec2( center.x + c.x * radius, center.y + c.y * radius ) );
    }
}

void ImDrawList::PathArcTo( const ImVec2& center, float radius, float a_min, float a_max, int num_segments )
{
    if ( radius == 0.0f )
    {
        _Path.push_back( center );
        return;
    }

    // Note that we are adding a point at both a_min and a_max.
    // If you are trying to draw a full closed circle you don't want the overlapping points!
    _Path.reserve( _Path.Size + (num_segments + 1) );
    for ( int i = 0; i <= num_segments; i++ )
    {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        _Path.push_back( ImVec2( center.x + ImCos( a ) * radius, center.y + ImSin( a ) * radius ) );
    }
}

ImVec2 ImBezierCalc( const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, float t )
{
    float u = 1.0f - t;
    float w1 = u*u*u;
    float w2 = 3*u*u*t;
    float w3 = 3*u*t*t;
    float w4 = t*t*t;
    return ImVec2( w1*p1.x + w2*p2.x + w3*p3.x + w4*p4.x, w1*p1.y + w2*p2.y + w3*p3.y + w4*p4.y );
}

// Closely mimics BezierClosestPointCasteljauStep() in imgui.cpp
static void PathBezierToCasteljau( ImVector<ImVec2>* path, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float tess_tol, int level )
{
    float dx = x4 - x1;
    float dy = y4 - y1;
    float d2 = ((x2 - x4) * dy - (y2 - y4) * dx);
    float d3 = ((x3 - x4) * dy - (y3 - y4) * dx);
    d2 = (d2 >= 0) ? d2 : -d2;
    d3 = (d3 >= 0) ? d3 : -d3;
    if ( (d2+d3) * (d2+d3) < tess_tol * (dx*dx + dy*dy) )
    {
        path->push_back( ImVec2( x4, y4 ) );
    } else if ( level < 10 )
    {
        float x12 = (x1+x2)*0.5f, y12 = (y1+y2)*0.5f;
        float x23 = (x2+x3)*0.5f, y23 = (y2+y3)*0.5f;
        float x34 = (x3+x4)*0.5f, y34 = (y3+y4)*0.5f;
        float x123 = (x12+x23)*0.5f, y123 = (y12+y23)*0.5f;
        float x234 = (x23+x34)*0.5f, y234 = (y23+y34)*0.5f;
        float x1234 = (x123+x234)*0.5f, y1234 = (y123+y234)*0.5f;
        PathBezierToCasteljau( path, x1, y1, x12, y12, x123, y123, x1234, y1234, tess_tol, level+1 );
        PathBezierToCasteljau( path, x1234, y1234, x234, y234, x34, y34, x4, y4, tess_tol, level+1 );
    }
}

void ImDrawList::PathBezierCurveTo( const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, int num_segments )
{
    ImVec2 p1 = _Path.back();
    if ( num_segments == 0 )
    {
        PathBezierToCasteljau( &_Path, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, _Data->CurveTessellationTol, 0 ); // Auto-tessellated
    } else
    {
        float t_step = 1.0f / (float)num_segments;
        for ( int i_step = 1; i_step <= num_segments; i_step++ )
            _Path.push_back( ImBezierCalc( p1, p2, p3, p4, t_step * i_step ) );
    }
}

void ImDrawList::PathRect( const ImVec2& a, const ImVec2& b, float rounding, ImDrawCornerFlags rounding_corners )
{
    rounding = ImMin( rounding, ImFabs( b.x - a.x ) * (((rounding_corners & ImDrawCornerFlags_Top)  == ImDrawCornerFlags_Top)  || ((rounding_corners & ImDrawCornerFlags_Bot)   == ImDrawCornerFlags_Bot) ? 0.5f : 1.0f) - 1.0f );
    rounding = ImMin( rounding, ImFabs( b.y - a.y ) * (((rounding_corners & ImDrawCornerFlags_Left) == ImDrawCornerFlags_Left) || ((rounding_corners & ImDrawCornerFlags_Right) == ImDrawCornerFlags_Right) ? 0.5f : 1.0f) - 1.0f );

    if ( rounding <= 0.0f || rounding_corners == 0 )
    {
        PathLineTo( a );
        PathLineTo( ImVec2( b.x, a.y ) );
        PathLineTo( b );
        PathLineTo( ImVec2( a.x, b.y ) );
    } else
    {
        const float rounding_tl = (rounding_corners & ImDrawCornerFlags_TopLeft) ? rounding : 0.0f;
        const float rounding_tr = (rounding_corners & ImDrawCornerFlags_TopRight) ? rounding : 0.0f;
        const float rounding_br = (rounding_corners & ImDrawCornerFlags_BotRight) ? rounding : 0.0f;
        const float rounding_bl = (rounding_corners & ImDrawCornerFlags_BotLeft) ? rounding : 0.0f;
        PathArcToFast( ImVec2( a.x + rounding_tl, a.y + rounding_tl ), rounding_tl, 6, 9 );
        PathArcToFast( ImVec2( b.x - rounding_tr, a.y + rounding_tr ), rounding_tr, 9, 12 );
        PathArcToFast( ImVec2( b.x - rounding_br, b.y - rounding_br ), rounding_br, 0, 3 );
        PathArcToFast( ImVec2( a.x + rounding_bl, b.y - rounding_bl ), rounding_bl, 3, 6 );
    }
}

void ImDrawList::AddLine( const ImVec2& p1, const ImVec2& p2, uint32_t col, float thickness )
{
    if ( (col & IM_COL32_A_MASK) == 0 )
        return;
    PathLineTo( p1 + ImVec2( 0.5f, 0.5f ) );
    PathLineTo( p2 + ImVec2( 0.5f, 0.5f ) );
    PathStroke( col, false, thickness );
}

// p_min = upper-left, p_max = lower-right
// Note we don't render 1 pixels sized rectangles properly.
void ImDrawList::AddRect( const ImVec2& p_min, const ImVec2& p_max, uint32_t col, float rounding, ImDrawCornerFlags rounding_corners, float thickness )
{
    if ( (col & IM_COL32_A_MASK) == 0 )
        return;
    if ( Flags & ImDrawListFlags_AntiAliasedLines )
        PathRect( p_min + ImVec2( 0.50f, 0.50f ), p_max - ImVec2( 0.50f, 0.50f ), rounding, rounding_corners );
    else
        PathRect( p_min + ImVec2( 0.50f, 0.50f ), p_max - ImVec2( 0.49f, 0.49f ), rounding, rounding_corners ); // Better looking lower-right corner and rounded non-AA shapes.
    PathStroke( col, true, thickness );
}

void ImDrawList::AddRectFilled( const ImVec2& p_min, const ImVec2& p_max, uint32_t col, float rounding, ImDrawCornerFlags rounding_corners )
{
    if ( (col & IM_COL32_A_MASK) == 0 )
        return;
    if ( rounding > 0.0f )
    {
        PathRect( p_min, p_max, rounding, rounding_corners );
        PathFillConvex( col );
    } else
    {
        PrimReserve( 6, 4 );
        PrimRect( p_min, p_max, col );
    }
}

// p_min = upper-left, p_max = lower-right
void ImDrawList::AddRectFilledMultiColor( const ImVec2& p_min, const ImVec2& p_max, uint32_t col_upr_left, uint32_t col_upr_right, uint32_t col_bot_right, uint32_t col_bot_left )
{
    if ( ((col_upr_left | col_upr_right | col_bot_right | col_bot_left) & IM_COL32_A_MASK) == 0 )
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;
    PrimReserve( 6, 4 );
    PrimWriteIdx( (ImDrawIdx)(_VtxCurrentIdx) ); PrimWriteIdx( (ImDrawIdx)(_VtxCurrentIdx+1) ); PrimWriteIdx( (ImDrawIdx)(_VtxCurrentIdx+2) );
    PrimWriteIdx( (ImDrawIdx)(_VtxCurrentIdx) ); PrimWriteIdx( (ImDrawIdx)(_VtxCurrentIdx+2) ); PrimWriteIdx( (ImDrawIdx)(_VtxCurrentIdx+3) );
    PrimWriteVtx( p_min, uv, col_upr_left );
    PrimWriteVtx( ImVec2( p_max.x, p_min.y ), uv, col_upr_right );
    PrimWriteVtx( p_max, uv, col_bot_right );
    PrimWriteVtx( ImVec2( p_min.x, p_max.y ), uv, col_bot_left );
}

void ImDrawList::AddQuad( const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, uint32_t col, float thickness )
{
    if ( (col & IM_COL32_A_MASK) == 0 )
        return;

    PathLineTo( p1 );
    PathLineTo( p2 );
    PathLineTo( p3 );
    PathLineTo( p4 );
    PathStroke( col, true, thickness );
}

void ImDrawList::AddQuadFilled( const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, uint32_t col )
{
    if ( (col & IM_COL32_A_MASK) == 0 )
        return;

    PathLineTo( p1 );
    PathLineTo( p2 );
    PathLineTo( p3 );
    PathLineTo( p4 );
    PathFillConvex( col );
}

void ImDrawList::AddTriangle( const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, uint32_t col, float thickness )
{
    if ( (col & IM_COL32_A_MASK) == 0 )
        return;

    PathLineTo( p1 );
    PathLineTo( p2 );
    PathLineTo( p3 );
    PathStroke( col, true, thickness );
}

void ImDrawList::AddTriangleFilled( const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, uint32_t col )
{
    if ( (col & IM_COL32_A_MASK) == 0 )
        return;

    PathLineTo( p1 );
    PathLineTo( p2 );
    PathLineTo( p3 );
    PathFillConvex( col );
}

void ImDrawList::AddCircle( const ImVec2& center, float radius, uint32_t col, int num_segments, float thickness )
{
    if ( (col & IM_COL32_A_MASK) == 0 || radius <= 0.0f )
        return;

    // Obtain segment count
    if ( num_segments <= 0 )
    {
        // Automatic segment count
        const int radius_idx = (int)radius - 1;
        if ( radius_idx < AN_ARRAY_SIZE( _Data->CircleSegmentCounts ) )
            num_segments = _Data->CircleSegmentCounts[radius_idx]; // Use cached value
        else
            num_segments = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC( radius, _Data->CircleSegmentMaxError );
    } else
    {
        // Explicit segment count (still clamp to avoid drawing insanely tessellated shapes)
        num_segments = ImClamp( num_segments, 3, IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_MAX );
    }

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    if ( num_segments == 12 )
        PathArcToFast( center, radius - 0.5f, 0, 12 );
    else
        PathArcTo( center, radius - 0.5f, 0.0f, a_max, num_segments - 1 );
    PathStroke( col, true, thickness );
}

void ImDrawList::AddCircleFilled( const ImVec2& center, float radius, uint32_t col, int num_segments )
{
    if ( (col & IM_COL32_A_MASK) == 0 || radius <= 0.0f )
        return;

    // Obtain segment count
    if ( num_segments <= 0 )
    {
        // Automatic segment count
        const int radius_idx = (int)radius - 1;
        if ( radius_idx < AN_ARRAY_SIZE( _Data->CircleSegmentCounts ) )
            num_segments = _Data->CircleSegmentCounts[radius_idx]; // Use cached value
        else
            num_segments = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC( radius, _Data->CircleSegmentMaxError );
    } else
    {
        // Explicit segment count (still clamp to avoid drawing insanely tessellated shapes)
        num_segments = ImClamp( num_segments, 3, IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_MAX );
    }

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    if ( num_segments == 12 )
        PathArcToFast( center, radius, 0, 12 );
    else
        PathArcTo( center, radius, 0.0f, a_max, num_segments - 1 );
    PathFillConvex( col );
}

// Guaranteed to honor 'num_segments'
void ImDrawList::AddNgon( const ImVec2& center, float radius, uint32_t col, int num_segments, float thickness )
{
    if ( (col & IM_COL32_A_MASK) == 0 || num_segments <= 2 )
        return;

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo( center, radius - 0.5f, 0.0f, a_max, num_segments - 1 );
    PathStroke( col, true, thickness );
}

// Guaranteed to honor 'num_segments'
void ImDrawList::AddNgonFilled( const ImVec2& center, float radius, uint32_t col, int num_segments )
{
    if ( (col & IM_COL32_A_MASK) == 0 || num_segments <= 2 )
        return;

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo( center, radius, 0.0f, a_max, num_segments - 1 );
    PathFillConvex( col );
}

// Cubic Bezier takes 4 controls points
void ImDrawList::AddBezierCurve( const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, uint32_t col, float thickness, int num_segments )
{
    if ( (col & IM_COL32_A_MASK) == 0 )
        return;

    PathLineTo( p1 );
    PathBezierCurveTo( p2, p3, p4, num_segments );
    PathStroke( col, false, thickness );
}

void ImDrawList::AddImage( ImTextureID user_texture_id, const ImVec2& a, const ImVec2& b, const ImVec2& uv_a, const ImVec2& uv_b, uint32_t col, int blend )
{
    if ( (col & IM_COL32_A_MASK) == 0 )
        return;

    const bool push_texture_id = _TextureIdStack.empty() || user_texture_id != _TextureIdStack.back();
    if ( push_texture_id )
        PushTextureID( user_texture_id );

    const bool push_blending_state = _BlendingStack.empty() || blend != _BlendingStack.back();
    if ( push_blending_state )
        PushBlendingState( blend );

    PrimReserve( 6, 4 );
    PrimRectUV( a, b, uv_a, uv_b, col );

    if ( push_blending_state )
        PopBlendingState();

    if ( push_texture_id )
        PopTextureID();
}

void ImDrawList::AddImageQuad( ImTextureID user_texture_id, const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, const ImVec2& uv_a, const ImVec2& uv_b, const ImVec2& uv_c, const ImVec2& uv_d, uint32_t col, int blend )
{
    if ( (col & IM_COL32_A_MASK) == 0 )
        return;

    const bool push_texture_id = _TextureIdStack.empty() || user_texture_id != _TextureIdStack.back();
    if ( push_texture_id )
        PushTextureID( user_texture_id );

    const bool push_blending_state = _BlendingStack.empty() || blend != _BlendingStack.back();
    if ( push_blending_state )
        PushBlendingState( blend );

    PrimReserve( 6, 4 );
    PrimQuadUV( a, b, c, d, uv_a, uv_b, uv_c, uv_d, col );

    if ( push_blending_state )
        PopBlendingState();

    if ( push_texture_id )
        PopTextureID();
}

// Distribute UV over (a, b) rectangle
void ShadeVertsLinearUV( ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, const ImVec2& a, const ImVec2& b, const ImVec2& uv_a, const ImVec2& uv_b, bool clamp )
{
    const ImVec2 size = b - a;
    const ImVec2 uv_size = uv_b - uv_a;
    const ImVec2 scale = ImVec2(
        size.x != 0.0f ? (uv_size.x / size.x) : 0.0f,
        size.y != 0.0f ? (uv_size.y / size.y) : 0.0f );

    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    if ( clamp )
    {
        const ImVec2 min = ImMin( uv_a, uv_b );
        const ImVec2 max = ImMax( uv_a, uv_b );
        for ( ImDrawVert* vertex = vert_start; vertex < vert_end; ++vertex )
            vertex->uv = ImClamp( uv_a + ImMul( ImVec2( vertex->pos.x, vertex->pos.y ) - a, scale ), min, max );
    } else
    {
        for ( ImDrawVert* vertex = vert_start; vertex < vert_end; ++vertex )
            vertex->uv = uv_a + ImMul( ImVec2( vertex->pos.x, vertex->pos.y ) - a, scale );
    }
}

void ImDrawList::AddImageRounded( ImTextureID user_texture_id, const ImVec2& a, const ImVec2& b, const ImVec2& uv_a, const ImVec2& uv_b, uint32_t col, float rounding, int rounding_corners, int blend )
{
    if ( (col & IM_COL32_A_MASK) == 0 )
        return;

    if ( rounding <= 0.0f || (rounding_corners & ImDrawCornerFlags_All) == 0 )
    {
        AddImage( user_texture_id, a, b, uv_a, uv_b, col, blend );
        return;
    }

    const bool push_texture_id = _TextureIdStack.empty() || user_texture_id != _TextureIdStack.back();
    if ( push_texture_id )
        PushTextureID( user_texture_id );

    const bool push_blending_state = _BlendingStack.empty() || blend != _BlendingStack.back();
    if ( push_blending_state )
        PushBlendingState( blend );

    int vert_start_idx = VtxBuffer.Size;
    PathRect( a, b, rounding, rounding_corners );
    PathFillConvex( col );
    int vert_end_idx = VtxBuffer.Size;
    ShadeVertsLinearUV( this, vert_start_idx, vert_end_idx, a, b, uv_a, uv_b, true );

    if ( push_blending_state )
        PopBlendingState();

    if ( push_texture_id )
        PopTextureID();
}

//-----------------------------------------------------------------------------
// ImDrawListSplitter
//-----------------------------------------------------------------------------
// FIXME: This may be a little confusing, trying to be a little too low-level/optimal instead of just doing vector swap..
//-----------------------------------------------------------------------------

void ImDrawListSplitter::ClearFreeMemory()
{
    for ( int i = 0; i < _Channels.Size; i++ )
    {
        if ( i == _Current )
            memset( &_Channels[i], 0, sizeof( _Channels[i] ) );  // Current channel is a copy of CmdBuffer/IdxBuffer, don't destruct again
        _Channels[i]._CmdBuffer.clear();
        _Channels[i]._IdxBuffer.clear();
    }
    _Current = 0;
    _Count = 1;
    _Channels.clear();
}

void ImDrawListSplitter::Split( ImDrawList* draw_list, int channels_count )
{
    AN_ASSERT( _Current == 0 && _Count <= 1 && "Nested channel splitting is not supported. Please use separate instances of ImDrawListSplitter." );
    int old_channels_count = _Channels.Size;
    if ( old_channels_count < channels_count )
        _Channels.resize( channels_count );
    _Count = channels_count;

    // Channels[] (24/32 bytes each) hold storage that we'll swap with draw_list->_CmdBuffer/_IdxBuffer
    // The content of Channels[0] at this point doesn't matter. We clear it to make state tidy in a debugger but we don't strictly need to.
    // When we switch to the next channel, we'll copy draw_list->_CmdBuffer/_IdxBuffer into Channels[0] and then Channels[1] into draw_list->CmdBuffer/_IdxBuffer
    memset( &_Channels[0], 0, sizeof( ImDrawChannel ) );
    for ( int i = 1; i < channels_count; i++ )
    {
        if ( i >= old_channels_count )
        {
            IM_PLACEMENT_NEW( &_Channels[i] ) ImDrawChannel();
        } else
        {
            _Channels[i]._CmdBuffer.resize( 0 );
            _Channels[i]._IdxBuffer.resize( 0 );
        }
        if ( _Channels[i]._CmdBuffer.Size == 0 )
        {
            ImDrawCmd draw_cmd;
            draw_cmd.ClipRect = draw_list->_ClipRectStack.back();
            draw_cmd.TextureId = draw_list->_TextureIdStack.back();
            draw_cmd.BlendingState = draw_list->_BlendingStack.back();
            _Channels[i]._CmdBuffer.push_back( draw_cmd );
        }
    }
}

static inline bool CanMergeDrawCommands( ImDrawCmd* a, ImDrawCmd* b )
{
    return memcmp( &a->ClipRect, &b->ClipRect, sizeof( a->ClipRect ) ) == 0
        && a->TextureId == b->TextureId
        && a->BlendingState == b->BlendingState
        && a->VtxOffset == b->VtxOffset
        && !a->UserCallback && !b->UserCallback;
}

void ImDrawListSplitter::Merge( ImDrawList* draw_list )
{
    // Note that we never use or rely on channels.Size because it is merely a buffer that we never shrink back to 0 to keep all sub-buffers ready for use.
    if ( _Count <= 1 )
        return;

    SetCurrentChannel( draw_list, 0 );
    if ( draw_list->CmdBuffer.Size != 0 && draw_list->CmdBuffer.back().ElemCount == 0 )
        draw_list->CmdBuffer.pop_back();

    // Calculate our final buffer sizes. Also fix the incorrect IdxOffset values in each command.
    int new_cmd_buffer_count = 0;
    int new_idx_buffer_count = 0;
    ImDrawCmd* last_cmd = (_Count > 0 && draw_list->CmdBuffer.Size > 0) ? &draw_list->CmdBuffer.back() : NULL;
    int idx_offset = last_cmd ? last_cmd->IdxOffset + last_cmd->ElemCount : 0;
    for ( int i = 1; i < _Count; i++ )
    {
        ImDrawChannel& ch = _Channels[i];
        if ( ch._CmdBuffer.Size > 0 && ch._CmdBuffer.back().ElemCount == 0 )
            ch._CmdBuffer.pop_back();
        if ( ch._CmdBuffer.Size > 0 && last_cmd != NULL && CanMergeDrawCommands( last_cmd, &ch._CmdBuffer[0] ) )
        {
            // Merge previous channel last draw command with current channel first draw command if matching.
            last_cmd->ElemCount += ch._CmdBuffer[0].ElemCount;
            idx_offset += ch._CmdBuffer[0].ElemCount;
            ch._CmdBuffer.erase( ch._CmdBuffer.Data ); // FIXME-OPT: Improve for multiple merges.
        }
        if ( ch._CmdBuffer.Size > 0 )
            last_cmd = &ch._CmdBuffer.back();
        new_cmd_buffer_count += ch._CmdBuffer.Size;
        new_idx_buffer_count += ch._IdxBuffer.Size;
        for ( int cmd_n = 0; cmd_n < ch._CmdBuffer.Size; cmd_n++ )
        {
            ch._CmdBuffer.Data[cmd_n].IdxOffset = idx_offset;
            idx_offset += ch._CmdBuffer.Data[cmd_n].ElemCount;
        }
    }
    draw_list->CmdBuffer.resize( draw_list->CmdBuffer.Size + new_cmd_buffer_count );
    draw_list->IdxBuffer.resize( draw_list->IdxBuffer.Size + new_idx_buffer_count );

    // Write commands and indices in order (they are fairly small structures, we don't copy vertices only indices)
    ImDrawCmd* cmd_write = draw_list->CmdBuffer.Data + draw_list->CmdBuffer.Size - new_cmd_buffer_count;
    ImDrawIdx* idx_write = draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size - new_idx_buffer_count;
    for ( int i = 1; i < _Count; i++ )
    {
        ImDrawChannel& ch = _Channels[i];
        if ( int sz = ch._CmdBuffer.Size ) { memcpy( cmd_write, ch._CmdBuffer.Data, sz * sizeof( ImDrawCmd ) ); cmd_write += sz; }
        if ( int sz = ch._IdxBuffer.Size ) { memcpy( idx_write, ch._IdxBuffer.Data, sz * sizeof( ImDrawIdx ) ); idx_write += sz; }
    }
    draw_list->_IdxWritePtr = idx_write;
    draw_list->UpdateClipRect(); // We call this instead of AddDrawCmd(), so that empty channels won't produce an extra draw call.
    draw_list->UpdateTextureID();
    _Count = 1;
}

void ImDrawListSplitter::SetCurrentChannel( ImDrawList* draw_list, int idx )
{
    AN_ASSERT( idx >= 0 && idx < _Count );
    if ( _Current == idx )
        return;
    // Overwrite ImVector (12/16 bytes), four times. This is merely a silly optimization instead of doing .swap()
    memcpy( &_Channels.Data[_Current]._CmdBuffer, &draw_list->CmdBuffer, sizeof( draw_list->CmdBuffer ) );
    memcpy( &_Channels.Data[_Current]._IdxBuffer, &draw_list->IdxBuffer, sizeof( draw_list->IdxBuffer ) );
    _Current = idx;
    memcpy( &draw_list->CmdBuffer, &_Channels.Data[idx]._CmdBuffer, sizeof( draw_list->CmdBuffer ) );
    memcpy( &draw_list->IdxBuffer, &_Channels.Data[idx]._IdxBuffer, sizeof( draw_list->IdxBuffer ) );
    draw_list->_IdxWritePtr = draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size;
}
