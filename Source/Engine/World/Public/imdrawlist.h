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

#pragma once

#include <Core/Public/CoreMath.h>

class AFont;

#define IM_ALLOC(_SIZE)                     malloc(_SIZE)
#define IM_FREE(_PTR)                       free(_PTR)

typedef int ImDrawCornerFlags;      // -> enum ImDrawCornerFlags_    // Flags: for ImDrawList::AddRect(), AddRectFilled() etc.
enum ImDrawCornerFlags_
{
    ImDrawCornerFlags_None = 0,
    ImDrawCornerFlags_TopLeft = 1 << 0, // 0x1
    ImDrawCornerFlags_TopRight = 1 << 1, // 0x2
    ImDrawCornerFlags_BotLeft = 1 << 2, // 0x4
    ImDrawCornerFlags_BotRight = 1 << 3, // 0x8
    ImDrawCornerFlags_Top = ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_TopRight,   // 0x3
    ImDrawCornerFlags_Bot = ImDrawCornerFlags_BotLeft | ImDrawCornerFlags_BotRight,   // 0xC
    ImDrawCornerFlags_Left = ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_BotLeft,    // 0x5
    ImDrawCornerFlags_Right = ImDrawCornerFlags_TopRight | ImDrawCornerFlags_BotRight,  // 0xA
    ImDrawCornerFlags_All = 0xF     // In your function calls you may use ~0 (= all bits sets) instead of ImDrawCornerFlags_All, as a convenience
};

#ifndef ImTextureID
typedef void* ImTextureID;          // User data to identify a texture (this is whatever to you want it to be! read the FAQ about ImTextureID in imgui.cpp)
#endif

#define IM_VEC2_CLASS_EXTRA                                                 \
        ImVec2(const Float2& f) { x = f.X; y = f.Y; }                       \
        operator Float2() const { return Float2(x,y); }


#define IM_VEC4_CLASS_EXTRA                                                 \
        ImVec4(const Float4& f) { x = f.X; y = f.Y; z = f.Z; w = f.W; }     \
        operator Float4() const { return Float4(x,y,z,w); }

struct ImVec2
{
    float     x, y;
    ImVec2() { x = y = 0.0f; }
    ImVec2( float _x, float _y ) { x = _x; y = _y; }
    float  operator[] ( size_t idx ) const { AN_ASSERT( idx <= 1 ); return (&x)[idx]; }    // We very rarely use this [] operator, the assert overhead is fine.
    float& operator[] ( size_t idx ) { AN_ASSERT( idx <= 1 ); return (&x)[idx]; }    // We very rarely use this [] operator, the assert overhead is fine.
#ifdef IM_VEC2_CLASS_EXTRA
    IM_VEC2_CLASS_EXTRA     // Define additional constructors and implicit cast operators in imconfig.h to convert back and forth between your math types and ImVec2.
#endif
};

struct ImVec4
{
    float     x, y, z, w;
    ImVec4() { x = y = z = w = 0.0f; }
    ImVec4( float _x, float _y, float _z, float _w ) { x = _x; y = _y; z = _z; w = _w; }
#ifdef IM_VEC4_CLASS_EXTRA
    IM_VEC4_CLASS_EXTRA     // Define additional constructors and implicit cast operators in imconfig.h to convert back and forth between your math types and ImVec4.
#endif
};

//-----------------------------------------------------------------------------
// Helper: ImVector<>
// Lightweight std::vector<>-like class to avoid dragging dependencies (also, some implementations of STL with debug enabled are absurdly slow, we bypass it so our code runs fast in debug).
//-----------------------------------------------------------------------------
// - You generally do NOT need to care or use this ever. But we need to make it available in imgui.h because some of our public structures are relying on it.
// - We use std-like naming convention here, which is a little unusual for this codebase.
// - Important: clear() frees memory, resize(0) keep the allocated buffer. We use resize(0) a lot to intentionally recycle allocated buffers across frames and amortize our costs.
// - Important: our implementation does NOT call C++ constructors/destructors, we treat everything as raw data! This is intentional but be extra mindful of that,
//   Do NOT use this class as a std::vector replacement in your own code! Many of the structures used by dear imgui can be safely initialized by a zero-memset.
//-----------------------------------------------------------------------------

