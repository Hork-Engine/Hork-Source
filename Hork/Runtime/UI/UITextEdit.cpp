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

#include "UITextEdit.h"
#include "UIScroll.h"
#include "UIManager.h"

#include <Hork/Runtime/GameApplication/FrameLoop.h>
#include <Hork/Runtime/GameApplication/GameApplication.h>

#include <Hork/Core/Platform.h>

using namespace Hk;

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

static constexpr WideChar STB_TEXTEDIT_NEWLINE = '\n';

#define STB_TEXTEDIT_STRINGLEN(obj) obj->GetText().Size()

WideChar STB_TEXTEDIT_GETCHAR(UITextEdit* obj, int pos)
{
    #if 1
    int rowNum = obj->FindRow(pos);
    TextRowW const& row    = obj->GetRows()[rowNum];

    if (pos == row.End - obj->GetText().CBegin())
        return STB_TEXTEDIT_NEWLINE;
    #endif

    return obj->GetText()[pos];
}

static HK_INLINE int STB_TEXTEDIT_KEYTOTEXT(int key)
{
    return key >= 0x10000 ? 0 : key;
}

static void STB_TEXTEDIT_LAYOUTROW(StbTexteditRow* row, UITextEdit* obj, int lineStartIndex)
{
    int rowNum = obj->FindRow(lineStartIndex);

    TextRowW const& r = obj->GetRows()[rowNum];

    FontStyle fontStyle;
    fontStyle.FontSize = obj->GetFontSize();

    TextMetrics metrics;
    obj->GetFont()->GetTextMetrics(fontStyle, metrics);

    row->x0               = 0.0f;
    row->x1               = r.MaxX; // - r.MinX;
    row->baseline_y_delta = metrics.LineHeight;
    row->ymin             = 0.0f;
    row->ymax             = metrics.LineHeight;
    row->num_chars        = (int)(r.End - r.Start/* + 1*/);
}

static float STB_TEXTEDIT_GETWIDTH(UITextEdit* obj, int lineStartIndex, int charIndex)
{
    int n = lineStartIndex + charIndex;
    if (n >= obj->GetText().Size())
        return 0;

    WideChar c = obj->GetText()[n];
    if (c == '\n')
        return STB_TEXTEDIT_GETWIDTH_NEWLINE;

    FontStyle fontStyle;
    fontStyle.FontSize = obj->GetFontSize();

    return obj->GetFont()->GetCharAdvance(fontStyle, c);
}

typedef struct
{
    float x, y;               // position of n'th character
    float height;             // height of line
    int   first_char, length; // first char of row, and length
    int   prev_first;         // first char of previous row
} StbFindState;

// find the x/y location of a character, and remember info about the previous row in
// case we get a move-up event (for page up, we'll have to rescan)
static void stb_textedit_find_charpos(StbFindState* find, UITextEdit* str, int n, int single_line)
{
    WideChar const* text = str->GetText().CBegin();
    int length = str->GetText().Size();

    FontResource* font = str->GetFont();

    FontStyle fontStyle;
    fontStyle.FontSize = str->GetFontSize();

    TextMetrics metrics;
    font->GetTextMetrics(fontStyle, metrics);

    if (n == length && single_line)
    {
        find->x          = 0;
        find->y          = 0;
        find->first_char = 0;
        find->length     = length;
        find->height     = metrics.LineHeight;
        return;
    }

    HK_ASSERT(n <= length);

    int rowNum = str->FindRow(n);    

    find->y = rowNum * metrics.LineHeight;

    find->first_char = (int)(str->GetRows()[rowNum].Start - text);
    find->length     = (int)(str->GetRows()[rowNum].End - str->GetRows()[rowNum].Start) + 1;
    find->height     = metrics.LineHeight;

    if (rowNum > 0)
        find->prev_first = (int)(str->GetRows()[rowNum - 1].Start - text);
    else
        find->prev_first = 0;

    find->x = 0;
    for (int charNum = find->first_char; charNum < n; ++charNum)
    {
        WideChar c = text[charNum];

        HK_ASSERT(c != '\n');

        find->x += font->GetCharAdvance(fontStyle, c);
    }
}

static int stb_text_locate_coord(UITextEdit* str, float x, float y)
{
    return str->LocateCoord(x, y);
}

