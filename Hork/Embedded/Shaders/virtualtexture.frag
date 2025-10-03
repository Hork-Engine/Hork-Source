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

#ifndef VIRTUALTEXTURE_h
#define VIRTUALTEXTURE_h

#define VT_MAX_ANISOTROPY_LEVEL 4
#define VT_MAX_ANISOTROPY_LEVEL_SQR ( VT_MAX_ANISOTROPY_LEVEL * VT_MAX_ANISOTROPY_LEVEL )

#define VT_MAX_TEXTURE_UNITS 256

#define VT_PAGE_BORDER_WIDTH 4.0

struct VirtualTextureUnit
{
    float MaxLod;
    float Log2Size;
};

layout( binding = 6, std140 ) uniform VirtualTextureUnits {
    VirtualTextureUnit TextureUnits[ VT_MAX_TEXTURE_UNITS ];
};

vec2 VT_CalcPhysicalUV( sampler2D TableOfIndirection, vec2 VirtualUV, uint TextureUnit )
{
    vec2 dx = dFdx( VirtualUV );
    vec2 dy = dFdy( VirtualUV );
    float dotdx = dot( dx, dx );
    float dotdy = dot( dy, dy );
    float Pmax = max( dotdx, dotdy );
    float Pmin = min( dotdx, dotdy );
    float lod = 0.5 * log2( Pmax / min( ceil( Pmax/Pmin ), VT_MAX_ANISOTROPY_LEVEL_SQR ) ) + TextureUnits[TextureUnit].Log2Size;// - reductionShift;
    lod = clamp( lod, 0.0, TextureUnits[TextureUnit].MaxLod );
    
    const float reducedLods = 0.0f;//TextureUnits[TextureUnit].ReducedLods;
    
    vec2 ind = /*floor*/(textureLod( TableOfIndirection, VirtualUV, lod + reducedLods ).rg * 255.0);

    // Position in cache
    ind.x = ind.x + (float(int(ind.y)&0x0f)*256.0);
    // Lod
    ind.y = float(int(ind.y)&0xf0) / 16.0;

    float div = ind.x / VTPageCacheCapacity.x;
    
    vec2 UV = vec2( fract(div), floor(div) / VTPageCacheCapacity.y );
    
    return fract( VirtualUV * exp2(ind.y) ) * VTPageTranslationOffsetAndScale.zw + VTPageTranslationOffsetAndScale.xy + UV;
}

float VT_FeedbackCalcLOD( vec2 VirtualUV, uint TextureUnit ) {
    vec2 coords = VirtualUV * FeedbackBufferResolutionRatio;
    vec2 dx = dFdx( coords );
    vec2 dy = dFdy( coords );
    float dotdx = dot( dx, dx );
    float dotdy = dot( dy, dy );
    float Pmax = max( dotdx, dotdy );
    float Pmin = min( dotdx, dotdy );
    float lod = 0.5 * log2( Pmax / min( ceil( Pmax / Pmin ), VT_MAX_ANISOTROPY_LEVEL_SQR ) ) + TextureUnits[TextureUnit].Log2Size;// - reductionShift;
    return clamp( lod, 0.0, TextureUnits[TextureUnit].MaxLod );
}


// Max 13 lods, 16 units
// RGBA8
// 11111111 11111111 1111 1111 1111 1111
// -------- -------- ---- ---- ---- ----
// X_low    Y_low    Un   Lod  Yh   Xh
vec4 VT_FeedbackPack_RGBA8_13LODS_16UNITS( vec2 VirtualUV, float VirtualLod, uint TextureUnit ) {
    // calc mipmap level
    float lod = round( TextureUnits[TextureUnit].MaxLod - VirtualLod );
    
    // calc relative page coords within mipmap level (allowed range 0..4095)
    vec2 pageOffset = max( floor( VirtualUV * exp2(lod) ), 0.0 );
    
    // pack coords
    const float div256 = 1.0/256.0;
    float pack = floor( pageOffset.x * div256 ) + floor( pageOffset.y * div256 ) * 16.0;
    
    // save data in range 0..1
    return vec4( mod( pageOffset, 256.0 ), lod + TextureUnit*16, pack ) / 255.0;
}

