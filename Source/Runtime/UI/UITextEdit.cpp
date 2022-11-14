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

#include "UITextEdit.h"
#include "UIScroll.h"

#include <Runtime/FrameLoop.h>
#include <Runtime/InputDefs.h>
#include <Runtime/ResourceManager.h>

#include <Platform/Platform.h>

static Float2 CalcTextRect(AFont const* font, FontStyle const& fontStyle, WideChar const* textBegin, WideChar const* textEnd, const WideChar** remaining, Float2* _OutOffset, bool _StopOnNewLine);

#undef STB_TEXTEDIT_STRING
#undef STB_TEXTEDIT_CHARTYPE
#define STB_TEXTEDIT_STRING           UITextEdit
#define STB_TEXTEDIT_CHARTYPE         WideChar
#define STB_TEXTEDIT_GETWIDTH_NEWLINE -1.0f
#define STB_TEXTEDIT_UNDOSTATECOUNT   99
#define STB_TEXTEDIT_UNDOCHARCOUNT    999

#define STB_TEXTEDIT_K_LEFT      0x10000 // keyboard input to move cursor left
#define STB_TEXTEDIT_K_RIGHT     0x10001 // keyboard input to move cursor right
#define STB_TEXTEDIT_K_UP        0x10002 // keyboard input to move cursor up
#define STB_TEXTEDIT_K_DOWN      0x10003 // keyboard input to move cursor down
#define STB_TEXTEDIT_K_LINESTART 0x10004 // keyboard input to move cursor to start of line
#define STB_TEXTEDIT_K_LINEEND   0x10005 // keyboard input to move cursor to end of line
#define STB_TEXTEDIT_K_TEXTSTART 0x10006 // keyboard input to move cursor to start of text
#define STB_TEXTEDIT_K_TEXTEND   0x10007 // keyboard input to move cursor to end of text
#define STB_TEXTEDIT_K_DELETE    0x10008 // keyboard input to delete selection or character under cursor
#define STB_TEXTEDIT_K_BACKSPACE 0x10009 // keyboard input to delete selection or character left of cursor
#define STB_TEXTEDIT_K_UNDO      0x1000A // keyboard input to perform undo
#define STB_TEXTEDIT_K_REDO      0x1000B // keyboard input to perform redo
#define STB_TEXTEDIT_K_WORDLEFT  0x1000C // keyboard input to move cursor left one word
#define STB_TEXTEDIT_K_WORDRIGHT 0x1000D // keyboard input to move cursor right one word
#define STB_TEXTEDIT_K_SHIFT     0x20000 // a power of two that is or'd in to a keyboard input to represent the shift key

#undef INCLUDE_STB_TEXTEDIT_H
#include "../stb/stb_textedit.h"

static WideChar STB_TEXTEDIT_NEWLINE = '\n';

#define STB_TEXTEDIT_STRINGLEN(_Obj) _Obj->GetTextLength()

#define STB_TEXTEDIT_GETCHAR(_Obj, i) _Obj->GetText()[i]

static int STB_TEXTEDIT_KEYTOTEXT(int key)
{
    return key >= 0x10000 ? 0 : key;
}

static void STB_TEXTEDIT_LAYOUTROW(StbTexteditRow* _Row, UITextEdit* _Obj, int _LineStartIndex)
{
    FontStyle fontStyle;
    fontStyle.FontSize = _Obj->GetFontSize();

    WideChar const* text           = _Obj->GetText();
    WideChar const* text_remaining = NULL;
    const Float2    size           = CalcTextRect(_Obj->GetFont(), fontStyle, text + _LineStartIndex, text + _Obj->GetTextLength(), &text_remaining, NULL, true);
    _Row->x0                       = 0.0f;
    _Row->x1                       = size.X;
    _Row->baseline_y_delta         = size.Y;
    _Row->ymin                     = 0.0f;
    _Row->ymax                     = size.Y;
    _Row->num_chars                = (int)(text_remaining - (text + _LineStartIndex));
}

static float STB_TEXTEDIT_GETWIDTH(UITextEdit* _Obj, int _LineStartIndex, int _CharIndex)
{
    WideChar c = _Obj->GetText()[_LineStartIndex + _CharIndex];
    if (c == '\n')
        return STB_TEXTEDIT_GETWIDTH_NEWLINE;

    FontStyle fontStyle;
    fontStyle.FontSize = _Obj->GetFontSize();

    return _Obj->GetFont()->GetCharAdvance(fontStyle, c); // *( FontSize / GetFont()->FontSize );
}

class UITextEditProxy
{
public:
    UITextEditProxy(UITextEdit* _Self) :
        Self(_Self) {}

    bool InsertCharsProxy(int offset, WideChar const* text, int textLength)
    {
        return Self->InsertCharsProxy(offset, text, textLength);
    }

    void DeleteCharsProxy(int first, int count)
    {
        return Self->DeleteCharsProxy(first, count);
    }

private:
    UITextEdit* Self;
};

#define STB_TEXTEDIT_DELETECHARS(_Obj, first, count) UITextEditProxy(_Obj).DeleteCharsProxy(first, count)

#define STB_TEXTEDIT_INSERTCHARS(_Obj, offset, text, textLength) UITextEditProxy(_Obj).InsertCharsProxy(offset, text, textLength)

HK_FORCEINLINE bool IsSeparator(WideChar c)
{
    return c == ',' || c == '.' || c == ';' || c == ':' || c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']' || c == '{' || c == '}' || c == '<' || c == '>' || c == '|' || c == '!' || c == '@' || c == '#' || c == '$' || c == '%' || c == '^' || c == '&' || c == '*' || c == '/' || c == '\\' || c == '+' || c == '=' || c == '-' || c == '~' || c == '`' || c == '\'' || c == '"' || c == '?' || c == '\n';
}

