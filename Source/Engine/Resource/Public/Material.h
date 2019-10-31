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

#include <Engine/Base/Public/BaseObject.h>
#include "Texture.h"

class ANGIE_API FMaterial : public FBaseObject, public IGPUResourceOwner {
    AN_CLASS( FMaterial, FBaseObject )

public:
    void Initialize( FMaterialBuildData const * _Data );

    /** Initialize internal resource */
    void InitializeInternalResource( const char * _InternalResourceName ) override;

    EMaterialType GetType() const { return Type; }

    FMaterialGPU * GetGPUResource() { return MaterialGPU; }

    int GetNumUniformVectors() const { return NumUniformVectors; }

protected:
    FMaterial();
    ~FMaterial();

    // IGPUResourceOwner interface
    void UploadResourceGPU( FResourceGPU * _Resource ) override {}

private:
    FMaterialGPU * MaterialGPU;
    EMaterialType Type;
    int NumUniformVectors;
};

class FMaterialInstance : public FBaseObject {
    AN_CLASS( FMaterialInstance, FBaseObject )

public:
    union
    {
        float Uniforms[16];
        Float4 UniformVectors[4];
    };

    /** Initialize internal resource */
    void InitializeInternalResource( const char * _InternalResourceName ) override;

    void SetMaterial( FMaterial * _Material );

    FMaterial * GetMaterial() const;

    void SetTexture( int _TextureSlot, FTexture * _Texture );

    FMaterialFrameData * RenderFrontend_Update( int _VisMarker );

protected:
    FMaterialInstance();
    ~FMaterialInstance() {}

private:
    TRef< FMaterial > Material;
    FMaterialFrameData * FrameData;
    TRef< FTexture > Textures[ MAX_MATERIAL_TEXTURES ];
    int VisMarker;
};