int UITextEdit::LocateCoord(float x, float y)
{
    if (m_Text.IsEmpty())
        return 0;

    FontResource* font = GetFont();

    TextMetrics metrics;
    font->GetTextMetrics(m_FontStyle, metrics);

    x -= m_Geometry.Mins.X;
    y -= m_Geometry.Mins.Y;

    int rowNum = y / metrics.LineHeight;
    if (rowNum < 0)
        return 0;

    if (rowNum >= m_Rows.Size())
        return (int)m_Text.Size() - 1;
    
    TextRowW* row = &m_Rows[rowNum];

    if (x <= row->MinX)
        return (int)(row->Start - m_Text.ToPtr());

    if (x >= row->MaxX)
        return (int)(row->End - m_Text.ToPtr());

    int numChars = row->End - row->Start;
    float prev_x       = row->MinX;
    for (int k = 0; k < numChars; ++k)
    {
        float w = font->GetCharAdvance(m_FontStyle, row->Start[k]);
        if (x < prev_x + w)
        {
            if (x < prev_x + w / 2)
                return (int)(row->Start - m_Text.ToPtr() + k);
            else
                return (int)(row->Start - m_Text.ToPtr() + k + 1);
        }
        prev_x += w;
    }

    return (int)(row->End - m_Text.ToPtr());
}

#define STB_TEXTEDIT_DELETECHARS(obj, first, count) (obj)->DeleteChars(first, count)

#define STB_TEXTEDIT_INSERTCHARS(obj, offset, text, textLength) (obj)->InsertChars(offset, WideStringView(text, textLength))

HK_FORCEINLINE bool IsSeparator(WideChar c)
{
    return c == ',' || c == '.' || c == ';' || c == ':' || c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']' || c == '{' || c == '}' || c == '<' || c == '>' || c == '|' || c == '!' || c == '@' || c == '#' || c == '$' || c == '%' || c == '^' || c == '&' || c == '*' || c == '/' || c == '\\' || c == '+' || c == '=' || c == '-' || c == '~' || c == '`' || c == '\'' || c == '"' || c == '?' || c == '\n';
}

static bool IsWordBoundary(WideChar const* s)
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

static int NextWord(UITextEdit* obj, int i)
{
    i++;
    int             len = obj->GetText().Size();
    WideChar const* s   = obj->GetText().CBegin() + i;
    while (i < len && !IsWordBoundary(s))
    {
        i++;
        s++;
    }
    return i > len ? len : i;
}