static bool IsWordBoundary(WideChar* s)
{
    if (Core::WideCharIsBlank(s[-1]) && !Core::WideCharIsBlank(s[0]))
    {
        return true;
    }

    if (s[-1] == '\n')
    {
        return true;
    }

    if (!Core::WideCharIsBlank(s[0]))
    {
        if ((IsSeparator(s[-1]) || IsSeparator(s[0])) && (s[-1] != s[0]))
        {
            return true;
        }
    }

    return false;
}

static int NextWord(UITextEdit* _Obj, int i)
{
    i++;
    int       len = _Obj->GetTextLength();
    WideChar* s   = _Obj->GetText() + i;
    while (i < len && !IsWordBoundary(s))
    {
        i++;
        s++;
    }
    return i > len ? len : i;
}

static int PrevWord(UITextEdit* _Obj, int i)
{
    i--;
    WideChar* s = _Obj->GetText() + i;
    while (i > 0 && !IsWordBoundary(s))
    {
        i--;
        s--;
    }
    return i < 0 ? 0 : i;
}

#define STB_TEXTEDIT_MOVEWORDRIGHT NextWord
#define STB_TEXTEDIT_MOVEWORDLEFT  PrevWord

#define STB_TEXTEDIT_IMPLEMENTATION
#include "../stb/stb_textedit.h"

static const bool bOSX = false;

enum FCharacterFilter
{
    CHARS_DECIMAL     = HK_BIT(0), // 0123456789.+-*/
    CHARS_HEXADECIMAL = HK_BIT(1), // 0123456789ABCDEFabcdef
    CHARS_UPPERCASE   = HK_BIT(2), // a..z -> A..Z
    CHARS_NO_BLANK    = HK_BIT(3), // filter out spaces, tabs
    CHARS_SCIENTIFIC  = HK_BIT(4), // 0123456789.+-*/eE (Scientific notation input)
};

static Float2 CalcTextRect(AFont const* font, FontStyle const& fontStyle, WideChar const* textBegin, WideChar const* textEnd, const WideChar** remaining, Float2* _OutOffset, bool _StopOnNewLine)
{
    TextMetrics metrics;
    font->GetTextMetrics(fontStyle, metrics);

    const float lineHeight = metrics.LineHeight;
    Float2      rectSize(0);
    float       lineWidth = 0.0f;

    WideChar const* s = textBegin;
    while (s < textEnd)
    {
        WideChar c = *s++;
        if (c == '\n')
        {
            rectSize.X = Math::Max(rectSize.X, lineWidth);
            rectSize.Y += lineHeight;
            lineWidth = 0.0f;
            if (_StopOnNewLine)
            {
                break;
            }
            continue;
        }
        if (c == '\r')
        {
            continue;
        }
        lineWidth += font->GetCharAdvance(fontStyle, c);
    }

    if (rectSize.X < lineWidth)
    {
        rectSize.X = lineWidth;
    }

    if (_OutOffset)
    {
        *_OutOffset = Float2(lineWidth, rectSize.Y + lineHeight);
    }

    if (lineWidth > 0 || rectSize.Y == 0.0f)
    {
        rectSize.Y += lineHeight;
    }

    if (remaining)
    {
        *remaining = s;
    }

    return rectSize;
}

// Returns top-left cursor position
static Float2 CalcCursorOffset(AFont const* font, FontStyle const& fontStyle, WideChar* text, int cursor, const WideChar** remaining)
{
    TextMetrics metrics;
    font->GetTextMetrics(fontStyle, metrics);

    const float lineHeight = metrics.LineHeight;
    Float2      offset(0);
    float       lineWidth = 0.0f;

    WideChar const* s   = text;
    WideChar const* end = text + cursor;
    while (s < end)
    {
        WideChar c = *s++;
        if (c == '\n')
        {
            offset.Y += lineHeight;
            lineWidth = 0.0f;
            continue;
        }
        if (c == '\r')
        {
            continue;
        }
        lineWidth += font->GetCharAdvance(fontStyle, c);
    }
    offset.X = lineWidth;
    if (remaining)
    {
        *remaining = s;
    }
    return offset;
}

UITextEdit::UITextEdit()
{
    m_bSingleLine = false;

    m_Stb = (STB_TexteditState*)Platform::GetHeapAllocator<HEAP_MISC>().Alloc(sizeof(STB_TexteditState));
    stb_textedit_initialize_state(m_Stb, m_bSingleLine);

    m_FontStyle.FontSize = 14;

    bShortcutsAllowed      = false;
    m_bReadOnly            = false;
    m_bPassword            = false;
    m_bCtrlEnterForNewLine = false;
    m_bAllowTabInput       = true;
    m_bAllowUndo           = true;
    m_bCustomCharFilter    = false;
    m_bStartDragging       = false;
    m_bShouldKeepSelection = false;
    m_CurTextLength        = 0;
    m_MaxChars             = 0;
    m_CharacterFilter      = 0;
    m_InsertSpacesOnTab    = 4;
    m_TempCursor           = 0;
    m_SelectionColor       = Color4(0.32f, 0.32f, 0.9f);
    m_TextColor            = Color4(0.9f, 0.9f, 0.9f);

    Cursor = GUIManager->TextInputCursor();
}

UITextEdit::~UITextEdit()
{
    Platform::GetHeapAllocator<HEAP_MISC>().Free(m_Stb);
}

UITextEdit& UITextEdit::WithFont(AFont* font)
{
    m_Font = font;
    return *this;
}

UITextEdit& UITextEdit::WithFontSize(float size)
{
    m_FontStyle.FontSize = size;
    return *this;
}

UITextEdit& UITextEdit::WithMaxChars(int maxChars)
{
    m_MaxChars = maxChars;
    return *this;
}

UITextEdit& UITextEdit::WithFilterDecimal(bool bEnabled)
{
    if (bEnabled)
    {
        m_CharacterFilter |= CHARS_DECIMAL;
    }
    else
    {
        m_CharacterFilter &= ~CHARS_DECIMAL;
    }
    return *this;
}