template<typename T>
struct ImVector
{
    int                 Size;
    int                 Capacity;
    T*                  Data;

    // Provide standard typedefs but we don't use them ourselves.
    typedef T                   value_type;
    typedef value_type*         iterator;
    typedef const value_type*   const_iterator;

    // Constructors, destructor
    inline ImVector() { Size = Capacity = 0; Data = NULL; }
    inline ImVector( const ImVector<T>& src ) { Size = Capacity = 0; Data = NULL; operator=( src ); }
    inline ImVector<T>& operator=( const ImVector<T>& src ) { clear(); resize( src.Size ); memcpy( Data, src.Data, (size_t)Size * sizeof( T ) ); return *this; }
    inline ~ImVector() { if ( Data ) IM_FREE( Data ); }

    inline bool         empty() const { return Size == 0; }
    inline int          size() const { return Size; }
    inline int          size_in_bytes() const { return Size * (int)sizeof( T ); }
    inline int          capacity() const { return Capacity; }
    inline T&           operator[]( int i ) { AN_ASSERT( i < Size ); return Data[i]; }
    inline const T&     operator[]( int i ) const { AN_ASSERT( i < Size ); return Data[i]; }

    inline void         clear() { if ( Data ) { Size = Capacity = 0; IM_FREE( Data ); Data = NULL; } }
    inline T*           begin() { return Data; }
    inline const T*     begin() const { return Data; }
    inline T*           end() { return Data + Size; }
    inline const T*     end() const { return Data + Size; }
    inline T&           front() { AN_ASSERT( Size > 0 ); return Data[0]; }
    inline const T&     front() const { AN_ASSERT( Size > 0 ); return Data[0]; }
    inline T&           back() { AN_ASSERT( Size > 0 ); return Data[Size - 1]; }
    inline const T&     back() const { AN_ASSERT( Size > 0 ); return Data[Size - 1]; }
    inline void         swap( ImVector<T>& rhs ) { int rhs_size = rhs.Size; rhs.Size = Size; Size = rhs_size; int rhs_cap = rhs.Capacity; rhs.Capacity = Capacity; Capacity = rhs_cap; T* rhs_data = rhs.Data; rhs.Data = Data; Data = rhs_data; }

    inline int          _grow_capacity( int sz ) const { int new_capacity = Capacity ? (Capacity + Capacity/2) : 8; return new_capacity > sz ? new_capacity : sz; }
    inline void         resize( int new_size ) { if ( new_size > Capacity ) reserve( _grow_capacity( new_size ) ); Size = new_size; }
    inline void         resize( int new_size, const T& v ) { if ( new_size > Capacity ) reserve( _grow_capacity( new_size ) ); if ( new_size > Size ) for ( int n = Size; n < new_size; n++ ) memcpy( &Data[n], &v, sizeof( v ) ); Size = new_size; }
    inline void         shrink( int new_size ) { AN_ASSERT( new_size <= Size ); Size = new_size; } // Resize a vector to a smaller size, guaranteed not to cause a reallocation
    inline void         reserve( int new_capacity ) { if ( new_capacity <= Capacity ) return; T* new_data = (T*)IM_ALLOC( (size_t)new_capacity * sizeof( T ) ); if ( Data ) { memcpy( new_data, Data, (size_t)Size * sizeof( T ) ); IM_FREE( Data ); } Data = new_data; Capacity = new_capacity; }

