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

#include "WMenuPopup.h"
#include "WScroll.h"
#include <Platform/Logger.h>

HK_CLASS_META(WMenuPopup)

WMenuPopup::WMenuPopup()
{
    Self = CreateInstanceOf<WWidget>();
    Self->SetStyle(WIDGET_STYLE_POPUP);
    Self->SetLayout(WIDGET_LAYOUT_EXPLICIT);
    Self->SetAutoWidth(true);
    Self->SetAutoHeight(true);
    //Self->SetMargin( 10,10,10,10 );

#if 1
    WWidget& scroll = WNew(WScroll)
                          .SetContentWidget(
                              WNewAssign(ContentWidget, WWidget)
                                  .SetAutoWidth(true)
                                  .SetAutoHeight(true)
                                  .SetLayout(WIDGET_LAYOUT_VERTICAL))
                          .SetScrollbarSize(12)
                          .SetButtonWidth(12)
                          .SetShowButtons(false)
                          .SetSliderRounding(4)
                          .SetBackgroundColor(Color4(0, 0, 0, 0))
                          .SetButtonColor(Color4(0.03f, 0.03f, 0.03f, 1.0f))
                          .SetSliderBackgroundColor(Color4(0))
                          //.SetSliderColor( SliderColor )
                          .SetSliderColor(Color4(0.03f, 0.03f, 0.03f, 1.0f))
                          .SetAutoScrollH(true)
                          .SetAutoScrollV(true)
                          //.SetAutoWidth( true )
                          //.SetAutoHeight( true )
                          .SetHorizontalAlignment(WIDGET_ALIGNMENT_STRETCH)
                          .SetVerticalAlignment(WIDGET_ALIGNMENT_STRETCH);

    Self->AddWidget(&scroll);
#endif

    //Self->AddDecorate( &(*CreateInstanceOf< WBorderDecorate >() )
    //                   .SetFillBackground( true ) );
}

WMenuPopup::~WMenuPopup()
{
    //    for ( WWidget * w : Widgets ) {
    //        w->RemoveRef();
    //    }
}

void WMenuPopup::SelectFirstItem()
{
    GLogger.Printf("SelectFirstItem\n");

    // TODO: ...
}

void WMenuPopup::SelectLastItem()
{
    GLogger.Printf("SelectLastItem\n");

    // TODO: ...
}

void WMenuPopup::SelectNextItem()
{
    GLogger.Printf("SelectNextItem\n");

    // TODO: ...
}

void WMenuPopup::SelectPrevItem()
{
    GLogger.Printf("SelectPrevItem\n");

    // TODO: ...
}

void WMenuPopup::SelectNextSubMenu()
{
    GLogger.Printf("SelectNextSubMenu\n");

    // TODO: ...
}

void WMenuPopup::SelectPrevSubMenu()
{
    GLogger.Printf("SelectPrevSubMenu\n");

    // TODO: ...
}

void WMenuPopup::SetMargin(float _Left, float _Top, float _Right, float _Bottom)
{
    Self->SetMargin(_Left, _Top, _Right, _Bottom);
}

void WMenuPopup::SetMargin(Float4 const& Margin)
{
    Self->SetMargin(Margin);
}

void WMenuPopup::SetVerticalPadding(float _Padding)
{
    Self->SetVerticalPadding(_Padding);
}
