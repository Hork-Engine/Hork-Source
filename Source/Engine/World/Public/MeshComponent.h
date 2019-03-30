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

#include "RenderableComponent.h"
#include <Engine/World/Public/Texture.h>
#include <Engine/World/Public/Material.h>

//enum EMeshTopology {
//    T_TRIANGLES
//};

class ANGIE_API FMeshComponent : public FDrawSurf {
    AN_COMPONENT( FMeshComponent, FDrawSurf )

public:
#if 0
    void SetMaterialInstance( const char * _ResourceName );
    void SetMaterialInstance( FMaterialInstance * _Instance );
    FMaterialInstance * GetMaterialInstance();
#endif

    void SetMaterialInstance( FMaterialInstance * _Instance ) { MaterialInstance = _Instance; }

    FMaterial * GetMaterial() const { return MaterialInstance ? MaterialInstance->Material.Object : nullptr; }
    FMaterialInstance * GetMaterialInstance() const { return MaterialInstance; }

    void EnableShadowCast( bool _ShadowCast );
    bool IsShadowCastEnabled() const;

    // Shadow receiving is managed by material
    //void SetShadowReceive( bool _ShadowReceive );
    //bool GetShadowReceive() const;

    // Render on main pass
    void EnableLightPass( bool _LightPass );
    bool IsLightPassEnabled() const;

    // Use specific shadow pass from material
    void EnableMaterialShadowPass( bool _MaterialShadowPass );
    bool IsMaterialShadowPassEnabled() const;

    // Render mesh to custom depth-stencil buffer
    // Render target must have custom depth-stencil buffer enabled
    void EnableCustomDepthStencilPass( bool _CustomDepthStencilPass );
    bool IsCustomDepthStencilPassEnabled() const;

    // Custom Depth-Stencil Value
    void SetCustomDepthStencilValue( byte _StencilValue );
    byte GetCustomDepthStencilValue() const;

protected:
    FMeshComponent();
#if 0
    void SetResource( FResource * _Resource ) override;
    FResource * GetResource( const char * _AttributeName ) override;
#endif
private:
    TRefHolder< FMaterialInstance > MaterialInstance;

    enum ERenderPassBits {
        LIGHT_PASS                  = AN_BIT(0),
        SHADOW_PASS                 = AN_BIT(1),
        MATERIAL_SHADOW_PASS        = AN_BIT(2),
        CUSTOM_DEPTHSTENCIL_PASS    = AN_BIT(3),
    };
    
    int RenderPassMask;
    byte StencilValue;
    //bool bShadowReceive;
};