static int PrevWord(UITextEdit* obj, int i)
{
    i--;
    WideChar const* s = obj->GetText().CBegin() + i;
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

enum
{
    CHARS_DECIMAL     = HK_BIT(0), // 0123456789.+-*/
    CHARS_HEXADECIMAL = HK_BIT(1), // 0123456789ABCDEFabcdef
    CHARS_UPPERCASE   = HK_BIT(2), // a..z -> A..Z
    CHARS_NO_BLANK    = HK_BIT(3), // filter out spaces, tabs
    CHARS_SCIENTIFIC  = HK_BIT(4), // 0123456789.+-*/eE (Scientific notation input)
};

int UITextEdit::FindRow(int cursor)
{
    // OPTIMIZ: Binary search
    int       n = 0;
    WideChar* s = m_Text.ToPtr();
    for (TextRowW& row : m_Rows)
    {
        size_t offset = row.End - s;
        if (cursor <= offset)
            return n;
        ++n;
    }
    return (int)m_Rows.Size() - 1;
}

// Returns top-left cursor position
Float2 UITextEdit::CalcCursorOffset(int cursor)
{
    Float2 offset(0);

    int rowNum = FindRow(cursor);
    if (rowNum >= 0)
    {
        TextRowW const* row = &m_Rows[rowNum];

        FontResource* font = GetFont();

        float lineWidth = 0.0f;

        WideChar const* s   = row->Start;
        WideChar const* end = m_Text.ToPtr() + cursor;
        while (s < end)
        {
            lineWidth += font->GetCharAdvance(m_FontStyle, *s++);
        }

        TextMetrics metrics;
        font->GetTextMetrics(m_FontStyle, metrics);

        offset.X = lineWidth;
        offset.Y = rowNum * metrics.LineHeight;
    }

    return offset;
}

UITextEdit::UITextEdit()
{
    m_bSingleLine = false;

    m_State = (STB_TexteditState*)Core::GetHeapAllocator<HEAP_MISC>().Alloc(sizeof(STB_TexteditState));
    stb_textedit_initialize_state(m_State, m_bSingleLine);

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
    Core::GetHeapAllocator<HEAP_MISC>().Free(m_State);
}

UITextEdit& UITextEdit::WithFont(FontHandle font)
{
    m_Font = font;

    UpdateRows();
    return *this;
}

UITextEdit& UITextEdit::WithFontSize(float size)
{
    m_FontStyle.FontSize = size;

    UpdateRows();
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
    stb_textedit_initialize_state(m_State, m_bSingleLine);
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

FontResource* UITextEdit::GetFont() const
{
    FontResource* resource = GameApplication::sGetResourceManager().TryGet(m_Font);

    return resource ? resource : GameApplication::sGetDefaultFont();
}

FontHandle UITextEdit::GetFontHandle() const
{
    FontResource* resource = GameApplication::sGetResourceManager().TryGet(m_Font);

    return resource ? m_Font : GameApplication::sGetDefaultFontHandle();
}

float UITextEdit::GetFontSize() const
{
    return m_FontStyle.FontSize;
}

int UITextEdit::GetCursorPosition() const
{
    return m_State->cursor;
}

int UITextEdit::GetSelectionStart() const
{
    return Math::Min(m_State->select_start, m_State->select_end);
}

int UITextEdit::GetSelectionEnd() const
{
    return Math::Max(m_State->select_start, m_State->select_end);
}

bool UITextEdit::InsertChars(int offset, WideStringView text)
{
    if (offset < 0 || offset > m_Text.Size())
    {
        return false;
    }

    bool bResizable = m_MaxChars > 0;
    if (bResizable && m_Text.Size() + text.Size() > m_MaxChars)
    {
        int textLength = m_MaxChars - m_Text.Size();
        if (textLength <= 0)
        {
            return false;
        }

        text = text.GetSubstring(0, textLength);
    }

    m_Text.InsertAt(offset, text);

    UpdateRows();

    E_OnTyping.Invoke(m_Text);

    return true;
}

void UITextEdit::DeleteChars(int first, int count)
{
    if (count <= 0 || first < 0)
    {
        return;
    }

    m_Text.Cut(first, count);

    UpdateRows();

    E_OnTyping.Invoke(m_Text);
}

void UITextEdit::PressKey(int key)
{
    if (key)
    {
        stb_textedit_key(this, m_State, key);
    }
}

void UITextEdit::ClearSelection()
{
    m_State->select_start = m_State->select_end = m_State->cursor;
}

void UITextEdit::SelectAll()
{
    m_State->select_start    = 0;
    m_State->select_end      = m_Text.Size();
    m_State->cursor          = m_State->select_end;
    m_State->has_preferred_x = 0;
}

bool UITextEdit::HasSelection() const
{
    return m_State->select_start != m_State->select_end;
}

UIScroll* UITextEdit::GetScroll()
{
    return dynamic_cast<UIScroll*>(m_Parent.RawPtr());
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

        float pageSize = scroll->GetViewSize().Y;
        pageSize       = Math::Snap(pageSize, metrics.LineHeight);

        int numLines = (int)(pageSize / metrics.LineHeight);

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

        float pageSize = scroll->GetViewSize().Y;
        pageSize       = Math::Snap(pageSize, metrics.LineHeight);

        int numLines = (int)(pageSize / metrics.LineHeight);

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

        scrollPosition.Y = Math::Snap(scrollPosition.Y, metrics.LineHeight);
        scrollPosition.Y -= numLines * metrics.LineHeight;

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

void UITextEdit::ScrollLineEnd()
{
    UIScroll* scroll = GetScroll();
    if (!scroll)
    {
        return;
    }

    int rowNum = FindRow(m_State->cursor);
    if (rowNum < 0)
        return;

    float lineWidth = m_Rows[rowNum].MaxX;// - m_Rows[rowNum].MinX;

    float  pageWidth      = scroll->GetViewSize().X;
    Float2 scrollPosition = scroll->GetScrollPosition();
    scrollPosition.X      = -lineWidth + pageWidth;
    scroll->SetScrollPosition(scrollPosition);
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

    FontResource const* font = GetFont();

    Float2 scrollMins = scroll->m_Geometry.PaddedMins;
    Float2 scrollMaxs = scroll->m_Geometry.PaddedMaxs;
    Float2 pageSize   = scrollMaxs - scrollMins;

    TextMetrics metrics;
    font->GetTextMetrics(m_FontStyle, metrics);

    Float2 cursorOffset = CalcCursorOffset(m_State->cursor);

    // Cursor global position
    Float2 cursor = m_Geometry.Mins + cursorOffset;

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
    stb_textedit_cut(this, m_State);

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
    const int       endOfs   = bHasSelection ? GetSelectionEnd() : m_Text.Size();
    WideChar* const start    = m_Text.ToPtr() + startOfs;
    WideChar* const end      = m_Text.ToPtr() + endOfs;

    CoreApplication::sSetClipboard(Core::GetString(WideStringView(start, end)));

    return true;
}

bool UITextEdit::Paste()
{
    if (m_bReadOnly)
    {
        // Can't modify readonly text
        return false;
    }

    const char* s = CoreApplication::sGetClipboard();

    int len = Core::UTF8StrLength(s);

    WideString wideStr;
    wideStr.Resize(len, STRING_RESIZE_NO_FILL_SPACES);

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
        stb_textedit_paste(this, m_State, wideStr.ToPtr(), i);
    }

    return true;
}

UITextEdit& UITextEdit::WithText(StringView text)
{
    int len = Core::UTF8StrLength(text.Begin(), text.End());

    const char* textcur = text.Begin();

    WideString wideStr;
    wideStr.Resize(len, STRING_RESIZE_NO_FILL_SPACES);

    WideChar ch;
    int      i, byteLen;
    i = 0;
    while (len-- > 0)
    {
        byteLen = Core::WideCharDecodeUTF8(textcur, ch);
        if (!byteLen)
        {
            break;
        }
        textcur += byteLen;
        if (!FilterCharacter(ch))
        {
            continue;
        }
        wideStr[i++] = ch;
    }

    return WithText(wideStr);
}

UITextEdit& UITextEdit::WithText(WideStringView text)
{
    SelectAll();

    stb_textedit_paste(this, m_State, text.ToPtr(), text.Size());

    return *this;
}

UITextEdit& UITextEdit::WithWordWrap(bool bWordWrap)
{
    m_bWithWordWrap = bWordWrap;

    UpdateRows();
    return *this;
}

void UITextEdit::OnKeyEvent(KeyEvent const& event)
{
    if (event.Action != InputAction::Released)
    {
        int key = 0;

        // OS X style: Shortcuts using Cmd/Super instead of Ctrl
        const bool bShortcutKey = (bOSX ? ((event.ModMask.Super) && !(event.ModMask.Control)) : ((event.ModMask.Control) && !(event.ModMask.Super))) && !(event.ModMask.Alt) && !(event.ModMask.Shift);

        const bool bShiftShortcutOSX = bOSX && (event.ModMask.Super) && (event.ModMask.Shift) && !(event.ModMask.Control) && !(event.ModMask.Alt);

        // OS X style: Text editing cursor movement using Alt instead of Ctrl
        const bool bWordmoveKeyDown = bOSX ? !!(event.ModMask.Alt) : !!(event.ModMask.Control);

        // OS X style: Line/Text Start and End using Cmd+Arrows instead of Home/End
        const bool bStartEndKeyDown = bOSX && (event.ModMask.Super) && !(event.ModMask.Control) && !(event.ModMask.Alt);

        const int KeyMask = (event.ModMask.Shift) ? STB_TEXTEDIT_K_SHIFT : 0;

        switch (event.Key)
        {
            case VirtualKey::Left:
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

            case VirtualKey::Right:
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

            case VirtualKey::Up:
                if (!m_bSingleLine)
                {
                    if (event.ModMask.Control)
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

            case VirtualKey::Down:
                if (!m_bSingleLine)
                {
                    if (event.ModMask.Control)
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

            case VirtualKey::Home:
                if (event.ModMask.Control)
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

            case VirtualKey::End:
                if (event.ModMask.Control)
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
                }
                break;

            case VirtualKey::PageUp:
                ScrollPageUp(true);
                break;

            case VirtualKey::PageDown:
                ScrollPageDown(true);
                break;

            case VirtualKey::Delete:
                if (!m_bReadOnly)
                {
                    key = STB_TEXTEDIT_K_DELETE | KeyMask;
                    PressKey(key);
                }
                break;

            case VirtualKey::Backspace:
                if (!m_bReadOnly)
                {
                    if (!HasSelection())
                    {
                        if (bWordmoveKeyDown)
                        {
                            PressKey(STB_TEXTEDIT_K_WORDLEFT | STB_TEXTEDIT_K_SHIFT);
                        }
                        else if (bOSX && (event.ModMask.Super) && !(event.ModMask.Alt) && !(event.ModMask.Control))
                        {
                            PressKey(STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_SHIFT);
                        }
                    }
                    PressKey(STB_TEXTEDIT_K_BACKSPACE | KeyMask);
                    ScrollToCursor();
                }
                break;

            case VirtualKey::Enter: {
                const bool bCtrl = !!(event.ModMask.Control);

                if (m_bSingleLine || (m_bCtrlEnterForNewLine && !bCtrl) || (!m_bCtrlEnterForNewLine && bCtrl))
                {
                    E_OnEnterPress.Invoke(m_Text);
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

            case VirtualKey::Tab: {
                bool bCtrl  = !!(event.ModMask.Control);
                bool bShift = !!(event.ModMask.Shift);
                bool bAlt   = !!(event.ModMask.Alt);

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

            case VirtualKey::Escape:
                E_OnEscapePress.Invoke();
                break;

            case VirtualKey::Z:
                if (m_bAllowUndo && !m_bReadOnly)
                {
                    if (bShortcutKey)
                    {
                        PressKey(STB_TEXTEDIT_K_UNDO);

                        ClearSelection();
                        ScrollToCursor();
                    }
                    else if (bShiftShortcutOSX || (event.ModMask.Shift && event.ModMask.Control))
                    {
                        PressKey(STB_TEXTEDIT_K_REDO);

                        ClearSelection();
                        ScrollToCursor();
                    }
                }
                break;

            case VirtualKey::Y:

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

            case VirtualKey::A:
                if (bShortcutKey)
                {
                    SelectAll();
                }
                break;

            case VirtualKey::Insert:
                if (event.ModMask == 0)
                {
                    GUIManager->SetInsertMode(!GUIManager->IsInsertMode());
                }
                break;
        }

        const bool bCtrlOnly  = (event.ModMask.Control) && !(event.ModMask.Shift) && !(event.ModMask.Alt) && !(event.ModMask.Super);
        const bool bShiftOnly = (event.ModMask.Shift) && !(event.ModMask.Control) && !(event.ModMask.Alt) && !(event.ModMask.Super);

        if ((bShortcutKey && event.Key == VirtualKey::X) || (bShiftOnly && event.Key == VirtualKey::Delete))
        {
            Cut();
            ScrollToCursor();
        }
        else if ((bShortcutKey && event.Key == VirtualKey::C) || (bCtrlOnly && event.Key == VirtualKey::Insert))
        {
            Copy();
            ScrollToCursor();
        }
        else if ((bShortcutKey && event.Key == VirtualKey::V) || (bShiftOnly && event.Key == VirtualKey::Insert))
        {
            Paste();
            ScrollToCursor();
        }
    }
}

void UITextEdit::OnMouseButtonEvent(MouseButtonEvent const& event)
{
    if (event.Button != VirtualKey::MouseLeftBtn && event.Button != VirtualKey::MouseRightBtn)
    {
        return;
    }

    if (event.Action == InputAction::Pressed)
    {
        Float2 CursorPos = GUIManager->CursorPosition;

        if (!HasSelection())
        {
            m_TempCursor = m_State->cursor;
        }

        if (event.Button == VirtualKey::MouseLeftBtn && (event.ModMask.Shift))
        {
            stb_textedit_click(this, m_State, CursorPos.X, CursorPos.Y);

            m_State->select_start = m_TempCursor > m_Text.Size() ? m_State->cursor : m_TempCursor;
            m_State->select_end   = m_State->cursor;

            if (m_State->select_start > m_State->select_end)
            {
                std::swap(m_State->select_start, m_State->select_end);
            }
        }
        else
        {
            stb_textedit_click(this, m_State, CursorPos.X, CursorPos.Y);

            m_TempCursor = m_State->cursor;
        }
    }

    m_bStartDragging = (event.Action == InputAction::Pressed) && event.Button == VirtualKey::MouseLeftBtn;
}

void UITextEdit::OnDblClickEvent(VirtualKey buttonKey, Float2 const& clickPos, uint64_t clickTime)
{
    if (buttonKey == VirtualKey::MouseLeftBtn)
    {
        PressKey(STB_TEXTEDIT_K_WORDLEFT);
        PressKey(STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT);

        int s = GetSelectionStart();
        int e = GetSelectionEnd();

        while (e-- > s)
        {
            if (!Core::WideCharIsBlank(m_Text[e]))
            {
                break;
            }
            PressKey(STB_TEXTEDIT_K_LEFT | STB_TEXTEDIT_K_SHIFT);
        }
    }
}

void UITextEdit::OnMouseWheelEvent(MouseWheelEvent const& event)
{
    if (m_bSingleLine)
    {
        Super::OnMouseWheelEvent(event);
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

void UITextEdit::OnMouseMoveEvent(MouseMoveEvent const& event)
{
    if (m_bStartDragging)
    {
        Float2 CursorPos = GUIManager->CursorPosition;

        stb_textedit_drag(this, m_State, CursorPos.X, CursorPos.Y);

        ScrollToCursor();
    }
}

void UITextEdit::OnCharEvent(CharEvent const& event)
{
    if (m_bReadOnly)
    {
        return;
    }

    // We ignore CTRL inputs, but need to allow ALT+CTRL as some keyboards (e.g. German) use AltGR (which _is_ Alt+Ctrl) to input certain characters.
    if ((event.ModMask.Control) && !(event.ModMask.Alt))
    {
        return;
    }

    if (bOSX && (event.ModMask.Super))
    {
        return;
    }

    WideChar ch = event.UnicodeCharacter;
    if (!FilterCharacter(ch))
    {
        return;
    }

    PressKey((int)ch);

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
        m_AdjustedSize.X = Math::Max(Size.X, m_CurSize.X);
    if (bAutoHeight)
        m_AdjustedSize.Y = Math::Max(Size.Y, m_CurSize.Y);
}

void UITextEdit::Draw(Canvas& cv)
{
    m_State->insert_mode = GUIManager->IsInsertMode();

    FontHandle fontHandle = GetFontHandle();
    FontResource* font = GameApplication::sGetResourceManager().TryGet(fontHandle);

    HK_ASSERT(font);

    cv.FontFace(fontHandle);

    TextMetrics metrics;
    font->GetTextMetrics(m_FontStyle, metrics);

    float lineHeight = metrics.LineHeight;

    if (HasSelection())
    {
        int start = GetSelectionStart();
        int end   = GetSelectionEnd();

        Float2 clipMins, clipMaxs;
        cv.GetIntersectedScissor(m_Geometry.Mins, m_Geometry.Maxs, clipMins, clipMaxs);

        float y0 = clipMins.Y - m_Geometry.Mins.Y;
        float y1 = clipMaxs.Y - m_Geometry.Mins.Y;

        int firstRow = y0 / lineHeight;
        int lastRow  = y1 / lineHeight + 1;

        firstRow = Math::Clamp(firstRow, 0, (int)m_Rows.Size() - 1);
        lastRow  = Math::Clamp(lastRow, 0, (int)m_Rows.Size());

        float y = m_Geometry.Mins.Y;
        y += firstRow * lineHeight;

        Float2 selstart = CalcCursorOffset(start);

        selstart += m_Geometry.Mins;

        if (selstart.Y < y)
        {
            selstart.Y = y;
            selstart.X = m_Geometry.Mins.X;

            start = m_Rows[firstRow].Start - m_Text.ToPtr();
        }
        else
        {
            firstRow = FindRow(start);
        }

        TextRowW* row = &m_Rows[firstRow];

        float           lineWidth  = 0.0f;
        WideChar const* s         = m_Text.ToPtr() + start;
        WideChar const* s_end      = m_Text.ToPtr() + end;
        while (s < s_end)
        {
            if (s >= row->End)
            {
                lineWidth = Math::Max(lineWidth, font->GetCharAdvance(m_FontStyle, ' ') * 0.4f);
                cv.DrawRectFilled(selstart, selstart + Float2(lineWidth, lineHeight), m_SelectionColor);
                selstart.X = m_Geometry.Mins.X;
                selstart.Y += lineHeight;
                lineWidth = 0.0f;
                if (selstart.Y > clipMaxs.Y)
                    break;
                row++;
                s = row->Start;
                continue;
            }
            WideChar c = *s++;
            if (c == '\r')
            {
                continue;
            }
            lineWidth += font->GetCharAdvance(m_FontStyle, c);
        }

        if (lineWidth > 0)
            cv.DrawRectFilled(selstart, selstart + Float2(lineWidth, lineHeight), m_SelectionColor);
    }

    if (HasFocus())
    {
        bool tick = (Core::SysMicroseconds() >> 19) & 1;

        if (tick || m_PrevCursorPos != m_State->cursor)
        {
            Float2 cursor = m_Geometry.Mins + CalcCursorOffset(m_State->cursor);

            if (m_State->insert_mode)
            {
                float w = m_State->cursor < m_Text.Size() ? font->GetCharAdvance(m_FontStyle, m_Text[m_State->cursor]) : font->GetCharAdvance(m_FontStyle, ' ');

                cv.DrawRectFilled(cursor, Float2(cursor.X + w, cursor.Y + m_FontStyle.FontSize), m_TextColor);
            }
            else
            {
                cv.DrawLine(cursor, Float2(cursor.X, cursor.Y + m_FontStyle.FontSize), m_TextColor);
            }
        }

        if (tick)
            m_PrevCursorPos = m_State->cursor;
    }
    
    if (!m_Text.IsEmpty())
    {
        cv.FillColor(m_TextColor);

        float x = m_Geometry.Mins.X;
        float y = m_Geometry.Mins.Y;

        Float2 clipMins, clipMaxs;
        cv.GetIntersectedScissor(m_Geometry.Mins, m_Geometry.Maxs, clipMins, clipMaxs);

        float y0 = clipMins.Y - m_Geometry.Mins.Y;
        float y1 = clipMaxs.Y - m_Geometry.Mins.Y;

        int firstRow = y0 / lineHeight;
        int lastRow  = y1 / lineHeight + 1;

        firstRow = Math::Clamp(firstRow, 0, (int)m_Rows.Size() - 1);
        lastRow  = Math::Clamp(lastRow, 0, (int)m_Rows.Size());

        y += firstRow * lineHeight;

        for (TextRowW *row = m_Rows.Begin() + firstRow, *end = m_Rows.Begin() + lastRow; row < end; row++)
        {
            cv.Text(m_FontStyle, x, y, TEXT_ALIGNMENT_LEFT, row->GetStringView());
            y += lineHeight;
        }
    }
}

// OPTIMIZ: Recalc only modified rows
void UITextEdit::UpdateRows()
{
    FontResource* font = GetFont();

    const bool bKeepSpaces = true;

    static TextRowW rows[128];
    int             nrows;
    float           w = 0;

    const float breakRowWidth = m_bWithWordWrap ? Size.X : Math::MaxValue<float>();

    WideStringView str = m_Text;

    m_Rows.Clear();

    while ((nrows = font->TextBreakLines(m_FontStyle, str, breakRowWidth, rows, HK_ARRAY_SIZE(rows), bKeepSpaces)) > 0)
    {
        for (int i = 0; i < nrows; i++)
        {
            TextRowW* row = &rows[i];

            row->MinX = 0;

            w = Math::Max(w, row->MaxX);

            m_Rows.Add(*row);
        }
        str = WideStringView(rows[nrows - 1].Next, str.End());
    }

    if (m_Text.IsEmpty() || (!m_bSingleLine && m_Text[m_Text.Size() - 1] == '\n'))
    {
        TextRowW& row = m_Rows.Add();
        row.Start     = m_Text.End();
        row.End       = m_Text.End();
        row.Next      = m_Text.End();
        row.Width     = 0;
        row.MinX      = 0;
        row.MaxX      = 0;
    }

    // Recalc widget bounds
    TextMetrics metrics;
    font->GetTextMetrics(m_FontStyle, metrics);

    const int granularity = 100;
    const int mod         = (int)w % granularity;

    w = mod ? w + granularity - mod : w;

    m_CurSize.X = w;
    m_CurSize.Y = Math::Max<int>(1, m_Rows.Size()) * metrics.LineHeight;
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
