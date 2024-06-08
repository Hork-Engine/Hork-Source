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

struct STB_TexteditState_s;
typedef STB_TexteditState_s STB_TexteditState;

HK_NAMESPACE_BEGIN

class UITextEdit : public UIWidget
{
    UI_CLASS(UITextEdit, UIWidget)

public:
    Delegate<void(WideStringView)> E_OnEnterPress;
    Delegate<void()>               E_OnEscapePress;
    Delegate<void(WideStringView)> E_OnTyping;

    UITextEdit();
    ~UITextEdit();

    UITextEdit& WithText(StringView text);
    UITextEdit& WithText(WideStringView text);
    UITextEdit& WithWordWrap(bool bWordWrap);
    UITextEdit& WithFont(FontHandle font);
    UITextEdit& WithFontSize(float size);
    UITextEdit& WithMaxChars(int maxChars);
    UITextEdit& WithFilterDecimal(bool bEnabled);
    UITextEdit& WithFilterHexadecimal(bool bEnabled);
    UITextEdit& WithFilterUppercase(bool bEnabled);
    UITextEdit& WithFilterNoBlank(bool bEnabled);
    UITextEdit& WithFilterScientific(bool bEnabled);
    UITextEdit& WithFilterCustomCallback(bool bEnabled);
    UITextEdit& WithInsertSpacesOnTab(int numSpaces);
    UITextEdit& WithSingleLine(bool bEnabled);
    UITextEdit& WithReadOnly(bool bEnabled);
    UITextEdit& WithPassword(bool bEnabled);
    UITextEdit& WithCtrlEnterForNewLine(bool bEnabled);
    UITextEdit& WithAllowTabInput(bool bEnabled);
    UITextEdit& WithAllowUndo(bool bEnabled);
    UITextEdit& WithSelectionColor(Color4 const& color);
    UITextEdit& WithTextColor(Color4 const& color);
    UITextEdit& ShouldKeepSelection(bool bShouldKeepSelection);

    template <typename T, typename... TArgs>
    UITextEdit& WithOnEnterPress(T* object, void (T::*method)(TArgs...))
    {
        E_OnEnterPress.Add(object, method);
        return *this;
    }

    template <typename T, typename... TArgs>
    UITextEdit& WithOnEscapePress(T* object, void (T::*method)(TArgs...))
    {
        E_OnEscapePress.Add(object, method);
        return *this;
    }

    template <typename T, typename... TArgs>
    UITextEdit& WithOnTyping(T* object, void (T::*method)(TArgs...))
    {
        E_OnTyping.Add(object, method);
        return *this;
    }

    void ClearSelection();

    void SelectAll();

    bool HasSelection() const;

    bool Cut();

    bool Copy();

    bool Paste();

    void ScrollHome();

    void ScrollEnd();

    void ScrollPageUp(bool bMoveCursor = false);

    void ScrollPageDown(bool bMoveCursor = false);

    void ScrollLineUp();

    void ScrollLineDown();

    void ScrollLines(int numLines);

    void ScrollLineStart();

    void ScrollLineEnd();

    void ScrollHorizontal(float delta);

    void ScrollToCursor();

    WideString const& GetText() const { return m_Text; }

    int GetCursorPosition() const;

    int GetSelectionStart() const;

    int GetSelectionEnd() const;

    FontResource* GetFont() const;

    float GetFontSize() const;

    bool InsertChars(int offset, WideStringView text);

    void DeleteChars(int first, int count);

    int  FindRow(int cursor);
    TextRowW const* GetRows() { return m_Rows.ToPtr(); }
    size_t          NumRows() const { return m_Rows.Size(); }

    int LocateCoord(float x, float y);

    // FUTURE:
    // UITextEdit& WithSyntaxHighlighter(ISyntaxHighlighter * syntaxHighlighterInterface);
    // class ISyntaxHighlighter {
    // public:
    //     virtual Color4 const& GetWordColor(WideChar* wordStart, WideChar* wordEnd) = 0;
    // };

protected:
    virtual bool OnFilterCharacter(WideChar& ch) { return true; }

    void OnKeyEvent(struct KeyEvent const& event) override;

    void OnMouseButtonEvent(struct MouseButtonEvent const& event) override;

    void OnDblClickEvent(int buttonKey, Float2 const& clickPos, uint64_t clickTime) override;

    void OnMouseWheelEvent(struct MouseWheelEvent const& event) override;

    void OnMouseMoveEvent(struct MouseMoveEvent const& event) override;

    void OnCharEvent(struct CharEvent const& event) override;

    void OnFocusLost() override;

    void Draw(Canvas& cv) override;

    void AdjustSize(Float2 const& size) override;

private:
    void            PressKey(int key);
    bool            FilterCharacter(WideChar& ch);
    class UIScroll* GetScroll();
    void            UpdateRows();
    Float2          CalcCursorOffset(int cursor);

    FontHandle      GetFontHandle() const;

    FontHandle         m_Font;
    WideString         m_Text;
    TVector<TextRowW>  m_Rows;
    FontStyle          m_FontStyle;
    Color4             m_TextColor;
    Color4             m_SelectionColor;
    int                m_MaxChars;
    int                m_CharacterFilter;
    int                m_InsertSpacesOnTab;
    STB_TexteditState* m_State;
    int                m_TempCursor;
    int                m_PrevCursorPos = -1;
    Float2             m_CurSize;
    bool               m_bSingleLine : 1;
    bool               m_bReadOnly : 1;
    bool               m_bPassword : 1;
    bool               m_bCtrlEnterForNewLine : 1;
    bool               m_bAllowTabInput : 1;
    bool               m_bAllowUndo : 1;
    bool               m_bCustomCharFilter : 1;
    bool               m_bStartDragging : 1;
    bool               m_bShouldKeepSelection : 1;
    bool               m_bWithWordWrap : 1;
};

HK_NAMESPACE_END