UITextEdit& UITextEdit::WithFilterHexadecimal(bool bEnabled)
{
    if (bEnabled)
    {
        m_CharacterFilter |= CHARS_HEXADECIMAL;
    }
    else
    {
        m_CharacterFilter &= ~CHARS_HEXADECIMAL;
    }
    return *this;
}

UITextEdit& UITextEdit::WithFilterUppercase(bool bEnabled)
{
    if (bEnabled)
    {
        m_CharacterFilter |= CHARS_UPPERCASE;
    }
    else
    {
        m_CharacterFilter &= ~CHARS_UPPERCASE;
    }
    return *this;
}

UITextEdit& UITextEdit::WithFilterNoBlank(bool bEnabled)
{
    if (bEnabled)
    {
        m_CharacterFilter |= CHARS_NO_BLANK;
    }
    else
    {
        m_CharacterFilter &= ~CHARS_NO_BLANK;
    }
    return *this;
}

UITextEdit& UITextEdit::WithFilterScientific(bool bEnabled)
{
    if (bEnabled)
    {
        m_CharacterFilter |= CHARS_SCIENTIFIC;
    }
    else
    {
        m_CharacterFilter &= ~CHARS_SCIENTIFIC;
    }
    return *this;
}

UITextEdit& UITextEdit::WithFilterCustomCallback(bool bEnabled)
{
    m_bCustomCharFilter = bEnabled;
    return *this;
}

UITextEdit& UITextEdit::WithInsertSpacesOnTab(int numSpaces)
{
    m_InsertSpacesOnTab = numSpaces;
    return *this;
}

UITextEdit& UITextEdit::WithSingleLine(bool bEnabled)
{
    m_bSingleLine = bEnabled;
    stb_textedit_initialize_state(m_Stb, m_bSingleLine);
    return *this;
}

UITextEdit& UITextEdit::WithReadOnly(bool bEnabled)
{
    m_bReadOnly = bEnabled;
    return *this;
}

UITextEdit& UITextEdit::WithPassword(bool bEnabled)
{
    m_bPassword = bEnabled;
    return *this;
}

UITextEdit& UITextEdit::WithCtrlEnterForNewLine(bool bEnabled)
{
    m_bCtrlEnterForNewLine = bEnabled;
    return *this;
}

UITextEdit& UITextEdit::WithAllowTabInput(bool bEnabled)
{
    m_bAllowTabInput = bEnabled;
    return *this;
}

UITextEdit& UITextEdit::WithAllowUndo(bool bEnabled)
{
    m_bAllowUndo = bEnabled;
    return *this;
}

UITextEdit& UITextEdit::WithSelectionColor(Color4 const& color)
{
    m_SelectionColor = color;
    return *this;
}

UITextEdit& UITextEdit::WithTextColor(Color4 const& color)
{
    m_TextColor = color;
    return *this;
}

UITextEdit& UITextEdit::ShouldKeepSelection(bool bShouldKeepSelection)
{
    m_bShouldKeepSelection = bShouldKeepSelection;
    return *this;
}

AFont* UITextEdit::GetFont() const
{
    return m_Font ? m_Font : ACanvas::GetDefaultFont();
}

float UITextEdit::GetFontSize() const
{
    return m_FontStyle.FontSize;
}

int UITextEdit::GetTextLength() const
{
    return m_CurTextLength;
}

int UITextEdit::GetCursorPosition() const
{
    return m_Stb->cursor;
}

int UITextEdit::GetSelectionStart() const
{
    return Math::Min(m_Stb->select_start, m_Stb->select_end);
}

int UITextEdit::GetSelectionEnd() const
{
    return Math::Max(m_Stb->select_start, m_Stb->select_end);
}

bool UITextEdit::InsertCharsProxy(int offset, WideChar const* _text, int textLength)
{
    if (offset > m_CurTextLength)
    {
        return false;
    }

    bool bResizable = m_MaxChars > 0;
    if (bResizable && m_CurTextLength + textLength > m_MaxChars)
    {
        textLength = m_MaxChars - m_CurTextLength;
        if (textLength <= 0)
        {
            return false;
        }
    }

    if (m_CurTextLength + textLength + 1 > m_TextData.Size())
    {
        m_TextData.Resize(m_CurTextLength + textLength + 1);
    }

    WideChar* text = m_TextData.ToPtr();
    if (offset != m_CurTextLength)
    {
        Platform::Memmove(text + offset + textLength, text + offset, (size_t)(m_CurTextLength - offset) * sizeof(WideChar));
    }
    Platform::Memcpy(text + offset, _text, (size_t)textLength * sizeof(WideChar));

    m_CurTextLength += textLength;
    m_TextData[m_CurTextLength] = '\0';

    UpdateWidgetSize();

    E_OnTyping.Dispatch(m_TextData.ToPtr());

    return true;
}

void UITextEdit::DeleteCharsProxy(int first, int count)
{
    if (count < 0 || first < 0)
    {
        return;
    }

    if (first >= m_CurTextLength)
    {
        return;
    }

    if (first + count > m_CurTextLength)
    {
        count = m_CurTextLength - first - 1;
    }

    m_CurTextLength -= count;

    Platform::Memmove(&m_TextData[first], &m_TextData[first + count], (m_CurTextLength - first) * sizeof(WideChar));
    m_TextData[m_CurTextLength] = '\0';

    UpdateWidgetSize();

    E_OnTyping.Dispatch(m_TextData.ToPtr());
}

void UITextEdit::PressKey(int key)
{
    if (key)
    {
        stb_textedit_key(this, m_Stb, key);
    }
}

void UITextEdit::ClearSelection()
{
    m_Stb->select_start = m_Stb->select_end = m_Stb->cursor;
}

