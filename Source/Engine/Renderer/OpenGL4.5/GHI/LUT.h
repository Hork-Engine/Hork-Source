/*

Graphics Hardware Interface (GHI) is part of Angie Engine Source Code

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

#include "GHITexture.h"

#include "GL/glew.h"

namespace GHI {

/*

Handle conversion

*/
#define GL_HANDLE( Handle ) ( GLuint )( size_t )(Handle)

/*

Conversion from BUFFER_TYPE to target and binding

*/

struct TableBufferTarget {
    GLenum  Target;
    GLenum  Binding;
};

constexpr TableBufferTarget BufferTargetLUT[] = {
    { GL_UNIFORM_BUFFER,            GL_UNIFORM_BUFFER_BINDING },
    { GL_SHADER_STORAGE_BUFFER,     GL_SHADER_STORAGE_BUFFER_BINDING },
    { GL_TRANSFORM_FEEDBACK_BUFFER, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING },
    { GL_ATOMIC_COUNTER_BUFFER,     GL_ATOMIC_COUNTER_BUFFER_BINDING }
};

/*

Conversion from INDEX_TYPE

*/

constexpr GLenum IndexTypeLUT[] = {
    GL_UNSIGNED_SHORT,      // INDEX_TYPE_UINT16
    GL_UNSIGNED_INT         // INDEX_TYPE_UINT32
};

constexpr size_t IndexTypeSizeOfLUT[2] = { sizeof( unsigned short ), sizeof( unsigned int ) };


/*

Conversion from IMAGE_ACCESS_MODE

*/

constexpr int ImageAccessModeLUT[] = {
    GL_READ_ONLY,
    GL_WRITE_ONLY,
    GL_READ_WRITE
};

/*

Conversion from BLEND_FUNC

*/

constexpr GLenum BlendFuncConvertionLUT[] = {
    GL_ZERO,
    GL_ONE,
    GL_SRC_COLOR,
    GL_ONE_MINUS_SRC_COLOR,
    GL_DST_COLOR,
    GL_ONE_MINUS_DST_COLOR,
    GL_SRC_ALPHA,
    GL_ONE_MINUS_SRC_ALPHA,
    GL_DST_ALPHA,
    GL_ONE_MINUS_DST_ALPHA,
    GL_CONSTANT_COLOR,
    GL_ONE_MINUS_CONSTANT_COLOR,
    GL_CONSTANT_ALPHA,
    GL_ONE_MINUS_CONSTANT_ALPHA,
    GL_SRC_ALPHA_SATURATE,
    GL_SRC1_COLOR,
    GL_ONE_MINUS_SRC1_COLOR,
    GL_SRC1_ALPHA,
    GL_ONE_MINUS_SRC1_ALPHA
};

/*

Conversion from BLEND_OP

*/

constexpr GLenum BlendEquationConvertionLUT[] = {
    GL_FUNC_ADD,
    GL_FUNC_SUBTRACT,
    GL_FUNC_REVERSE_SUBTRACT,
    GL_MIN,
    GL_MAX
};

/*

Conversion from LOGIC_OP

*/

constexpr GLenum LogicOpLUT[] = {
    GL_COPY,
    GL_COPY_INVERTED,
    GL_CLEAR,
    GL_SET,
    GL_NOOP,
    GL_INVERT,
    GL_AND,
    GL_NAND,
    GL_OR,
    GL_NOR,
    GL_XOR,
    GL_EQUIV,
    GL_AND_REVERSE,
    GL_AND_INVERTED,
    GL_OR_REVERSE,
    GL_OR_INVERTED
};

/*

Conversion from STENCIL_OP

*/

constexpr GLenum StencilOpLUT[] = {
    GL_KEEP,
    GL_ZERO,
    GL_REPLACE,
    GL_INCR,
    GL_DECR,
    GL_INVERT,
    GL_INCR_WRAP,
    GL_DECR_WRAP
};

/*

Conversion from COMPARISON_FUNCTION

*/

constexpr int ComparisonFuncLUT[] = {
    GL_NEVER,
    GL_LESS,
    GL_EQUAL,
    GL_LEQUAL,
    GL_GREATER,
    GL_NOTEQUAL,
    GL_GEQUAL,
    GL_ALWAYS
};

/*

Conversion from POLYGON_FILL

*/

constexpr GLenum FillModeLUT[] = {
    GL_FILL,
    GL_LINE
};

/*

Conversion from POLYGON_CULL

*/

constexpr GLenum CullModeLUT[] = {
    GL_BACK,
    GL_FRONT,
    0
};

/*

Conversion from TEXTURE_TYPE

*/

struct TableTextureType {
    GLenum Target;
    GLenum Binding;
};