    // NB: It is illegal to call push_back/push_front/insert with a reference pointing inside the ImVector data itself! e.g. v.push_back(v[10]) is forbidden.
    inline void         push_back( const T& v ) { if ( Size == Capacity ) reserve( _grow_capacity( Size + 1 ) ); memcpy( &Data[Size], &v, sizeof( v ) ); Size++; }
    inline void         pop_back() { AN_ASSERT( Size > 0 ); Size--; }
    inline void         push_front( const T& v ) { if ( Size == 0 ) push_back( v ); else insert( Data, v ); }
    inline T*           erase( const T* it ) { AN_ASSERT( it >= Data && it < Data+Size ); const ptrdiff_t off = it - Data; memmove( Data + off, Data + off + 1, ((size_t)Size - (size_t)off - 1) * sizeof( T ) ); Size--; return Data + off; }
    inline T*           erase( const T* it, const T* it_last ) { AN_ASSERT( it >= Data && it < Data+Size && it_last > it && it_last <= Data+Size ); const ptrdiff_t count = it_last - it; const ptrdiff_t off = it - Data; memmove( Data + off, Data + off + count, ((size_t)Size - (size_t)off - count) * sizeof( T ) ); Size -= (int)count; return Data + off; }
    inline T*           erase_unsorted( const T* it ) { AN_ASSERT( it >= Data && it < Data+Size );  const ptrdiff_t off = it - Data; if ( it < Data+Size-1 ) memcpy( Data + off, Data + Size - 1, sizeof( T ) ); Size--; return Data + off; }
    inline T*           insert( const T* it, const T& v ) { AN_ASSERT( it >= Data && it <= Data+Size ); const ptrdiff_t off = it - Data; if ( Size == Capacity ) reserve( _grow_capacity( Size + 1 ) ); if ( off < (int)Size ) memmove( Data + off + 1, Data + off, ((size_t)Size - (size_t)off) * sizeof( T ) ); memcpy( &Data[off], &v, sizeof( v ) ); Size++; return Data + off; }
    inline bool         contains( const T& v ) const { const T* data = Data;  const T* data_end = Data + Size; while ( data < data_end ) if ( *data++ == v ) return true; return false; }
    inline T*           find( const T& v ) { T* data = Data;  const T* data_end = Data + Size; while ( data < data_end ) if ( *data == v ) break; else ++data; return data; }
    inline const T*     find( const T& v ) const { const T* data = Data;  const T* data_end = Data + Size; while ( data < data_end ) if ( *data == v ) break; else ++data; return data; }
    inline bool         find_erase( const T& v ) { const T* it = find( v ); if ( it < Data + Size ) { erase( it ); return true; } return false; }
    inline bool         find_erase_unsorted( const T& v ) { const T* it = find( v ); if ( it < Data + Size ) { erase_unsorted( it ); return true; } return false; }
    inline int          index_from_ptr( const T* it ) const { AN_ASSERT( it >= Data && it < Data + Size ); const ptrdiff_t off = it - Data; return (int)off; }
};

typedef int ImDrawListFlags;        // -> enum ImDrawListFlags_      // Flags: for ImDrawList

                                    // Data shared between all ImDrawList instances
                                    // You may want to create your own instance of this if you want to use ImDrawList completely without ImGui. In that case, watch out for future changes to this structure.
struct ImDrawListSharedData
{
    ImVec2          TexUvWhitePixel;            // UV of white pixel in the atlas
    AFont const *   Font;                       // Current/default font (optional, for simplified AddText overload)
    float           FontSize;                   // Current/default font size (optional, for simplified AddText overload)
    float           CurveTessellationTol;       // Tessellation tolerance when using PathBezierCurveTo()
    float           CircleSegmentMaxError;      // Number of circle segments to use per pixel of radius for AddCircle() etc
    ImVec4          ClipRectFullscreen;         // Value for PushClipRectFullscreen()
    ImDrawListFlags InitialFlags;               // Initial flags at the beginning of the frame (it is possible to alter flags on a per-drawlist basis afterwards)

                                                // [Internal] Lookup tables
    ImVec2          CircleVtx12[12];            // FIXME: Bake rounded corners fill/borders in atlas
    uint8_t         CircleSegmentCounts[64];    // Precomputed segment count for given radius (array index + 1) before we calculate it dynamically (to avoid calculation overhead)