void UITextEdit::SelectAll()
{
    m_Stb->select_start = 0;
    m_Stb->cursor = m_Stb->select_end = m_CurTextLength;
    m_Stb->has_preferred_x            = 0;
}

bool UITextEdit::HasSelection() const
{
    return m_Stb->select_start != m_Stb->select_end;
}

UIScroll* UITextEdit::GetScroll()
{
    return dynamic_cast<UIScroll*>(Parent.GetObject());
}

void UITextEdit::ScrollHome()
{
    if (m_bSingleLine)
    {
        return;
    }

    UIScroll* scroll = GetScroll();
    if (scroll)
    {
        scroll->ScrollHome();
    }
}

void UITextEdit::ScrollEnd()
{
    if (m_bSingleLine)
    {
        return;
    }

    UIScroll* scroll = GetScroll();
    if (scroll)
    {
        scroll->ScrollEnd();
    }
}

void UITextEdit::ScrollPageUp(bool bMoveCursor)
{
    if (m_bSingleLine)
    {
        return;
    }

    UIScroll* scroll = GetScroll();
    if (scroll)
    {
        TextMetrics metrics;
        GetFont()->GetTextMetrics(m_FontStyle, metrics);

        const float lineHeight = metrics.LineHeight;

        float PageSize = scroll->GetViewSize().Y;
        PageSize       = Math::Snap(PageSize, lineHeight);

        int numLines = (int)(PageSize / lineHeight);

        if (bMoveCursor)
        {
            for (int i = 0; i < numLines; i++)
            {
                PressKey(STB_TEXTEDIT_K_UP);
            }
        }

        ScrollLines(numLines);
    }
}

void UITextEdit::ScrollPageDown(bool bMoveCursor)
{
    if (m_bSingleLine)
    {
        return;
    }

    UIScroll* scroll = GetScroll();
    if (scroll)
    {
        TextMetrics metrics;
        GetFont()->GetTextMetrics(m_FontStyle, metrics);

        const float lineHeight = metrics.LineHeight;

        float PageSize = scroll->GetViewSize().Y;
        PageSize       = Math::Snap(PageSize, lineHeight);

        int numLines = (int)(PageSize / lineHeight);

        if (bMoveCursor)
        {
            for (int i = 0; i < numLines; i++)
            {
                PressKey(STB_TEXTEDIT_K_DOWN);
            }
        }

        ScrollLines(-numLines);
    }
}

void UITextEdit::ScrollLineUp()
{
    ScrollLines(1);
}

void UITextEdit::ScrollLineDown()
{
    ScrollLines(-1);
}

void UITextEdit::ScrollLines(int numLines)
{
    if (m_bSingleLine)
    {
        return;
    }

    UIScroll* scroll = GetScroll();
    if (scroll)
    {
        Float2 scrollPosition = scroll->GetScrollPosition();

        TextMetrics metrics;
        GetFont()->GetTextMetrics(m_FontStyle, metrics);

        const float lineHeight = metrics.LineHeight;

        scrollPosition.Y = Math::Snap(scrollPosition.Y, lineHeight);
        scrollPosition.Y -= numLines * lineHeight;

        Float2 delta = scroll->GetScrollPosition() - scrollPosition;
        scroll->ScrollDelta(delta);
    }
}

void UITextEdit::ScrollLineStart()
{
    UIScroll* scroll = GetScroll();
    if (scroll)
    {
        Float2 scrollPosition = scroll->GetScrollPosition();

        scrollPosition.X = 0;

        scroll->SetScrollPosition(scrollPosition);
    }
}

bool UITextEdit::FindLineStartEnd(int cursor, WideChar** _LineStart, WideChar** _LineEnd)
{
    if (cursor < 0 || cursor >= m_CurTextLength)
    {
        return false;
    }

    WideChar* text      = m_TextData.ToPtr();
    WideChar* textEnd   = m_TextData.ToPtr() + m_CurTextLength;
    WideChar* lineStart = text + m_Stb->cursor;
    WideChar* lineEnd   = text + m_Stb->cursor + 1;

    if (*lineStart != '\n')
    {
        while (lineEnd < textEnd)
        {
            if (*lineEnd == '\n')
            {
                break;
            }
            lineEnd++;
        }
    }
    else
    {
        lineEnd = lineStart;
        lineStart--;
    }
    while (lineStart >= text)
    {
        if (*lineStart == '\n')
        {
            break;
        }
        lineStart--;
    }
    lineStart++;

    *_LineStart = lineStart;
    *_LineEnd   = lineEnd;

    return true;
}

void UITextEdit::ScrollLineEnd()
{
    UIScroll* scroll = GetScroll();
    if (!scroll)
    {
        return;
    }

    AFont const* font = GetFont();
    WideChar*   lineStart;
    WideChar*   lineEnd;

    if (FindLineStartEnd(m_Stb->cursor, &lineStart, &lineEnd))
    {
        float lineWidth = 0;
        for (WideChar* s = lineStart; s < lineEnd; s++)
        {
            lineWidth += font->GetCharAdvance(m_FontStyle, *s);
        }

        float  PageWidth      = scroll->GetViewSize().X;
        Float2 scrollPosition = scroll->GetScrollPosition();
        scrollPosition.X      = -lineWidth + PageWidth * 0.5f;
        scroll->SetScrollPosition(scrollPosition);
    }
}

void UITextEdit::ScrollHorizontal(float delta)
{
    UIScroll* scroll = GetScroll();
    if (scroll)
    {
        scroll->ScrollDelta(Float2(delta, 0.0f));
    }
}

