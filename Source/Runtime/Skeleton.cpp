/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include <Assets/Asset.h>
#include <Platform/Logger.h>

HK_CLASS_META(ASkeleton)

///////////////////////////////////////////////////////////////////////////////////////////////////////

ASkeleton::ASkeleton()
{
    BindposeBounds.Clear();
}

ASkeleton::~ASkeleton()
{
    Purge();
}

void ASkeleton::Purge()
{
    Joints.Clear();
}

void ASkeleton::Initialize(SJoint* _Joints, int _JointsCount, BvAxisAlignedBox const& _BindposeBounds)
{
    Purge();

    if (_JointsCount < 0)
    {
        LOG("ASkeleton::Initialize: joints count < 0\n");
        _JointsCount = 0;
    }

    // Copy joints
    Joints.ResizeInvalidate(_JointsCount);
    if (_JointsCount > 0)
    {
        Platform::Memcpy(Joints.ToPtr(), _Joints, sizeof(*_Joints) * _JointsCount);
    }

    BindposeBounds = _BindposeBounds;
}

void ASkeleton::LoadInternalResource(AStringView _Path)
{
    Purge();

    if (!_Path.Icmp("/Default/Skeleton/Default"))
    {
        Initialize(nullptr, 0, BvAxisAlignedBox::Empty());
        return;
    }

    LOG("Unknown internal skeleton {}\n", _Path);

    LoadInternalResource("/Default/Skeleton/Default");
}

bool ASkeleton::LoadResource(IBinaryStreamReadInterface& Stream)
{
    uint32_t fileFormat = Stream.ReadUInt32();

    if (fileFormat != ASSET_SKELETON)
    {
        LOG("Expected file format {}\n", ASSET_SKELETON);
        return false;
    }

    uint32_t fileVersion = Stream.ReadUInt32();

    if (fileVersion != ASSET_VERSION_SKELETON)
    {
        LOG("Expected file version {}\n", ASSET_VERSION_SKELETON);
        return false;
    }

    Purge();

    AString guid = Stream.ReadString();

    Stream.ReadArray(Joints);
    Stream.ReadObject(BindposeBounds);

    return true;
}

int ASkeleton::FindJoint(const char* _Name) const
{
    for (int j = 0; j < Joints.Size(); j++)
    {
        if (!Platform::Stricmp(Joints[j].Name, _Name))
        {
            return j;
        }
    }
    return -1;
}
