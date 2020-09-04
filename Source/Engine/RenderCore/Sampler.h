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

#include "GraphicsDefs.h"

namespace RenderCore {

class ITexture;

enum SAMPLER_FILTER : uint8_t
{
    FILTER_MIN_NEAREST_MAG_NEAREST = 0,
    FILTER_MIN_LINEAR_MAG_NEAREST,
    FILTER_MIN_NEAREST_MIPMAP_NEAREST_MAG_NEAREST,
    FILTER_MIN_LINEAR_MIPMAP_NEAREST_MAG_NEAREST,
    FILTER_MIN_NEAREST_MIPMAP_LINEAR_MAG_NEAREST,
    FILTER_MIN_LINEAR_MIPMAP_LINEAR_MAG_NEAREST,

    FILTER_MIN_NEAREST_MAG_LINEAR,
    FILTER_MIN_LINEAR_MAG_LINEAR,
    FILTER_MIN_NEAREST_MIPMAP_NEAREST_MAG_LINEAR,
    FILTER_MIN_LINEAR_MIPMAP_NEAREST_MAG_LINEAR,
    FILTER_MIN_NEAREST_MIPMAP_LINEAR_MAG_LINEAR,
    FILTER_MIN_LINEAR_MIPMAP_LINEAR_MAG_LINEAR,

    FILTER_NEAREST          = FILTER_MIN_NEAREST_MAG_NEAREST,
    FILTER_LINEAR           = FILTER_MIN_LINEAR_MAG_LINEAR,
    FILTER_MIPMAP_NEAREST   = FILTER_MIN_NEAREST_MIPMAP_NEAREST_MAG_NEAREST,
    FILTER_MIPMAP_BILINEAR  = FILTER_MIN_LINEAR_MIPMAP_NEAREST_MAG_LINEAR,
    FILTER_MIPMAP_NLINEAR   = FILTER_MIN_NEAREST_MIPMAP_LINEAR_MAG_NEAREST,
    FILTER_MIPMAP_TRILINEAR = FILTER_MIN_LINEAR_MIPMAP_LINEAR_MAG_LINEAR
};

enum SAMPLER_ADDRESS_MODE : uint8_t
{
    SAMPLER_ADDRESS_WRAP         = 0,
    SAMPLER_ADDRESS_MIRROR       = 1,
    SAMPLER_ADDRESS_CLAMP        = 2,
    SAMPLER_ADDRESS_BORDER       = 3,
    SAMPLER_ADDRESS_MIRROR_ONCE  = 4
};

struct SSamplerInfo
{
    SAMPLER_FILTER       Filter;             // filtering method to use when sampling a texture
    SAMPLER_ADDRESS_MODE AddressU;
    SAMPLER_ADDRESS_MODE AddressV;
    SAMPLER_ADDRESS_MODE AddressW;
    float                MipLODBias;
    uint8_t              MaxAnisotropy;      // only with IsTextureAnisotropySupported
    COMPARISON_FUNCTION  ComparisonFunc;     // a function that compares sampled data against existing sampled data
    bool                 bCompareRefToTexture;
    float                BorderColor[4];
    float                MinLOD;
    float                MaxLOD;
    bool                 bCubemapSeamless;

    SSamplerInfo()
    {
        // Default values from OpenGL specification
        Filter = FILTER_MIN_NEAREST_MIPMAP_LINEAR_MAG_LINEAR;
        AddressU = SAMPLER_ADDRESS_WRAP;
        AddressV = SAMPLER_ADDRESS_WRAP;
        AddressW = SAMPLER_ADDRESS_WRAP;
        MipLODBias = 0;
        MaxAnisotropy = 0;
        ComparisonFunc = CMPFUNC_LEQUAL;
        bCompareRefToTexture = false;
        BorderColor[0] = 0;
        BorderColor[1] = 0;
        BorderColor[2] = 0;
        BorderColor[3] = 0;
        MinLOD = -1000;
        MaxLOD = 1000;
        bCubemapSeamless = false;
    }

    SSamplerInfo & SetFilter( SAMPLER_FILTER InFilter ) {
        Filter = InFilter;
        return *this;
    }

    SSamplerInfo & SetAddress( SAMPLER_ADDRESS_MODE InAddress ) {
        AddressU = InAddress;
        AddressV = InAddress;
        AddressW = InAddress;
        return *this;
    }

    SSamplerInfo & SetAddressU( SAMPLER_ADDRESS_MODE InAddressU ) {
        AddressU = InAddressU;
        return *this;
    }

    SSamplerInfo & SetAddressV( SAMPLER_ADDRESS_MODE InAddressV ) {
        AddressV = InAddressV;
        return *this;
    }

    SSamplerInfo & SetAddressW( SAMPLER_ADDRESS_MODE InAddressW ) {
        AddressW = InAddressW;
        return *this;
    }

    SSamplerInfo & SetMipLODBias( float InMipLODBias ) {
        MipLODBias = InMipLODBias;
        return *this;
    }

    SSamplerInfo & SetMaxAnisotropy( uint8_t InMaxAnisotropy ) {
        MaxAnisotropy = InMaxAnisotropy;
        return *this;
    }

    SSamplerInfo & SetComparisonFunc( COMPARISON_FUNCTION InComparisonFunc ) {
        ComparisonFunc = InComparisonFunc;
        return *this;
    }

    SSamplerInfo & SetCompareRefToTexture( bool InbCompareRefToTexture ) {
        bCompareRefToTexture = InbCompareRefToTexture;
        return *this;
    }

    SSamplerInfo & SetBorderColor( float InR, float InG, float InB, float InA ) {
        BorderColor[0] = InR;
        BorderColor[1] = InG;
        BorderColor[2] = InB;
        BorderColor[3] = InA;
        return *this;
    }

    SSamplerInfo & SetMinLOD( float InMinLOD ) {
        MinLOD = InMinLOD;
        return *this;
    }

    SSamplerInfo & SetMaxLOD( float InMaxLOD ) {
        MaxLOD = InMaxLOD;
        return *this;
    }

    SSamplerInfo & SetCubemapSeamless( bool InbCubemapSeamless ) {
        bCubemapSeamless = InbCubemapSeamless;
        return *this;
    }
};

class IBindlessSampler : public IObjectInterface
{
public:
    virtual void MakeResident() = 0;

    virtual void MakeNonResident() = 0;

    virtual bool IsResident() const = 0;

    uint64_t GetHandle() const { return Handle; }

protected:
    uint64_t Handle;
};

}
