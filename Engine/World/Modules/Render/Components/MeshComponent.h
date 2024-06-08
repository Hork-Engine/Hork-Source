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

#include <Engine/World/Component.h>
#include <Engine/World/Modules/Render/ProceduralMesh.h>
#include <Engine/World/Resources/Resource_Mesh.h>
#include <Engine/World/Resources/Resource_MaterialInstance.h>

HK_NAMESPACE_BEGIN

class MeshComponent : public Component
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    struct Surface
    {
        Vector<MaterialInstance*>  Materials;
        //BvAxisAlignedBox            BoundingBox;
        //BvAxisAlignedBox            WorldBoundingBox;
    };

    MeshHandle              m_Resource;
    Vector<Surface>        m_Surfaces;
    Ref<SkeletonPose>      m_Pose;
    bool                    m_Outline = false;
    bool                    m_CastShadow = true;
    uint32_t                m_CascadeMask = 0;

    // Transform from current and previous fixed state
    Float3                  m_Position[2];
    Quat                    m_Rotation[2];
    Float3                  m_Scale[2];
    // Interpolated transform
    Float3                  m_LerpPosition;
    Quat                    m_LerpRotation;
    Float3                  m_LerpScale;
    // Transform from previous frame
    Float3                  m_PrevPosition;
    Quat                    m_PrevRotation;
    Float3                  m_PrevScale;

    void                    BeginPlay();
    void                    EndPlay();

    void                    UpdateBoundingBox();

    void                    DrawDebug(DebugRenderer& renderer);
};

class ProceduralMeshComponent : public Component
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    struct Surface
    {
        Vector<MaterialInstance*>  Materials;
        //BvAxisAlignedBox            BoundingBox;
        //BvAxisAlignedBox            WorldBoundingBox;
    };

    Ref<ProceduralMesh_ECS> m_Mesh;
    Surface                 m_Surface;
    bool                    m_Outline = false;
    bool                    m_CastShadow = true;

    // Transform from current and previous fixed state
    Float3                  m_Position[2];
    Quat                    m_Rotation[2];
    Float3                  m_Scale[2];
    // Interpolated transform
    Float3                  m_LerpPosition;
    Quat                    m_LerpRotation;
    Float3                  m_LerpScale;
    // Transform from previous frame
    Float3                  m_PrevPosition;
    Quat                    m_PrevRotation;
    Float3                  m_PrevScale;

    void                    UpdateBoundingBox();

    void                    DrawDebug(DebugRenderer& renderer);
};

HK_NAMESPACE_END
