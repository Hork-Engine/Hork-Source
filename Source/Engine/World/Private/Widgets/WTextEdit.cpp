/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include <World/Public/Widgets/WTextEdit.h>
#include <World/Public/Widgets/WScroll.h>
#include <World/Public/Widgets/WDesktop.h>
#include <Runtime/Public/Runtime.h>
#include <Runtime/Public/InputDefs.h>
#include <World/Public/Base/ResourceManager.h>

// Forward decl
static Float2 CalcTextRect( AFont const * _Font, SWideChar const * _TextBegin, SWideChar const * _TextEnd, const SWideChar** _Remaining, Float2 * _OutOffset, bool _StopOnNewLine );

#undef STB_TEXTEDIT_STRING
#undef STB_TEXTEDIT_CHARTYPE
#define STB_TEXTEDIT_STRING             WTextEdit
#define STB_TEXTEDIT_CHARTYPE           SWideChar
#define STB_TEXTEDIT_GETWIDTH_NEWLINE   -1.0f
#define STB_TEXTEDIT_UNDOSTATECOUNT     99
#define STB_TEXTEDIT_UNDOCHARCOUNT      999

#define STB_TEXTEDIT_K_LEFT         0x10000 // keyboard input to move cursor left
#define STB_TEXTEDIT_K_RIGHT        0x10001 // keyboard input to move cursor right
#define STB_TEXTEDIT_K_UP           0x10002 // keyboard input to move cursor up
#define STB_TEXTEDIT_K_DOWN         0x10003 // keyboard input to move cursor down
#define STB_TEXTEDIT_K_LINESTART    0x10004 // keyboard input to move cursor to start of line
#define STB_TEXTEDIT_K_LINEEND      0x10005 // keyboard input to move cursor to end of line
#define STB_TEXTEDIT_K_TEXTSTART    0x10006 // keyboard input to move cursor to start of text
#define STB_TEXTEDIT_K_TEXTEND      0x10007 // keyboard input to move cursor to end of text
#define STB_TEXTEDIT_K_DELETE       0x10008 // keyboard input to delete selection or character under cursor
#define STB_TEXTEDIT_K_BACKSPACE    0x10009 // keyboard input to delete selection or character left of cursor
#define STB_TEXTEDIT_K_UNDO         0x1000A // keyboard input to perform undo
#define STB_TEXTEDIT_K_REDO         0x1000B // keyboard input to perform redo
#define STB_TEXTEDIT_K_WORDLEFT     0x1000C // keyboard input to move cursor left one word
#define STB_TEXTEDIT_K_WORDRIGHT    0x1000D // keyboard input to move cursor right one word
#define STB_TEXTEDIT_K_SHIFT        0x20000 // a power of two that is or'd in to a keyboard input to represent the shift key

#undef INCLUDE_STB_TEXTEDIT_H
#include "stb_textedit.h"

static SWideChar STB_TEXTEDIT_NEWLINE = '\n';

#define STB_TEXTEDIT_STRINGLEN( _Obj ) _Obj->GetTextLength()

#define STB_TEXTEDIT_GETCHAR( _Obj, i ) _Obj->GetText()[i]

static int STB_TEXTEDIT_KEYTOTEXT( int _Key ) { return _Key >= 0x10000 ? 0 : _Key; }

static void STB_TEXTEDIT_LAYOUTROW( StbTexteditRow * _Row, WTextEdit * _Obj, int _LineStartIndex ) {
    SWideChar const * text = _Obj->GetText();
    SWideChar const * text_remaining = NULL;
    const Float2 size = CalcTextRect( _Obj->GetFont(), text + _LineStartIndex, text + _Obj->GetTextLength(), &text_remaining, NULL, true );
    _Row->x0 = 0.0f;
    _Row->x1 = size.X;
    _Row->baseline_y_delta = size.Y;
    _Row->ymin = 0.0f;
    _Row->ymax = size.Y;
    _Row->num_chars = ( int )( text_remaining - ( text + _LineStartIndex ) );
}

static float STB_TEXTEDIT_GETWIDTH( WTextEdit * _Obj, int _LineStartIndex, int _CharIndex ) {
    SWideChar c = _Obj->GetText()[ _LineStartIndex + _CharIndex ];
    if ( c == '\n' )
        return STB_TEXTEDIT_GETWIDTH_NEWLINE;

    return _Obj->GetFont()->GetCharAdvance( c );// *( FontSize / GetFont()->FontSize );
}

class WTextEditProxy {
public:
    WTextEditProxy( WTextEdit * _Self ) : Self( _Self ) {}

    bool InsertCharsProxy( int _Offset, SWideChar const * _Text, int _TextLength ) {
        return Self->InsertCharsProxy( _Offset, _Text, _TextLength );
    }

    void DeleteCharsProxy( int _First, int _Count ) {
        return Self->DeleteCharsProxy( _First, _Count );
    }

private:
    WTextEdit * Self;
};

#define STB_TEXTEDIT_DELETECHARS( _Obj, _First, _Count ) WTextEditProxy(_Obj).DeleteCharsProxy( _First, _Count )

#define STB_TEXTEDIT_INSERTCHARS( _Obj, _Offset, _Text, _TextLength ) WTextEditProxy(_Obj).InsertCharsProxy( _Offset, _Text, _TextLength )

AN_FORCEINLINE bool IsSeparator( SWideChar c ) {
    return     c == ','
            || c == '.'
            || c == ';'
            || c == ':'
            || c == '('
            || c == ')'
            || c == '{'
            || c == '}'
            || c == '['
            || c == ']'
            || c == '{'
            || c == '}'
            || c == '<'
            || c == '>'
            || c == '|'
            || c == '!'
            || c == '@'
            || c == '#'
            || c == '$'
            || c == '%'
            || c == '^'
            || c == '&'
            || c == '*'
            || c == '/'
            || c == '\\'
            || c == '+'
            || c == '='
            || c == '-'
            || c == '~'
            || c == '`'
            || c == '\''
            || c == '"'
            || c == '?'
            || c == '\n';
}