constexpr TableTextureType TextureTargetLUT[] = {
    { GL_TEXTURE_1D,                    GL_TEXTURE_BINDING_1D },
    { GL_TEXTURE_1D_ARRAY,              GL_TEXTURE_BINDING_1D_ARRAY },
    { GL_TEXTURE_2D,                    GL_TEXTURE_BINDING_2D },
    { GL_TEXTURE_2D_MULTISAMPLE,        GL_TEXTURE_BINDING_2D_MULTISAMPLE },
    { GL_TEXTURE_2D_ARRAY,              GL_TEXTURE_BINDING_2D_ARRAY },
    { GL_TEXTURE_2D_MULTISAMPLE_ARRAY,  GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY },
    { GL_TEXTURE_3D,                    GL_TEXTURE_BINDING_3D },
    { GL_TEXTURE_CUBE_MAP,              GL_TEXTURE_BINDING_CUBE_MAP },
    { GL_TEXTURE_CUBE_MAP_ARRAY,        GL_TEXTURE_BINDING_CUBE_MAP_ARRAY },
    { GL_TEXTURE_RECTANGLE,             GL_TEXTURE_BINDING_RECTANGLE }
};


/*

Conversion from INTERNAL_PIXEL_FORMAT

*/

enum {
    CLEAR_TYPE_FLOAT32,
    CLEAR_TYPE_INT32,
    CLEAR_TYPE_UINT32,
    CLEAR_TYPE_STENCIL_ONLY,
    CLEAR_TYPE_DEPTH_ONLY,
    CLEAR_TYPE_DEPTH_STENCIL,
};

struct TableInternalPixelFormat {
    GLint  InternalFormat;
    GLenum Format;
    const char * ShaderImageFormatQualifier;
    uint8_t ClearType;
};

