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

#pragma once

#include "DeviceObject.h"
#include "StaticLimits.h"
#include "ShaderModule.h"
#include "Texture.h"
#include "Buffer.h"

#include <Hork/Core/Containers/ArrayView.h>

HK_NAMESPACE_BEGIN

namespace RHI
{

enum
{
    DEFAULT_STENCIL_READ_MASK  = 0xff,
    DEFAULT_STENCIL_WRITE_MASK = 0xff
};

//
// Blending state
//

enum BLEND_OP : uint8_t
{
    BLEND_OP_ADD,              /// Rr=RssR+RddR Gr=GssG+GddG Br=BssB+BddB 	Ar=AssA+AddA
    BLEND_OP_SUBTRACT,         /// Rr=RssR−RddR Gr=GssG−GddG Br=BssB−BddB 	Ar=AssA−AddA
    BLEND_OP_REVERSE_SUBTRACT, /// Rr=RddR−RssR Gr=GddG−GssG Br=BddB−BssB 	Ar=AddA−AssA
    BLEND_OP_MIN,              /// Rr=min(Rs,Rd) Gr=min(Gs,Gd) Br=min(Bs,Bd) 	Ar=min(As,Ad)
    BLEND_OP_MAX               /// Rr=max(Rs,Rd) Gr=max(Gs,Gd) Br=max(Bs,Bd) 	Ar=max(As,Ad)
};

enum BLEND_FUNC : uint8_t
{
    BLEND_FUNC_ZERO, /// ( 0,  0,  0,  0 )
    BLEND_FUNC_ONE,  /// ( 1,  1,  1,  1 )

    BLEND_FUNC_SRC_COLOR,     /// ( Rs0 / kr,   Gs0 / kg,   Bs0 / kb,   As0 / ka )
    BLEND_FUNC_INV_SRC_COLOR, /// ( 1,  1,  1,  1 ) - ( Rs0 / kr,   Gs0 / kg,   Bs0 / kb,   As0 / ka )

    BLEND_FUNC_DST_COLOR,     /// ( Rd0 / kr,   Gd0 / kg,   Bd0 / kb,   Ad0 / ka )
    BLEND_FUNC_INV_DST_COLOR, /// ( 1,  1,  1,  1 ) - ( Rd0 / kr,   Gd0 / kg,   Bd0 / kb,   Ad0 / ka )

    BLEND_FUNC_SRC_ALPHA,     /// ( As0 / kA,   As0 / kA,   As0 / kA,   As0 / kA )
    BLEND_FUNC_INV_SRC_ALPHA, /// ( 1,  1,  1,  1 ) - ( As0 / kA,   As0 / kA,   As0 / kA,   As0 / kA )

    BLEND_FUNC_DST_ALPHA,     /// ( Ad / kA,    Ad / kA,    Ad / kA,    Ad / kA )
    BLEND_FUNC_INV_DST_ALPHA, /// ( 1,  1,  1,  1 ) - ( Ad / kA,    Ad / kA,    Ad / kA,    Ad / kA )

    BLEND_FUNC_CONSTANT_COLOR,     /// ( Rc, Gc, Bc, Ac )
    BLEND_FUNC_INV_CONSTANT_COLOR, /// ( 1,  1,  1,  1 ) - ( Rc, Gc, Bc, Ac )

    BLEND_FUNC_CONSTANT_ALPHA,     /// ( Ac, Ac, Ac, Ac )
    BLEND_FUNC_INV_CONSTANT_ALPHA, /// ( 1,  1,  1,  1 ) - ( Ac, Ac, Ac, Ac )

    BLEND_FUNC_SRC_ALPHA_SATURATE, /// ( i,  i,  i,  1 )

    BLEND_FUNC_SRC1_COLOR,     /// ( Rs1 / kR,   Gs1 / kG,   Bs1 / kB,   As1 / kA )
    BLEND_FUNC_INV_SRC1_COLOR, /// ( 1,  1,  1,  1 ) - ( Rs1 / kR,   Gs1 / kG,   Bs1 / kB,   As1 / kA )

    BLEND_FUNC_SRC1_ALPHA,    /// ( As1 / kA,   As1 / kA,   As1 / kA,   As1 / kA )
    BLEND_FUNC_INV_SRC1_ALPHA /// ( 1,  1,  1,  1 ) - ( As1 / kA,   As1 / kA,   As1 / kA,   As1 / kA )
};

enum BLENDING_PRESET : uint8_t
{
    BLENDING_NO_BLEND,
    BLENDING_ALPHA,
    BLENDING_PREMULTIPLIED_ALPHA,
    BLENDING_COLOR_ADD,
    BLENDING_MULTIPLY,
    BLENDING_SOURCE_TO_DEST,
    BLENDING_ADD_MUL,
    BLENDING_ADD_ALPHA,

