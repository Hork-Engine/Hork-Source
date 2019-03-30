#include <Engine/World/Public/MeshComponent.h>
//#include <Engine/Renderer/Public/MaterialResource.h>
//#include <Engine/Renderer/Public/TextureResource.h>
//#include <Engine/Resource/Public/ResourceManager.h>

AN_CLASS_META_NO_ATTRIBS( FMeshComponent )

//AN_SCENE_COMPONENT_BEGIN_DECL( FMeshComponent, CCF_HIDDEN_IN_EDITOR )
//AN_ATTRIBUTE_CONST( "Material Instance", FProperty( byte(0) ), "MaterialInstance\0\0Material instance resource", AF_RESOURCE )
//AN_ATTRIBUTE( "Shadow Cast", FProperty( true ), EnableShadowCast, IsShadowCastEnabled, "Shadow casting", AF_DEFAULT )
//AN_ATTRIBUTE( "Light Pass", FProperty( true ), EnableLightPass, IsLightPassEnabled, "Render on main pass", AF_DEFAULT )
//AN_ATTRIBUTE( "Material Shadow Pass", FProperty( false ), EnableMaterialShadowPass, IsMaterialShadowPassEnabled, "Use specific shadow pass from material", AF_DEFAULT )
//AN_ATTRIBUTE( "Custom Depth-Stencil Pass", FProperty( false ), EnableCustomDepthStencilPass, IsCustomDepthStencilPassEnabled, "Render mesh to custom depth-stencil buffer", AF_DEFAULT )
//AN_ATTRIBUTE( "Custom Depth-Stencil Value", FProperty( byte(0) ), SetCustomDepthStencilValue, GetCustomDepthStencilValue, "Render mesh to custom depth-stencil buffer", AF_DEFAULT )
////AN_ATTRIBUTE( "ShadowReceive", FProperty( true ), SetShadowReceive, GetShadowReceive, "Shadow receiving", AF_DEFAULT )
//AN_SCENE_COMPONENT_END_DECL

FMeshComponent::FMeshComponent() {
    RenderPassMask = LIGHT_PASS | SHADOW_PASS;
    StencilValue = 0;
    //bShadowReceive = true;
}

#if 0
void FMeshComponent::SetResource( FResource * _Resource ) {
    if ( !_Resource ) {
        return;
    }
    if ( _Resource->GetFinalClassId() == FMaterialInstance::GetClassId() ) {
        SetMaterialInstance( static_cast< FMaterialInstance * >( _Resource ) );
    } else {
        Super::SetResource( _Resource );
    }
}

FResource * FMeshComponent::GetResource( const char * _AttributeName ) {
    if ( !FString::CmpCase( _AttributeName, "Material Instance" ) ) {
        return MaterialInstance;
    }
    return Super::GetResource( _AttributeName );
}

void FMeshComponent::SetMaterialInstance( const char * _ResourceName ) {
    if ( _ResourceName && *_ResourceName ) {
        FMaterialInstance * ResourceMaterial = GResourceManager->GetResource< FMaterialInstance >( _ResourceName );
        SetMaterialInstance( ResourceMaterial );
    } else {
        SetMaterialInstance( ( FMaterialInstance *)0 );
    }
}

void FMeshComponent::SetMaterialInstance( FMaterialInstance * _Instance ) {
    MaterialInstance = _Instance;
}

FMaterialInstance * FMeshComponent::GetMaterialInstance() {
    return MaterialInstance;
}
#endif
void FMeshComponent::EnableShadowCast( bool _ShadowCast ) {
    if ( _ShadowCast ) {
        RenderPassMask |= SHADOW_PASS;
    } else {
        RenderPassMask &= ~SHADOW_PASS;
    }
}

bool FMeshComponent::IsShadowCastEnabled() const {
    return !!(RenderPassMask & SHADOW_PASS);
}

void FMeshComponent::EnableLightPass( bool _LightPass ) {
    if ( _LightPass ) {
        RenderPassMask |= LIGHT_PASS;
    } else {
        RenderPassMask &= ~LIGHT_PASS;
    }
}

bool FMeshComponent::IsLightPassEnabled() const {
    return !!(RenderPassMask & LIGHT_PASS);
}

void FMeshComponent::EnableMaterialShadowPass( bool _MaterialShadowPass ) {
    if ( _MaterialShadowPass ) {
        RenderPassMask |= MATERIAL_SHADOW_PASS;
    } else {
        RenderPassMask &= ~MATERIAL_SHADOW_PASS;
    }
}

bool FMeshComponent::IsMaterialShadowPassEnabled() const {
    return !!(RenderPassMask & MATERIAL_SHADOW_PASS);
}

void FMeshComponent::EnableCustomDepthStencilPass( bool _CustomDepthStencilPass ) {
    if ( _CustomDepthStencilPass ) {
        RenderPassMask |= CUSTOM_DEPTHSTENCIL_PASS;
    } else {
        RenderPassMask &= ~CUSTOM_DEPTHSTENCIL_PASS;
    }
}