// Max 11 lods, 256 units
// RGBA8
// 11111111 11111111 1111 11  11 11111111
// -------- -------- ---- --  -- --------
// X_low    Y_low    Lod  Yh  Xh Un
#if 0
uint VT_FeedbackPack_RGBA8_11LODS_256UNITS( vec2 VirtualUV, float VirtualLod, uint TextureUnit ) {
    // calc mipmap level
    float lod = round( TextureUnits[TextureUnit].MaxLod - VirtualLod );

    // calc relative page coords within mipmap level (allowed range 0..1023)
    vec2 pageOffset = max( floor( VirtualUV * exp2(lod) ), 0.0 );

    // pack coords
    const float div256 = 1.0/256.0;
    float pack = floor( pageOffset.x * div256 ) + floor( pageOffset.y * div256 ) * 4.0 + lod * 16.0;

    ivec4 v = ivec4( mod( pageOffset, 256.0 ), pack, TextureUnit );

    return ( v.x << 24 ) | ( v.y << 16 ) | ( v.z << 8 ) | v.w;
}
#endif
vec4 VT_FeedbackPack_RGBA8_11LODS_256UNITS( vec2 VirtualUV, float VirtualLod, uint TextureUnit ) {
    // calc mipmap level
    float lod = round( TextureUnits[TextureUnit].MaxLod - VirtualLod );

    // calc relative page coords within mipmap level (allowed range 0..1023)
    float exp2lod = exp2(lod);
    vec2 pageOffset = clamp( floor( VirtualUV * exp2lod ), 0.0, exp2lod - 1.0 );
    
    #if 1
    // pack coords
    const float div256 = 1.0/256.0;
    float pack = floor( pageOffset.x * div256 ) + floor( pageOffset.y * div256 ) * 4.0 + lod * 16.0;
    
    // save data in range 0..1
    return vec4( mod( pageOffset, 256.0 ), pack, TextureUnit ) / 255.0;
    #endif
    
    #if 0
    // 11111111 11111111 1111 11  11 11111111
    ivec2 pageOffsetInt = ivec2( pageOffset );
    int x_low = pageOffsetInt.x & 0xff;
    int y_low = pageOffsetInt.y & 0xff;
    int x_hi = ( pageOffsetInt.x >> 8 ) & 3;
    int y_hi = ( pageOffsetInt.y >> 8 ) & 3;
    int lodInt = int(lod) & 0xf;
    
    return vec4( x_low, y_low, lodInt | (y_hi << 2 ) | x_hi, TextureUnit ) / 255.0;
    #endif
}
/*
// 11111111 11111111 1111 11  11 11111111
// -------- -------- ---- --  -- --------
// X_low    Y_low    Lod  Yh  Xh Un
AN_FORCEINLINE void VT_FeedbackUnpack_RGBA8_11LODS_256UNITS( SFeedbackData const *_Data,
                                                             int & _PageX, int & _PageY, int & _Lod,
                                                             int & _TextureUnit ) {
    _PageX = _Data->Byte3 | ((_Data->Byte1 & 3) << 8);
    _PageY = _Data->Byte2 | ((_Data->Byte1 & 12) << 6);
    _Lod = _Data->Byte1 >> 4;
    _TextureUnit = _Data->Byte0;
}
*/
// Max 8 lods, 32768 units
// RGBA8
// 1111111x 1111111x 111 11111 11111111
// X      U Y      U Lod U     U
uint VT_FeedbackPack_RGBA8_8LODS_32768UNITS( vec2 VirtualUV, float VirtualLod, uint TextureUnit ) {
    // calc mipmap level
    float lod = round( TextureUnits[TextureUnit].MaxLod - VirtualLod );

    // calc relative page coords within mipmap level (allowed range 0..1023)
    ivec2 pageOffset = ivec2( max( VirtualUV * exp2(lod), 0.0 ) );

    int ilod = int(lod);

    return ( pageOffset.x << 24 ) | ( pageOffset.y << 16 ) | TextureUnit | ( ( ilod & 4 ) << 29 ) | ( ( ilod & 2 ) << 22 ) | ( ( ilod & 1 ) << 15 );
}

// Max 13 lods, 256 units (was not checked)
// RGB12
// 111111111111 111111111111 11111111 1111
// ------------ ------------ -------- ----
// X            Y            Un       Lod
vec4 VT_FeedbackPack_RGB12_13LODS_256_UNITS( vec2 VirtualUV, float VirtualLod, uint TextureUnit ) {
    // calc mipmap level
    float lod = round( TextureUnits[TextureUnit].MaxLod - VirtualLod );
    
    // calc relative page coords within mipmap level (allowed range 0..4095)
    vec2 pageOffset = max( floor( VirtualUV * exp2(lod) ), 0.0 );
    
    // save data in range 0..1
    return vec4( pageOffset, lod + TextureUnit*16, 0.0 ) / 4095.0;
}

// Max 11 lods, 1 unit
// RGB8
// 11111111 11111111 1111 11  11
// -------- -------- ---- --  --
// X_low    Y_low    Lod  Yh  Xh
vec4 VT_FeedbackPack_RGB8_11LODS_1UNIT( vec2 VirtualUV, float VirtualLod ) {
    // calc mipmap level
    float lod = round( TextureUnits[0].MaxLod - VirtualLod );
    
    // calc relative page coords within mipmap level (allowed range 0..1023)
    vec2 pageOffset = max( floor( VirtualUV * exp2(lod) ), 0.0 );
    
    // pack coords
    const float div256 = 1.0/256.0;
    float pack = floor( pageOffset.x * div256 ) + floor( pageOffset.y * div256 ) * 4.0 + lod * 16.0;
    
    // save data in range 0..1
    return vec4( mod( pageOffset, 256.0 ), pack, 255.0 ) / 255.0;
}

// Max 10 lods, 4 units
// RGB8
// 11111111 11111111 1111 11  1   1
// -------- -------- ---- --  -   -
// X_low    Y_low    Lod  Un  Yh  Xh
vec4 VT_FeedbackPack_RGB8_10LODS_4UNITS( vec2 VirtualUV, float VirtualLod, uint TextureUnit ) {
    // calc mipmap level
    float lod = round( TextureUnits[TextureUnit].MaxLod - VirtualLod );
    
    // calc relative page coords within mipmap level (allowed range 0..511)
    vec2 pageOffset = max( floor( VirtualUV * exp2(lod) ), 0.0 );
    
    // pack coords
    const float div256 = 1.0/256.0;
    float pack = floor( pageOffset.x * div256 ) + floor( pageOffset.y * div256 ) * 2.0 + TextureUnit * 4 + lod * 16.0;
    
    // save data in range 0..1
    return vec4( mod( pageOffset, 256.0 ), pack, 255.0 ) / 255.0;
}

#endif // VIRTUALTEXTURE_h