    BLENDING_MAX_PRESETS
};

enum LOGIC_OP : uint8_t
{
    LOGIC_OP_COPY,
    LOGIC_OP_COPY_INV,
    LOGIC_OP_CLEAR,
    LOGIC_OP_SET,
    LOGIC_OP_NOOP,
    LOGIC_OP_INVERT,
    LOGIC_OP_AND,
    LOGIC_OP_NAND,
    LOGIC_OP_OR,
    LOGIC_OP_NOR,
    LOGIC_OP_XOR,
    LOGIC_OP_EQUIV,
    LOGIC_OP_AND_REV,
    LOGIC_OP_AND_INV,
    LOGIC_OP_OR_REV,
    LOGIC_OP_OR_INV
};

enum COLOR_WRITE_MASK : uint8_t
{
    COLOR_WRITE_DISABLED = 0,
    COLOR_WRITE_R_BIT    = 1,
    COLOR_WRITE_G_BIT    = 2,
    COLOR_WRITE_B_BIT    = 4,
    COLOR_WRITE_A_BIT    = 8,
    COLOR_WRITE_RGBA     = COLOR_WRITE_R_BIT | COLOR_WRITE_G_BIT | COLOR_WRITE_B_BIT | COLOR_WRITE_A_BIT,
    COLOR_WRITE_RGB      = COLOR_WRITE_R_BIT | COLOR_WRITE_G_BIT | COLOR_WRITE_B_BIT
};

struct RenderTargetBlendingInfo
{
    struct Operation
    {
        BLEND_OP ColorRGB = BLEND_OP_ADD;
        BLEND_OP Alpha    = BLEND_OP_ADD;
    } Op;

    struct Function
    {
        BLEND_FUNC SrcFactorRGB   = BLEND_FUNC_ONE;
        BLEND_FUNC DstFactorRGB   = BLEND_FUNC_ZERO;
        BLEND_FUNC SrcFactorAlpha = BLEND_FUNC_ONE;
        BLEND_FUNC DstFactorAlpha = BLEND_FUNC_ZERO;
    } Func;

    bool             bBlendEnable = false;
    COLOR_WRITE_MASK ColorWriteMask = COLOR_WRITE_RGBA;

    // General blend equation:
    // if ( BlendEnable ) {
    //     ResultColorRGB = ( SourceColor.rgb * SrcFactorRGB ) Op.ColorRGB ( DestColor.rgb * DstFactorRGB )
    //     ResultAlpha    = ( SourceColor.a * SrcFactorAlpha ) Op.Alpha    ( DestColor.a * DstFactorAlpha )
    // } else {
    //     ResultColorRGB = SourceColor.rgb;
    //     ResultAlpha    = SourceColor.a;
    // }

    RenderTargetBlendingInfo() = default;

    void SetBlendingPreset(BLENDING_PRESET _Preset);

    bool operator==(RenderTargetBlendingInfo const& Rhs) const
    {
        return (Op.ColorRGB == Rhs.Op.ColorRGB &&
                Op.Alpha == Rhs.Op.Alpha &&
                Func.SrcFactorRGB == Rhs.Func.SrcFactorRGB &&
                Func.DstFactorRGB == Rhs.Func.DstFactorRGB &&
                Func.SrcFactorAlpha == Rhs.Func.SrcFactorAlpha &&
                Func.DstFactorAlpha == Rhs.Func.DstFactorAlpha &&
                bBlendEnable == Rhs.bBlendEnable &&
                ColorWriteMask == Rhs.ColorWriteMask);
    }

    bool operator!=(RenderTargetBlendingInfo const& Rhs) const
    {
        return !(operator==(Rhs));
    }