static bool IsWordBoundary( SWideChar * s ) {
    if ( Core::WideCharIsBlank( s[-1] ) && !Core::WideCharIsBlank( s[0] ) ) {
        return true;
    }

    if ( s[-1] == '\n' ) {
        return true;
    }

    if ( !Core::WideCharIsBlank( s[0] ) ) {
        if ( ( IsSeparator(s[-1]) || IsSeparator(s[0]) ) && ( s[-1] != s[0] ) ) {
            return true;
        }
    }

    return false;
}

static int NextWord( WTextEdit * _Obj, int i ) {
    i++;
    int len = _Obj->GetTextLength();
    SWideChar * s = _Obj->GetText() + i;
    while ( i < len && !IsWordBoundary( s ) ) {
        i++;
        s++;
    }
    return i > len ? len : i;
}

static int PrevWord( WTextEdit * _Obj, int i ) {
    i--;
    SWideChar * s = _Obj->GetText() + i;
    while ( i > 0 && !IsWordBoundary( s ) ) {
        i--;
        s--;
    }
    return i < 0 ? 0 : i;
}

#define STB_TEXTEDIT_MOVEWORDRIGHT  NextWord
#define STB_TEXTEDIT_MOVEWORDLEFT   PrevWord

#define STB_TEXTEDIT_IMPLEMENTATION
#include "stb_textedit.h"

static const bool bOSX = false;

enum FCharacterFilter {
    CHARS_DECIMAL       = AN_BIT( 0 ),   // 0123456789.+-*/
    CHARS_HEXADECIMAL   = AN_BIT( 1 ),   // 0123456789ABCDEFabcdef
    CHARS_UPPERCASE     = AN_BIT( 2 ),   // a..z -> A..Z
    CHARS_NO_BLANK      = AN_BIT( 3 ),   // filter out spaces, tabs
    CHARS_SCIENTIFIC    = AN_BIT( 4 ),   // 0123456789.+-*/eE (Scientific notation input)
};

static Float2 CalcTextRect( AFont const * _Font, SWideChar const * _TextBegin, SWideChar const * _TextEnd, const SWideChar** _Remaining, Float2 * _OutOffset, bool _StopOnNewLine ) {
    const float lineHeight = _Font->GetFontSize();
    Float2 rectSize( 0, 0 );
    float lineWidth = 0.0f;

    SWideChar const * s = _TextBegin;
    while ( s < _TextEnd ) {
        SWideChar c = *s++;
        if ( c == '\n' ) {
            rectSize.X = Math::Max( rectSize.X, lineWidth );
            rectSize.Y += lineHeight;
            lineWidth = 0.0f;
            if ( _StopOnNewLine ) {
                break;
            }
            continue;
        }
        if ( c == '\r' ) {
            continue;
        }
        lineWidth += _Font->GetCharAdvance( c );
    }

    if ( rectSize.X < lineWidth ) {
        rectSize.X = lineWidth;
    }

    if ( _OutOffset ) {
        *_OutOffset = Float2( lineWidth, rectSize.Y + lineHeight );
    }

    if ( lineWidth > 0 || rectSize.Y == 0.0f ) {
        rectSize.Y += lineHeight;
    }

    if ( _Remaining ) {
        *_Remaining = s;
    }

    return rectSize;
}

static Float2 CalcCursorOffset( AFont const * _Font, SWideChar * _Text, int _Cursor, const SWideChar ** _Remaining ) {
    const float lineHeight = _Font->GetFontSize();
    Float2 offset( 0 );
    float lineWidth = 0.0f;
    SWideChar const * s = _Text;
    SWideChar const * end = _Text + _Cursor;
    while ( s < end ) {
        SWideChar c = *s++;
        if ( c == '\n' ) {
            offset.Y += lineHeight;
            lineWidth = 0.0f;
            continue;
        }
        if ( c == '\r' ) {
            continue;
        }
        lineWidth += _Font->GetCharAdvance( c );
    }
    offset.X = lineWidth;
    if ( _Remaining ) {
        *_Remaining = s;
    }
    return offset;
}

AN_CLASS_META( WTextEdit )

WTextEdit::WTextEdit() {
    bSingleLine = false;

    Stb = ( STB_TexteditState * )GZoneMemory.Alloc( sizeof( STB_TexteditState ) );
    stb_textedit_initialize_state( Stb, bSingleLine );

    bReadOnly = false;
    bPassword = false;
    bCtrlEnterForNewLine = false;
    bAllowTabInput = true;
    bAllowUndo = true;
    bCustomCharFilter = false;
    bStartDragging = false;

    CurTextLength = 0;
    MaxChars = 0;
    CharacterFilter = 0;
    InsertSpacesOnTab = 4;
    TempCursor = 0;

    SelectionColor = AColor4( 0.32f, 0.32f, 0.4f );
    TextColor = AColor4( 0.9f, 0.9f, 0.9f );

    SetSize( 0, 0 );
}

WTextEdit::~WTextEdit() {
    GZoneMemory.Free( Stb );
}

WTextEdit & WTextEdit::SetFont( AFont * _Font ) {
    Font = _Font;
    return *this;
}

WTextEdit & WTextEdit::SetMaxChars( int _MaxChars ) {
    MaxChars = _MaxChars;
    return *this;
}

WTextEdit & WTextEdit::SetFilterDecimal( bool _Enabled ) {
    if ( _Enabled ) {
        CharacterFilter |= CHARS_DECIMAL;
    } else {
        CharacterFilter &= ~CHARS_DECIMAL;
    }
    return *this;
}

