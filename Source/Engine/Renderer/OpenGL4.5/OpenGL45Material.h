/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

class FDepthPass : public GHI::Pipeline {
public:
    void Create( const char * _VertexSourceCode, GHI::POLYGON_CULL _CullMode, bool _Skinned );
};

class FWireframePass : public GHI::Pipeline {
public:
    void Create( const char * _VertexSourceCode, const char * _GeometrySourceCode, const char * _FragmentSourceCode, GHI::POLYGON_CULL _CullMode, bool _Skinned );
};

class FColorPassHUD : public GHI::Pipeline {
public:
    void Create( const char * _VertexSourceCode, const char * _FragmentSourceCode );
};

class FColorPass : public GHI::Pipeline {
public:
    void Create( const char * _VertexSourceCode, const char * _FragmentSourceCode, GHI::POLYGON_CULL _CullMode, bool _Unlit, bool _Skinned, bool _DepthTest );
};

class FColorPassLightmap : public GHI::Pipeline {
public:
    void Create( const char * _VertexSourceCode, const char * _FragmentSourceCode, GHI::POLYGON_CULL _CullMode, bool _DepthTest );
};

class FColorPassVertexLight : public GHI::Pipeline {
public:
    void Create( const char * _VertexSourceCode, const char * _FragmentSourceCode, GHI::POLYGON_CULL _CullMode, bool _DepthTest );
};

struct FShadeModelPBR {
    FDepthPass DepthPass;
    FDepthPass DepthPassSkinned;
    FWireframePass WireframePass;
    FWireframePass WireframePassSkinned;
    FColorPass ColorPassSimple;
    FColorPass ColorPassSkinned;
    FColorPassLightmap LightmapPass;
    FColorPassVertexLight VertexLightPass;
};

struct FShadeModelUnlit {
    FDepthPass DepthPass;
    FDepthPass DepthPassSkinned;
    FWireframePass WireframePass;
    FWireframePass WireframePassSkinned;
    FColorPass ColorPassSimple;
    FColorPass ColorPassSkinned;
};

struct FShadeModelHUD {
    FColorPassHUD ColorPassHUD;
};

}
