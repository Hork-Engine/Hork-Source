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
    TRef<UIBrush> SliderBrush;
    Color4        LineColor = Color4::White();

    TEvent<float> E_OnUpdateValue;    

    UISlider() = default;
    ~UISlider() = default;

    UISlider& WithVerticalOrientation(bool verticalOrientation)
    {
        m_bVerticalOrientation = verticalOrientation;
        return *this;
    }

    UISlider& WithSliderBrush(UIBrush* sliderBrush)
    {
        SliderBrush = sliderBrush;
        return *this;
    }

    // TODO: Replace line color by brush
    UISlider& WithLineColor(Color4 const& lineColor)
    {
        LineColor = lineColor;
        return *this;
    }

    UISlider& WithSliderWidth(float width)
    {
        m_SliderWidth = Math::Max(m_SliderWidth, 1.0f);
        return *this;
    }

    template <typename T, typename... TArgs>
    UISlider& SetOnUpdateValue(T* _Object, void (T::*_Method)(TArgs...))
    {
        E_OnUpdateValue.Add(_Object, _Method);
        return *this;
    }

    UISlider& SetValue(float value);
    UISlider& SetMaxValue(float maxValue);
    UISlider& SetMinValue(float minValue);
    UISlider& SetStep(float step);

    float GetValue() const { return m_Value; }
    float GetMinValue() const { return m_MinValue; }
    float GetMaxValue() const { return m_MaxValue; }

protected:
    // You can override OnDrawEvent and use GetSliderGeometry to
    // draw your own style slider.
    UISliderGeometry const& GetSliderGeometry() const;

    bool IsVertical() const { return m_bVerticalOrientation; }

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

    int              m_Action{A_NONE};
    float            m_DragCursor{};
    UISliderGeometry m_SliderGeometry;
    float            m_MinValue{0};
    float            m_MaxValue{1};
    float            m_Step{};
    float            m_Value{};
    float            m_SliderWidth{12};
    bool             m_bVerticalOrientation{};
};