    ImDrawListSharedData();
    void SetCircleSegmentMaxError( float max_error );
};

struct ImDrawList;
struct ImDrawCmd;

#ifndef ImDrawCallback
typedef void ( *ImDrawCallback )(const ImDrawList* parent_list, const ImDrawCmd* cmd);
#endif

// Typically, 1 command = 1 GPU draw call (unless command is a callback)
struct ImDrawCmd
{
    unsigned int    ElemCount;              // Number of indices (multiple of 3) to be rendered as triangles. Vertices are stored in the callee ImDrawList's vtx_buffer[] array, indices in idx_buffer[].
    ImVec4          ClipRect;               // Clipping rectangle (x1, y1, x2, y2). Subtract ImDrawData->DisplayPos to get clipping rectangle in "viewport" coordinates
    ImTextureID     TextureId;              // User-provided texture ID. Set by user in ImfontAtlas::SetTexID() for fonts or passed to Image*() functions. Ignore if never using images or multiple fonts atlas.
    unsigned int    VtxOffset;              // Start offset in vertex buffer. Pre-1.71 or without ImGuiBackendFlags_RendererHasVtxOffset: always 0. With ImGuiBackendFlags_RendererHasVtxOffset: may be >0 to support meshes larger than 64K vertices with 16-bit indices.
    unsigned int    IdxOffset;              // Start offset in index buffer. Always equal to sum of ElemCount drawn so far.
    ImDrawCallback  UserCallback;           // If != NULL, call the function instead of rendering the vertices. clip_rect and texture_id will be set normally.
    void*           UserCallbackData;       // The draw callback code can access this.
    uint32_t        BlendingState;

    ImDrawCmd() { ElemCount = 0; TextureId = (ImTextureID)NULL; VtxOffset = IdxOffset = 0;  UserCallback = NULL; UserCallbackData = NULL; BlendingState = 0; }
};

// Vertex index
// (to allow large meshes with 16-bit indices: set 'io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset' and handle ImDrawCmd::VtxOffset in the renderer back-end)
// (to use 32-bit indices: override with '#define ImDrawIdx unsigned int' in imconfig.h)
#ifndef ImDrawIdx
typedef unsigned short ImDrawIdx;
#endif

struct ImDrawVert
{
    ImVec2  pos;
    ImVec2  uv;
    uint32_t col;
};

typedef int ImDrawListFlags;        // -> enum ImDrawListFlags_      // Flags: for ImDrawList
enum ImDrawListFlags_
{
    ImDrawListFlags_None = 0,
    ImDrawListFlags_AntiAliasedLines = 1 << 0,  // Lines are anti-aliased (*2 the number of triangles for 1.0f wide line, otherwise *3 the number of triangles)
    ImDrawListFlags_AntiAliasedFill = 1 << 1,  // Filled shapes have anti-aliased edges (*2 the number of vertices)
};

// For use by ImDrawListSplitter.
struct ImDrawChannel
{
    ImVector<ImDrawCmd>         _CmdBuffer;
    ImVector<ImDrawIdx>         _IdxBuffer;
};

// Split/Merge functions are used to split the draw list into different layers which can be drawn into out of order.
// This is used by the Columns api, so items of each column can be batched together in a same draw call.
struct ImDrawListSplitter
{
    int                         _Current;    // Current channel number (0)
    int                         _Count;      // Number of active channels (1+)
    ImVector<ImDrawChannel>     _Channels;   // Draw channels (not resized down so _Count might be < Channels.Size)

    inline ImDrawListSplitter() { Clear(); }
    inline ~ImDrawListSplitter() { ClearFreeMemory(); }
    inline void                 Clear() { _Current = 0; _Count = 1; } // Do not clear Channels[] so our allocations are reused next frame
    void              ClearFreeMemory();
    void              Split( ImDrawList* draw_list, int count );
    void              Merge( ImDrawList* draw_list );
    void              SetCurrentChannel( ImDrawList* draw_list, int channel_idx );
};

