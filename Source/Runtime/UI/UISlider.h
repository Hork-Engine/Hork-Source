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

#include "UIWidget.h"

struct UISliderGeometry
{
    Float2 SliderMins;
    Float2 SliderMaxs;

    Float2 BgMins;
    Float2 BgMaxs;
};

class UISlider : public UIWidget
{
    UI_CLASS(UISlider, UIWidget)

public:
    TEvent<float> E_OnUpdateValue;

    template <typename T, typename... TArgs>
    UISlider& SetOnUpdateValue(T* _Object, void (T::*_Method)(TArgs...))
    {
        E_OnUpdateValue.Add(_Object, _Method);
        return *this;
    }

    UISlider& SetValue(float _Value);
    UISlider& SetMaxValue(float _MaxValue);
    UISlider& SetMinValue(float _MinValue);
    UISlider& SetStep(float _Step);
    UISlider& SetSliderWidth(float _Width);
    UISlider& SetVerticalOrientation(bool _VerticalOrientation);
    UISlider& SetBackgroundColor(Color4 const& _Color);
    UISlider& SetSliderColor(Color4 const& _Color);
    UISlider& SetLineColor(Color4 const& _Color);

    float GetValue() const { return Value; }
    float GetMinValue() const { return MinValue; }
    float GetMaxValue() const { return MaxValue; }

    UISlider();
    ~UISlider();

protected:
    // You can override OnDrawEvent and use GetSliderGeometry to
    // draw your own style slider.
    UISliderGeometry const& GetSliderGeometry() const;

    bool IsVertical() const { return bVerticalOrientation; }

    void OnMouseButtonEvent(struct SMouseButtonEvent const& event, double timeStamp) override;

    void OnMouseMoveEvent(struct SMouseMoveEvent const& event, double timeStamp) override;

    void Draw(ACanvas& cv) override;

private:
    void UpdateSliderGeometry();
    void MoveSlider(float _Vec);

    enum SCROLL_ACTION
    {
        A_NONE,
        A_DECREASE,
        A_INCREASE,
        A_MOVE
    };

    Color4 BackgroundColor;
    Color4 SliderColor;
    Color4 LineColor;

    int             Action;
    float           DragCursor;
    UISliderGeometry m_SliderGeometry;
    float           MinValue;
    float           MaxValue;
    float           Step;
    float           Value;
    float           SliderWidth;
    bool            bVerticalOrientation;
};