constexpr TableInternalPixelFormat InternalFormatLUT[] = {
    { GL_R8,             GL_RED, "r8", CLEAR_TYPE_FLOAT32 },  //  8
    { GL_R8_SNORM,       GL_RED, "r8_snorm", CLEAR_TYPE_FLOAT32 },  //  s8
    { GL_R16,            GL_RED, "r16", CLEAR_TYPE_FLOAT32 },  //  16
    { GL_R16_SNORM,      GL_RED, "r16_snorm", CLEAR_TYPE_FLOAT32 },  //  s16
    { GL_RG8,            GL_RG, "rg8", CLEAR_TYPE_FLOAT32 },   //  8   8
    { GL_RG8_SNORM,      GL_RG, "rg8_snorm", CLEAR_TYPE_FLOAT32 },   //  s8  s8
    { GL_RG16,           GL_RG, "rg16", CLEAR_TYPE_FLOAT32 },   //  16  16
    { GL_RG16_SNORM,     GL_RG, "rg16_snorm", CLEAR_TYPE_FLOAT32 },   //  s16 s16
    { GL_R3_G3_B2,       GL_RGB, "", CLEAR_TYPE_FLOAT32 },  //  3  3  2
    { GL_RGB4,           GL_RGB, "", CLEAR_TYPE_FLOAT32 },  //  4  4  4
    { GL_RGB5,           GL_RGB, "", CLEAR_TYPE_FLOAT32 },  //  5  5  5
    { GL_RGB8,           GL_RGB, "", CLEAR_TYPE_FLOAT32 },  //  8  8  8
    { GL_RGB8_SNORM,     GL_RGB, "", CLEAR_TYPE_FLOAT32 },  //  s8  s8  s8
    { GL_RGB10,          GL_RGB, "", CLEAR_TYPE_FLOAT32 },  //  10  10  10
    { GL_RGB12,          GL_RGB, "", CLEAR_TYPE_FLOAT32 },  //  12  12  12
    { GL_RGB16,          GL_RGB, "", CLEAR_TYPE_FLOAT32 },  //  16  16  16
    { GL_RGB16_SNORM,    GL_RGB, "", CLEAR_TYPE_FLOAT32 },  //  s16  s16  s16
    { GL_RGBA2,          GL_RGB, "", CLEAR_TYPE_FLOAT32 },  //  2  2  2  2
    { GL_RGBA4,          GL_RGB, "", CLEAR_TYPE_FLOAT32 },  //  4  4  4  4
    { GL_RGB5_A1,        GL_RGBA, "", CLEAR_TYPE_FLOAT32 }, //  5  5  5  1
    { GL_RGBA8,          GL_RGBA, "rgba8", CLEAR_TYPE_FLOAT32 }, //  8  8  8  8
    { GL_RGBA8_SNORM,    GL_RGBA, "rgba8_snorm", CLEAR_TYPE_FLOAT32 }, //  s8  s8  s8  s8
    { GL_RGB10_A2,       GL_RGBA, "rgb10_a2", CLEAR_TYPE_FLOAT32 }, //  10  10  10  2
    { GL_RGB10_A2UI,     GL_RGBA, "rgb10_a2ui", CLEAR_TYPE_UINT32 }, //  ui10  ui10  ui10  ui2
    { GL_RGBA12,         GL_RGBA, "", CLEAR_TYPE_FLOAT32 }, //  12  12  12  12
    { GL_RGBA16,         GL_RGBA, "rgba16", CLEAR_TYPE_FLOAT32 }, //  16  16  16  16
    { GL_RGBA16_SNORM,   GL_RGBA, "rgba16_snorm", CLEAR_TYPE_FLOAT32 }, //  16  16  16  16
    { GL_SRGB8,          GL_RGB, "", CLEAR_TYPE_FLOAT32 },  //  8  8  8
    { GL_SRGB8_ALPHA8,   GL_RGBA, "", CLEAR_TYPE_FLOAT32 }, //  8  8  8  8
    { GL_R16F,           GL_RED, "r16f", CLEAR_TYPE_FLOAT32 },  //  f16
    { GL_RG16F,          GL_RG, "rg16f", CLEAR_TYPE_FLOAT32 },   //  f16  f16
    { GL_RGB16F,         GL_RGB, "", CLEAR_TYPE_FLOAT32 },  //  f16  f16  f16
    { GL_RGBA16F,        GL_RGBA, "rgba16f", CLEAR_TYPE_FLOAT32 }, //  f16  f16  f16  f16
    { GL_R32F,           GL_RED, "r32f", CLEAR_TYPE_FLOAT32 },  //  f32
    { GL_RG32F,          GL_RG, "rg32f", CLEAR_TYPE_FLOAT32 },   //  f32  f32
    { GL_RGB32F,         GL_RGB, "", CLEAR_TYPE_FLOAT32 },  //  f32  f32  f32
    { GL_RGBA32F,        GL_RGBA, "rgba32f", CLEAR_TYPE_FLOAT32 }, //  f32  f32  f32  f32
    { GL_R11F_G11F_B10F, GL_RGB, "r11f_g11f_b10f", CLEAR_TYPE_FLOAT32 },  //  f11  f11  f10
    { GL_RGB9_E5,        GL_RGB, "", CLEAR_TYPE_FLOAT32 },  //  9  9  9     5
    { GL_R8I,            GL_RED, "r8i", CLEAR_TYPE_INT32 },  //  i8
    { GL_R8UI,           GL_RED, "r8ui", CLEAR_TYPE_UINT32 },  //  ui8
    { GL_R16I,           GL_RED, "r16i", CLEAR_TYPE_INT32 },  //  i16
    { GL_R16UI,          GL_RED, "r16ui", CLEAR_TYPE_UINT32 },  //  ui16
    { GL_R32I,           GL_RED, "r32i", CLEAR_TYPE_INT32 },  //  i32
    { GL_R32UI,          GL_RED, "r32ui", CLEAR_TYPE_UINT32 },  //  ui32
    { GL_RG8I,           GL_RG, "rg8i", CLEAR_TYPE_INT32 },   //  i8   i8
    { GL_RG8UI,          GL_RG, "rg8ui", CLEAR_TYPE_UINT32 },   //  ui8   ui8
    { GL_RG16I,          GL_RG, "rg16i", CLEAR_TYPE_INT32 },   //  i16   i16
    { GL_RG16UI,         GL_RG, "rg16ui", CLEAR_TYPE_UINT32 },   //  ui16  ui16
    { GL_RG32I,          GL_RG, "rg32i", CLEAR_TYPE_INT32 },   //  i32   i32
    { GL_RG32UI,         GL_RG, "rg32ui", CLEAR_TYPE_UINT32 },   //  ui32  ui32
    { GL_RGB8I,          GL_RGB, "", CLEAR_TYPE_INT32 },  //  i8   i8   i8
    { GL_RGB8UI,         GL_RGB, "", CLEAR_TYPE_UINT32 },  //  ui8  ui8  ui8
    { GL_RGB16I,         GL_RGB, "", CLEAR_TYPE_INT32 },  //  i16  i16  i16
    { GL_RGB16UI,        GL_RGB, "", CLEAR_TYPE_UINT32 },  //  ui16 ui16 ui16
    { GL_RGB32I,         GL_RGB, "", CLEAR_TYPE_INT32 },  //  i32  i32  i32
    { GL_RGB32UI,        GL_RGB, "", CLEAR_TYPE_UINT32 },  //  ui32 ui32 ui32
    { GL_RGBA8I,         GL_RGBA, "rgba8i", CLEAR_TYPE_INT32 }, //  i8   i8   i8   i8
    { GL_RGBA8UI,        GL_RGBA, "rgba8ui", CLEAR_TYPE_UINT32 }, //  ui8  ui8  ui8  ui8
    { GL_RGBA16I,        GL_RGBA, "rgba16i", CLEAR_TYPE_INT32 }, //  i16  i16  i16  i16
    { GL_RGBA16UI,       GL_RGBA, "rgba16ui", CLEAR_TYPE_UINT32 }, //  ui16 ui16 ui16 ui16
    { GL_RGBA32I,        GL_RGBA, "rgba32i", CLEAR_TYPE_INT32 }, //  i32  i32  i32  i32
    { GL_RGBA32UI,       GL_RGBA, "rgba32ui", CLEAR_TYPE_UINT32 }, //  ui32 ui32 ui32 ui32

    // Compressed formats:
    { GL_COMPRESSED_RED,                     GL_RED, "", 0 },  //  Generic
    { GL_COMPRESSED_RG,                      GL_RG, "", 0 },   //  Generic
    { GL_COMPRESSED_RGB,                     GL_RGB, "", 0 },  //  Generic
    { GL_COMPRESSED_RGBA,                    GL_RGBA, "", 0 }, //  Generic
    { GL_COMPRESSED_SRGB,                    GL_RGB, "", 0 },  //  Generic
    { GL_COMPRESSED_SRGB_ALPHA,              GL_RGBA, "", 0 }, //  Generic
    { GL_COMPRESSED_RED_RGTC1,               GL_RED, "", 0 },  //  Specific GL_ARB_texture_compression_rgtc or GL_EXT_texture_compression_rgtc
    { GL_COMPRESSED_SIGNED_RED_RGTC1,        GL_RED, "", 0 },  //  Specific GL_ARB_texture_compression_rgtc or GL_EXT_texture_compression_rgtc
    { GL_COMPRESSED_RG_RGTC2,                GL_RG, "", 0 },   //  Specific  GL_ARB_texture_compression_rgtc or GL_EXT_texture_compression_rgtc
    { GL_COMPRESSED_SIGNED_RG_RGTC2,         GL_RG, "", 0 },   //  Specific  GL_ARB_texture_compression_rgtc or GL_EXT_texture_compression_rgtc
    { GL_COMPRESSED_RGBA_BPTC_UNORM,         GL_RGBA, "", 0 }, //  Specific    4.2
    { GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,   GL_RGBA, "", 0 }, //  Specific    4.2
    { GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,   GL_RGB, "", 0 },  //  Specific     4.2
    { GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT, GL_RGB, "", 0 },  //  Specific     4.2

    { GL_COMPRESSED_RGB_S3TC_DXT1_EXT,       GL_RGB, "", 0 },  // only with IsTextureCompressionS3tcSupported()
    { GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,      GL_RGBA, "", 0 }, // only with IsTextureCompressionS3tcSupported()
    { GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,      GL_RGBA, "", 0 }, // only with IsTextureCompressionS3tcSupported()
    { GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,      GL_RGBA, "", 0 }, // only with IsTextureCompressionS3tcSupported()

    { GL_COMPRESSED_SRGB_S3TC_DXT1_EXT,       GL_RGB, "", 0 },  // only with IsTextureCompressionS3tcSupported() and GL_EXT_texture_sRGB supported
    { GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, GL_RGBA, "", 0 }, // only with IsTextureCompressionS3tcSupported() and GL_EXT_texture_sRGB supported
    { GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, GL_RGBA, "", 0 }, // only with IsTextureCompressionS3tcSupported() and GL_EXT_texture_sRGB supported
    { GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_RGBA, "", 0 }, // only with IsTextureCompressionS3tcSupported() and GL_EXT_texture_sRGB supported

    // Depth and stencil formats:
    { GL_STENCIL_INDEX1,                     GL_STENCIL_INDEX, "", CLEAR_TYPE_STENCIL_ONLY },
    { GL_STENCIL_INDEX4,                     GL_STENCIL_INDEX, "", CLEAR_TYPE_STENCIL_ONLY },
    { GL_STENCIL_INDEX8,                     GL_STENCIL_INDEX, "", CLEAR_TYPE_STENCIL_ONLY },
    { GL_STENCIL_INDEX16,                    GL_STENCIL_INDEX, "", CLEAR_TYPE_STENCIL_ONLY },
    { GL_DEPTH_COMPONENT16,                  GL_DEPTH_COMPONENT, "", CLEAR_TYPE_DEPTH_ONLY },
    { GL_DEPTH_COMPONENT24,                  GL_DEPTH_COMPONENT, "", CLEAR_TYPE_DEPTH_ONLY },
    { GL_DEPTH_COMPONENT32,                  GL_DEPTH_COMPONENT, "", CLEAR_TYPE_DEPTH_ONLY },
    { GL_DEPTH24_STENCIL8,                   GL_DEPTH_STENCIL, "", CLEAR_TYPE_DEPTH_STENCIL },
    { GL_DEPTH32F_STENCIL8,                  GL_DEPTH_STENCIL, "", CLEAR_TYPE_DEPTH_STENCIL }
};

