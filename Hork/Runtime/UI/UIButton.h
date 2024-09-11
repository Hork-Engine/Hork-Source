/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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
#include "UIText.h"
#include "UIDecorator.h"
#include "UIAction.h"

HK_NAMESPACE_BEGIN

class UIButton : public UIWidget
{
    UI_CLASS(UIButton, UIWidget)

    Ref<UIAction>    m_Action;
    Ref<UIDecorator> m_Decorator;
    Ref<UIText>      m_Text;
    bool              m_bTryPress{};
    bool              m_bTriggerOnPress{};

public:
    UIButton& WithAction(UIAction* action)
    {
        m_Action = action;
        return *this;
    }

    UIButton& WithDecorator(UIDecorator* decorator)
    {
        m_Decorator = decorator;
        return *this;
    }

    UIButton& WithText(UIText* text)
    {
        m_Text = text;
        return *this;
    }

    UIButton& WithTriggerOnPress(bool bTriggerOnPress)
    {
        m_bTriggerOnPress = bTriggerOnPress;
        return *this;
    }

    bool IsDisabled() const override
    {
        if (m_Action ? m_Action->bDisabled : bDisabled)
            return true;
        return m_Parent ? m_Parent->IsDisabled() : false;
    }

protected:
    void AdjustSize(Float2 const& size) override;

    void Draw(Canvas& canvas) override;

    void OnMouseButtonEvent(struct MouseButtonEvent const& event) override;
    
private:
    enum DRAW
    {
        DRAW_INACTIVE,
        DRAW_ACTIVE,
        DRAW_HOVERED,
        DRAW_DISABLED
    };

    DRAW GetDrawType() const;
};

HK_NAMESPACE_END
