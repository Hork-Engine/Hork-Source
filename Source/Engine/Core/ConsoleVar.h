/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "String.h"
#include "CompileTimeString.h"

enum CVAR_FLAGS
{
    CVAR_LATCHED    = AN_BIT(0),
    CVAR_READONLY   = AN_BIT(1),
    CVAR_NOSAVE     = AN_BIT(2),
    CVAR_CHEAT      = AN_BIT(3),
    CVAR_SERVERONLY = AN_BIT(4),
    CVAR_NOINGAME   = AN_BIT(5),

    // Internal
    _CVAR_MODIFIED = AN_BIT(6)
};

enum CVAR_ENVIRONMENT_FLAGS
{
    /** Is cheats allowed for the game. This allow to change console variables with flag CVAR_CHEAT */
    CVAR_CHEATS_ALLOWED = AN_BIT(0),

    /** Is game server. This allow to change console variables with flag CVAR_SERVERONLY */
    CVAR_SERVER_ACTIVE  = AN_BIT(1),

    /** Is in game. This blocks changing console variables with flag CVAR_NOINGAME */
    CVAR_INGAME_STATUS  = AN_BIT(2)
};

class AConsoleVar final
{
    AN_FORBID_COPY(AConsoleVar)

public:
    static int EnvironmentFlags;

    template <char... NameChars>
    AConsoleVar(TCompileTimeString<NameChars...> const& _Name) :
        AConsoleVar(_Name.CStr(), "0", 0, "") {}

    template <char... NameChars, char... ValueChars>
    AConsoleVar(TCompileTimeString<NameChars...> const& _Name, TCompileTimeString<ValueChars...> const& _Value) :
        AConsoleVar(_Name.CStr(), _Value.CStr(), 0, "") {}

    template <char... NameChars, char... ValueChars>
    AConsoleVar(TCompileTimeString<NameChars...> const& _Name, TCompileTimeString<ValueChars...> const& _Value, uint16_t _Flags) :
        AConsoleVar(_Name.CStr(), _Value.CStr(), _Flags, "") {}

    template <char... NameChars, char... ValueChars, char... CommentChars>
    AConsoleVar(TCompileTimeString<NameChars...> const& _Name, TCompileTimeString<ValueChars...> const& _Value, uint16_t _Flags, TCompileTimeString<CommentChars...> const& _Comment) :
        AConsoleVar(_Name.CStr(), _Value.CStr(), _Flags, _Comment.CStr()) {}

    ~AConsoleVar();

    char const* GetName() const { return Name; }

    char const* GetComment() const { return Comment; }

    bool CanChangeValue() const;

    const char* GetDefaultValue() const { return DefaultValue; }

    AString const& GetValue() const { return Value; }

    AString const& GetLatchedValue() const { return LatchedValue; }

    bool GetBool() const { return !!I32; }

    int32_t GetInteger() const { return I32; }

    float GetFloat() const { return F32; }

    bool IsModified() const { return !!(Flags & _CVAR_MODIFIED); }

    void MarkModified() { Flags |= _CVAR_MODIFIED; }

    void UnmarkModified() { Flags &= ~_CVAR_MODIFIED; }

    bool IsReadOnly() const { return !!(Flags & CVAR_READONLY); }

    bool IsNoSave() const { return !!(Flags & CVAR_NOSAVE); }

    bool IsCheat() const { return !!(Flags & CVAR_CHEAT); }

    bool IsServerOnly() const { return !!(Flags & CVAR_SERVERONLY); }

    bool IsNoInGame() const { return !!(Flags & CVAR_NOINGAME); }

    void SetString(const char* _String);

    void SetString(AString const& _String);

    void SetBool(bool _Bool);

    void SetInteger(int32_t _Integer);

    void SetFloat(float _Float);

    void ForceString(const char* _String);

    void ForceString(AString const& _String);

    void ForceBool(bool _Bool);

    void ForceInteger(int32_t _Integer);

    void ForceFloat(float _Float);

    void SetLatched();

    void Print();

    AConsoleVar const& operator=(const char* _String)
    {
        SetString(_String);
        return *this;
    }
    AConsoleVar const& operator=(AString const& _String)
    {
        SetString(_String);
        return *this;
    }
    AConsoleVar const& operator=(bool _Bool)
    {
        SetBool(_Bool);
        return *this;
    }
    AConsoleVar const& operator=(int32_t _Integer)
    {
        SetInteger(_Integer);
        return *this;
    }
    AConsoleVar const& operator=(float _Float)
    {
        SetFloat(_Float);
        return *this;
    }
    operator bool() const { return GetBool(); }

    AConsoleVar* GetNext() { return Next; }

    static AConsoleVar* GlobalVariableList();

    static AConsoleVar* FindVariable(const char* _Name);

    // Internal
    static void AllocateVariables();
    static void FreeVariables();

private:
    AConsoleVar(const char* _Name,
                     const char* _Value,
                     uint16_t    _Flags,
                     const char* _Comment);

    char const* const Name;
    char const* const DefaultValue;
    char const* const Comment;
    AString           Value;
    AString           LatchedValue;
    int32_t           I32;
    float             F32;
    uint16_t          Flags;
    AConsoleVar* Next;
};