    uint32_t Hash() const
    {
        // clang-format off
        static_assert(sizeof(*this) ==
            sizeof(Op) +
            sizeof(Func) +
            sizeof(bBlendEnable) +
            sizeof(ColorWriteMask), "Unexpected alignment");
        // clang-format on

        return HashTraits::SDBMHash(reinterpret_cast<const char*>(this), sizeof(*this));
    }
};

HK_INLINE void RenderTargetBlendingInfo::SetBlendingPreset(BLENDING_PRESET _Preset)
{
    switch (_Preset)
    {
        case BLENDING_ALPHA:
            bBlendEnable      = true;
            ColorWriteMask    = COLOR_WRITE_RGBA;
            Func.SrcFactorRGB = Func.SrcFactorAlpha = BLEND_FUNC_SRC_ALPHA;
            Func.DstFactorRGB = Func.DstFactorAlpha = BLEND_FUNC_INV_SRC_ALPHA;
            Op.ColorRGB = Op.Alpha = BLEND_OP_ADD;
            break;
        case BLENDING_PREMULTIPLIED_ALPHA:
            bBlendEnable      = true;
            ColorWriteMask    = COLOR_WRITE_RGBA;
            Func.SrcFactorRGB = Func.SrcFactorAlpha = BLEND_FUNC_ONE;
            Func.DstFactorRGB = Func.DstFactorAlpha = BLEND_FUNC_INV_SRC_ALPHA;
            Op.ColorRGB = Op.Alpha = BLEND_OP_ADD;
            break;
        case BLENDING_COLOR_ADD:
            bBlendEnable      = true;
            ColorWriteMask    = COLOR_WRITE_RGBA;
            Func.SrcFactorRGB = Func.SrcFactorAlpha = BLEND_FUNC_ONE;
            Func.DstFactorRGB = Func.DstFactorAlpha = BLEND_FUNC_ONE;
            Op.ColorRGB = Op.Alpha = BLEND_OP_ADD;
            break;
        case BLENDING_MULTIPLY:
            bBlendEnable      = true;
            ColorWriteMask    = COLOR_WRITE_RGBA;
            Func.SrcFactorRGB = Func.SrcFactorAlpha = BLEND_FUNC_DST_COLOR;
            Func.DstFactorRGB = Func.DstFactorAlpha = BLEND_FUNC_ZERO;
            Op.ColorRGB = Op.Alpha = BLEND_OP_ADD;
            break;
        case BLENDING_SOURCE_TO_DEST:
            bBlendEnable      = true;
            ColorWriteMask    = COLOR_WRITE_RGBA;
            Func.SrcFactorRGB = Func.SrcFactorAlpha = BLEND_FUNC_SRC_COLOR;
            Func.DstFactorRGB = Func.DstFactorAlpha = BLEND_FUNC_ONE;
            Op.ColorRGB = Op.Alpha = BLEND_OP_ADD;
            break;
        case BLENDING_ADD_MUL:
            bBlendEnable      = true;
            ColorWriteMask    = COLOR_WRITE_RGBA;
            Func.SrcFactorRGB = Func.SrcFactorAlpha = BLEND_FUNC_INV_DST_COLOR;
            Func.DstFactorRGB = Func.DstFactorAlpha = BLEND_FUNC_ONE;
            Op.ColorRGB = Op.Alpha = BLEND_OP_ADD;
            break;
        case BLENDING_ADD_ALPHA:
            bBlendEnable      = true;
            ColorWriteMask    = COLOR_WRITE_RGBA;
            Func.SrcFactorRGB = Func.SrcFactorAlpha = BLEND_FUNC_SRC_ALPHA;
            Func.DstFactorRGB = Func.DstFactorAlpha = BLEND_FUNC_ONE;
            Op.ColorRGB = Op.Alpha = BLEND_OP_ADD;
            break;
        case BLENDING_NO_BLEND:
        default:
            bBlendEnable      = false;
            ColorWriteMask    = COLOR_WRITE_RGBA;
            Func.SrcFactorRGB = Func.SrcFactorAlpha = BLEND_FUNC_ONE;
            Func.DstFactorRGB = Func.DstFactorAlpha = BLEND_FUNC_ZERO;
            Op.ColorRGB = Op.Alpha = BLEND_OP_ADD;
            break;
    }
}

struct BlendingStateInfo
{
    bool                      bSampleAlphaToCoverage = false;
    bool                      bIndependentBlendEnable = false;
    LOGIC_OP                  LogicOp                 = LOGIC_OP_COPY;
    RenderTargetBlendingInfo RenderTargetSlots[MAX_COLOR_ATTACHMENTS];

    BlendingStateInfo() = default;

    bool operator==(BlendingStateInfo const& Rhs) const
    {
        if (bSampleAlphaToCoverage != Rhs.bSampleAlphaToCoverage ||
            bIndependentBlendEnable != Rhs.bIndependentBlendEnable ||
            LogicOp != Rhs.LogicOp)
        {
            return false;
        }

        for (int i = 0; i < MAX_COLOR_ATTACHMENTS; ++i)
            if (RenderTargetSlots[i] != Rhs.RenderTargetSlots[i])
                return false;

        return true;
    }

    bool operator!=(BlendingStateInfo const& Rhs) const
    {
        return !(operator==(Rhs));
    }

    uint32_t Hash() const
    {
        uint32_t h = HashTraits::HashCombine(HashTraits::HashCombine(HashTraits::Hash(bSampleAlphaToCoverage), bIndependentBlendEnable), uint8_t(LogicOp));

        for (int i = 0; i < MAX_COLOR_ATTACHMENTS; ++i)
            h = HashTraits::HashCombine(h, RenderTargetSlots[i]);

        return h;
    }
};

//
// Rasterizer state
//

enum POLYGON_FILL : uint8_t
{
    POLYGON_FILL_SOLID = 0,
    POLYGON_FILL_WIRE  = 1
};

enum POLYGON_CULL : uint8_t
{
    POLYGON_CULL_BACK     = 0,
    POLYGON_CULL_FRONT    = 1,
    POLYGON_CULL_DISABLED = 2
};

struct RasterizerStateInfo
{
    POLYGON_FILL FillMode; // POLYGON_FILL_SOLID;
    POLYGON_CULL CullMode; // POLYGON_CULL_BACK;
    bool         bFrontClockwise;

