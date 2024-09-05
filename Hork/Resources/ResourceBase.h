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

#include <Hork/Core/BaseTypes.h>

HK_NAMESPACE_BEGIN

enum RESOURCE_TYPE : uint8_t
{
    RESOURCE_UNDEFINED,
    RESOURCE_MESH,//ok
    RESOURCE_ANIMATION,//ok
    RESOURCE_NODE_MOTION,     // todo
    RESOURCE_TEXTURE,//ok
    RESOURCE_MATERIAL,//ok
    RESOURCE_COLLISION,// todo
    RESOURCE_SOUND,//ok
    RESOURCE_FONT,//ok
    RESOURCE_TERRAIN,// ok
    RESOURCE_VIRTUAL_TEXTURE,// todo


    // BAKE:
    //
    // Navigation Mesh
    // Lightmaps
    // Photometric profiles
    // Envmaps? - can be streamed lod by lod
    // Collision models
    // Areas and portals (spatial structure)

    RESOURCE_TYPE_MAX
};

class ResourceBase
{
public:
    virtual ~ResourceBase() = default;

    virtual void Upload() {}
};

HK_FORCEINLINE uint32_t MakeResourceMagic(uint8_t type, uint8_t version)
{
    return (uint32_t('H')) | (uint32_t('k') << 8) | (uint32_t(type) << 16) | (uint32_t(version) << 24);
}

HK_NAMESPACE_END
