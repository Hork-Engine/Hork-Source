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

#include <Engine/World/Modules/Skeleton/SkeletalAnimation.h>
#include <Engine/World/Resources/Resource_Mesh.h>
#include <Engine/World/Component.h>
#include <Engine/World/TickFunction.h>

HK_NAMESPACE_BEGIN

class SkinnedMeshComponent final : public Component
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    Ref<AnimationInstance> AnimInstance;
    Ref<SkeletonPose> Pose;
    MeshHandle Mesh;

    void FixedUpdate()
    {
        UpdatePoses();
    }

    void LateUpdate()
    {
        UpdateSkins();
    }
    
    void DrawDebug(DebugRenderer& renderer);

private:
    void UpdatePoses();
    void UpdateSkins();
};

namespace TickGroup_FixedUpdate
{
    template <>
    HK_INLINE void InitializeTickFunction<SkinnedMeshComponent>(TickFunctionDesc& desc)
    {
        desc.Name.FromString("Update Poses");
    }
}

namespace TickGroup_LateUpdate
{
    template <>
    HK_INLINE void InitializeTickFunction<SkinnedMeshComponent>(TickFunctionDesc& desc)
    {
        desc.Name.FromString("Update Skins");
    }
}

HK_NAMESPACE_END