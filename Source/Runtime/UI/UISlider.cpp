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

#include "UISlider.h"
#include "UIManager.h"
#include <Runtime/FrameLoop.h>
#include <Runtime/InputDefs.h>

UISlider::UISlider()
{
    Action               = A_NONE;
    DragCursor           = 0.0f;
    MinValue             = 0;
    MaxValue             = 1;
    Step                 = 0;
    Value                = 0;
    SliderWidth          = 12;
    bVerticalOrientation = false;
    BackgroundColor      = Color4(0.4f, 0.4f, 0.4f, 1.0f);
    SliderColor          = Color4::White();
    LineColor            = Color4::White();
}

UISlider::~UISlider()
{
}

UISlider& UISlider::SetValue(float _Value)
{
    float NewValue = Math::Clamp(Step > 0.0f ? Math::Snap(_Value, Step) : _Value, MinValue, MaxValue);

    if (Value != NewValue)
    {
        Value           = NewValue;

        E_OnUpdateValue.Dispatch(Value);
    }

    return *this;
}

UISlider& UISlider::SetMaxValue(float _MaxValue)
{
    MaxValue = _MaxValue;

    // Correct min value
    if (MinValue > MaxValue)
    {
        MinValue = MaxValue;
    }

    // Update value
    SetValue(Value);

    return *this;
}

UISlider& UISlider::SetMinValue(float _MinValue)
{
    MinValue = _MinValue;

    // Correct max value
    if (MaxValue < MinValue)
    {
        MaxValue = MinValue;
    }

    // Update value
    SetValue(Value);

    return *this;
}

UISlider& UISlider::SetStep(float _Step)
{
    Step = _Step;
    return *this;
}

UISlider& UISlider::SetSliderWidth(float _Width)
{
    SliderWidth = Math::Max(_Width, 1.0f);
    return *this;
}

UISlider& UISlider::SetVerticalOrientation(bool _VerticalOrientation)
{
    bVerticalOrientation = _VerticalOrientation;
    return *this;
}

UISlider& UISlider::SetBackgroundColor(Color4 const& _Color)
{
    BackgroundColor = _Color;
    return *this;
}

UISlider& UISlider::SetSliderColor(Color4 const& _Color)
{
    SliderColor = _Color;
    return *this;
}

UISlider& UISlider::SetLineColor(Color4 const& _Color)
{
    LineColor = _Color;
    return *this;
}

void UISlider::UpdateSliderGeometry()
{
    Float2 const& mins = Geometry.Mins;
    Float2 const& maxs = Geometry.Maxs;

    Platform::ZeroMem(&m_SliderGeometry, sizeof(m_SliderGeometry));

    if (bVerticalOrientation)
    {
        float AvailableWidth    = maxs.Y - mins.Y;
        float SliderActualWidth = Math::Min(AvailableWidth / 4.0f, SliderWidth);
        float SliderHalfWidth   = SliderActualWidth * 0.5f;

        m_SliderGeometry.BgMins = mins;
        m_SliderGeometry.BgMaxs = maxs;

        m_SliderGeometry.BgMins.Y += SliderHalfWidth;
        m_SliderGeometry.BgMaxs.Y -= SliderHalfWidth;

        float SliderBarSize = m_SliderGeometry.BgMaxs.Y - m_SliderGeometry.BgMins.Y;
        float SliderPos     = (Value - MinValue) / (MaxValue - MinValue) * SliderBarSize;

        m_SliderGeometry.SliderMins.X = m_SliderGeometry.BgMins.X;
        m_SliderGeometry.SliderMins.Y = m_SliderGeometry.BgMins.Y + SliderPos - SliderHalfWidth;

        m_SliderGeometry.SliderMaxs.X = m_SliderGeometry.BgMaxs.X;
        m_SliderGeometry.SliderMaxs.Y = m_SliderGeometry.SliderMins.Y + SliderActualWidth;
    }
    else
    {
        float AvailableWidth    = maxs.X - mins.X;
        float SliderActualWidth = Math::Min(AvailableWidth / 4.0f, SliderWidth);
        float SliderHalfWidth   = SliderActualWidth * 0.5f;

        m_SliderGeometry.BgMins = mins;
        m_SliderGeometry.BgMaxs = maxs;

        m_SliderGeometry.BgMins.X += SliderHalfWidth;
        m_SliderGeometry.BgMaxs.X -= SliderHalfWidth;

        float SliderBarSize = m_SliderGeometry.BgMaxs.X - m_SliderGeometry.BgMins.X;
        float SliderPos     = (Value - MinValue) / (MaxValue - MinValue) * SliderBarSize;

        m_SliderGeometry.SliderMins.X = m_SliderGeometry.BgMins.X + SliderPos - SliderHalfWidth;
        m_SliderGeometry.SliderMins.Y = m_SliderGeometry.BgMins.Y;

        m_SliderGeometry.SliderMaxs.X = m_SliderGeometry.SliderMins.X + SliderActualWidth;
        m_SliderGeometry.SliderMaxs.Y = m_SliderGeometry.BgMaxs.Y;
    }
}

