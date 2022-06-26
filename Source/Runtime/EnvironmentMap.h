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

/**
An environment map in terms of the engine is both an irradiance map for image-based lighting and a prefiltered reflection map.
*/
class AEnvironmentMap : public AResource
{
    HK_CLASS(AEnvironmentMap, AResource)

public:
    AEnvironmentMap()
    {}

    void InitializeFromImages(TArray<AImage, 6> const& Faces);

    RenderCore::BindlessHandle GetIrradianceHandle() const { return IrradianceMapHandle; }
    RenderCore::BindlessHandle GetReflectionHandle() const { return ReflectionMapHandle; }

protected:
    /** Load resource from file */
    bool LoadResource(IBinaryStreamReadInterface& Stream) override;

    /** Create internal resource */
    void LoadInternalResource(AStringView _Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/EnvMaps/Default"; }

private:
    void Purge();
    void CreateTextures(int IrradianceMapWidth, int ReflectionMapWidth);
    void UpdateSamplers();

    TRef<RenderCore::ITexture> IrradianceMap;
    TRef<RenderCore::ITexture> ReflectionMap;

    RenderCore::BindlessHandle IrradianceMapHandle{};
    RenderCore::BindlessHandle ReflectionMapHandle{};
};