WTextEdit & WTextEdit::SetFilterHexadecimal( bool _Enabled ) {
    if ( _Enabled ) {
        CharacterFilter |= CHARS_HEXADECIMAL;
    } else {
        CharacterFilter &= ~CHARS_HEXADECIMAL;
    }
    return *this;
}

WTextEdit & WTextEdit::SetFilterUppercase( bool _Enabled ) {
    if ( _Enabled ) {
        CharacterFilter |= CHARS_UPPERCASE;
    } else {
        CharacterFilter &= ~CHARS_UPPERCASE;
    }
    return *this;
}

WTextEdit & WTextEdit::SetFilterNoBlank( bool _Enabled ) {
    if ( _Enabled ) {
        CharacterFilter |= CHARS_NO_BLANK;
    } else {
        CharacterFilter &= ~CHARS_NO_BLANK;
    }
    return *this;
}

WTextEdit & WTextEdit::SetFilterScientific( bool _Enabled ) {
    if ( _Enabled ) {
        CharacterFilter |= CHARS_SCIENTIFIC;
    } else {
        CharacterFilter &= ~CHARS_SCIENTIFIC;
    }
    return *this;
}

WTextEdit & WTextEdit::SetFilterCustomCallback( bool _Enabled ) {
    bCustomCharFilter = _Enabled;
    return *this;
}

WTextEdit & WTextEdit::SetInsertSpacesOnTab( int _NumSpaces ) {
    InsertSpacesOnTab = _NumSpaces;
    return *this;
}

WTextEdit & WTextEdit::SetSingleLine( bool _Enabled ) {
    bSingleLine = _Enabled;
    stb_textedit_initialize_state( Stb, bSingleLine );
    return *this;
}

WTextEdit & WTextEdit::SetReadOnly( bool _Enabled ) {
    bReadOnly = _Enabled;
    return *this;
}

WTextEdit & WTextEdit::SetPassword( bool _Enabled ) {
    bPassword = _Enabled;
    return *this;
}

WTextEdit & WTextEdit::SetCtrlEnterForNewLine( bool _Enabled ) {
    bCtrlEnterForNewLine = _Enabled;
    return *this;
}

WTextEdit & WTextEdit::SetAllowTabInput( bool _Enabled ) {
    bAllowTabInput = _Enabled;
    return *this;
}

WTextEdit & WTextEdit::SetAllowUndo( bool _Enabled ) {
    bAllowUndo = _Enabled;
    return *this;
}

WTextEdit & WTextEdit::SetSelectionColor( AColor4 const & _Color ) {
    SelectionColor = _Color;
    return *this;
}

WTextEdit & WTextEdit::SetTextColor( AColor4 const & _Color ) {
    TextColor = _Color;
    return *this;
}

AFont const * WTextEdit::GetFont() const {
    return Font ? Font : ACanvas::GetDefaultFont();
}

int WTextEdit::GetTextLength() const {
    return CurTextLength;
}

int WTextEdit::GetCursorPosition() const {
    return Stb->cursor;
}

int WTextEdit::GetSelectionStart() const {
    return Math::Min( Stb->select_start, Stb->select_end );
}

int WTextEdit::GetSelectionEnd() const {
    return Math::Max( Stb->select_start, Stb->select_end );
}

bool WTextEdit::InsertCharsProxy( int _Offset, SWideChar const * _Text, int _TextLength ) {
    if ( _Offset > CurTextLength ) {
        return false;
    }

    bool bResizable = MaxChars > 0;
    if ( bResizable && CurTextLength + _TextLength > MaxChars ) {
        _TextLength = MaxChars - CurTextLength;
        if ( _TextLength <= 0 ) {
            return false;
        }
    }

    if ( CurTextLength + _TextLength + 1 > TextData.Size() ) {
        TextData.Resize( CurTextLength + _TextLength + 1 );
    }

    SWideChar * text = TextData.ToPtr();
    if ( _Offset != CurTextLength ) {
        Core::Memmove( text + _Offset + _TextLength, text + _Offset, ( size_t )( CurTextLength - _Offset ) * sizeof( SWideChar ) );
    }
    Core::Memcpy( text + _Offset, _Text, ( size_t )_TextLength * sizeof( SWideChar ) );

    CurTextLength += _TextLength;
    TextData[ CurTextLength ] = '\0';

    UpdateWidgetSize();

    E_OnTyping.Dispatch( TextData.ToPtr() );

    return true;
}

void WTextEdit::DeleteCharsProxy( int _First, int _Count ) {
    if ( _Count < 0 || _First < 0 ) {
        return;
    }

    if ( _First >= CurTextLength ) {
        return;
    }

    if ( _First + _Count > CurTextLength ) {
        _Count = CurTextLength - _First - 1;
    }

    CurTextLength -= _Count;

    Core::Memmove( &TextData[_First], &TextData[_First + _Count], ( CurTextLength - _First ) * sizeof( SWideChar ) );
    TextData[CurTextLength] = '\0';

    UpdateWidgetSize();

    E_OnTyping.Dispatch( TextData.ToPtr() );
}

void WTextEdit::PressKey( int _Key ) {
    if ( _Key ) {
        stb_textedit_key( this, Stb, _Key );
    }
}

void WTextEdit::ClearSelection() {
    Stb->select_start = Stb->select_end = Stb->cursor;
}

void WTextEdit::SelectAll() {
    Stb->select_start = 0;
    Stb->cursor = Stb->select_end = CurTextLength;
    Stb->has_preferred_x = 0;
}

bool WTextEdit::HasSelection() const {
    return Stb->select_start != Stb->select_end;
}

WScroll * WTextEdit::GetScroll() {
    return Upcast< WScroll >( GetParent() );
}

