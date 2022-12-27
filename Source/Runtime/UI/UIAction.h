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

#include "UIBrush.h"

class UIAction : public UIObject
{
    UI_CLASS(UIAction, UIObject)

public:
    // If Stick is enabled, the button stays pressed after being clicked until the user click.
    bool bStick{};

    bool bDisabled{};

    TEvent<UIAction*> E_OnActivate;
    TEvent<UIAction*> E_OnDeactivate;

    UIAction() = default;

    UIAction(TEvent<UIAction*> OnActivate) :
        E_OnActivate(std::move(OnActivate))
    {}

    UIAction(TEvent<UIAction*> OnActivate, TEvent<UIAction*> OnDeactivate) :
        E_OnActivate(std::move(OnActivate)),
        E_OnDeactivate(std::move(OnDeactivate))
    {}

    template <typename T>
    UIAction(T* object, void (T::*OnActivate)(UIAction*))
    {
        E_OnActivate.Add(object, OnActivate);
    }

    template <typename T>
    UIAction(T* object, void (T::* OnActivate)(UIAction*), void (T::* OnDeactivate)(UIAction*))
    {
        E_OnActivate.Add(object, OnActivate);
        E_OnDeactivate.Add(object, OnDeactivate);
    }

    void Activate()
    {
        if (bDisabled)
            return;

        E_OnActivate.Dispatch(this);

        if (bStick)
            m_bActive = true;
    }

    void Deactivate()
    {
        if (bDisabled)
            return;

        m_bActive = false;
        E_OnDeactivate.Dispatch(this);
    }

    bool IsActive() const
    {
        return m_bActive;
    }

    bool IsInactive() const
    {
        return !m_bActive;
    }

    void Triggered()
    {
        if (bStick)
        {
            if (IsActive())
                Deactivate();
            else
                Activate();
        }
        else
            Activate();
    }

private:
    bool m_bActive{};
};
