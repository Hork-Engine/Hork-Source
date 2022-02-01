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

#include "WWidget.h"

class WMenuPopup : public ABaseObject
{
    AN_CLASS(WMenuPopup, ABaseObject)

    friend class WDesktop;

public:
    /** Add child widget */
    WMenuPopup& AddWidget(WWidget* _Widget)
    {
        ContentWidget->AddWidget(_Widget);
        return *this;
    }

    /** Helper. Add child widget */
    WMenuPopup& operator[](WWidget& _Widget)
    {
        return AddWidget(&_Widget);
    }

    /** Helper. Add child decorate */
    WMenuPopup& operator[](WDecorate& _Decorate)
    {
        Self->AddDecorate(&_Decorate);
        return *this;
    }

    /** Add widget decoration */
    WMenuPopup& AddDecorate(WDecorate* _Decorate)
    {
        Self->AddDecorate(_Decorate);
        return *this;
    }

    /** Remove widget decoration */
    WMenuPopup& RemoveDecorate(WDecorate* _Decorate)
    {
        Self->RemoveDecorate(_Decorate);
        return *this;
    }

    /** Remove all widget decorations */
    WMenuPopup& RemoveDecorates()
    {
        Self->RemoveDecorates();
        return *this;
    }

    void SelectFirstItem();
    void SelectLastItem();
    void SelectNextItem();
    void SelectPrevItem();
    void SelectNextSubMenu();
    void SelectPrevSubMenu();

    /** Determines the padding of the client area within the widget */
    void SetMargin(float _Left, float _Top, float _Right, float _Bottom);

    /** Determines the padding of the client area within the widget */
    void SetMargin(Float4 const& _Margin);

    /** Vertical padding */
    void SetVerticalPadding(float _Padding);

    WMenuPopup();
    ~WMenuPopup();

private:
    TRef<WWidget> Self;
    TRef<WWidget> ContentWidget;
    //float CurWidth;
    //float CurHeight;
};
