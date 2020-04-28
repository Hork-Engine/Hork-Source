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

#include "OpenGL45Common.h"

namespace OpenGL45 {

class ADepthPass : public GHI::Pipeline {
public:
    void Create( const char * _SourceCode, GHI::POLYGON_CULL _CullMode, bool _Skinned );
};

class AWireframePass : public GHI::Pipeline {
public:
    void Create( const char * _SourceCode, GHI::POLYGON_CULL _CullMode, bool _Skinned );
};

class ANormalsPass : public GHI::Pipeline {
public:
    void Create( const char * _SourceCode, GHI::POLYGON_CULL _CullMode, bool _Skinned );
};

class AColorPassHUD : public GHI::Pipeline {
public:
    void Create( const char * _SourceCode );
};

class AColorPass : public GHI::Pipeline {
public:
    void Create( const char * _SourceCode, GHI::POLYGON_CULL _CullMode, bool _Skinned, bool _DepthTest, bool _Translucent, EColorBlending _Blending );
};

class AColorPassLightmap : public GHI::Pipeline {
public:
    void Create( const char * _SourceCode, GHI::POLYGON_CULL _CullMode, bool _DepthTest, bool _Translucent, EColorBlending _Blending );
};

class AColorPassVertexLight : public GHI::Pipeline {
public:
    void Create( const char * _SourceCode, GHI::POLYGON_CULL _CullMode, bool _DepthTest, bool _Translucent, EColorBlending _Blending );
};

class AShadowMapPass : public GHI::Pipeline {
public:
    void Create( const char * _SourceCode, bool _ShadowMasking, bool _Skinned );
};


struct AShadeModelLit {
    ADepthPass DepthPass;
    ADepthPass DepthPassSkinned;
    AWireframePass WireframePass;
    AWireframePass WireframePassSkinned;
    ANormalsPass NormalsPass;
    ANormalsPass NormalsPassSkinned;
    AColorPass ColorPassSimple;
    AColorPass ColorPassSkinned;
    AColorPassLightmap ColorPassLightmap;
    AColorPassVertexLight ColorPassVertexLight;
    AShadowMapPass ShadowPass;
    AShadowMapPass ShadowPassSkinned;
};

struct AShadeModelUnlit {
    ADepthPass DepthPass;
    ADepthPass DepthPassSkinned;
    AWireframePass WireframePass;
    AWireframePass WireframePassSkinned;
    ANormalsPass NormalsPass;
    ANormalsPass NormalsPassSkinned;
    AColorPass ColorPassSimple;
    AColorPass ColorPassSkinned;
    AShadowMapPass ShadowPass;
    AShadowMapPass ShadowPassSkinned;
};

struct AShadeModelHUD {
    AColorPassHUD ColorPassHUD;
};

}