UISliderGeometry const& UISlider::GetSliderGeometry() const
{
    return m_SliderGeometry;
}

void UISlider::MoveSlider(float Vec)
{
    UISliderGeometry const& geometry = m_SliderGeometry;

    float SliderBarSize = bVerticalOrientation ? geometry.BgMaxs.Y - geometry.BgMins.Y : geometry.BgMaxs.X - geometry.BgMins.X;

    SetValue(Vec * (MaxValue - MinValue) / SliderBarSize + MinValue);
}

HK_FORCEINLINE bool InRect(Float2 const& _Mins, Float2 const& _Maxs, Float2 const& _Position)
{
    return _Position.X >= _Mins.X && _Position.X < _Maxs.X && _Position.Y >= _Mins.Y && _Position.Y < _Maxs.Y;
}

void UISlider::OnMouseButtonEvent(SMouseButtonEvent const& event, double timeStamp)
{
    Action = A_NONE;

    if (event.Button != MOUSE_BUTTON_LEFT || event.Action != IA_PRESS)
    {
        return;
    }

    Float2 const& CursorPos = GUIManager->CursorPosition;

    UISliderGeometry const& geometry = m_SliderGeometry;

    if (InRect(geometry.SliderMins, geometry.SliderMaxs, CursorPos))
    {
        Action = A_MOVE;

        float SliderBarSize = bVerticalOrientation ? geometry.BgMaxs.Y - geometry.BgMins.Y : geometry.BgMaxs.X - geometry.BgMins.X;

        float Cursor = bVerticalOrientation ? CursorPos.Y : CursorPos.X;

        DragCursor = Cursor - ((Value - MinValue) / (MaxValue - MinValue) * SliderBarSize);
        return;
    }

    if (InRect(geometry.BgMins, geometry.BgMaxs, CursorPos))
    {

        float CursorLocalOffset = bVerticalOrientation ? CursorPos.Y - geometry.BgMins.Y : CursorPos.X - geometry.BgMins.X;

        MoveSlider(CursorLocalOffset);
        return;
    }
}

void UISlider::OnMouseMoveEvent(SMouseMoveEvent const& event, double timeStamp)
{
    if (Action == A_MOVE)
    {
        Float2 CursorPos = GUIManager->CursorPosition;

        MoveSlider((bVerticalOrientation ? CursorPos.Y : CursorPos.X) - DragCursor);
    }
}

void UISlider::Draw(ACanvas& cv)
{
    UpdateSliderGeometry();

    UISliderGeometry const& geometry = m_SliderGeometry;

    // Draw slider background
    if (geometry.BgMaxs.X > geometry.BgMins.X && geometry.BgMaxs.Y > geometry.BgMins.Y)
    {
        //cv.DrawRectFilled( geometry.BgMins, geometry.BgMaxs, Color4(0.4f,0.4f,0.4f) );

        Float2 h = bVerticalOrientation ? Float2((geometry.BgMaxs.X - geometry.BgMins.X) * 0.5f, 0.0f) : Float2(0.0f, (geometry.BgMaxs.Y - geometry.BgMins.Y) * 0.5f);

        cv.DrawLine(geometry.BgMins + h, geometry.BgMaxs - h, LineColor, 2.0f);
    }

    // Draw slider
    if (geometry.SliderMaxs.X > geometry.SliderMins.X && geometry.SliderMaxs.Y > geometry.SliderMins.Y)
    {
        cv.DrawRectFilled(geometry.SliderMins, geometry.SliderMaxs, SliderColor, RoundingDesc(4.0f));
    }
}
