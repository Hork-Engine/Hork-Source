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

#include "UIButton.h"
#include "UIManager.h"

#include <Runtime/FrameLoop.h>

UIButton* UICreateCheckBox(UIAction* action)
{
    auto button = CreateInstanceOf<UIButton>();
    button->BindAction(action);

    auto decorator = CreateInstanceOf<UIBrushDecorator>();

    //decorator->InactiveBrush = ...;
    //decorator->ActiveBrush   = ...;
    //decorator->HoverBrush    = ...;
    //decorator->SelectedBrush = ...;
    //decorator->DisabledBrush = ...;

    button->SetDecorator(decorator);
    return button;
}

void UIButton::AdjustSize(Float2 const& size)
{
    Super::AdjustSize(size);

    if (!m_Text)
        return;

    bool autoW = bAutoWidth && !m_Text->bWrap;
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
        AdjustedSize.X = boxSize.X;// + Padding.Left + Padding.Right;
    if (autoH)
        AdjustedSize.Y = boxSize.Y;// + Padding.Top + Padding.Bottom;
}

void UIButton::Draw(ACanvas& canvas)
{
    if (m_Decorator)
    {
        DRAW drawType = GetDrawType();

        switch (drawType)
        {
            case DRAW_INACTIVE:
                m_Decorator->DrawInactive(canvas, Geometry);
                break;
            case DRAW_ACTIVE:
                m_Decorator->DrawActive(canvas, Geometry);
                break;
            case DRAW_HOVERED:
                m_Decorator->DrawHovered(canvas, Geometry);
                break;
            case DRAW_DISABLED:
                m_Decorator->DrawDisabled(canvas, Geometry);
                break;
        }
    }

    if (m_Text)
        m_Text->Draw(canvas, Geometry.PaddedMins, Geometry.PaddedMaxs);
}

void UIButton::OnMouseButtonEvent(SMouseButtonEvent const& event, double timeStamp)
{
    if (IsDisabled() || !m_Action || event.Button != 0)
        return;

    if (event.Action == IA_PRESS)
    {
        m_bTryPress = true;
    }
    else if (event.Action == IA_RELEASE)
    {
        if (GUIManager->HoveredWidget.GetObject() == this && m_bTryPress)
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

    if (m_Action->IsActive())
        return m_bTryPress ? (GUIManager->HoveredWidget.GetObject() == this ? DRAW_HOVERED : DRAW_INACTIVE) : DRAW_ACTIVE;
    else
        return m_bTryPress ? DRAW_ACTIVE : (GUIManager->HoveredWidget.GetObject() == this ? DRAW_HOVERED : DRAW_INACTIVE);
}