struct TableMagicTextureFormat {
    GLint   InternalFormat;
    GLenum  Format;
    GLenum  PixelType;
    int     SizeOf;
};

constexpr TableMagicTextureFormat MagicTextureFormatLUT[] = {
    { GL_STENCIL_INDEX1,    GL_STENCIL_INDEX,   GL_UNSIGNED_BYTE, 1 },
    { GL_STENCIL_INDEX4,    GL_STENCIL_INDEX,   GL_UNSIGNED_BYTE, 1 },
    { GL_STENCIL_INDEX8,    GL_STENCIL_INDEX,   GL_UNSIGNED_BYTE, 1 },
    { GL_STENCIL_INDEX16,   GL_STENCIL_INDEX,   GL_UNSIGNED_BYTE, 1 },
    { GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_FLOAT, 4 },
    { GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT, 4 },
    { GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT, 4 },
    { GL_DEPTH24_STENCIL8,  GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8, 4 },
    { GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL,   GL_FLOAT_32_UNSIGNED_INT_24_8_REV, 8 } // FIXME: SizeOf 8 or 4? // 3.0 or GL_ARB_depth_buffer_float
};

/*

Conversion from TEXTURE_PIXEL_FORMAT

*/

struct TableTexturePixelFormat {
    GLenum   Format;
    GLenum   PixelType;
    int      SizeOf;
};