void UITextEdit::ScrollToCursor()
{
    UIScroll* scroll = GetScroll();
    if (!scroll)
    {
        return;
    }

    AFont const* font = GetFont();

    Float2 scrollMins = scroll->Geometry.PaddedMins;
    Float2 scrollMaxs = scroll->Geometry.PaddedMaxs;
    Float2 pageSize   = scrollMaxs - scrollMins;

    TextMetrics metrics;
    font->GetTextMetrics(m_FontStyle, metrics);

    Float2 cursorOffset = CalcCursorOffset(font, m_FontStyle, m_TextData.ToPtr(), m_Stb->cursor, nullptr);

    // Cursor global position
    Float2 cursor = Geometry.Mins + cursorOffset;

    Float2 scrollPosition = scroll->GetScrollPosition();

    if (cursor.X < scrollMins.X)
    {
        scrollPosition.X = -cursorOffset.X + pageSize.X * 0.5f;
    }
    else if (cursor.X > scrollMaxs.X)
    {
        scrollPosition.X = -cursorOffset.X + pageSize.X * 0.5f;
        scrollPosition.X = Math::Max(scrollPosition.X, -m_CurSize.X + pageSize.X);
    }

    if (cursor.Y < scrollMins.Y)
    {
        scrollPosition.Y = -cursorOffset.Y;
    }
    else if (cursor.Y + metrics.LineHeight > scrollMaxs.Y)
    {
        float newY = scrollMaxs.Y - metrics.LineHeight;
        float delta = newY - cursor.Y;

        scrollPosition.Y += delta;
    }

    scroll->SetScrollPosition(scrollPosition);
}

bool UITextEdit::Cut()
{
    if (m_bReadOnly)
    {
        // Can't modify readonly text
        return false;
    }

    if (!Copy())
    {
        return false;
    }

    if (!HasSelection())
    {
        SelectAll();
    }
    stb_textedit_cut(this, m_Stb);

    return true;
}

bool UITextEdit::Copy()
{
    if (m_bPassword)
    {
        // Can't copy password
        return false;
    }

    bool bHasSelection = HasSelection();

    if (!m_bSingleLine && !bHasSelection)
    {
        // Can't copy multiline text if no selection
        return false;
    }

    const int       startOfs = bHasSelection ? GetSelectionStart() : 0;
    const int       endOfs   = bHasSelection ? GetSelectionEnd() : m_CurTextLength;
    WideChar* const start    = m_TextData.ToPtr() + startOfs;
    WideChar* const end      = m_TextData.ToPtr() + endOfs;

    Platform::SetClipboard(Core::GetString(AWideStringView(start, end)).CStr());

    return true;
}

bool UITextEdit::Paste()
{
    if (m_bReadOnly)
    {
        // Can't modify readonly text
        return false;
    }

    const char* s = Platform::GetClipboard();

    int len = Core::UTF8StrLength(s);

    TPodVector<WideChar> wideStr;
    wideStr.Resize(len);

    WideChar ch;
    int      i, byteLen;
    i = 0;
    while (len-- > 0)
    {
        byteLen = Core::WideCharDecodeUTF8(s, ch);
        if (!byteLen)
        {
            break;
        }
        s += byteLen;
        if (!FilterCharacter(ch))
        {
            continue;
        }
        wideStr[i++] = ch;
    }
    if (i > 0)
    {
        stb_textedit_paste(this, m_Stb, wideStr.ToPtr(), i);
    }

    return true;
}

UITextEdit& UITextEdit::WithText(const char* text)
{
    int len = Core::UTF8StrLength(text);

    TPodVector<WideChar> wideStr;
    wideStr.Resize(len + 1);

    WideChar ch;
    int      i, byteLen;
    i = 0;
    while (len-- > 0)
    {
        byteLen = Core::WideCharDecodeUTF8(text, ch);
        if (!byteLen)
        {
            break;
        }
        text += byteLen;
        if (!FilterCharacter(ch))
        {
            continue;
        }
        wideStr[i++] = ch;
    }

    wideStr[i] = 0;

    return WithText(wideStr.ToPtr());
}

UITextEdit& UITextEdit::WithText(const WideChar* text)
{
    int len = Core::WideStrLength(text);

    SelectAll();

    stb_textedit_paste(this, m_Stb, text, len);

    return *this;
}

