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

#include "UIButton.h"
#include "UIManager.h"

#include <Hork/Runtime/GameApplication/FrameLoop.h>

HK_NAMESPACE_BEGIN

UIButton* UICreateCheckBox(UIAction* action)
{
    auto decorator = UINew(UIBrushDecorator);

    //decorator->InactiveBrush = ...;
    //decorator->ActiveBrush   = ...;
    //decorator->HoverBrush    = ...;
    //decorator->SelectedBrush = ...;
    //decorator->DisabledBrush = ...;

    return UINew(UIButton)
        .WithAction(action)
        .WithDecorator(decorator);
}

void UIButton::AdjustSize(Float2 const& size)
{
    Super::AdjustSize(size);

    if (!m_Text)
        return;

    bool autoW = bAutoWidth && !m_Text->IsWordWrapEnabled();
    bool autoH = bAutoHeight;

    if (!autoW && !autoH)
        return;

    float breakRowWidth;
    if (size.X > 0.0f)
        breakRowWidth = Math::Max(0.0f, size.X - Padding.Left - Padding.Right);
    else
        breakRowWidth = Math::MaxValue<float>();  

    Float2 boxSize = m_Text->GetTextBoxSize(breakRowWidth);

    if (autoW)
        m_AdjustedSize.X = boxSize.X;
    if (autoH)
        m_AdjustedSize.Y = boxSize.Y;
}

void UIButton::Draw(Canvas& canvas)
{
    if (m_Decorator)
    {
        DRAW drawType = GetDrawType();

        switch (drawType)
        {
            case DRAW_INACTIVE:
                m_Decorator->DrawInactive(canvas, m_Geometry);
                break;
            case DRAW_ACTIVE:
                m_Decorator->DrawActive(canvas, m_Geometry);
                break;
            case DRAW_HOVERED:
                m_Decorator->DrawHovered(canvas, m_Geometry);
                break;
            case DRAW_DISABLED:
                m_Decorator->DrawDisabled(canvas, m_Geometry);
                break;
        }
    }

    if (m_Text)
        m_Text->Draw(canvas, m_Geometry.PaddedMins, m_Geometry.PaddedMaxs);
}

void UIButton::OnMouseButtonEvent(MouseButtonEvent const& event)
{
    if (IsDisabled() || !m_Action || event.Button != VirtualKey::MouseLeftBtn)
        return;

    if (event.Action == InputAction::Pressed)
    {
        if (m_bTriggerOnPress)
        {
            m_Action->Triggered();
        }
        else
            m_bTryPress = true;
    }
    else if (event.Action == InputAction::Released)
    {
        auto& uiManager = UIManager::sInstance();
        if (uiManager.HoveredWidget.RawPtr() == this && m_bTryPress && !m_bTriggerOnPress)
        {
            m_Action->Triggered();
        }
        m_bTryPress = false;
    }
}

UIButton::DRAW UIButton::GetDrawType() const
{
    if (IsDisabled())
        return DRAW_DISABLED;

    if (!m_Action)
        return DRAW_INACTIVE;

    auto& uiManager = UIManager::sInstance();
    if (m_Action->IsActive())
        return m_bTryPress ? (uiManager.HoveredWidget.RawPtr() == this ? DRAW_HOVERED : DRAW_INACTIVE) : DRAW_ACTIVE;
    else
        return m_bTryPress ? DRAW_ACTIVE : (uiManager.HoveredWidget.RawPtr() == this ? DRAW_HOVERED : DRAW_INACTIVE);
}

HK_NAMESPACE_END