    struct
    {
        /*
                       _
                      |       MaxDepthSlope x Slope + r * Bias,           if Clamp = 0 or NaN;
                      |
        DepthOffset = <   min(MaxDepthSlope x Slope + r * Bias, Clamp),   if Clamp > 0;
                      |
                      |_  max(MaxDepthSlope x Slope + r * Bias, Clamp),   if Clamp < 0.

        */
        float Slope;
        int   Bias;
        float Clamp;
    } DepthOffset;

    // If enabled, the −wc ≤ zc ≤ wc plane equation is ignored by view volume clipping
    // (effectively, there is no near or far plane clipping). See viewport->MinDepth, viewport->MaxDepth.
    bool bDepthClampEnable;

    bool bScissorEnable;
    bool bMultisampleEnable;
    bool bAntialiasedLineEnable;

    // If enabled, primitives are discarded after the optional transform feedback stage, but before rasterization
    bool bRasterizerDiscard;

    RasterizerStateInfo()
    {
        // NOTE: Call ZeroMem to clear the garbage int the paddings for the correct hashing.
        Core::ZeroMem(this, sizeof(*this));
    }

    bool operator==(RasterizerStateInfo const& Rhs) const
    {
        return (FillMode == Rhs.FillMode &&
                CullMode == Rhs.CullMode &&
                bFrontClockwise == Rhs.bFrontClockwise &&
                DepthOffset.Slope == Rhs.DepthOffset.Slope &&
                DepthOffset.Bias == Rhs.DepthOffset.Bias &&
                DepthOffset.Clamp == Rhs.DepthOffset.Clamp &&
                bDepthClampEnable == Rhs.bDepthClampEnable &&
                bScissorEnable == Rhs.bScissorEnable &&
                bMultisampleEnable == Rhs.bMultisampleEnable &&
                bAntialiasedLineEnable == Rhs.bAntialiasedLineEnable &&
                bRasterizerDiscard == Rhs.bRasterizerDiscard);
    }

    bool operator!=(RasterizerStateInfo const& Rhs) const
    {
        return !(operator==(Rhs));
    }

    uint32_t Hash() const
    {
        return HashTraits::SDBMHash(reinterpret_cast<const char*>(this), sizeof(*this));
    }
};


//
// Depth-Stencil state
//

enum STENCIL_OP : uint8_t
{
    STENCIL_OP_KEEP     = 0,
    STENCIL_OP_ZERO     = 1,
    STENCIL_OP_REPLACE  = 2,
    STENCIL_OP_INCR_SAT = 3,
    STENCIL_OP_DECR_SAT = 4,
    STENCIL_OP_INVERT   = 5,
    STENCIL_OP_INCR     = 6,
    STENCIL_OP_DECR     = 7
};

struct StencilTestInfo
{
    STENCIL_OP          StencilFailOp = STENCIL_OP_KEEP;
    STENCIL_OP          DepthFailOp   = STENCIL_OP_KEEP;
    STENCIL_OP          DepthPassOp   = STENCIL_OP_KEEP;
    COMPARISON_FUNCTION StencilFunc   = CMPFUNC_ALWAYS;
    //int              Reference = 0;

    StencilTestInfo() = default;

    bool operator==(StencilTestInfo const& Rhs) const
    {
        return (StencilFailOp == Rhs.StencilFailOp &&
                DepthFailOp == Rhs.DepthFailOp &&
                DepthPassOp == Rhs.DepthPassOp &&
                StencilFunc == Rhs.StencilFunc);
    }
};

struct DepthStencilStateInfo
{
    bool                bDepthEnable;//     = true;
    bool                bDepthWrite;  //      = true;
    COMPARISON_FUNCTION DepthFunc;   //        = CMPFUNC_LESS;
    bool                bStencilEnable; //   = false;
    uint8_t             StencilReadMask; //  = DEFAULT_STENCIL_READ_MASK;
    uint8_t             StencilWriteMask; // = DEFAULT_STENCIL_WRITE_MASK;
    StencilTestInfo    FrontFace;
    StencilTestInfo    BackFace;