// Draw command list
// This is the low-level list of polygons that ImGui:: functions are filling. At the end of the frame,
// all command lists are passed to your ImGuiIO::RenderDrawListFn function for rendering.
// Each dear imgui window contains its own ImDrawList. You can use ImGui::GetWindowDrawList() to
// access the current window draw list and draw custom primitives.
// You can interleave normal ImGui:: calls and adding primitives to the current draw list.
// All positions are generally in pixel coordinates (top-left at (0,0), bottom-right at io.DisplaySize), but you are totally free to apply whatever transformation matrix to want to the data (if you apply such transformation you'll want to apply it to ClipRect as well)
// Important: Primitives are always added to the list and not culled (culling is done at higher-level by ImGui:: functions), if you use this API a lot consider coarse culling your drawn objects.
struct ImDrawList
{
    // This is what you have to render
    ImVector<ImDrawCmd>     CmdBuffer;          // Draw commands. Typically 1 command = 1 GPU draw call, unless the command is a callback.
    ImVector<ImDrawIdx>     IdxBuffer;          // Index buffer. Each command consume ImDrawCmd::ElemCount of those
    ImVector<ImDrawVert>    VtxBuffer;          // Vertex buffer.
    ImDrawListFlags         Flags;              // Flags, you may poke into these to adjust anti-aliasing settings per-primitive.

                                                // [Internal, used while building lists]
    const ImDrawListSharedData* _Data;          // Pointer to shared draw data (you can use ImGui::GetDrawListSharedData() to get the one from current ImGui context)
    const char*             _OwnerName;         // Pointer to owner window's name for debugging
    unsigned int            _VtxCurrentOffset;  // [Internal] Always 0 unless 'Flags & ImDrawListFlags_AllowVtxOffset'.
    unsigned int            _VtxCurrentIdx;     // [Internal] Generally == VtxBuffer.Size unless we are past 64K vertices, in which case this gets reset to 0.
    ImDrawVert*             _VtxWritePtr;       // [Internal] point within VtxBuffer.Data after each add command (to avoid using the ImVector<> operators too much)
    ImDrawIdx*              _IdxWritePtr;       // [Internal] point within IdxBuffer.Data after each add command (to avoid using the ImVector<> operators too much)
    ImVector<ImVec4>        _ClipRectStack;     // [Internal]
    ImVector<ImTextureID>   _TextureIdStack;    // [Internal]
    ImVector<uint32_t>      _BlendingStack;
    ImVector<ImVec2>        _Path;              // [Internal] current path building
    ImDrawListSplitter      _Splitter;          // [Internal] for channels api

                                                // If you want to create ImDrawList instances, pass them ImGui::GetDrawListSharedData() or create and use your own ImDrawListSharedData (so you can use ImDrawList without ImGui)
    ImDrawList( const ImDrawListSharedData* shared_data ) { _Data = shared_data; _OwnerName = NULL; Clear(); }
    ~ImDrawList() { ClearFreeMemory(); }
    void  PushClipRect( ImVec2 clip_rect_min, ImVec2 clip_rect_max, bool intersect_with_current_clip_rect = false );  // Render-level scissoring. This is passed down to your render function but not used for CPU-side coarse clipping. Prefer using higher-level ImGui::PushClipRect() to affect logic (hit-testing and widget culling)
    void  PushClipRectFullScreen();
    void  PopClipRect();
    void  PushBlendingState( uint32_t blending );
    void  PopBlendingState();
    void  PushTextureID( ImTextureID texture_id );
    void  PopTextureID();
    inline ImVec2   GetClipRectMin() const { const ImVec4& cr = _ClipRectStack.back(); return ImVec2( cr.x, cr.y ); }
    inline ImVec2   GetClipRectMax() const { const ImVec4& cr = _ClipRectStack.back(); return ImVec2( cr.z, cr.w ); }