constexpr TableTexturePixelFormat TexturePixelFormatLUT[] = {
    { GL_RED,  GL_BYTE, 1 },
    { GL_RG,   GL_BYTE, 2 },
    { GL_RGB,  GL_BYTE, 3 },
    { GL_RGBA, GL_BYTE, 4 },

    { GL_BGR,  GL_BYTE, 3 },
    { GL_BGRA, GL_BYTE, 4 },

    { GL_RED,  GL_UNSIGNED_BYTE, 1 },
    { GL_RG,   GL_UNSIGNED_BYTE, 2 },
    { GL_RGB,  GL_UNSIGNED_BYTE, 3 },
    { GL_RGBA, GL_UNSIGNED_BYTE, 4 },

    { GL_BGR,  GL_UNSIGNED_BYTE, 3 },
    { GL_BGRA, GL_UNSIGNED_BYTE, 4 },

    { GL_RED,  GL_SHORT, 2 },
    { GL_RG,   GL_SHORT, 4 },
    { GL_RGB,  GL_SHORT, 6 },
    { GL_RGBA, GL_SHORT, 8 },

    { GL_BGR,  GL_SHORT, 6 },
    { GL_BGRA, GL_SHORT, 8 },

    { GL_RED,  GL_UNSIGNED_SHORT, 2 },
    { GL_RG,   GL_UNSIGNED_SHORT, 4 },
    { GL_RGB,  GL_UNSIGNED_SHORT, 6 },
    { GL_RGBA, GL_UNSIGNED_SHORT, 8 },

    { GL_BGR,  GL_UNSIGNED_SHORT, 6 },
    { GL_BGRA, GL_UNSIGNED_SHORT, 8 },

    { GL_RED_INTEGER,  GL_INT, 4 },
    { GL_RG_INTEGER,   GL_INT, 8 },
    { GL_RGB_INTEGER,  GL_INT, 12 },
    { GL_RGBA_INTEGER, GL_INT, 16 },

    { GL_BGR_INTEGER,  GL_INT, 12 },
    { GL_BGRA_INTEGER, GL_INT, 16 },

    { GL_RED_INTEGER,  GL_UNSIGNED_INT, 4 },
    { GL_RG_INTEGER,   GL_UNSIGNED_INT, 8 },
    { GL_RGB_INTEGER,  GL_UNSIGNED_INT, 12 },
    { GL_RGBA_INTEGER, GL_UNSIGNED_INT, 16 },

    { GL_BGR_INTEGER,  GL_UNSIGNED_INT, 12 },
    { GL_BGRA_INTEGER, GL_UNSIGNED_INT, 16 },

    { GL_RED,  GL_HALF_FLOAT, 2 },
    { GL_RG,   GL_HALF_FLOAT, 4 },
    { GL_RGB,  GL_HALF_FLOAT, 6 },
    { GL_RGBA, GL_HALF_FLOAT, 8 },

    { GL_BGR,  GL_HALF_FLOAT, 6 },
    { GL_BGRA, GL_HALF_FLOAT, 8 },

    { GL_RED,  GL_FLOAT, 4 },
    { GL_RG,   GL_FLOAT, 8 },
    { GL_RGB,  GL_FLOAT, 12 },
    { GL_RGBA, GL_FLOAT, 16 },

    { GL_BGR,  GL_FLOAT, 12 },
    { GL_BGRA, GL_FLOAT, 16 },

    { GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 0, 0 },
    { GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 0, 0 },
    { GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 0, 0 },
    { GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 0, 0 },

    { GL_COMPRESSED_SRGB_S3TC_DXT1_EXT, 0, 0 },
    { GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, 0, 0 },
    { GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, 0, 0 },
    { GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, 0, 0 },

    { GL_COMPRESSED_RED_RGTC1, 0, 0 },
    { GL_COMPRESSED_RG_RGTC2, 0, 0 },

    { GL_COMPRESSED_RGBA_BPTC_UNORM, 0, 0 },
    { GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM, 0, 0 },
    { GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT, 0, 0 },
    { GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT, 0, 0 },

    { GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, 1 },
    { GL_DEPTH_COMPONENT, GL_FLOAT, 4 },
    { GL_DEPTH_COMPONENT, GL_FLOAT, 4 },
    { GL_DEPTH_COMPONENT, GL_FLOAT, 4 },
    { GL_DEPTH_STENCIL, GL_FLOAT, 4 },
    { GL_DEPTH_STENCIL, GL_FLOAT, 8 }  // FIXME: SizeOf 8 or 4?
};