void WTextEdit::ScrollHome() {
    if ( bSingleLine ) {
        return;
    }

    WScroll * scroll = GetScroll();
    if ( scroll ) {
        scroll->ScrollHome();
    }
}

void WTextEdit::ScrollEnd() {
    if ( bSingleLine ) {
        return;
    }

    WScroll * scroll = GetScroll();
    if ( scroll ) {
        scroll->ScrollEnd();
    }
}

void WTextEdit::ScrollPageUp( bool _MoveCursor ) {
    if ( bSingleLine ) {
        return;
    }

    WScroll * scroll = GetScroll();
    if ( scroll ) {
        AFont const * font = GetFont();
        const float lineHeight = font->GetFontSize();

        float PageSize = scroll->GetAvailableHeight();
        PageSize = Math::Snap( PageSize, lineHeight );

        int numLines = (int)(PageSize / lineHeight);

        if ( _MoveCursor ) {
            for ( int i = 0 ; i < numLines ; i++ ) {
                PressKey( STB_TEXTEDIT_K_UP );
            }
        }

        ScrollLines( numLines );
    }
}

void WTextEdit::ScrollPageDown( bool _MoveCursor ) {
    if ( bSingleLine ) {
        return;
    }

    WScroll * scroll = GetScroll();
    if ( scroll ) {
        AFont const * font = GetFont();
        const float lineHeight = font->GetFontSize();

        float PageSize = scroll->GetAvailableHeight();
        PageSize = Math::Snap( PageSize, lineHeight );

        int numLines = (int)(PageSize / lineHeight);

        if ( _MoveCursor ) {
            for ( int i = 0 ; i < numLines ; i++ ) {
                PressKey( STB_TEXTEDIT_K_DOWN );
            }
        }

        ScrollLines( -numLines );
    }
}

void WTextEdit::ScrollLineUp() {
    ScrollLines( 1 );
}

void WTextEdit::ScrollLineDown() {
    ScrollLines( -1 );
}

void WTextEdit::ScrollLines( int _NumLines ) {
    if ( bSingleLine ) {
        return;
    }

    WScroll * scroll = GetScroll();
    if ( scroll ) {
        Float2 scrollPosition = scroll->GetScrollPosition();

        AFont const * font = GetFont();
        const float lineHeight = font->GetFontSize();

        scrollPosition.Y = Math::Snap( scrollPosition.Y, lineHeight );
        scrollPosition.Y += _NumLines * lineHeight;

        scroll->SetScrollPosition( scrollPosition );
    }
}

void WTextEdit::ScrollLineStart() {
    WScroll * scroll = GetScroll();
    if ( scroll ) {
        Float2 scrollPosition = scroll->GetScrollPosition();

        scrollPosition.X = 0;

        scroll->SetScrollPosition( scrollPosition );
    }
}

bool WTextEdit::FindLineStartEnd( int _Cursor, SWideChar ** _LineStart, SWideChar ** _LineEnd ) {
    if ( _Cursor < 0 || _Cursor >= CurTextLength ) {
        return false;
    }

    SWideChar * text = TextData.ToPtr();
    SWideChar * textEnd = TextData.ToPtr() + CurTextLength;
    SWideChar * lineStart = text + Stb->cursor;
    SWideChar * lineEnd = text + Stb->cursor + 1;

    if ( *lineStart != '\n' ) {
        while ( lineEnd < textEnd ) {
            if ( *lineEnd == '\n' ) {
                break;
            }
            lineEnd++;
        }
    } else {
        lineEnd = lineStart;
        lineStart--;
    }
    while ( lineStart >= text ) {
        if ( *lineStart == '\n' ) {
            break;
        }
        lineStart--;
    }
    lineStart++;

    *_LineStart = lineStart;
    *_LineEnd = lineEnd;

    return true;
}

void WTextEdit::ScrollLineEnd() {
    WScroll * scroll = GetScroll();
    if ( !scroll ) {
        return;
    }

    AFont const * font = GetFont();
    SWideChar * lineStart;
    SWideChar * lineEnd;

    if ( FindLineStartEnd( Stb->cursor, &lineStart, &lineEnd ) ) {
        float lineWidth = 0;
        for ( SWideChar * s = lineStart ; s < lineEnd ; s++ ) {
            lineWidth += font->GetCharAdvance( *s );
        }

        float PageWidth = scroll->GetAvailableWidth();
        Float2 scrollPosition = scroll->GetScrollPosition();
        scrollPosition.X = -lineWidth + PageWidth * 0.5f;
        scroll->SetScrollPosition( scrollPosition );
    }
}

void WTextEdit::ScrollHorizontal( float _Delta ) {
    WScroll * scroll = GetScroll();
    if ( scroll ) {
        scroll->ScrollDelta( Float2( _Delta, 0.0f ) );
    }
}

void WTextEdit::ScrollToCursor() {
    WScroll * scroll = GetScroll();
    if ( !scroll ) {
        return;
    }

    AFont const * font = GetFont();
    Float2 mins, maxs;
    Float2 scrollMins, scrollMaxs;

    GetDesktopRect( mins, maxs, false );
    scroll->GetDesktopRect( scrollMins, scrollMaxs, true );

    Float2 cursorOffset = CalcCursorOffset( font, TextData.ToPtr(), Stb->cursor, nullptr );
    Float2 cursor = mins + cursorOffset;

    Float2 scrollPosition = scroll->GetScrollPosition();
    bool bUpdateScroll = false;

    Float2 pageSize = scroll->GetAvailableSize();

    pageSize.Y = Math::Snap< float >( pageSize.Y, font->GetFontSize() );

    if ( cursor.X < scrollMins.X ) {
        scrollPosition.X = Math::Snap< float >( -cursorOffset.X + pageSize.X*0.5f, font->GetFontSize() );
        bUpdateScroll = true;
    } else if ( cursor.X > scrollMaxs.X ) {
        scrollPosition.X = Math::Snap< float >( -cursorOffset.X + pageSize.X*0.5f, font->GetFontSize() );
        bUpdateScroll = true;
    }

    if ( cursor.Y < scrollMins.Y ) {
        scrollPosition.Y = Math::Snap< float >( -cursorOffset.Y, font->GetFontSize() );
        bUpdateScroll = true;
    } else if ( cursor.Y + font->GetFontSize()*2 > scrollMaxs.Y ) {
        scrollPosition.Y = Math::Snap< float >( -cursorOffset.Y - font->GetFontSize()*2 + pageSize.Y, font->GetFontSize() );
        bUpdateScroll = true;
    }

    if ( bUpdateScroll ) {
        scroll->SetScrollPosition( scrollPosition );
    }
}

