/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#pragma once

#include "WWidget.h"

#undef INCLUDE_STB_TEXTEDIT_H
#include <stb/stb_textedit.h>

class WScroll;

class ANGIE_API WTextEdit : public WWidget {
    AN_CLASS( WTextEdit, WWidget )

    friend class WTextEditProxy;

public:
    TWidgetEvent<> E_OnEnterPress;
    TWidgetEvent<> E_OnEscapePress;

    WTextEdit & SetFont( FFontAtlas * _Atlas, int _FontId );
    WTextEdit & SetMaxChars( int _MaxChars );
    WTextEdit & SetFilterDecimal( bool _Enabled );
    WTextEdit & SetFilterHexadecimal( bool _Enabled );
    WTextEdit & SetFilterUppercase( bool _Enabled );
    WTextEdit & SetFilterNoBlank( bool _Enabled );
    WTextEdit & SetFilterScientific( bool _Enabled );
    WTextEdit & SetFilterCustomCallback( bool _Enabled );
    WTextEdit & SetInsertSpacesOnTab( int _NumSpaces );
    WTextEdit & SetSingleLine( bool _Enabled );
    WTextEdit & SetReadOnly( bool _Enabled );
    WTextEdit & SetPassword( bool _Enabled );
    WTextEdit & SetCtrlEnterForNewLine( bool _Enabled );
    WTextEdit & SetAllowTabInput( bool _Enabled );
    WTextEdit & SetAllowUndo( bool _Enabled );
    WTextEdit & SetSelectionColor( FColor4 const & _Color );
    WTextEdit & SetTextColor( FColor4 const & _Color );

    void ClearSelection();

    void SelectAll();

    bool HasSelection() const;

    bool Cut();

    bool Copy();

    bool Paste();

    void ScrollHome();

    void ScrollEnd();

    void ScrollPageUp( bool _MoveCursor = false );

    void ScrollPageDown( bool _MoveCursor = false );

    void ScrollLineUp();

    void ScrollLineDown();

    void ScrollLines( int _NumLines );

    void ScrollLineStart();

    void ScrollLineEnd();

    void ScrollHorizontal( float _Delta );

    void ScrollToCursor();

    FWideChar * GetText() { return TextData.ToPtr(); }

    FWideChar const * GetText() const { return TextData.ToPtr(); }

    int GetTextLength() const;

    int GetCursorPosition() const;

    int GetSelectionStart() const;

    int GetSelectionEnd() const;

    FFont const * GetFont() const;

    // FUTURE:
    // WTextEdit & SetSyntaxHighlighter( ISyntaxHighlighter * _SyntaxHighlighterInterface );
    // class ISyntaxHighlighter {
    // public:
    //     virtual FColor4 const & GetWordColor( FWideChar * _WordStart, FWideChar * _WordEnd ) = 0;
    // };

protected:
    WTextEdit();
    ~WTextEdit();

    virtual bool OnFilterCharacter( FWideChar & _Char ) { return true; }

    void OnKeyEvent( struct FKeyEvent const & _Event, double _TimeStamp ) override;

    void OnMouseButtonEvent( struct FMouseButtonEvent const & _Event, double _TimeStamp ) override;

    void OnDblClickEvent( int _ButtonKey, Float2 const & _ClickPos, uint64_t _ClickTime ) override;

    void OnMouseWheelEvent( struct FMouseWheelEvent const & _Event, double _TimeStamp ) override;

    void OnMouseMoveEvent( struct FMouseMoveEvent const & _Event, double _TimeStamp ) override;

    void OnCharEvent( struct FCharEvent const & _Event, double _TimeStamp ) override;

    void OnFocusLost() override;

    void OnFocusReceive() override;

    void OnWindowHovered( bool _Hovered ) override;

    void OnDrawEvent( FCanvas & _Canvas ) override;

private:
    void PressKey( int _Key );
    bool FilterCharacter( FWideChar & _Char );
    void UpdateWidgetSize();
    bool InsertCharsProxy( int _Offset, FWideChar const * _Text, int _TextLength );
    void DeleteCharsProxy( int _First, int _Count );
    bool FindLineStartEnd( int _Cursor, FWideChar ** _LineStart, FWideChar ** _LineEnd );
    WScroll * GetScroll();

    FColor4 SelectionColor;
    FColor4 TextColor;

    TRef< FFontAtlas > FontAtlas;
    int FontId;

    TPodArray< FWideChar > TextData;
    int CurTextLength;
    int MaxChars;
    int CharacterFilter;
    int InsertSpacesOnTab;
    bool bSingleLine;
    bool bReadOnly;
    bool bPassword;
    bool bCtrlEnterForNewLine;
    bool bAllowTabInput;
    bool bAllowUndo;
    bool bCustomCharFilter;
    bool bStartDragging;
    STB_TexteditState Stb;
    int TempCursor;
};
