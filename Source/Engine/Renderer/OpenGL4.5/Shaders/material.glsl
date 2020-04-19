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

//
// Material shader
//


// Built-in predefines
#include "$PREDEFINES$"

#include "base/core.glsl"
#include "base/texture.glsl"
#include "base/viewuniforms.glsl"

#if defined MATERIAL_PASS_SHADOWMAP
#   include "shadowcastInstance.glsl"
#else
#   include "instanceUniforms.glsl"
#endif

#if defined MATERIAL_PASS_DEPTH
#   ifdef VERTEX_SHADER
#       include "$DEPTH_PASS_SAMPLERS$"
#       include "instance.vert"
#   endif
#endif // MATERIAL_PASS_DEPTH

#if defined MATERIAL_PASS_WIREFRAME
#   ifdef VERTEX_SHADER
#       include "$WIREFRAME_PASS_SAMPLERS$"
#       include "instance.vert"
#   endif
#   ifdef GEOMETRY_SHADER
#       include "instance_wireframe.geom"
#   endif
#   ifdef FRAGMENT_SHADER
#       include "instance_wireframe.frag"
#   endif
#endif // MATERIAL_PASS_WIREFRAME

#if defined MATERIAL_PASS_SHADOWMAP
#   ifdef VERTEX_SHADER
#       include "$SHADOWMAP_PASS_SAMPLERS$"
#       include "instance.vert"
#   endif
#   ifdef GEOMETRY_SHADER
#       include "instance_shadowmap.geom"
#   endif
#   ifdef FRAGMENT_SHADER
#       include "instance_shadowmap.frag"
#   endif
#endif // MATERIAL_PASS_SHADOWMAP

#if defined MATERIAL_PASS_COLOR
#   if defined MATERIAL_TYPE_PBR || defined MATERIAL_TYPE_BASELIGHT
#       define COMPUTE_TBN
#   endif
#   ifdef VERTEX_SHADER
#       include "$DEPTH_PASS_SAMPLERS$"
#       include "instance_color.vert"
#   endif
#   ifdef FRAGMENT_SHADER
#       include "instance_color.frag"
#   endif
#endif // MATERIAL_PASS_DEPTH