/*

Conversion from FRAMEBUFFER_CHANNEL

*/

constexpr GLenum FramebufferChannelLUT[] = {
    GL_RED,
    GL_GREEN,
    GL_BLUE,
    GL_RGB,
    GL_BGR,
    GL_RGBA,
    GL_BGRA,
    GL_STENCIL_INDEX,
    GL_DEPTH_COMPONENT,
    GL_DEPTH_STENCIL
};

/*

Conversion from FRAMEBUFFER_OUTPUT

*/

constexpr GLenum FramebufferOutputLUT[] = {
    GL_UNSIGNED_BYTE,
    GL_BYTE,
    GL_UNSIGNED_SHORT,
    GL_SHORT,
    GL_UNSIGNED_INT,
    GL_INT,
    GL_HALF_FLOAT,      // FIXME: Only if IsHalfFloatVertexSupported
    GL_FLOAT
};

/*

Conversion from FRAMEBUFFER_ATTACHMENT

*/

constexpr GLenum FramebufferAttachmentLUT[] = {
    GL_DEPTH_ATTACHMENT,
    GL_STENCIL_ATTACHMENT,
    GL_DEPTH_STENCIL_ATTACHMENT,
    GL_FRONT,
    GL_BACK,
    GL_FRONT_LEFT,
    GL_FRONT_RIGHT,
    GL_BACK_LEFT,
    GL_BACK_RIGHT,
    GL_COLOR,
    GL_DEPTH,
    GL_STENCIL
};

/*

Conversion from BUFFER_DATA_TYPE

*/

struct TableBufferDataType {
    GLint     InternalFormat;
    GLenum    BaseType;
    GLenum    Format;
    bool      Normalized;
    int       NumComponents;
    int       SizeOfComponent;
    INTERNAL_PIXEL_FORMAT IPF;
};