bool WTextEdit::Cut() {
    if ( bReadOnly ) {
        // Can't modify readonly text
        return false;
    }

    if ( !Copy() ) {
        return false;
    }

    if ( !HasSelection() ) {
        SelectAll();
    }
    stb_textedit_cut( this, Stb );

    return true;
}

bool WTextEdit::Copy() {
    if ( bPassword ) {
        // Can't copy password
        return false;
    }

    bool bHasSelection = HasSelection();

    if ( !bSingleLine && !bHasSelection ) {
        // Can't copy multiline text if no selection
        return false;
    }

    const int startOfs = bHasSelection ? GetSelectionStart() : 0;
    const int endOfs = bHasSelection ? GetSelectionEnd() : CurTextLength;
    SWideChar * const start = TextData.ToPtr() + startOfs;
    SWideChar * const end = TextData.ToPtr() + endOfs;

    const int clipboardDataLen = Core::WideStrUTF8Bytes( start, end ) + 1;

    char * pClipboardData = ( char * )GZoneMemory.Alloc( clipboardDataLen );

    Core::WideStrEncodeUTF8( pClipboardData, clipboardDataLen, start, end );

    GRuntime.SetClipboard( pClipboardData );

    GZoneMemory.Free( pClipboardData );

    return true;
}

bool WTextEdit::Paste() {
    if ( bReadOnly ) {
        // Can't modify readonly text
        return false;
    }

    AString const & clipboard = GRuntime.GetClipboard();

    const char * s = clipboard.CStr();

    int len = Core::UTF8StrLength( s );

    TPodArray< SWideChar > wideStr;
    wideStr.Resize( len );

    SWideChar ch;
    int i, byteLen;
    i = 0;
    while ( len-- > 0 ) {
        byteLen = Core::WideCharDecodeUTF8( s, ch );
        if ( !byteLen ) {
            break;
        }
        s += byteLen;
        if ( !FilterCharacter( ch ) ) {
            continue;
        }
        wideStr[ i++ ] = ch;

    }
    if ( i > 0 ) {
        stb_textedit_paste( this, Stb, wideStr.ToPtr(), i );
    }

    return true;
}

WTextEdit & WTextEdit::SetText( const char * _Text ) {
    int len = Core::UTF8StrLength( _Text );

    TPodArray< SWideChar > wideStr;
    wideStr.Resize( len + 1 );

    SWideChar ch;
    int i, byteLen;
    i = 0;
    while ( len-- > 0 ) {
        byteLen = Core::WideCharDecodeUTF8( _Text, ch );
        if ( !byteLen ) {
            break;
        }
        _Text += byteLen;
        if ( !FilterCharacter( ch ) ) {
            continue;
        }
        wideStr[ i++ ] = ch;
    }

    wideStr[ i ] = 0;

    return SetText( wideStr.ToPtr() );
}

WTextEdit & WTextEdit::SetText( const SWideChar * _Text ) {
    int len = Core::WideStrLength( _Text );

    SelectAll();

    stb_textedit_paste( this, Stb, _Text, len );

    return *this;
}