    DepthStencilStateInfo()
    {
        // NOTE: Call ZeroMem to clear the garbage in the paddings for the correct hashing.
        Core::ZeroMem(this, sizeof(*this));

        bDepthEnable            = true;
        bDepthWrite             = true;
        DepthFunc               = CMPFUNC_LESS;
        StencilReadMask         = DEFAULT_STENCIL_READ_MASK;
        StencilWriteMask        = DEFAULT_STENCIL_WRITE_MASK;
        FrontFace.StencilFailOp = STENCIL_OP_KEEP;
        FrontFace.DepthFailOp   = STENCIL_OP_KEEP;
        FrontFace.DepthPassOp   = STENCIL_OP_KEEP;
        FrontFace.StencilFunc   = CMPFUNC_ALWAYS;
        BackFace.StencilFailOp  = STENCIL_OP_KEEP;
        BackFace.DepthFailOp    = STENCIL_OP_KEEP;
        BackFace.DepthPassOp    = STENCIL_OP_KEEP;
        BackFace.StencilFunc    = CMPFUNC_ALWAYS;
    }

    bool operator==(DepthStencilStateInfo const& Rhs) const
    {
        return (bDepthEnable == Rhs.bDepthEnable &&
                bDepthWrite == Rhs.bDepthWrite &&
                DepthFunc == Rhs.DepthFunc &&
                StencilReadMask == Rhs.StencilReadMask &&
                StencilWriteMask == Rhs.StencilWriteMask &&
                FrontFace == Rhs.FrontFace &&
                BackFace == Rhs.BackFace);
    }

    bool operator!=(DepthStencilStateInfo const& Rhs) const
    {
        return !(operator==(Rhs));
    }

    uint32_t Hash() const
    {
        return HashTraits::SDBMHash(reinterpret_cast<const char*>(this), sizeof(*this));
    }
};


//
// Pipeline resource layout
//

enum IMAGE_ACCESS_MODE : uint8_t
{
    IMAGE_ACCESS_READ,
    IMAGE_ACCESS_WRITE,
    IMAGE_ACCESS_RW
};

struct ImageInfo
{
    IMAGE_ACCESS_MODE AccessMode = IMAGE_ACCESS_READ;
    TEXTURE_FORMAT    TextureFormat = TEXTURE_FORMAT_RGBA8_UNORM; // FIXME: get texture format from texture?
};

struct BufferInfo
{
    BufferInfo() = default;
    BufferInfo(BUFFER_BINDING binding) :
        BufferBinding(binding)
    {}

    BUFFER_BINDING BufferBinding = BUFFER_BIND_CONSTANT;
};

struct PipelineResourceLayout
{
    int                 NumSamplers = 0;
    struct SamplerDesc const* Samplers = nullptr;

    int        NumImages = 0;
    ImageInfo const* Images = nullptr;

    int         NumBuffers = 0;
    BufferInfo const* Buffers = nullptr;

    PipelineResourceLayout() = default;
};

//
// Vertex bindings and attributes
//

constexpr int VertexAttribType_NormalizedBit() { return 1 << 7; }

constexpr int VertexAttribType_CountBit(int Count) { return ((((Count)-1) & 3) << 5); }

constexpr int _5BitNumber(int Number) { return ((Number)&31); }

enum VERTEX_ATTRIB_COMPONENT : uint8_t
{
    COMPONENT_BYTE   = _5BitNumber(0),
    COMPONENT_UBYTE  = _5BitNumber(1),
    COMPONENT_SHORT  = _5BitNumber(2),
    COMPONENT_USHORT = _5BitNumber(3),
    COMPONENT_INT    = _5BitNumber(4),
    COMPONENT_UINT   = _5BitNumber(5),
    COMPONENT_HALF   = _5BitNumber(6),
    COMPONENT_FLOAT  = _5BitNumber(7),
    COMPONENT_DOUBLE = _5BitNumber(8)

    // Add here other types

    // MAX = 31
};

enum VERTEX_ATTRIB_TYPE : uint8_t
{
    /// Signed byte
    VAT_BYTE1  = COMPONENT_BYTE | VertexAttribType_CountBit(1),
    VAT_BYTE2  = COMPONENT_BYTE | VertexAttribType_CountBit(2),
    VAT_BYTE3  = COMPONENT_BYTE | VertexAttribType_CountBit(3),
    VAT_BYTE4  = COMPONENT_BYTE | VertexAttribType_CountBit(4),
    VAT_BYTE1N = VAT_BYTE1 | VertexAttribType_NormalizedBit(),
    VAT_BYTE2N = VAT_BYTE2 | VertexAttribType_NormalizedBit(),
    VAT_BYTE3N = VAT_BYTE3 | VertexAttribType_NormalizedBit(),
    VAT_BYTE4N = VAT_BYTE4 | VertexAttribType_NormalizedBit(),