constexpr TableBufferDataType BufferDataTypeLUT[] = {
    { GL_R8	       ,GL_UNSIGNED_BYTE	    ,GL_RED	,true , 1, 1, INTERNAL_PIXEL_FORMAT_R8}, // R	0	0	1
    { GL_R16	   ,GL_UNSIGNED_SHORT       ,GL_RED	,true , 1, 2, INTERNAL_PIXEL_FORMAT_R16}, // R	0	0	1
    { GL_R16F	   ,GL_HALF_FLOAT	        ,GL_RED	,false, 1, 2, INTERNAL_PIXEL_FORMAT_R16F}, // R	0	0	1
    { GL_R32F	   ,GL_FLOAT	            ,GL_RED	,false, 1, 4, INTERNAL_PIXEL_FORMAT_R32F}, // R	0	0	1
    { GL_R8I	   ,GL_BYTE	                ,GL_RED	,false, 1, 1, INTERNAL_PIXEL_FORMAT_R8I}, // R	0	0	1
    { GL_R16I	   ,GL_SHORT	            ,GL_RED	,false, 1, 2, INTERNAL_PIXEL_FORMAT_R16I}, // R	0	0	1
    { GL_R32I	   ,GL_INT	                ,GL_RED	,false, 1, 4, INTERNAL_PIXEL_FORMAT_R32I}, // R	0	0	1
    { GL_R8UI	   ,GL_UNSIGNED_BYTE	    ,GL_RED	,false, 1, 1, INTERNAL_PIXEL_FORMAT_R8UI}, // R	0	0	1
    { GL_R16UI     ,GL_UNSIGNED_SHORT       ,GL_RED	,false, 1, 2, INTERNAL_PIXEL_FORMAT_R16UI}, // R	0	0	1
    { GL_R32UI     ,GL_UNSIGNED_INT	        ,GL_RED	,false, 1, 4, INTERNAL_PIXEL_FORMAT_R32UI}, // R	0	0	1
    { GL_RG8	   ,GL_UNSIGNED_BYTE	    ,GL_RG	,true , 2, 1, INTERNAL_PIXEL_FORMAT_RG8}, // R	G	0	1
    { GL_RG16	   ,GL_UNSIGNED_SHORT       ,GL_RG	,true , 2, 2, INTERNAL_PIXEL_FORMAT_RG16}, // R	G	0	1
    { GL_RG16F     ,GL_HALF_FLOAT	        ,GL_RG	,false, 2, 2, INTERNAL_PIXEL_FORMAT_RG16F}, // R	G	0	1
    { GL_RG32F     ,GL_FLOAT	            ,GL_RG	,false, 2, 4, INTERNAL_PIXEL_FORMAT_RG32F}, // R	G	0	1
    { GL_RG8I	   ,GL_BYTE	                ,GL_RG	,false, 2, 1, INTERNAL_PIXEL_FORMAT_RG8I}, // R	G	0	1
    { GL_RG16I     ,GL_SHORT	            ,GL_RG	,false, 2, 2, INTERNAL_PIXEL_FORMAT_RG16I}, // R	G	0	1
    { GL_RG32I     ,GL_INT	                ,GL_RG	,false, 2, 4, INTERNAL_PIXEL_FORMAT_RG32I}, // R	G	0	1
    { GL_RG8UI     ,GL_UNSIGNED_BYTE	    ,GL_RG	,false, 2, 1, INTERNAL_PIXEL_FORMAT_RG8UI}, // R	G	0	1
    { GL_RG16UI    ,GL_UNSIGNED_SHORT       ,GL_RG	,false, 2, 2, INTERNAL_PIXEL_FORMAT_RG16UI}, // R	G	0	1
    { GL_RG32UI    ,GL_UNSIGNED_INT	        ,GL_RG	,false, 2, 4, INTERNAL_PIXEL_FORMAT_RG32UI}, // R	G	0	1
    { GL_RGB32F    ,GL_FLOAT	            ,GL_RGB	,false, 3, 4, INTERNAL_PIXEL_FORMAT_RGB32F}, // R	G	B	1
    { GL_RGB32I    ,GL_INT	                ,GL_RGB	,false, 3, 4, INTERNAL_PIXEL_FORMAT_RGB32I}, // R	G	B	1
    { GL_RGB32UI   ,GL_UNSIGNED_INT	        ,GL_RGB	,false, 3, 4, INTERNAL_PIXEL_FORMAT_RGB32UI}, // R	G	B	1
    { GL_RGBA8     ,GL_UNSIGNED_INT	        ,GL_RGBA,true , 4, 4, INTERNAL_PIXEL_FORMAT_RGBA8}, // R	G	B	A
    { GL_RGBA16    ,GL_SHORT	            ,GL_RGBA,true , 4, 2, INTERNAL_PIXEL_FORMAT_RGBA16}, // R	G	B	A
    { GL_RGBA16F   ,GL_HALF_FLOAT	        ,GL_RGBA,false, 4, 2, INTERNAL_PIXEL_FORMAT_RGBA16F}, // R	G	B	A
    { GL_RGBA32F   ,GL_FLOAT	            ,GL_RGBA,false, 4, 4, INTERNAL_PIXEL_FORMAT_RGBA32F}, // R	G	B	A
    { GL_RGBA8I    ,GL_BYTE	                ,GL_RGBA,false, 4, 1, INTERNAL_PIXEL_FORMAT_RGBA8I}, // R	G	B	A
    { GL_RGBA16I   ,GL_SHORT	            ,GL_RGBA,false, 4, 2, INTERNAL_PIXEL_FORMAT_RGBA16I}, // R	G	B	A
    { GL_RGBA32I   ,GL_INT	                ,GL_RGBA,false, 4, 4, INTERNAL_PIXEL_FORMAT_RGBA32I}, // R	G	B	A
    { GL_RGBA8UI   ,GL_UNSIGNED_BYTE	    ,GL_RGBA,false, 4, 1, INTERNAL_PIXEL_FORMAT_RGBA8UI}, // R	G	B	A
    { GL_RGBA16UI  ,GL_UNSIGNED_SHORT       ,GL_RGBA,false, 4, 2, INTERNAL_PIXEL_FORMAT_RGBA16UI}, // R	G	B	A
    { GL_RGBA32UI  ,GL_UNSIGNED_INT         ,GL_RGBA,false, 4, 4, INTERNAL_PIXEL_FORMAT_RGBA32UI}  // R	G	B	A
};


