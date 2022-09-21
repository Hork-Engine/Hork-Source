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

#include "BaseObject.h"
#include "Canvas.h"

#include "WCommon.h"

class WWidget;

/*

WDecorate

Specifies the base class for non interactive widgets (decorates)

*/
class WDecorate : public ABaseObject
{
    HK_CLASS(WDecorate, ABaseObject)

    friend class WWidget;

public:
    WWidget*       GetOwner() { return Owner; }
    WWidget const* GetOwner() const { return Owner; }

    WDecorate();
    ~WDecorate();

protected:
    virtual void OnDrawEvent(ACanvas& _Canvas);

private:
    WWidget* Owner = nullptr;
};

class WTextDecorate : public WDecorate
{
    HK_CLASS(WTextDecorate, WDecorate)

public:
    WTextDecorate& SetText(AStringView _Text);
    WTextDecorate& SetFont(AFont* _Font);
    WTextDecorate& SetFontSize(float _FontSize);
    WTextDecorate& SetColor(Color4 const& _Color);
    WTextDecorate& SetHorizontalAlignment(EWidgetAlignment _Alignment);
    WTextDecorate& SetVerticalAlignment(EWidgetAlignment _Alignment);
    WTextDecorate& SetWordWrap(bool _WordWrap);
    WTextDecorate& SetOffset(Float2 const& _Offset);
    WTextDecorate& SetShadow(bool bShadow);

    WTextDecorate();
    ~WTextDecorate();

protected:
    void OnDrawEvent(ACanvas& _Canvas) override;

    AFont* GetFont() const;

private:
    TRef<AFont>      Font;
    float            FontSize{20};
    AString          Text;
    Color4           Color;
    Float2           Offset;
    bool             bWordWrap;
    EWidgetAlignment HorizontalAlignment;
    EWidgetAlignment VerticalAlignment;
    bool             bShadow = false;
};

class WBorderDecorate : public WDecorate
{
    HK_CLASS(WBorderDecorate, WDecorate)

public:
    WBorderDecorate& SetColor(Color4 const& _Color);
    WBorderDecorate& SetFillBackground(bool _FillBackgrond);
    WBorderDecorate& SetBackgroundColor(Color4 const& _Color);
    WBorderDecorate& SetThickness(float _Thickness);
    WBorderDecorate& SetRoundingTL(float _Rounding);
    WBorderDecorate& SetRoundingTR(float _Rounding);
    WBorderDecorate& SetRoundingBL(float _Rounding);
    WBorderDecorate& SetRoundingBR(float _Rounding);
    WBorderDecorate& SetRounding(float roundingTL, float roundingTR, float roundingBL, float roundingBR);
    WBorderDecorate& SetRounding(float rounding);

    WBorderDecorate();
    ~WBorderDecorate();

protected:
    void OnDrawEvent(ACanvas& _Canvas) override;

private:
    Color4             Color;
    Color4             BgColor;
    RoundingDesc       Rounding;
    float              Thickness;
    bool               bFillBackgrond;
};

class WImageDecorate : public WDecorate
{
    HK_CLASS(WImageDecorate, WDecorate)

public:
    WImageDecorate& SetTint(Color4 const& tintColor);
    WImageDecorate& SetRoundingTL(float _Rounding);
    WImageDecorate& SetRoundingTR(float _Rounding);
    WImageDecorate& SetRoundingBL(float _Rounding);
    WImageDecorate& SetRoundingBR(float _Rounding);
    WImageDecorate& SetRounding(float roundingTL, float roundingTR, float roundingBL, float roundingBR);
    WImageDecorate& SetRounding(float rounding);
    WImageDecorate& SetAngle(float angle);
    WImageDecorate& SetTexture(ATexture* _Texture);
    WImageDecorate& SetComposite(CANVAS_COMPOSITE composite);
    WImageDecorate& SetOffset(Float2 const& _Offset);
    WImageDecorate& SetSize(Float2 const& _Size);
    WImageDecorate& SetHorizontalAlignment(EWidgetAlignment _Alignment);
    WImageDecorate& SetVerticalAlignment(EWidgetAlignment _Alignment);
    WImageDecorate& SetUseOriginalSize(bool _UseOriginalSize);
    WImageDecorate& SetUVOffset(Float2 const& UVOffset);
    WImageDecorate& SetUVScale(Float2 const& UVScale);
    WImageDecorate& SetTiledX(bool bTiledX);
    WImageDecorate& SetTiledY(bool bTiledY);
    WImageDecorate& SetFlipY(bool bFlipY);
    WImageDecorate& SetAlphaPremultiplied(bool bAlphaPremultiplied);
    WImageDecorate& SetNearestFilter(bool bNearestFilter);

    WImageDecorate();
    ~WImageDecorate();

protected:
    void OnDrawEvent(ACanvas& _Canvas) override;

private:
    Color4             TintColor;
    RoundingDesc       Rounding;
    float              Angle{};
    TRef<ATexture>     Texture;
    CANVAS_COMPOSITE   Composite = CANVAS_COMPOSITE_SOURCE_OVER;
    Float2             Offset;
    Float2             Size;
    Float2             m_UVOffset{0, 0};
    Float2             m_UVScale{1, 1};
    EWidgetAlignment   HorizontalAlignment;
    EWidgetAlignment   VerticalAlignment;
    bool               bTiledX : 1;
    bool               bTiledY : 1;
    bool               bFlipY : 1;
    bool               bAlphaPremultiplied : 1;
    bool               bNearestFilter : 1;
    bool               bUseOriginalSize : 1;
};