    /// Unsigned byte
    VAT_UBYTE1  = COMPONENT_UBYTE | VertexAttribType_CountBit(1),
    VAT_UBYTE2  = COMPONENT_UBYTE | VertexAttribType_CountBit(2),
    VAT_UBYTE3  = COMPONENT_UBYTE | VertexAttribType_CountBit(3),
    VAT_UBYTE4  = COMPONENT_UBYTE | VertexAttribType_CountBit(4),
    VAT_UBYTE1N = VAT_UBYTE1 | VertexAttribType_NormalizedBit(),
    VAT_UBYTE2N = VAT_UBYTE2 | VertexAttribType_NormalizedBit(),
    VAT_UBYTE3N = VAT_UBYTE3 | VertexAttribType_NormalizedBit(),
    VAT_UBYTE4N = VAT_UBYTE4 | VertexAttribType_NormalizedBit(),

    /// Signed short (16 bit integer)
    VAT_SHORT1  = COMPONENT_SHORT | VertexAttribType_CountBit(1),
    VAT_SHORT2  = COMPONENT_SHORT | VertexAttribType_CountBit(2),
    VAT_SHORT3  = COMPONENT_SHORT | VertexAttribType_CountBit(3),
    VAT_SHORT4  = COMPONENT_SHORT | VertexAttribType_CountBit(4),
    VAT_SHORT1N = VAT_SHORT1 | VertexAttribType_NormalizedBit(),
    VAT_SHORT2N = VAT_SHORT2 | VertexAttribType_NormalizedBit(),
    VAT_SHORT3N = VAT_SHORT3 | VertexAttribType_NormalizedBit(),
    VAT_SHORT4N = VAT_SHORT4 | VertexAttribType_NormalizedBit(),

    /// Unsigned short (16 bit integer)
    VAT_USHORT1  = COMPONENT_USHORT | VertexAttribType_CountBit(1),
    VAT_USHORT2  = COMPONENT_USHORT | VertexAttribType_CountBit(2),
    VAT_USHORT3  = COMPONENT_USHORT | VertexAttribType_CountBit(3),
    VAT_USHORT4  = COMPONENT_USHORT | VertexAttribType_CountBit(4),
    VAT_USHORT1N = VAT_USHORT1 | VertexAttribType_NormalizedBit(),
    VAT_USHORT2N = VAT_USHORT2 | VertexAttribType_NormalizedBit(),
    VAT_USHORT3N = VAT_USHORT3 | VertexAttribType_NormalizedBit(),
    VAT_USHORT4N = VAT_USHORT4 | VertexAttribType_NormalizedBit(),

    /// 32-bit signed integer
    VAT_INT1  = COMPONENT_INT | VertexAttribType_CountBit(1),
    VAT_INT2  = COMPONENT_INT | VertexAttribType_CountBit(2),
    VAT_INT3  = COMPONENT_INT | VertexAttribType_CountBit(3),
    VAT_INT4  = COMPONENT_INT | VertexAttribType_CountBit(4),
    VAT_INT1N = VAT_INT1 | VertexAttribType_NormalizedBit(),
    VAT_INT2N = VAT_INT2 | VertexAttribType_NormalizedBit(),
    VAT_INT3N = VAT_INT3 | VertexAttribType_NormalizedBit(),
    VAT_INT4N = VAT_INT4 | VertexAttribType_NormalizedBit(),

    /// 32-bit unsigned integer
    VAT_UINT1  = COMPONENT_UINT | VertexAttribType_CountBit(1),
    VAT_UINT2  = COMPONENT_UINT | VertexAttribType_CountBit(2),
    VAT_UINT3  = COMPONENT_UINT | VertexAttribType_CountBit(3),
    VAT_UINT4  = COMPONENT_UINT | VertexAttribType_CountBit(4),
    VAT_UINT1N = VAT_UINT1 | VertexAttribType_NormalizedBit(),
    VAT_UINT2N = VAT_UINT2 | VertexAttribType_NormalizedBit(),
    VAT_UINT3N = VAT_UINT3 | VertexAttribType_NormalizedBit(),
    VAT_UINT4N = VAT_UINT4 | VertexAttribType_NormalizedBit(),

    /// 16-bit floating point
    VAT_HALF1 = COMPONENT_HALF | VertexAttribType_CountBit(1), // only with IsHalfFloatVertexSupported
    VAT_HALF2 = COMPONENT_HALF | VertexAttribType_CountBit(2), // only with IsHalfFloatVertexSupported
    VAT_HALF3 = COMPONENT_HALF | VertexAttribType_CountBit(3), // only with IsHalfFloatVertexSupported
    VAT_HALF4 = COMPONENT_HALF | VertexAttribType_CountBit(4), // only with IsHalfFloatVertexSupported

    /// 32-bit floating point
    VAT_FLOAT1 = COMPONENT_FLOAT | VertexAttribType_CountBit(1),
    VAT_FLOAT2 = COMPONENT_FLOAT | VertexAttribType_CountBit(2),
    VAT_FLOAT3 = COMPONENT_FLOAT | VertexAttribType_CountBit(3),
    VAT_FLOAT4 = COMPONENT_FLOAT | VertexAttribType_CountBit(4),

