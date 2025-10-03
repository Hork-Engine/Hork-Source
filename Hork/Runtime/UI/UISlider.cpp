/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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
#include <Hork/Runtime/GameApplication/FrameLoop.h>
#include <Hork/Geometry/BV/BvIntersect.h>

HK_NAMESPACE_BEGIN

UISlider& UISlider::SetValue(float value)
{
    float newValue = Math::Clamp(m_Step > 0.0f ? Math::Snap(value, m_Step) : value, m_MinValue, m_MaxValue);

    if (m_Value != newValue)
    {
        m_Value = newValue;

        E_OnUpdateValue.Invoke(m_Value);
    }

    return *this;
}

UISlider& UISlider::SetMaxValue(float maxValue)
{
    m_MaxValue = maxValue;

    // Correct min value
    if (m_MinValue > m_MaxValue)
    {
        m_MinValue = m_MaxValue;
    }

    // Update value
    SetValue(m_Value);

    return *this;
}

UISlider& UISlider::SetMinValue(float minValue)
{
    m_MinValue = minValue;

    // Correct max value
    if (m_MaxValue < m_MinValue)
    {
        m_MaxValue = m_MinValue;
    }

    // Update value
    SetValue(m_Value);

    return *this;
}

UISlider& UISlider::SetStep(float step)
{
    m_Step = step;
    return *this;
}

void UISlider::UpdateSliderGeometry()
{
    Float2 const& mins = m_Geometry.Mins;
    Float2 const& maxs = m_Geometry.Maxs;

    Core::ZeroMem(&m_SliderGeometry, sizeof(m_SliderGeometry));

    if (m_bVerticalOrientation)
    {
        float availableWidth    = maxs.Y - mins.Y;
        float sliderActualWidth = Math::Min(availableWidth / 4.0f, m_SliderWidth);
        float sliderHalfWidth   = sliderActualWidth * 0.5f;

        m_SliderGeometry.BgMins = mins;
        m_SliderGeometry.BgMaxs = maxs;

        m_SliderGeometry.BgMins.Y += sliderHalfWidth;
        m_SliderGeometry.BgMaxs.Y -= sliderHalfWidth;

        float sliderBarSize = m_SliderGeometry.BgMaxs.Y - m_SliderGeometry.BgMins.Y;
        float sliderPos     = (m_Value - m_MinValue) / (m_MaxValue - m_MinValue) * sliderBarSize;

        m_SliderGeometry.SliderMins.X = m_SliderGeometry.BgMins.X;
        m_SliderGeometry.SliderMins.Y = m_SliderGeometry.BgMins.Y + sliderPos - sliderHalfWidth;

        m_SliderGeometry.SliderMaxs.X = m_SliderGeometry.BgMaxs.X;
        m_SliderGeometry.SliderMaxs.Y = m_SliderGeometry.SliderMins.Y + sliderActualWidth;
    }
    else
    {
        float availableWidth    = maxs.X - mins.X;
        float sliderActualWidth = Math::Min(availableWidth / 4.0f, m_SliderWidth);
        float sliderHalfWidth   = sliderActualWidth * 0.5f;

        m_SliderGeometry.BgMins = mins;
        m_SliderGeometry.BgMaxs = maxs;

        m_SliderGeometry.BgMins.X += sliderHalfWidth;
        m_SliderGeometry.BgMaxs.X -= sliderHalfWidth;

        float sliderBarSize = m_SliderGeometry.BgMaxs.X - m_SliderGeometry.BgMins.X;
        float sliderPos     = (m_Value - m_MinValue) / (m_MaxValue - m_MinValue) * sliderBarSize;

        m_SliderGeometry.SliderMins.X = m_SliderGeometry.BgMins.X + sliderPos - sliderHalfWidth;
        m_SliderGeometry.SliderMins.Y = m_SliderGeometry.BgMins.Y;

        m_SliderGeometry.SliderMaxs.X = m_SliderGeometry.SliderMins.X + sliderActualWidth;
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

    float sliderBarSize = m_bVerticalOrientation ? geometry.BgMaxs.Y - geometry.BgMins.Y : geometry.BgMaxs.X - geometry.BgMins.X;

    SetValue(Vec * (m_MaxValue - m_MinValue) / sliderBarSize + m_MinValue);
}

void UISlider::OnMouseButtonEvent(MouseButtonEvent const& event)
{
    m_Action = A_NONE;

    if (event.Button != VirtualKey::MouseLeftBtn || event.Action != InputAction::Pressed)
    {
        return;
    }

    Float2 cursorPos = UIManager::sInstance().CursorPosition;

    UISliderGeometry const& geometry = m_SliderGeometry;

    if (BvPointInRect(geometry.SliderMins, geometry.SliderMaxs, cursorPos))
    {
        m_Action = A_MOVE;

        float sliderBarSize = m_bVerticalOrientation ? geometry.BgMaxs.Y - geometry.BgMins.Y : geometry.BgMaxs.X - geometry.BgMins.X;

        float cursor = m_bVerticalOrientation ? cursorPos.Y : cursorPos.X;

        m_DragCursor = cursor - ((m_Value - m_MinValue) / (m_MaxValue - m_MinValue) * sliderBarSize);
        return;
    }

    if (BvPointInRect(geometry.BgMins, geometry.BgMaxs, cursorPos))
    {
        float CursorLocalOffset = m_bVerticalOrientation ? cursorPos.Y - geometry.BgMins.Y : cursorPos.X - geometry.BgMins.X;

        MoveSlider(CursorLocalOffset);
        return;
    }
}

void UISlider::OnMouseMoveEvent(MouseMoveEvent const& event)
{
    if (m_Action == A_MOVE)
    {
        Float2 cursorPos = UIManager::sInstance().CursorPosition;

        MoveSlider((m_bVerticalOrientation ? cursorPos.Y : cursorPos.X) - m_DragCursor);
    }
}

void UISlider::Draw(Canvas& cv)
{
    UpdateSliderGeometry();

    UISliderGeometry const& geometry = m_SliderGeometry;

    // Draw slider background
    if (geometry.BgMaxs.X > geometry.BgMins.X && geometry.BgMaxs.Y > geometry.BgMins.Y)
    {
        //cv.DrawRectFilled( geometry.BgMins, geometry.BgMaxs, Color4(0.4f,0.4f,0.4f) );

        Float2 h = m_bVerticalOrientation ? Float2((geometry.BgMaxs.X - geometry.BgMins.X) * 0.5f, 0.0f) : Float2(0.0f, (geometry.BgMaxs.Y - geometry.BgMins.Y) * 0.5f);

        cv.DrawLine(geometry.BgMins + h, geometry.BgMaxs - h, LineColor, 2.0f);
    }

    // Draw slider
    if (geometry.SliderMaxs.X > geometry.SliderMins.X && geometry.SliderMaxs.Y > geometry.SliderMins.Y)
    {
        if (!SliderBrush)
            SliderBrush = UIManager::sInstance().DefaultSliderBrush();

        Hk::DrawBrush(cv, geometry.SliderMins, geometry.SliderMaxs, {}, SliderBrush);
    }
}

HK_NAMESPACE_END