    // Primitives
    // - For rectangular primitives, "p_min" and "p_max" represent the upper-left and lower-right corners.
    // - For circle primitives, use "num_segments == 0" to automatically calculate tessellation (preferred). 
    //   In future versions we will use textures to provide cheaper and higher-quality circles. 
    //   Use AddNgon() and AddNgonFilled() functions if you need to guaranteed a specific number of sides.
    void  AddLine( const ImVec2& p1, const ImVec2& p2, uint32_t col, float thickness = 1.0f );
    void  AddRect( const ImVec2& p_min, const ImVec2& p_max, uint32_t col, float rounding = 0.0f, ImDrawCornerFlags rounding_corners = ImDrawCornerFlags_All, float thickness = 1.0f );   // a: upper-left, b: lower-right (== upper-left + size), rounding_corners_flags: 4 bits corresponding to which corner to round
    void  AddRectFilled( const ImVec2& p_min, const ImVec2& p_max, uint32_t col, float rounding = 0.0f, ImDrawCornerFlags rounding_corners = ImDrawCornerFlags_All );                     // a: upper-left, b: lower-right (== upper-left + size)
    void  AddRectFilledMultiColor( const ImVec2& p_min, const ImVec2& p_max, uint32_t col_upr_left, uint32_t col_upr_right, uint32_t col_bot_right, uint32_t col_bot_left );
    void  AddQuad( const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, uint32_t col, float thickness = 1.0f );
    void  AddQuadFilled( const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, uint32_t col );
    void  AddTriangle( const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, uint32_t col, float thickness = 1.0f );
    void  AddTriangleFilled( const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, uint32_t col );
    void  AddCircle( const ImVec2& center, float radius, uint32_t col, int num_segments = 12, float thickness = 1.0f );
    void  AddCircleFilled( const ImVec2& center, float radius, uint32_t col, int num_segments = 12 );
    void  AddNgon( const ImVec2& center, float radius, uint32_t col, int num_segments, float thickness = 1.0f );
    void  AddNgonFilled( const ImVec2& center, float radius, uint32_t col, int num_segments );
    void  AddPolyline( const ImVec2* points, int num_points, uint32_t col, bool closed, float thickness );
    void  AddConvexPolyFilled( const ImVec2* points, int num_points, uint32_t col ); // Note: Anti-aliased filling requires points to be in clockwise order.
    void  AddBezierCurve( const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, uint32_t col, float thickness, int num_segments = 0 );

    // Image primitives
    // - Read FAQ to understand what ImTextureID is.
    // - "p_min" and "p_max" represent the upper-left and lower-right corners of the rectangle.
    // - "uv_min" and "uv_max" represent the normalized texture coordinates to use for those corners. Using (0,0)->(1,1) texture coordinates will generally display the entire texture.
    void  AddImage( ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min = ImVec2( 0, 0 ), const ImVec2& uv_max = ImVec2( 1, 1 ), uint32_t col = 0xFFFFFFFF, uint32_t blend = 0 );
    void  AddImageQuad( ImTextureID user_texture_id, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& uv1 = ImVec2( 0, 0 ), const ImVec2& uv2 = ImVec2( 1, 0 ), const ImVec2& uv3 = ImVec2( 1, 1 ), const ImVec2& uv4 = ImVec2( 0, 1 ), uint32_t col = 0xFFFFFFFF, uint32_t blend = 0 );
    void  AddImageRounded( ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max, uint32_t col, float rounding, ImDrawCornerFlags rounding_corners = ImDrawCornerFlags_All, uint32_t blend = 0 );