    /// 64-bit floating point
    VAT_DOUBLE1 = COMPONENT_DOUBLE | VertexAttribType_CountBit(1),
    VAT_DOUBLE2 = COMPONENT_DOUBLE | VertexAttribType_CountBit(2),
    VAT_DOUBLE3 = COMPONENT_DOUBLE | VertexAttribType_CountBit(3),
    VAT_DOUBLE4 = COMPONENT_DOUBLE | VertexAttribType_CountBit(4)
};

enum VERTEX_ATTRIB_MODE : uint8_t
{
    VAM_FLOAT,
    VAM_DOUBLE,
    VAM_INTEGER,
};

enum VERTEX_INPUT_RATE : uint8_t
{
    INPUT_RATE_PER_VERTEX   = 0,
    INPUT_RATE_PER_INSTANCE = 1
};

struct VertexBindingInfo
{
    VERTEX_INPUT_RATE InputRate = INPUT_RATE_PER_VERTEX; /// per vertex / per instance
    uint8_t           InputSlot = 0;                     /// vertex buffer binding
    uint16_t          Pad       = 0;
    uint32_t          Stride    = 0;                     /// vertex stride

    VertexBindingInfo() = default;

    VertexBindingInfo(uint8_t InputSlot, uint32_t Stride, VERTEX_INPUT_RATE InputRate = INPUT_RATE_PER_VERTEX) :
        InputRate(InputRate), InputSlot(InputSlot), Stride(Stride)
    {}

    bool operator==(VertexBindingInfo const& Rhs) const
    {
        return InputRate == Rhs.InputRate && InputSlot == Rhs.InputSlot && Stride == Rhs.Stride;
    }

    bool operator!=(VertexBindingInfo const& Rhs) const
    {
        return !(operator==(Rhs));
    }

    uint32_t Hash() const
    {
        static_assert(sizeof(*this) == 8, "Unexpected alignment");
        return HashTraits::SDBMHash(reinterpret_cast<const char*>(this), sizeof(*this));
    }
};

struct VertexAttribInfo
{
    const char* SemanticName = "Undefined";
    uint32_t    Location     = 0;

    /// vertex buffer binding
    uint32_t           InputSlot = 0;
    VERTEX_ATTRIB_TYPE Type      = VAT_FLOAT1;

    /// float / double / integer
    VERTEX_ATTRIB_MODE Mode = VAM_FLOAT;

    /// Only for INPUT_RATE_PER_INSTANCE. The number of instances to draw using same
    /// per-instance data before advancing in the buffer by one element. This value must
    /// /// by 0 for an element that contains per-vertex data (InputRate = INPUT_RATE_PER_VERTEX)
    uint32_t InstanceDataStepRate = 0;

    /// attribute offset
    uint32_t Offset = 0;

    /// Number of vector components 1,2,3,4
    int NumComponents() const { return ((Type >> 5) & 3) + 1; }

    /// Type of vector components COMPONENT_BYTE, COMPONENT_SHORT, COMPONENT_HALF, COMPONENT_FLOAT, etc.
    VERTEX_ATTRIB_COMPONENT TypeOfComponent() const { return (VERTEX_ATTRIB_COMPONENT)_5BitNumber(Type); }

    /// Components are normalized
    bool IsNormalized() const { return !!(Type & VertexAttribType_NormalizedBit()); }

    bool operator==(VertexAttribInfo const& Rhs) const
    {
        // NOTE: We intentionally do not compare SemanticName.
        return (Location == Rhs.Location &&
                InputSlot == Rhs.InputSlot &&
                Type == Rhs.Type &&
                Mode == Rhs.Mode &&
                InstanceDataStepRate == Rhs.InstanceDataStepRate &&
                Offset == Rhs.Offset);
    }

    bool operator!=(VertexAttribInfo const& Rhs) const
    {
        return !(operator==(Rhs));
    }

