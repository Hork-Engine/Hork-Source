/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include <World/Public/Widgets/WMenuPopup.h>
#include <Core/Public/Logger.h>

AN_CLASS_META( WMenuPopup )

WMenuPopup::WMenuPopup() {
    Self = NewObject< WWidget >();
    Self->SetStyle( WIDGET_STYLE_POPUP );
    Self->SetLayout( WIDGET_LAYOUT_VERTICAL );
    Self->SetAutoWidth( true );
    Self->SetAutoHeight( true );
    Self->SetMargin( 10,10,10,10 );
    Self->SetVerticalPadding( 4 );
    Self->AddDecorate( &(*CreateInstanceOf< WBorderDecorate >() )
                       .SetFillBackground( true ) );
}

WMenuPopup::~WMenuPopup() {
//    for ( WWidget * w : Widgets ) {
//        w->RemoveRef();
//    }
}

void WMenuPopup::SelectFirstItem() {
    GLogger.Printf( "SelectFirstItem\n" );

    // TODO: ...
}

void WMenuPopup::SelectLastItem() {
    GLogger.Printf( "SelectLastItem\n" );

    // TODO: ...
}

void WMenuPopup::SelectNextItem() {
    GLogger.Printf( "SelectNextItem\n" );

    // TODO: ...
}

void WMenuPopup::SelectPrevItem() {
    GLogger.Printf( "SelectPrevItem\n" );

    // TODO: ...
}

void WMenuPopup::SelectNextSubMenu() {
    GLogger.Printf( "SelectNextSubMenu\n" );

    // TODO: ...
}

void WMenuPopup::SelectPrevSubMenu() {
    GLogger.Printf( "SelectPrevSubMenu\n" );

    // TODO: ...
}