void WTextEdit::OnKeyEvent( struct SKeyEvent const & _Event, double _TimeStamp ) {

    if ( _Event.Action != IA_RELEASE ) {

        int key = 0;

        // OS X style: Shortcuts using Cmd/Super instead of Ctrl
        const bool bShortcutKey = (bOSX ? ( (_Event.ModMask & KMOD_MASK_SUPER) && !(_Event.ModMask & KMOD_MASK_CONTROL) )
                                        : ( (_Event.ModMask & KMOD_MASK_CONTROL) && !(_Event.ModMask & KMOD_MASK_SUPER) ) ) && !(_Event.ModMask & KMOD_MASK_ALT) && !(_Event.ModMask & KMOD_MASK_SHIFT);

        const bool bShiftShortcutOSX = bOSX && (_Event.ModMask & KMOD_MASK_SUPER) && (_Event.ModMask & KMOD_MASK_SHIFT) && !(_Event.ModMask & KMOD_MASK_CONTROL) && !(_Event.ModMask & KMOD_MASK_ALT);

        // OS X style: Text editing cursor movement using Alt instead of Ctrl
        const bool bWordmoveKeyDown = bOSX ? !!(_Event.ModMask & KMOD_MASK_ALT) : !!(_Event.ModMask & KMOD_MASK_CONTROL);

        // OS X style: Line/Text Start and End using Cmd+Arrows instead of Home/End
        const bool bStartEndKeyDown = bOSX && (_Event.ModMask & KMOD_MASK_SUPER) && !(_Event.ModMask & KMOD_MASK_CONTROL) && !(_Event.ModMask & KMOD_MASK_ALT);

        const int KeyMask = ( _Event.ModMask & KMOD_MASK_SHIFT ) ? STB_TEXTEDIT_K_SHIFT : 0;

        switch ( _Event.Key ) {
        case KEY_LEFT:
            if ( bStartEndKeyDown ) {
                key = STB_TEXTEDIT_K_LINESTART;
            } else if ( bWordmoveKeyDown ) {
                key = STB_TEXTEDIT_K_WORDLEFT;
            } else {
                key = STB_TEXTEDIT_K_LEFT;
            }
            key |= KeyMask;
            PressKey( key );
            ScrollToCursor();
            break;

        case KEY_RIGHT:
            if ( bStartEndKeyDown ) {
                key = STB_TEXTEDIT_K_LINEEND;
            } else if ( bWordmoveKeyDown ) {
                key = STB_TEXTEDIT_K_WORDRIGHT;
            } else {
                key = STB_TEXTEDIT_K_RIGHT;
            }
            key |= KeyMask;
            PressKey( key );
            ScrollToCursor();
            break;

        case KEY_UP:
            if ( !bSingleLine ) {
                if ( _Event.ModMask & KMOD_MASK_CONTROL ) {
                    ScrollLineUp();
                } else {
                    if ( bStartEndKeyDown ) {
                        key = STB_TEXTEDIT_K_TEXTSTART;
                    } else {
                        key = STB_TEXTEDIT_K_UP;
                    }
                    key |= KeyMask;
                    PressKey( key );
                    ScrollToCursor();
                }
            }
            break;

        case KEY_DOWN:
            if ( !bSingleLine ) {
                if ( _Event.ModMask & KMOD_MASK_CONTROL ) {
                    ScrollLineDown();
                } else {
                    if ( bStartEndKeyDown ) {
                        key = STB_TEXTEDIT_K_TEXTEND;
                    } else {
                        key = STB_TEXTEDIT_K_DOWN;
                    }
                    key |= KeyMask;
                    PressKey( key );
                    ScrollToCursor();
                }
            }

            break;

        case KEY_HOME:
            if ( _Event.ModMask & KMOD_MASK_CONTROL ) {
                key = STB_TEXTEDIT_K_TEXTSTART | KeyMask;

                ScrollHome();
            } else {
                key = STB_TEXTEDIT_K_LINESTART | KeyMask;

                ScrollLineStart();
            }
            PressKey( key );
            break;

        case KEY_END:
            if ( _Event.ModMask & KMOD_MASK_CONTROL ) {
                key = STB_TEXTEDIT_K_TEXTEND | KeyMask;

                ScrollEnd();

                PressKey( key );
            } else {
                key = STB_TEXTEDIT_K_LINEEND | KeyMask;

                PressKey( key );
                ScrollToCursor();

                //ScrollLineEnd();
            }

            break;

        case KEY_PAGE_UP:
            ScrollPageUp( true );
            break;

        case KEY_PAGE_DOWN:
            ScrollPageDown( true );
            break;

        case KEY_DELETE:
            if ( !bReadOnly ) {
                key = STB_TEXTEDIT_K_DELETE | KeyMask;
                PressKey( key );
            }
            break;

        case KEY_BACKSPACE:
            if ( !bReadOnly ) {
                if ( !HasSelection() ) {
                    if ( bWordmoveKeyDown ) {
                        PressKey( STB_TEXTEDIT_K_WORDLEFT | STB_TEXTEDIT_K_SHIFT );
                    } else if ( bOSX && ( _Event.ModMask & KMOD_MASK_SUPER ) && !( _Event.ModMask & KMOD_MASK_ALT ) && !( _Event.ModMask & KMOD_MASK_CONTROL ) ) {
                        PressKey( STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_SHIFT );
                    }
                }
                PressKey( STB_TEXTEDIT_K_BACKSPACE | KeyMask );
                ScrollToCursor();
            }
            break;

        case KEY_ENTER: {
            const bool bCtrl = !!(_Event.ModMask & KMOD_MASK_CONTROL);

            if ( bSingleLine || ( bCtrlEnterForNewLine && !bCtrl ) || ( !bCtrlEnterForNewLine && bCtrl ) ) {
                E_OnEnterPress.Dispatch( TextData.ToPtr() );
            } else if ( !bReadOnly ) {
                SWideChar ch = '\n';
                if ( FilterCharacter( ch ) ) {
                    PressKey( (int)ch );
                    ScrollToCursor();
                }
            }

            break;
            }

        case KEY_TAB: {
            bool bCtrl = !!(_Event.ModMask & KMOD_MASK_CONTROL);
            bool bShift = !!(_Event.ModMask & KMOD_MASK_SHIFT);
            bool bAlt = !!(_Event.ModMask & KMOD_MASK_ALT);

            if ( bAllowTabInput && !bReadOnly && !bCtrl && !bShift && !bAlt ) {
                if ( InsertSpacesOnTab > 0 ) {
                    SWideChar ch = ' ';
                    if ( FilterCharacter( ch ) ) {
                        for ( int i = 0 ; i < InsertSpacesOnTab ; i++ ) {
                            PressKey( ( int )ch );
                        }
                        ScrollToCursor();
                    }
                } else {
                    SWideChar ch = '\t';
                    if ( FilterCharacter( ch ) ) {
                        PressKey( ( int )ch );
                        ScrollToCursor();
                    }
                }
            }

            break;
            }

        case KEY_ESCAPE:
            E_OnEscapePress.Dispatch();
            break;

        case KEY_Z:

            if ( bAllowUndo && !bReadOnly ) {

                if ( bShortcutKey ) {
                    GLogger.Printf( "Undo\n");

                    PressKey( STB_TEXTEDIT_K_UNDO );

                    ClearSelection();
                    ScrollToCursor();

                } else if ( bShiftShortcutOSX
                            || ( _Event.ModMask & (KMOD_MASK_SHIFT|KMOD_MASK_CONTROL) ) == (KMOD_MASK_SHIFT|KMOD_MASK_CONTROL) ) {

                    GLogger.Printf( "Redo\n");

                    PressKey( STB_TEXTEDIT_K_REDO );

                    ClearSelection();
                    ScrollToCursor();
                }
            }
            break;

        case KEY_Y:

            if ( bAllowUndo && !bReadOnly ) {
                if ( bShortcutKey ) {
                    PressKey( STB_TEXTEDIT_K_REDO );

                    ClearSelection();
                    ScrollToCursor();
                }
            }

            break;

        case KEY_A:
            if ( bShortcutKey ) {
                SelectAll();
            }
            break;

        }

        const bool bCtrlOnly = ( _Event.ModMask & KMOD_MASK_CONTROL ) && !( _Event.ModMask & KMOD_MASK_SHIFT ) && !( _Event.ModMask & KMOD_MASK_ALT ) && !( _Event.ModMask & KMOD_MASK_SUPER );
        const bool bShiftOnly = ( _Event.ModMask & KMOD_MASK_SHIFT ) && !( _Event.ModMask & KMOD_MASK_CONTROL ) && !( _Event.ModMask & KMOD_MASK_ALT ) && !( _Event.ModMask & KMOD_MASK_SUPER );

        if ( ( bShortcutKey && _Event.Key == KEY_X ) || ( bShiftOnly && _Event.Key == KEY_DELETE ) ) {
            Cut();
            ScrollToCursor();
        } else if ( ( bShortcutKey && _Event.Key == KEY_C ) || ( bCtrlOnly  && _Event.Key == KEY_INSERT ) ) {
            Copy();
            ScrollToCursor();
        } else if ( ( bShortcutKey && _Event.Key == KEY_V ) || ( bShiftOnly && _Event.Key == KEY_INSERT ) ) {
            Paste();
            ScrollToCursor();
        }
    }
}