    uint32_t Hash() const
    {
        // NOTE: We intentionally do not hash SemanticName.
        return HashTraits::HashCombine(HashTraits::HashCombine(HashTraits::HashCombine(HashTraits::HashCombine(HashTraits::HashCombine(HashTraits::Hash(Location), InputSlot), uint8_t(Type)), uint8_t(Mode)), InstanceDataStepRate), Offset);
    }
};

enum PRIMITIVE_TOPOLOGY : uint8_t
{
    PRIMITIVE_UNDEFINED          = 0,
    PRIMITIVE_POINTS             = 1,
    PRIMITIVE_LINES              = 2,
    PRIMITIVE_LINE_STRIP         = 3,
    PRIMITIVE_LINE_LOOP          = 4,
    PRIMITIVE_TRIANGLES          = 5,
    PRIMITIVE_TRIANGLE_STRIP     = 6,
    PRIMITIVE_TRIANGLE_FAN       = 7,
    PRIMITIVE_LINES_ADJ          = 8,
    PRIMITIVE_LINE_STRIP_ADJ     = 9,
    PRIMITIVE_TRIANGLES_ADJ      = 10,
    PRIMITIVE_TRIANGLE_STRIP_ADJ = 11,
    PRIMITIVE_PATCHES_1          = 12,
    PRIMITIVE_PATCHES_2          = PRIMITIVE_PATCHES_1 + 1,
    PRIMITIVE_PATCHES_3          = PRIMITIVE_PATCHES_1 + 2,
    PRIMITIVE_PATCHES_4          = PRIMITIVE_PATCHES_1 + 3,
    PRIMITIVE_PATCHES_5          = PRIMITIVE_PATCHES_1 + 4,
    PRIMITIVE_PATCHES_6          = PRIMITIVE_PATCHES_1 + 5,
    PRIMITIVE_PATCHES_7          = PRIMITIVE_PATCHES_1 + 6,
    PRIMITIVE_PATCHES_8          = PRIMITIVE_PATCHES_1 + 7,
    PRIMITIVE_PATCHES_9          = PRIMITIVE_PATCHES_1 + 8,
    PRIMITIVE_PATCHES_10         = PRIMITIVE_PATCHES_1 + 9,
    PRIMITIVE_PATCHES_11         = PRIMITIVE_PATCHES_1 + 10,
    PRIMITIVE_PATCHES_12         = PRIMITIVE_PATCHES_1 + 11,
    PRIMITIVE_PATCHES_13         = PRIMITIVE_PATCHES_1 + 12,
    PRIMITIVE_PATCHES_14         = PRIMITIVE_PATCHES_1 + 13,
    PRIMITIVE_PATCHES_15         = PRIMITIVE_PATCHES_1 + 14,
    PRIMITIVE_PATCHES_16         = PRIMITIVE_PATCHES_1 + 15,
    PRIMITIVE_PATCHES_17         = PRIMITIVE_PATCHES_1 + 16,
    PRIMITIVE_PATCHES_18         = PRIMITIVE_PATCHES_1 + 17,
    PRIMITIVE_PATCHES_19         = PRIMITIVE_PATCHES_1 + 18,
    PRIMITIVE_PATCHES_20         = PRIMITIVE_PATCHES_1 + 19,
    PRIMITIVE_PATCHES_21         = PRIMITIVE_PATCHES_1 + 20,
    PRIMITIVE_PATCHES_22         = PRIMITIVE_PATCHES_1 + 21,
    PRIMITIVE_PATCHES_23         = PRIMITIVE_PATCHES_1 + 22,
    PRIMITIVE_PATCHES_24         = PRIMITIVE_PATCHES_1 + 23,
    PRIMITIVE_PATCHES_25         = PRIMITIVE_PATCHES_1 + 24,
    PRIMITIVE_PATCHES_26         = PRIMITIVE_PATCHES_1 + 25,
    PRIMITIVE_PATCHES_27         = PRIMITIVE_PATCHES_1 + 26,
    PRIMITIVE_PATCHES_28         = PRIMITIVE_PATCHES_1 + 27,
    PRIMITIVE_PATCHES_29         = PRIMITIVE_PATCHES_1 + 28,
    PRIMITIVE_PATCHES_30         = PRIMITIVE_PATCHES_1 + 29,
    PRIMITIVE_PATCHES_31         = PRIMITIVE_PATCHES_1 + 30,
    PRIMITIVE_PATCHES_32         = PRIMITIVE_PATCHES_1 + 31
};

struct PipelineInputAssemblyInfo
{
    PRIMITIVE_TOPOLOGY Topology = PRIMITIVE_TRIANGLES;
};

struct PipelineDesc
{
    PipelineInputAssemblyInfo IA;
    BlendingStateInfo         BS;
    RasterizerStateInfo       RS;
    DepthStencilStateInfo     DSS;
    PipelineResourceLayout    ResourceLayout;
    Ref<IShaderModule>       pVS;
    Ref<IShaderModule>       pTCS;
    Ref<IShaderModule>       pTES;
    Ref<IShaderModule>       pGS;
    Ref<IShaderModule>       pFS;
    Ref<IShaderModule>       pCS;
    uint32_t                  NumVertexBindings = 0;
    VertexBindingInfo const*  pVertexBindings = nullptr;
    uint32_t                  NumVertexAttribs = 0;
    VertexAttribInfo const*   pVertexAttribs = nullptr;
};

class IPipeline : public IDeviceObject
{
public:
    static constexpr DEVICE_OBJECT_PROXY_TYPE PROXY_TYPE = DEVICE_OBJECT_TYPE_PIPELINE;

    IPipeline(IDevice* pDevice) :
        IDeviceObject(pDevice, PROXY_TYPE)
    {}
};

} // namespace RHI

HK_NAMESPACE_END
