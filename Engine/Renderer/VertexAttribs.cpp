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

#include "VertexAttribs.h"
#include "RenderDefs.h"

HK_NAMESPACE_BEGIN

using namespace RenderCore;

static const VertexAttribInfo VertexAttribsSkinned[] = {
    {"InPosition",
     0, // location
     0, // buffer input slot
     VAT_FLOAT3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Position)},
    {"InTexCoord",
     1, // location
     0, // buffer input slot
     VAT_HALF2,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, TexCoord)},
    {"InNormal",
     2, // location
     0, // buffer input slot
     VAT_HALF3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Normal)},
    {"InTangent",
     3, // location
     0, // buffer input slot
     VAT_HALF3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Tangent)},
    {"InHandedness",
     4, // location
     0, // buffer input slot
     VAT_BYTE1,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Handedness)},
    {"InJointIndices",
     5, // location
     1, // buffer input slot
     VAT_USHORT4,
     VAM_INTEGER,
     0, // InstanceDataStepRate
     HK_OFS(SkinVertex, JointIndices)},
    {"InJointWeights",
     6, // location
     1, // buffer input slot
     VAT_UBYTE4N,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(SkinVertex, JointWeights)}};

ArrayView<VertexAttribInfo> g_VertexAttribsSkinned(VertexAttribsSkinned);

static const VertexAttribInfo VertexAttribsStatic[] = {
    {"InPosition",
     0, // location
     0, // buffer input slot
     VAT_FLOAT3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Position)},
    {"InTexCoord",
     1, // location
     0, // buffer input slot
     VAT_HALF2,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, TexCoord)},
    {"InNormal",
     2, // location
     0, // buffer input slot
     VAT_HALF3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Normal)},
    {"InTangent",
     3, // location
     0, // buffer input slot
     VAT_HALF3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Tangent)},
    {"InHandedness",
     4, // location
     0, // buffer input slot
     VAT_BYTE1,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Handedness)}};

ArrayView<VertexAttribInfo> g_VertexAttribsStatic(VertexAttribsStatic);

static const VertexAttribInfo VertexAttribsStaticLightmap[] = {
    {"InPosition",
     0, // location
     0, // buffer input slot
     VAT_FLOAT3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Position)},
    {"InTexCoord",
     1, // location
     0, // buffer input slot
     VAT_HALF2,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, TexCoord)},
    {"InNormal",
     2, // location
     0, // buffer input slot
     VAT_HALF3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Normal)},
    {"InTangent",
     3, // location
     0, // buffer input slot
     VAT_HALF3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Tangent)},
    {"InHandedness",
     4, // location
     0, // buffer input slot
     VAT_BYTE1,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Handedness)},
    {"InLightmapTexCoord",
     5, // location
     1, // buffer input slot
     VAT_FLOAT2,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertexUV, TexCoord)}};

ArrayView<VertexAttribInfo> g_VertexAttribsStaticLightmap(VertexAttribsStaticLightmap);

static const VertexAttribInfo VertexAttribsStaticVertexLight[] = {
    {"InPosition",
     0, // location
     0, // buffer input slot
     VAT_FLOAT3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Position)},
    {"InTexCoord",
     1, // location
     0, // buffer input slot
     VAT_HALF2,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, TexCoord)},
    {"InNormal",
     2, // location
     0, // buffer input slot
     VAT_HALF3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Normal)},
    {"InTangent",
     3, // location
     0, // buffer input slot
     VAT_HALF3,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Tangent)},
    {"InHandedness",
     4, // location
     0, // buffer input slot
     VAT_BYTE1,
     VAM_FLOAT,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertex, Handedness)},
    {"InVertexLight",
     5,          // location
     1,          // buffer input slot
     VAT_UBYTE4, //VAT_UBYTE4N,
     VAM_INTEGER,
     0, // InstanceDataStepRate
     HK_OFS(MeshVertexLight, VertexLight)}};

ArrayView<VertexAttribInfo> g_VertexAttribsStaticVertexLight(VertexAttribsStaticVertexLight);

static const VertexAttribInfo VertexAttribsTerrain[] = {
    {"InPosition",
     0, // location
     0, // buffer input slot
     VAT_SHORT2,
     VAM_INTEGER,
     0, // InstanceDataStepRate
     HK_OFS(TerrainVertex, X)}};

ArrayView<VertexAttribInfo> g_VertexAttribsTerrain(VertexAttribsTerrain);

static const VertexAttribInfo VertexAttribsTerrainInstanced[] = {
    {"InPosition",
     0, // location
     0, // buffer input slot
     VAT_SHORT2,
     VAM_INTEGER,
     0, // InstanceDataStepRate
     HK_OFS(TerrainVertex, X)},
    {"VertexScaleAndTranslate",
     1, // location
     1, // buffer input slot
     VAT_INT4,
     VAM_INTEGER,
     1, // InstanceDataStepRate
     HK_OFS(TerrainPatchInstance, VertexScale)},
    {"TexcoordOffset",
     2, // location
     1, // buffer input slot
     VAT_INT2,
     VAM_INTEGER,
     1, // InstanceDataStepRate
     HK_OFS(TerrainPatchInstance, TexcoordOffset)},
    {"QuadColor",
     3, // location
     1, // buffer input slot
     VAT_FLOAT4,
     VAM_FLOAT,
     1, // InstanceDataStepRate
     HK_OFS(TerrainPatchInstance, QuadColor)}};

ArrayView<VertexAttribInfo> g_VertexAttribsTerrainInstanced(VertexAttribsTerrainInstanced);

HK_NAMESPACE_END
