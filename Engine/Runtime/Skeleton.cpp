/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "Skeleton.h"
#include "IndexedMesh.h"

#include <Engine/Assets/Asset.h>
#include <Engine/Core/Platform/Logger.h>

HK_NAMESPACE_BEGIN

HK_CLASS_META(Skeleton)

Skeleton::Skeleton()
{
    m_BindposeBounds.Clear();
}

Skeleton::~Skeleton()
{
    Purge();
}

void Skeleton::Purge()
{
    m_Joints.Clear();
}

void Skeleton::Initialize(SkeletonJoint* joints, int jointsCount, BvAxisAlignedBox const& bindposeBounds)
{
    Purge();

    if (jointsCount < 0)
    {
        LOG("Skeleton::Initialize: joints count < 0\n");
        jointsCount = 0;
    }

    // Copy joints
    m_Joints.ResizeInvalidate(jointsCount);
    if (jointsCount > 0)
    {
        Platform::Memcpy(m_Joints.ToPtr(), joints, sizeof(*joints) * jointsCount);
    }

    m_BindposeBounds = bindposeBounds;
}

void Skeleton::LoadInternalResource(StringView path)
{
    Purge();

    if (!path.Icmp("/Default/Skeleton/Default"))
    {
        Initialize(nullptr, 0, BvAxisAlignedBox::Empty());
        return;
    }

    LOG("Unknown internal skeleton {}\n", path);

    LoadInternalResource("/Default/Skeleton/Default");
}

bool Skeleton::LoadResource(IBinaryStreamReadInterface& stream)
{
    uint32_t fileFormat = stream.ReadUInt32();

    if (fileFormat != ASSET_SKELETON)
    {
        LOG("Expected file format {}\n", ASSET_SKELETON);
        return false;
    }

    uint32_t fileVersion = stream.ReadUInt32();

    if (fileVersion != ASSET_VERSION_SKELETON)
    {
        LOG("Expected file version {}\n", ASSET_VERSION_SKELETON);
        return false;
    }

    Purge();

    String guid = stream.ReadString();

    stream.ReadArray(m_Joints);
    stream.ReadObject(m_BindposeBounds);

    return true;
}

int Skeleton::FindJoint(const char* name) const
{
    for (int j = 0; j < m_Joints.Size(); j++)
    {
        if (!Platform::Stricmp(m_Joints[j].Name, name))
        {
            return j;
        }
    }
    return -1;
}

HK_NAMESPACE_END