void UITextEdit::OnKeyEvent(struct SKeyEvent const& event, double timeStamp)
{
    if (event.Action != IA_RELEASE)
    {
        int key = 0;

        // OS X style: Shortcuts using Cmd/Super instead of Ctrl
        const bool bShortcutKey = (bOSX ? ((event.ModMask & MOD_MASK_SUPER) && !(event.ModMask & MOD_MASK_CONTROL)) : ((event.ModMask & MOD_MASK_CONTROL) && !(event.ModMask & MOD_MASK_SUPER))) && !(event.ModMask & MOD_MASK_ALT) && !(event.ModMask & MOD_MASK_SHIFT);

        const bool bShiftShortcutOSX = bOSX && (event.ModMask & MOD_MASK_SUPER) && (event.ModMask & MOD_MASK_SHIFT) && !(event.ModMask & MOD_MASK_CONTROL) && !(event.ModMask & MOD_MASK_ALT);

        // OS X style: Text editing cursor movement using Alt instead of Ctrl
        const bool bWordmoveKeyDown = bOSX ? !!(event.ModMask & MOD_MASK_ALT) : !!(event.ModMask & MOD_MASK_CONTROL);

        // OS X style: Line/Text Start and End using Cmd+Arrows instead of Home/End
        const bool bStartEndKeyDown = bOSX && (event.ModMask & MOD_MASK_SUPER) && !(event.ModMask & MOD_MASK_CONTROL) && !(event.ModMask & MOD_MASK_ALT);

        const int KeyMask = (event.ModMask & MOD_MASK_SHIFT) ? STB_TEXTEDIT_K_SHIFT : 0;

        switch (event.Key)
        {
            case KEY_LEFT:
                if (bStartEndKeyDown)
                {
                    key = STB_TEXTEDIT_K_LINESTART;
                }
                else if (bWordmoveKeyDown)
                {
                    key = STB_TEXTEDIT_K_WORDLEFT;
                }
                else
                {
                    key = STB_TEXTEDIT_K_LEFT;
                }
                key |= KeyMask;
                PressKey(key);
                ScrollToCursor();
                break;

            case KEY_RIGHT:
                if (bStartEndKeyDown)
                {
                    key = STB_TEXTEDIT_K_LINEEND;
                }
                else if (bWordmoveKeyDown)
                {
                    key = STB_TEXTEDIT_K_WORDRIGHT;
                }
                else
                {
                    key = STB_TEXTEDIT_K_RIGHT;
                }
                key |= KeyMask;
                PressKey(key);
                ScrollToCursor();
                break;

            case KEY_UP:
                if (!m_bSingleLine)
                {
                    if (event.ModMask & MOD_MASK_CONTROL)
                    {
                        ScrollLineUp();
                    }
                    else
                    {
                        if (bStartEndKeyDown)
                        {
                            key = STB_TEXTEDIT_K_TEXTSTART;
                        }
                        else
                        {
                            key = STB_TEXTEDIT_K_UP;
                        }
                        key |= KeyMask;
                        PressKey(key);
                        ScrollToCursor();
                    }
                }
                break;

            case KEY_DOWN:
                if (!m_bSingleLine)
                {
                    if (event.ModMask & MOD_MASK_CONTROL)
                    {
                        ScrollLineDown();
                    }
                    else
                    {
                        if (bStartEndKeyDown)
                        {
                            key = STB_TEXTEDIT_K_TEXTEND;
                        }
                        else
                        {
                            key = STB_TEXTEDIT_K_DOWN;
                        }
                        key |= KeyMask;
                        PressKey(key);
                        ScrollToCursor();
                    }
                }

                break;

            case KEY_HOME:
                if (event.ModMask & MOD_MASK_CONTROL)
                {
                    key = STB_TEXTEDIT_K_TEXTSTART | KeyMask;

                    ScrollHome();
                }
                else
                {
                    key = STB_TEXTEDIT_K_LINESTART | KeyMask;

                    ScrollLineStart();
                }
                PressKey(key);
                break;

            case KEY_END:
                if (event.ModMask & MOD_MASK_CONTROL)
                {
                    key = STB_TEXTEDIT_K_TEXTEND | KeyMask;

                    ScrollEnd();

                    PressKey(key);
                }
                else
                {
                    key = STB_TEXTEDIT_K_LINEEND | KeyMask;

                    PressKey(key);
                    ScrollToCursor();

                    //ScrollLineEnd();
                }

                break;

            case KEY_PAGE_UP:
                ScrollPageUp(true);
                break;

            case KEY_PAGE_DOWN:
                ScrollPageDown(true);
                break;

            case KEY_DELETE:
                if (!m_bReadOnly)
                {
                    key = STB_TEXTEDIT_K_DELETE | KeyMask;
                    PressKey(key);
                }
                break;

            case KEY_BACKSPACE:
                if (!m_bReadOnly)
                {
                    if (!HasSelection())
                    {
                        if (bWordmoveKeyDown)
                        {
                            PressKey(STB_TEXTEDIT_K_WORDLEFT | STB_TEXTEDIT_K_SHIFT);
                        }
                        else if (bOSX && (event.ModMask & MOD_MASK_SUPER) && !(event.ModMask & MOD_MASK_ALT) && !(event.ModMask & MOD_MASK_CONTROL))
                        {
                            PressKey(STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_SHIFT);
                        }
                    }
                    PressKey(STB_TEXTEDIT_K_BACKSPACE | KeyMask);
                    ScrollToCursor();
                }
                break;

            case KEY_ENTER: {
                const bool bCtrl = !!(event.ModMask & MOD_MASK_CONTROL);

                if (m_bSingleLine || (m_bCtrlEnterForNewLine && !bCtrl) || (!m_bCtrlEnterForNewLine && bCtrl))
                {
                    E_OnEnterPress.Dispatch(m_TextData.ToPtr());
                }
                else if (!m_bReadOnly)
                {
                    WideChar ch = '\n';
                    if (FilterCharacter(ch))
                    {
                        PressKey((int)ch);
                        ScrollToCursor();
                    }
                }

                break;
            }

            case KEY_TAB: {
                bool bCtrl  = !!(event.ModMask & MOD_MASK_CONTROL);
                bool bShift = !!(event.ModMask & MOD_MASK_SHIFT);
                bool bAlt   = !!(event.ModMask & MOD_MASK_ALT);

                if (m_bAllowTabInput && !m_bReadOnly && !bCtrl && !bShift && !bAlt)
                {
                    if (m_InsertSpacesOnTab > 0)
                    {
                        WideChar ch = ' ';
                        if (FilterCharacter(ch))
                        {
                            for (int i = 0; i < m_InsertSpacesOnTab; i++)
                            {
                                PressKey((int)ch);
                            }
                            ScrollToCursor();
                        }
                    }
                    else
                    {
                        WideChar ch = '\t';
                        if (FilterCharacter(ch))
                        {
                            PressKey((int)ch);
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
                if (m_bAllowUndo && !m_bReadOnly)
                {
                    if (bShortcutKey)
                    {
                        PressKey(STB_TEXTEDIT_K_UNDO);

                        ClearSelection();
                        ScrollToCursor();
                    }
                    else if (bShiftShortcutOSX || (event.ModMask & (MOD_MASK_SHIFT | MOD_MASK_CONTROL)) == (MOD_MASK_SHIFT | MOD_MASK_CONTROL))
                    {
                        PressKey(STB_TEXTEDIT_K_REDO);

                        ClearSelection();
                        ScrollToCursor();
                    }
                }
                break;

            case KEY_Y:

                if (m_bAllowUndo && !m_bReadOnly)
                {
                    if (bShortcutKey)
                    {
                        PressKey(STB_TEXTEDIT_K_REDO);

                        ClearSelection();
                        ScrollToCursor();
                    }
                }

                break;

            case KEY_A:
                if (bShortcutKey)
                {
                    SelectAll();
                }
                break;

            case KEY_INSERT:
                if (event.ModMask == 0)
                {
                    GUIManager->SetInsertMode(!GUIManager->IsInsertMode());
                }
                break;
        }

        const bool bCtrlOnly  = (event.ModMask & MOD_MASK_CONTROL) && !(event.ModMask & MOD_MASK_SHIFT) && !(event.ModMask & MOD_MASK_ALT) && !(event.ModMask & MOD_MASK_SUPER);
        const bool bShiftOnly = (event.ModMask & MOD_MASK_SHIFT) && !(event.ModMask & MOD_MASK_CONTROL) && !(event.ModMask & MOD_MASK_ALT) && !(event.ModMask & MOD_MASK_SUPER);

        if ((bShortcutKey && event.Key == KEY_X) || (bShiftOnly && event.Key == KEY_DELETE))
        {
            Cut();
            ScrollToCursor();
        }
        else if ((bShortcutKey && event.Key == KEY_C) || (bCtrlOnly && event.Key == KEY_INSERT))
        {
            Copy();
            ScrollToCursor();
        }
        else if ((bShortcutKey && event.Key == KEY_V) || (bShiftOnly && event.Key == KEY_INSERT))
        {
            Paste();
            ScrollToCursor();
        }
    }
}

void UITextEdit::OnMouseButtonEvent(struct SMouseButtonEvent const& event, double timeStamp)
{
    if (event.Button != MOUSE_BUTTON_1 && event.Button != MOUSE_BUTTON_2)
    {
        return;
    }

    if (event.Action == IA_PRESS)
    {
        Float2 CursorPos = GUIManager->CursorPosition;

        CursorPos -= Geometry.Mins;

        if (!HasSelection())
        {
            m_TempCursor = m_Stb->cursor;
        }

        if (event.Button == MOUSE_BUTTON_1 && (event.ModMask & MOD_MASK_SHIFT))
        {
            stb_textedit_click(this, m_Stb, CursorPos.X, CursorPos.Y);

            m_Stb->select_start = m_TempCursor > m_CurTextLength ? m_Stb->cursor : m_TempCursor;
            m_Stb->select_end   = m_Stb->cursor;

            if (m_Stb->select_start > m_Stb->select_end)
            {
                std::swap(m_Stb->select_start, m_Stb->select_end);
            }
        }
        else
        {
            stb_textedit_click(this, m_Stb, CursorPos.X, CursorPos.Y);

            m_TempCursor = m_Stb->cursor;
        }
    }

    m_bStartDragging = (event.Action == IA_PRESS) && event.Button == MOUSE_BUTTON_1;
}

void UITextEdit::OnDblClickEvent(int buttonKey, Float2 const& clickPos, uint64_t clickTime)
{
    if (buttonKey == 0)
    {
        PressKey(STB_TEXTEDIT_K_WORDLEFT);
        PressKey(STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT);

        int s = GetSelectionStart();
        int e = GetSelectionEnd();

        while (e-- > s)
        {
            if (!Core::WideCharIsBlank(m_TextData[e]))
            {
                break;
            }
            PressKey(STB_TEXTEDIT_K_LEFT | STB_TEXTEDIT_K_SHIFT);
        }
    }
}

void UITextEdit::OnMouseWheelEvent(struct SMouseWheelEvent const& event, double timeStamp)
{
    if (m_bSingleLine)
    {
        Super::OnMouseWheelEvent(event, timeStamp);
        return;
    }

    if (event.WheelY < 0)
    {
        ScrollLines(-2);
    }
    else if (event.WheelY > 0)
    {
        ScrollLines(2);
    }
}

void UITextEdit::OnMouseMoveEvent(struct SMouseMoveEvent const& event, double timeStamp)
{
    if (m_bStartDragging)
    {
        Float2 CursorPos = GUIManager->CursorPosition;

        CursorPos -= Geometry.Mins;

        stb_textedit_drag(this, m_Stb, CursorPos.X, CursorPos.Y);

        ScrollToCursor();
    }
}

void UITextEdit::OnCharEvent(struct SCharEvent const& event, double timeStamp)
{
    if (m_bReadOnly)
    {
        return;
    }

    // We ignore CTRL inputs, but need to allow ALT+CTRL as some keyboards (e.g. German) use AltGR (which _is_ Alt+Ctrl) to input certain characters.
    if ((event.ModMask & MOD_MASK_CONTROL) && !(event.ModMask & MOD_MASK_ALT))
    {
        return;
    }

    if (bOSX && (event.ModMask & MOD_MASK_SUPER))
    {
        return;
    }

    WideChar ch = event.UnicodeCharacter;
    if (!FilterCharacter(ch))
    {
        return;
    }

    stb_textedit_key(this, m_Stb, (int)ch);

    ScrollToCursor();
}

void UITextEdit::OnFocusLost()
{
    if (!m_bShouldKeepSelection)
    {
        ClearSelection();
    }
}

void UITextEdit::AdjustSize(Float2 const& size)
{
    Super::AdjustSize(size);

    if (bAutoWidth)
        AdjustedSize.X = Math::Max(Size.X, m_CurSize.X);
    if (bAutoHeight)
        AdjustedSize.Y = Math::Max(Size.Y, m_CurSize.Y);
}

void UITextEdit::Draw(ACanvas& cv)
{
    m_Stb->insert_mode = GUIManager->IsInsertMode();

    AFont* font = GetFont();

    Float2 pos  = Geometry.Mins;

    cv.FontFace(m_Font);

    TextMetrics metrics;
    font->GetTextMetrics(m_FontStyle, metrics);

    if (HasSelection())
    {
        int start = GetSelectionStart();
        int end   = GetSelectionEnd();

        WideChar const* seltext;
        Float2          selstart = CalcCursorOffset(font, m_FontStyle, m_TextData.ToPtr(), start, &seltext);

        const float     lineHeight = metrics.LineHeight;

        float           lineWidth  = 0.0f;
        WideChar const* s          = seltext;
        WideChar const* s_end      = m_TextData.ToPtr() + end;
        while (s < s_end)
        {
            WideChar c = *s++;
            if (c == '\n')
            {
                lineWidth = Math::Max(lineWidth, font->GetCharAdvance(m_FontStyle, ' ') * 0.4f);
                cv.DrawRectFilled(pos + selstart, pos + selstart + Float2(lineWidth, lineHeight), m_SelectionColor);
                selstart.X = 0;
                selstart.Y += lineHeight;
                lineWidth = 0.0f;
                continue;
            }
            if (c == '\r')
            {
                continue;
            }
            lineWidth += font->GetCharAdvance(m_FontStyle, c);
        }
        cv.DrawRectFilled(pos + selstart, pos + selstart + Float2(lineWidth, lineHeight), m_SelectionColor);
    }

    if (GetDesktop()->m_FocusWidget == this)
    {
        if ((Platform::SysMicroseconds() >> 18) & 1)
        {
            Float2 cursor = pos + CalcCursorOffset(font, m_FontStyle, m_TextData.ToPtr(), m_Stb->cursor, nullptr);

            if (m_Stb->insert_mode)
            {
                float w = m_Stb->cursor < m_CurTextLength ? font->GetCharAdvance(m_FontStyle, m_TextData[m_Stb->cursor]) : font->GetCharAdvance(m_FontStyle, ' ');

                cv.DrawRectFilled(cursor, Float2(cursor.X + w, cursor.Y + m_FontStyle.FontSize), m_TextColor);
            }
            else
            {
                cv.DrawLine(cursor, Float2(cursor.X, cursor.Y + m_FontStyle.FontSize), m_TextColor);
            }
        }
    }

    if (m_CurTextLength > 0)
    {
        //cv.DrawTextWrapWChar(m_FontStyle, pos, m_TextColor, AWideStringView(m_TextData.ToPtr(), m_CurTextLength), Math::MaxValue<float>());

        TVector<char> str(m_TextData.Size() * 4 + 1); // In worst case WideChar transforms to 4 bytes,
                                                      // one additional byte is reserved for trailing '\0'

        Core::WideStrEncodeUTF8(str.ToPtr(), str.Size(), m_TextData.Begin(), m_TextData.Begin() + m_CurTextLength);

        cv.FillColor(m_TextColor);
        cv.TextBox(m_FontStyle, Geometry.Mins, Geometry.Maxs, TEXT_ALIGNMENT_LEFT | TEXT_ALIGNMENT_TOP | TEXT_ALIGNMENT_KEEP_SPACES, false, str.ToPtr());
    }
}

void UITextEdit::UpdateWidgetSize()
{
#if 1
    AFont* font = GetFont();

    TextMetrics metrics;
    font->GetTextMetrics(m_FontStyle, metrics);

    Float2 size(0.0f, metrics.Ascender);
    float  lineWidth = 0.0f;

    WideChar const* s     = m_TextData.ToPtr();
    WideChar const* s_end = m_TextData.ToPtr() + m_CurTextLength;
    while (s < s_end)
    {
        WideChar c = *s++;
        if (c == '\n')
        {
            size.X = Math::Max(size.X, lineWidth);
            size.Y += metrics.LineHeight;
            lineWidth = 0.0f;
            continue;
        }
        if (c == '\r')
        {
            continue;
        }
        lineWidth += font->GetCharAdvance(m_FontStyle, c);
    }
    size.X = Math::Max(size.X, lineWidth);

    const int granularity = 100;
    const int mod         = (int)size.X % granularity;

    size.X = mod ? size.X + granularity - mod : size.X;

#else
    Float2 size = CalcTextRect(GetFont(), m_FontSize, m_TextData.ToPtr(), m_TextData.ToPtr() + m_CurTextLength, nullptr, nullptr, false);
#endif

    m_CurSize = size;
}

bool UITextEdit::FilterCharacter(WideChar& ch)
{
    WideChar c = ch;

    if (c < 128 && c != ' ' && !isprint((int)(c & 0xFF)))
    {
        if ((c != '\n' || m_bSingleLine) && (c != '\t' || !m_bAllowTabInput))
        {
            return false;
        }
    }

    if (c >= 0xE000 && c <= 0xF8FF)
    {
        // Private Unicode range
        return false;
    }

    if (m_CharacterFilter & (CHARS_DECIMAL | CHARS_HEXADECIMAL | CHARS_UPPERCASE | CHARS_NO_BLANK | CHARS_SCIENTIFIC))
    {
        if (m_CharacterFilter & CHARS_DECIMAL)
            if (!(c >= '0' && c <= '9') && (c != '.') && (c != '-') && (c != '+') && (c != '*') && (c != '/'))
                return false;

        if (m_CharacterFilter & CHARS_SCIENTIFIC)
            if (!(c >= '0' && c <= '9') && (c != '.') && (c != '-') && (c != '+') && (c != '*') && (c != '/') && (c != 'e') && (c != 'E'))
                return false;

        if (m_CharacterFilter & CHARS_HEXADECIMAL)
            if (!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f') && !(c >= 'A' && c <= 'F'))
                return false;

        if (m_CharacterFilter & CHARS_UPPERCASE)
        {
            if (c >= 'a' && c <= 'z')
            {
                c  = c + 'A' - 'a';
                ch = c;
            }
        }

        if (m_CharacterFilter & CHARS_NO_BLANK)
            if (Core::WideCharIsBlank(c))
                return false;
    }

    if (m_bCustomCharFilter)
    {
        if (!OnFilterCharacter(c))
        {
            return false;
        }
        ch = c;
        if (!c)
        {
            return false;
        }
    }

    return true;
}
