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

#include "GHISampler.h"
#include "GHIDevice.h"
#include "GHIState.h"
#include "LUT.h"

#include <algorithm>
#include "GL/glew.h"

namespace GHI {

#if 0
template< typename T > T clamp( T const & _a, T const & _min, T const & _max ) {
    return std::max( std::min( _a, _max ), _min );
}

Sampler::Sampler() {
    pDevice = nullptr;
    Handle = nullptr;
}

Sampler::~Sampler() {
    Deinitialize();
}

void Sampler::Initialize( SamplerCreateInfo const & _CreateInfo ) {
    State * state = GetCurrentState();

    GLuint id;

    Deinitialize();

    pDevice = state->GetDevice();

    // 3.3 or GL_ARB_sampler_objects

    glCreateSamplers( 1, &id ); // 4.5

    glSamplerParameteri( id, GL_TEXTURE_MIN_FILTER, SamplerFilterModeLUT[ _CreateInfo.Filter ].Min );
    glSamplerParameteri( id, GL_TEXTURE_MAG_FILTER, SamplerFilterModeLUT[ _CreateInfo.Filter ].Mag );
    glSamplerParameteri( id, GL_TEXTURE_WRAP_S, SamplerAddressModeLUT[ _CreateInfo.AddressU ] );
    glSamplerParameteri( id, GL_TEXTURE_WRAP_T, SamplerAddressModeLUT[ _CreateInfo.AddressV ] );
    glSamplerParameteri( id, GL_TEXTURE_WRAP_R, SamplerAddressModeLUT[ _CreateInfo.AddressW ] );
    glSamplerParameterf( id, GL_TEXTURE_LOD_BIAS, _CreateInfo.MipLODBias );
    if ( pDevice->IsTextureAnisotropySupported() ) {
        glSamplerParameteri( id, GL_TEXTURE_MAX_ANISOTROPY_EXT, clamp< unsigned int >( _CreateInfo.MaxAnisotropy, 0u, pDevice->MaxTextureAnisotropy ) );
    }
    if ( _CreateInfo.bCompareRefToTexture ) {
        glSamplerParameteri( id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
    }
    glSamplerParameteri( id, GL_TEXTURE_COMPARE_FUNC, ComparisonFuncLUT[ _CreateInfo.ComparisonFunc ] );
    glSamplerParameterfv( id, GL_TEXTURE_BORDER_COLOR, _CreateInfo.BorderColor );
    glSamplerParameterf( id, GL_TEXTURE_MIN_LOD, _CreateInfo.MinLOD );
    glSamplerParameterf( id, GL_TEXTURE_MAX_LOD, _CreateInfo.MaxLOD );

    // We can use also these functions to retrieve sampler parameters:
    //glGetSamplerParameterfv
    //glGetSamplerParameterIiv
    //glGetSamplerParameterIuiv
    //glGetSamplerParameteriv

    Handle = ( void * )( size_t )id;

    pDevice->TotalSamplers++;
}

void Sampler::Deinitialize() {
    if ( !Handle ) {
        return;
    }

    GLuint id = GL_HANDLE( Handle );
    glDeleteSamplers( 1, &id );
    pDevice->TotalSamplers--;

    Handle = nullptr;
    pDevice = nullptr;
}
#endif

BindlessSampler::BindlessSampler()
    : Handle( 0 )
{
}

void BindlessSampler::Initialize( Texture * _Texture, void * _Sampler ) {
    Handle = glGetTextureSamplerHandleARB( GL_HANDLE( _Texture->GetHandle() ), GL_HANDLE( _Sampler ) );
}

void BindlessSampler::MakeResident() {
    glMakeTextureHandleResidentARB( Handle );
}

void BindlessSampler::MakeNonResident() {
    glMakeTextureHandleNonResidentARB( Handle );
}

bool BindlessSampler::IsResident() const {
    return !!glIsTextureHandleResidentARB( Handle );
}

}
