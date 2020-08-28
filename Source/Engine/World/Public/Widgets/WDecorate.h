/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <World/Public/Base/BaseObject.h>
#include <World/Public/Canvas.h>

#include "WCommon.h"

class WWidget;

/*

WDecorate

Specifies the base class for non interactive widgets (decorates)

*/
class WDecorate : public ABaseObject {
    AN_CLASS( WDecorate, ABaseObject )

    friend class WWidget;

public:

    template< typename T /*= WDecorate*/ >
    static T & New() {
        WDecorate * d = CreateInstanceOf< T >(); // compile time check: T must be subclass of WDecorate
        return *static_cast< T * >( d );
    }

    WWidget * GetOwner() { return Owner; }
    WWidget const * GetOwner() const { return Owner; }

protected:
    WDecorate();
    ~WDecorate();

    virtual void OnDrawEvent( ACanvas & _Canvas );

private:
    WWidget * Owner = nullptr;
};

class WTextDecorate : public WDecorate {
    AN_CLASS( WTextDecorate, WDecorate )

public:
    WTextDecorate & SetText( const char * _Text );
    WTextDecorate & SetFont( AFont * _Font );
    WTextDecorate & SetColor( AColor4 const & _Color );
    WTextDecorate & SetHorizontalAlignment( EWidgetAlignment _Alignment );
    WTextDecorate & SetVerticalAlignment( EWidgetAlignment _Alignment );
    WTextDecorate & SetWordWrap( bool _WordWrap );
    WTextDecorate & SetOffset( Float2 const & _Offset );

protected:
    WTextDecorate();
    ~WTextDecorate();

    void OnDrawEvent( ACanvas & _Canvas ) override;

    AFont const * GetFont() const;

private:
    TRef< AFont > Font;
    AString Text;
    AColor4 Color;
    Float2 Offset;
    bool bWordWrap;
    EWidgetAlignment HorizontalAlignment;
    EWidgetAlignment VerticalAlignment;
};

class WBorderDecorate : public WDecorate {
    AN_CLASS( WBorderDecorate, WDecorate )

public:
    WBorderDecorate & SetColor( AColor4 const & _Color );
    WBorderDecorate & SetFillBackground( bool _FillBackgrond );
    WBorderDecorate & SetBackgroundColor( AColor4 const & _Color );
    WBorderDecorate & SetThickness( float _Thickness );
    WBorderDecorate & SetRounding( float _Rounding );
    WBorderDecorate & SetRoundingCorners( EDrawCornerFlags _RoundingCorners );

protected:
    WBorderDecorate();
    ~WBorderDecorate();

    void OnDrawEvent( ACanvas & _Canvas ) override;

private:
    AColor4 Color;
    AColor4 BgColor;
    EDrawCornerFlags RoundingCorners;
    float Rounding;
    float Thickness;
    bool bFillBackgrond;
};

class WImageDecorate : public WDecorate {
    AN_CLASS( WImageDecorate, WDecorate )

public:
    WImageDecorate & SetColor( AColor4 const & _Color );
    WImageDecorate & SetRounding( float _Rounding );
    WImageDecorate & SetRoundingCorners( EDrawCornerFlags _RoundingCorners );
    WImageDecorate & SetTexture( ATexture * _Texture );
    WImageDecorate & SetColorBlending( EColorBlending _Blending );
    WImageDecorate & SetSamplerType( EHUDSamplerType _SamplerType );
    WImageDecorate & SetOffset( Float2 const & _Offset );
    WImageDecorate & SetSize( Float2 const & _Size );
    WImageDecorate & SetHorizontalAlignment( EWidgetAlignment _Alignment );
    WImageDecorate & SetVerticalAlignment( EWidgetAlignment _Alignment );
    WImageDecorate & SetUseOriginalSize( bool _UseOriginalSize );
    WImageDecorate & SetUVs( Float2 const & _UV0, Float2 const & _UV1 );

protected:
    WImageDecorate();
    ~WImageDecorate();

    void OnDrawEvent( ACanvas & _Canvas ) override;

private:
    AColor4 Color;
    float Rounding;
    EDrawCornerFlags RoundingCorners;
    TRef< ATexture > Texture;
    EColorBlending ColorBlending;
    EHUDSamplerType SamplerType;
    Float2 Offset;
    Float2 Size;
    Float2 UV0;
    Float2 UV1;
    bool bUseOriginalSize;
    EWidgetAlignment HorizontalAlignment;
    EWidgetAlignment VerticalAlignment;
};