/*

Conversion from VERTEX_ATTRIB_COMPONENT

*/

constexpr GLenum VertexAttribTypeLUT[] = {
    GL_BYTE,
    GL_UNSIGNED_BYTE,
    GL_SHORT,
    GL_UNSIGNED_SHORT,
    GL_INT,
    GL_UNSIGNED_INT,
    GL_HALF_FLOAT,          // FIXME: only with ARB_half_float_vertex ???
    GL_FLOAT,
    GL_DOUBLE
};

/*

Conversion from PRIMITIVE_TOPOLOGY

*/

constexpr GLenum PrimitiveTopologyLUT[] = {
    GL_TRIANGLES,           // Use triangles for undefined topology
    GL_POINTS,
    GL_LINES,
    GL_LINE_STRIP,
    GL_LINE_LOOP,
    GL_TRIANGLES,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLE_FAN,
    GL_LINES_ADJACENCY,
    GL_LINE_STRIP_ADJACENCY,
    GL_TRIANGLES_ADJACENCY,
    GL_TRIANGLE_STRIP_ADJACENCY
};

/*

Conversion from SAMPLER_FILTER

*/

struct TableSamplerFilter {
    int Min;
    int Mag;
};

constexpr TableSamplerFilter SamplerFilterModeLUT[] = {
    { GL_NEAREST,                   GL_NEAREST },
    { GL_LINEAR,                    GL_NEAREST },
    { GL_NEAREST_MIPMAP_NEAREST,    GL_NEAREST },
    { GL_LINEAR_MIPMAP_NEAREST,     GL_NEAREST },
    { GL_NEAREST_MIPMAP_LINEAR,     GL_NEAREST },
    { GL_LINEAR_MIPMAP_LINEAR,      GL_NEAREST },

    { GL_NEAREST,                   GL_LINEAR },
    { GL_LINEAR,                    GL_LINEAR },
    { GL_NEAREST_MIPMAP_NEAREST,    GL_LINEAR },
    { GL_LINEAR_MIPMAP_NEAREST,     GL_LINEAR },
    { GL_NEAREST_MIPMAP_LINEAR,     GL_LINEAR },
    { GL_LINEAR_MIPMAP_LINEAR,      GL_LINEAR }
};

/*

Conversion from SAMPLER_ADDRESS_MODE

*/

constexpr int SamplerAddressModeLUT[] = {
    GL_REPEAT,
    GL_MIRRORED_REPEAT,
    GL_CLAMP_TO_EDGE,
    GL_CLAMP_TO_BORDER,
    GL_MIRROR_CLAMP_TO_EDGE     // GL 4.4 or greater
};

/*

Conversion from SHADER_TYPE

*/

constexpr GLenum ShaderTypeLUT[] = {
    GL_VERTEX_SHADER,
    GL_FRAGMENT_SHADER,
    GL_TESS_CONTROL_SHADER,
    GL_TESS_EVALUATION_SHADER,
    GL_GEOMETRY_SHADER,
    GL_COMPUTE_SHADER
};

/*

Conversion from COLOR_CLAMP

*/

constexpr GLenum ColorClampLUT[] = {
    GL_FALSE,
    GL_TRUE,
    GL_FIXED_ONLY
};

/*

Conversion from QUERY_TYPE

*/

constexpr GLenum TableQueryTarget[] = {
    GL_SAMPLES_PASSED,
    GL_ANY_SAMPLES_PASSED,
    GL_ANY_SAMPLES_PASSED_CONSERVATIVE,
    GL_TIME_ELAPSED,
    GL_TIMESTAMP,
    GL_PRIMITIVES_GENERATED,
    GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN
};

/*

Conversion from CONDITIONAL_RENDER_MODE

*/

constexpr GLenum TableConditionalRenderMode[] = {
    GL_QUERY_WAIT,
    GL_QUERY_NO_WAIT,
    GL_QUERY_BY_REGION_WAIT,
    GL_QUERY_BY_REGION_NO_WAIT,
    GL_QUERY_WAIT_INVERTED,
    GL_QUERY_NO_WAIT_INVERTED,
    GL_QUERY_BY_REGION_WAIT_INVERTED,
    GL_QUERY_BY_REGION_NO_WAIT_INVERTED
};

/*

Conversion from TEXTURE_SWIZZLE

*/

constexpr GLenum SwizzleLUT[] = {
    0,
    GL_ZERO,
    GL_ONE,
    GL_RED,
    GL_GREEN,
    GL_BLUE,
    GL_ALPHA
};

}