    // Stateful path API, add points then finish with PathFillConvex() or PathStroke()
    inline    void  PathClear() { _Path.Size = 0; }
    inline    void  PathLineTo( const ImVec2& pos ) { _Path.push_back( pos ); }
    inline    void  PathLineToMergeDuplicate( const ImVec2& pos ) { if ( _Path.Size == 0 || memcmp( &_Path.Data[_Path.Size-1], &pos, 8 ) != 0 ) _Path.push_back( pos ); }
    inline    void  PathFillConvex( uint32_t col ) { AddConvexPolyFilled( _Path.Data, _Path.Size, col ); _Path.Size = 0; }  // Note: Anti-aliased filling requires points to be in clockwise order.
    inline    void  PathStroke( uint32_t col, bool closed, float thickness = 1.0f ) { AddPolyline( _Path.Data, _Path.Size, col, closed, thickness ); _Path.Size = 0; }
    void  PathArcTo( const ImVec2& center, float radius, float a_min, float a_max, int num_segments = 10 );
    void  PathArcToFast( const ImVec2& center, float radius, int a_min_of_12, int a_max_of_12 );                                            // Use precomputed angles for a 12 steps circle
    void  PathBezierCurveTo( const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, int num_segments = 0 );
    void  PathRect( const ImVec2& rect_min, const ImVec2& rect_max, float rounding = 0.0f, ImDrawCornerFlags rounding_corners = ImDrawCornerFlags_All );

    // Advanced
    void  AddCallback( ImDrawCallback callback, void* callback_data );  // Your rendering function must check for 'UserCallback' in ImDrawCmd and call the function instead of rendering triangles.
    void  AddDrawCmd();                                               // This is useful if you need to forcefully create a new draw call (to allow for dependent rendering / blending). Otherwise primitives are merged into the same draw-call as much as possible
    ImDrawList* CloneOutput() const;                                  // Create a clone of the CmdBuffer/IdxBuffer/VtxBuffer.

                                                                      // Advanced: Channels
                                                                      // - Use to split render into layers. By switching channels to can render out-of-order (e.g. submit FG primitives before BG primitives)
                                                                      // - Use to minimize draw calls (e.g. if going back-and-forth between multiple clipping rectangles, prefer to append into separate channels then merge at the end)
                                                                      // - FIXME-OBSOLETE: This API shouldn't have been in ImDrawList in the first place!
                                                                      //   Prefer using your own persistent copy of ImDrawListSplitter as you can stack them.
                                                                      //   Using the ImDrawList::ChannelsXXXX you cannot stack a split over another.
    inline void     ChannelsSplit( int count ) { _Splitter.Split( this, count ); }
    inline void     ChannelsMerge() { _Splitter.Merge( this ); }
    inline void     ChannelsSetCurrent( int n ) { _Splitter.SetCurrentChannel( this, n ); }

    // Internal helpers
    // NB: all primitives needs to be reserved via PrimReserve() beforehand!
    void  Clear();
    void  ClearFreeMemory();
    void  PrimReserve( int idx_count, int vtx_count );
    void  PrimUnreserve( int idx_count, int vtx_count );
    void  PrimRect( const ImVec2& a, const ImVec2& b, uint32_t col );      // Axis aligned rectangle (composed of two triangles)
    void  PrimRectUV( const ImVec2& a, const ImVec2& b, const ImVec2& uv_a, const ImVec2& uv_b, uint32_t col );
    void  PrimQuadUV( const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, const ImVec2& uv_a, const ImVec2& uv_b, const ImVec2& uv_c, const ImVec2& uv_d, uint32_t col );
    inline    void  PrimWriteVtx( const ImVec2& pos, const ImVec2& uv, uint32_t col ) { _VtxWritePtr->pos = pos; _VtxWritePtr->uv = uv; _VtxWritePtr->col = col; _VtxWritePtr++; _VtxCurrentIdx++; }
    inline    void  PrimWriteIdx( ImDrawIdx idx ) { *_IdxWritePtr = idx; _IdxWritePtr++; }
    inline    void  PrimVtx( const ImVec2& pos, const ImVec2& uv, uint32_t col ) { PrimWriteIdx( (ImDrawIdx)_VtxCurrentIdx ); PrimWriteVtx( pos, uv, col ); }
    void  UpdateClipRect();
    void  UpdateBlendingState();
    void  UpdateTextureID();
};