bool FMeshComponent::IsCustomDepthStencilPassEnabled() const {
    return !!(RenderPassMask & CUSTOM_DEPTHSTENCIL_PASS);
}

void FMeshComponent::SetCustomDepthStencilValue( byte _StencilValue ) {
    StencilValue = _StencilValue;
}

byte FMeshComponent::GetCustomDepthStencilValue() const {
    return StencilValue;
}

/*



// ѕересечение луча и меша
// ≈сли есть пересечение, то возвращает длину луча до ближайшей точки пересечени€.
// ≈сли пересечени€ нет, то возвращает значение < 0.
float Intersect( const Float3 & _RayStart,
                               const Float3 & _RayDir,
                               const Float3 & _ObjectOrigin,
                               const Float3x3 & _ObjectOrient ) {
    const float Epsilon = 0.000001f;
    bool intersected = false;
    float dist = 999999999.0f;

    Float3 polygonVerts[3];
    PlaneF polygonPlane;
    Float3 a,b,v;

    for ( int index = 0 ; index < m_NumIndices ; index += 3 ) {
        polygonVerts[0] = _ObjectOrient*m_Verts[ m_Indices[ index + 0 ] ].position + _ObjectOrigin;
        polygonVerts[1] = _ObjectOrient*m_Verts[ m_Indices[ index + 1 ] ].position + _ObjectOrigin;
        polygonVerts[2] = _ObjectOrient*m_Verts[ m_Indices[ index + 2 ] ].position + _ObjectOrigin;

        polygonPlane.FromPoints( polygonVerts );

        if ( FMath::Dot( _RayDir, polygonPlane.normal ) < 0 ) {
            continue;
        }

        a = polygonVerts[0] - _RayStart;
        b = polygonVerts[1] - _RayStart;
        v = FMath::Cross( a, b );
        float ip1 = FMath::Dot( _RayDir, v );

        a = polygonVerts[1] - _RayStart;
        b = polygonVerts[2] - _RayStart;
        v = FMath::Cross( a, b );
        float ip2 = FMath::Dot( _RayDir, v );

        a = polygonVerts[2] - _RayStart;
        b = polygonVerts[0] - _RayStart;
        v = FMath::Cross( a, b );
        float ip3 = FMath::Dot( _RayDir, v );

        if ( ( ip1 < 0 && ip2 < 0 && ip3 < 0 ) || ( ip1 > 0 && ip2 > 0 && ip3 > 0 ) ) {
            float d = FMath::Dot( polygonPlane.normal, _RayDir );
            if ( fabs( d ) < Epsilon ) {
                continue;
            }

            d = -(FMath::Dot( polygonPlane.normal, _RayStart ) - polygonPlane.Dist() ) / d;
            if ( d >= 0 ) {
                dist = FMath::Min( dist, d );
                intersected = true;
            }
        }
    }

    return intersected ? dist : -1.0f;
}

// ѕересечение луча и меша
// ¬озвращает есть пересечение или нет, работает быстрее, чем Intersect
bool HasIntersection( const Float3 & _RayStart,
                                    const Float3 & _RayDir,
                                    const Float3 & _ObjectOrigin,
                                    const Float3x3 & _ObjectOrient ) {
    const float Epsilon = 0.000001f;
    Float3 polygonVerts[3];
    PlaneF polygonPlane;
    Float3 a,b,v;

    for ( int index = 0 ; index < m_NumIndices ; index += 3 ) {
        polygonVerts[0] = _ObjectOrient*m_Verts[ m_Indices[ index + 0 ] ].position + _ObjectOrigin;
        polygonVerts[1] = _ObjectOrient*m_Verts[ m_Indices[ index + 1 ] ].position + _ObjectOrigin;
        polygonVerts[2] = _ObjectOrient*m_Verts[ m_Indices[ index + 2 ] ].position + _ObjectOrigin;

        polygonPlane.FromPoints( polygonVerts );

        if ( FMath::Dot( _RayDir, polygonPlane.normal ) < 0 ) {
            continue;
        }

        a = polygonVerts[0] - _RayStart;
        b = polygonVerts[1] - _RayStart;
        v = FMath::Cross( a, b );
        float ip1 = FMath::Dot( _RayDir, v );

        a = polygonVerts[1] - _RayStart;
        b = polygonVerts[2] - _RayStart;
        v = FMath::Cross( a, b );
        float ip2 = FMath::Dot( _RayDir, v );

        a = polygonVerts[2] - _RayStart;
        b = polygonVerts[0] - _RayStart;
        v = FMath::Cross( a, b );
        float ip3 = FMath::Dot( _RayDir, v );

        if ( ( ip1 < 0 && ip2 < 0 && ip3 < 0 ) || ( ip1 > 0 && ip2 > 0 && ip3 > 0 ) ) {
            float d = FMath::Dot( polygonPlane.normal, _RayDir );
            if ( fabs( d ) < Epsilon ) {
                continue;
            }

            d = -(FMath::Dot( polygonPlane.normal, _RayStart ) - polygonPlane.Dist() ) / d;
            if ( d >= 0 ) {
                return true;
            }
        }
    }

    return false;
}

*/