void WTextEdit::OnMouseButtonEvent( struct SMouseButtonEvent const & _Event, double _TimeStamp ) {
    if ( _Event.Action == IA_PRESS ) {
        Float2 CursorPos = GetDesktop()->GetCursorPosition();

        FromDesktopToWidget( CursorPos );

        if ( !HasSelection() ) {
            TempCursor = Stb->cursor;
        }

        if ( _Event.Button == 0 && ( _Event.ModMask & KMOD_MASK_SHIFT ) ) {

            stb_textedit_click( this, Stb, CursorPos.X, CursorPos.Y );

            Stb->select_start = TempCursor > CurTextLength ? Stb->cursor : TempCursor;
            Stb->select_end = Stb->cursor;

            if ( Stb->select_start > Stb->select_end ) {
                StdSwap( Stb->select_start, Stb->select_end );
            }

        } else {
            stb_textedit_click( this, Stb, CursorPos.X, CursorPos.Y );

            TempCursor = Stb->cursor;
        }
    }

    bStartDragging = ( _Event.Action == IA_PRESS ) && _Event.Button == 0;
}

void WTextEdit::OnDblClickEvent( int _ButtonKey, Float2 const & _ClickPos, uint64_t _ClickTime ) {
    if ( _ButtonKey == 0 ) {
        PressKey( STB_TEXTEDIT_K_WORDLEFT );
        PressKey( STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT );

        int s = GetSelectionStart();
        int e = GetSelectionEnd();

        while ( e-- > s ) {
            if ( !Core::WideCharIsBlank( TextData[e] ) ) {
                break;
            }
            PressKey( STB_TEXTEDIT_K_LEFT | STB_TEXTEDIT_K_SHIFT );
        }
    }
}

void WTextEdit::OnMouseWheelEvent( struct SMouseWheelEvent const & _Event, double _TimeStamp ) {
    if ( _Event.WheelY < 0 ) {
        ScrollLines( -2 );
    }
    else if ( _Event.WheelY > 0 ) {
        ScrollLines( 2 );
    }
}

void WTextEdit::OnMouseMoveEvent( struct SMouseMoveEvent const & _Event, double _TimeStamp ) {
    if ( bStartDragging ) {
        Float2 CursorPos = GetDesktop()->GetCursorPosition();

        FromDesktopToWidget( CursorPos );

        stb_textedit_drag( this, Stb, CursorPos.X, CursorPos.Y );

        ScrollToCursor();
    }
}

void WTextEdit::OnCharEvent( struct SCharEvent const & _Event, double _TimeStamp ) {
    if ( bReadOnly ) {
        return;
    }

    // We ignore CTRL inputs, but need to allow ALT+CTRL as some keyboards (e.g. German) use AltGR (which _is_ Alt+Ctrl) to input certain characters.
    if ( (_Event.ModMask & KMOD_MASK_CONTROL) && !(_Event.ModMask & KMOD_MASK_ALT) ) {
        return;
    }

    if ( bOSX && (_Event.ModMask & KMOD_MASK_SUPER) ) {
        return;
    }

    SWideChar ch = _Event.UnicodeCharacter;
    if ( !FilterCharacter( ch ) ) {
        return;
    }

    stb_textedit_key( this, Stb, (int)ch );

    ScrollToCursor();
}

void WTextEdit::OnFocusLost() {

}

void WTextEdit::OnFocusReceive() {
    
}

void WTextEdit::OnWindowHovered( bool _Hovered ) {
    if ( _Hovered ) {
        GetDesktop()->SetCursor( DRAW_CURSOR_TEXT_INPUT );
    } else {
        GetDesktop()->SetCursor( DRAW_CURSOR_ARROW );
    }
}

