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

#pragma once

#include "Resource.h"
#include <Renderer/RenderDefs.h>

HK_NAMESPACE_BEGIN

/**
An environment map in terms of the engine is both an irradiance map for image-based lighting and a prefiltered reflection map.
*/
class EnvironmentMap : public Resource
{
    HK_CLASS(EnvironmentMap, Resource)

public:
    EnvironmentMap()
    {}

    static EnvironmentMap* CreateFromImage(ImageStorage const& Image)
    {
        EnvironmentMap* envmap = NewObj<EnvironmentMap>();
        envmap->InitializeFromImage(Image);
        return envmap;
    }

    RenderCore::BindlessHandle GetIrradianceHandle() const { return m_IrradianceMapHandle; }
    RenderCore::BindlessHandle GetReflectionHandle() const { return m_ReflectionMapHandle; }

protected:
    void InitializeFromImage(ImageStorage const& Image);

    /** Load resource from file */
    bool LoadResource(IBinaryStreamReadInterface& Stream) override;

    /** Create internal resource */
    void LoadInternalResource(StringView Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/EnvMaps/Default"; }

private:
    void Purge();
    void CreateTextures(int IrradianceMapWidth, int ReflectionMapWidth);
    void UpdateSamplers();

    TRef<RenderCore::ITexture> m_IrradianceMap;
    TRef<RenderCore::ITexture> m_ReflectionMap;

    RenderCore::BindlessHandle m_IrradianceMapHandle{};
    RenderCore::BindlessHandle m_ReflectionMapHandle{};
};

HK_NAMESPACE_END