void WTextEdit::OnDrawEvent( ACanvas & _Canvas ) {

    //Stb->insert_mode = true;//GRuntime.GlobalInsertMode(); // TODO

    DrawDecorates( _Canvas );

    AFont const * font = GetFont();
    float fontSize = font->GetFontSize();

    Float2 pos = GetDesktopPosition();

    Float2 mins, maxs;
    GetDesktopRect( mins, maxs, false );

    //_Canvas.DrawRect( mins, maxs, AColor4::White() );

    if ( HasSelection() ) {
        int start = GetSelectionStart();
        int end = GetSelectionEnd();

        SWideChar const * seltext;
        Float2 selstart = CalcCursorOffset( font, TextData.ToPtr(), start, &seltext );
        const float lineHeight = fontSize;
        float lineWidth = 0.0f;
        SWideChar const * s = seltext;
        SWideChar const * s_end = TextData.ToPtr() + end;
        while ( s < s_end ) {
            SWideChar c = *s++;
            if ( c == '\n' ) {
                lineWidth = Math::Max( lineWidth, font->GetCharAdvance(' ') * 0.4f );
                _Canvas.DrawRectFilled( mins + selstart, mins + selstart + Float2( lineWidth, lineHeight ), SelectionColor );
                selstart.X = 0;
                selstart.Y += lineHeight;
                lineWidth = 0.0f;
                continue;
            }
            if ( c == '\r' ) {
                continue;
            }
            lineWidth += font->GetCharAdvance( c );
        }
        _Canvas.DrawRectFilled( mins + selstart, mins + selstart + Float2( lineWidth, lineHeight ), SelectionColor );
    }

    if ( IsFocus() ) {
        if ( ( GRuntime.SysFrameTimeStamp() >> 18 ) & 1 ) {
            Float2 cursor = mins + CalcCursorOffset( font, TextData.ToPtr(), Stb->cursor, nullptr );

            if ( Stb->insert_mode ) {
                float w = Stb->cursor < CurTextLength ? font->GetCharAdvance( TextData[Stb->cursor] ) : font->GetCharAdvance( ' ' );

                _Canvas.DrawRectFilled( cursor, Float2( cursor.X + w, cursor.Y + fontSize ), TextColor );
            } else {
                _Canvas.DrawLine( cursor, Float2( cursor.X, cursor.Y + fontSize ), TextColor );
            }
        }
    }

    _Canvas.PushFont( font );
    _Canvas.DrawTextWChar( fontSize, pos, TextColor, TextData.ToPtr(), TextData.ToPtr() + CurTextLength, 0.0F );
    _Canvas.PopFont();
}

void WTextEdit::UpdateWidgetSize() {
#if 1
    AFont const * font = GetFont();
    const float lineHeight = font->GetFontSize();
    Float2 size( 0.0f, lineHeight );
    float lineWidth = 0.0f;
    SWideChar const * s = TextData.ToPtr();
    SWideChar const * s_end = TextData.ToPtr() + CurTextLength;
    while ( s < s_end ) {
        SWideChar c = *s++;
        if ( c == '\n' ) {
            size.X = Math::Max( size.X, lineWidth );
            size.Y += lineHeight;
            lineWidth = 0.0f;
            continue;
        }
        if ( c == '\r' ) {
            continue;
        }
        lineWidth += font->GetCharAdvance( c );
    }
    size.X = Math::Max( size.X, lineWidth );
#else
    Float2 size = CalcTextRect( GetFont(), TextData.ToPtr(), TextData.ToPtr() + CurTextLength, nullptr, nullptr, false );
#endif
    WWidget * parent = GetParent();
    //if ( parent ) {
    //    float w = parent->GetAvailableWidth();
    //    float h = parent->GetAvailableHeight();
    //    if ( size.X < w-1 ) {
    //        size.X = w-1;
    //    }
    //    if ( size.Y < h-1 ) {
    //        size.Y = h-1;
    //    }
    //}
    SetSize( size );

    if ( parent ) {
        parent->MarkTransformDirty();
    }
}

bool WTextEdit::FilterCharacter( SWideChar & _Char ) {
    SWideChar c = _Char;

    if ( c < 128 && c != ' ' && !isprint( ( int )( c & 0xFF ) ) ) {
        if ( ( c != '\n' || bSingleLine ) && ( c != '\t' || !bAllowTabInput ) ) {
            return false;
        }
    }

    if ( c >= 0xE000 && c <= 0xF8FF ) {
        // Private Unicode range
        return false;
    }

    if ( CharacterFilter & ( CHARS_DECIMAL | CHARS_HEXADECIMAL | CHARS_UPPERCASE | CHARS_NO_BLANK | CHARS_SCIENTIFIC ) ) {
        if ( CharacterFilter & CHARS_DECIMAL )
            if ( !( c >= '0' && c <= '9' ) && ( c != '.' ) && ( c != '-' ) && ( c != '+' ) && ( c != '*' ) && ( c != '/' ) )
                return false;

        if ( CharacterFilter & CHARS_SCIENTIFIC )
            if ( !( c >= '0' && c <= '9' ) && ( c != '.' ) && ( c != '-' ) && ( c != '+' ) && ( c != '*' ) && ( c != '/' ) && ( c != 'e' ) && ( c != 'E' ) )
                return false;

        if ( CharacterFilter & CHARS_HEXADECIMAL )
            if ( !( c >= '0' && c <= '9' ) && !( c >= 'a' && c <= 'f' ) && !( c >= 'A' && c <= 'F' ) )
                return false;

        if ( CharacterFilter & CHARS_UPPERCASE ) {
            if ( c >= 'a' && c <= 'z' ) {
                c = c + 'A' - 'a';
                _Char = c;
            }
        }

        if ( CharacterFilter & CHARS_NO_BLANK )
            if ( Core::WideCharIsBlank( c ) )
                return false;
    }

    if ( bCustomCharFilter ) {
        if ( !OnFilterCharacter( c ) ) {
            return false;
        }
        _Char = c;
        if ( !c ) {
            return false;
        }
    }

    return true;
}
