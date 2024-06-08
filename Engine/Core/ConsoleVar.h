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

#include "String.h"

HK_NAMESPACE_BEGIN

enum CVAR_FLAGS
{
    CVAR_LATCHED    = HK_BIT(0),
    CVAR_READONLY   = HK_BIT(1),
    CVAR_NOSAVE     = HK_BIT(2),
    CVAR_CHEAT      = HK_BIT(3),
    CVAR_SERVERONLY = HK_BIT(4),
    CVAR_NOINGAME   = HK_BIT(5),

    // Internal
    _CVAR_MODIFIED = HK_BIT(6)
};

enum CVAR_ENVIRONMENT_FLAGS
{
    /** Is cheats allowed for the game. This allow to change console variables with flag CVAR_CHEAT */
    CVAR_CHEATS_ALLOWED = HK_BIT(0),

    /** Is game server. This allow to change console variables with flag CVAR_SERVERONLY */
    CVAR_SERVER_ACTIVE  = HK_BIT(1),

    /** Is in game. This blocks changing console variables with flag CVAR_NOINGAME */
    CVAR_INGAME_STATUS  = HK_BIT(2)
};

class ConsoleVar final : public Noncopyable
{
public:
    static int EnvironmentFlags;

    ConsoleVar(GlobalStringView _Name) :
        ConsoleVar(_Name, "0"s, 0, ""s) {}

    ConsoleVar(GlobalStringView _Name, GlobalStringView _Value) :
        ConsoleVar(_Name, _Value, 0, ""s) {}

    ConsoleVar(GlobalStringView _Name, GlobalStringView _Value, uint16_t _Flags) :
        ConsoleVar(_Name, _Value, _Flags, ""s) {}

    ConsoleVar(GlobalStringView _Name, GlobalStringView _Value, uint16_t _Flags, GlobalStringView _Comment);

    ~ConsoleVar();

    char const* GetName() const { return m_Name; }

    char const* GetComment() const { return m_Comment; }

    bool CanChangeValue() const;

    const char* GetDefaultValue() const { return m_DefaultValue; }

    String const& GetLatchedValue() const { return m_LatchedValue; }

    String const& GetString() const { return m_Value; }

    bool GetBool() const { return !!m_I32; }

    int32_t GetInteger() const { return m_I32; }

    float GetFloat() const { return m_F32; }

    bool IsModified() const { return !!(m_Flags & _CVAR_MODIFIED); }

    void MarkModified() { m_Flags |= _CVAR_MODIFIED; }

    void UnmarkModified() { m_Flags &= ~_CVAR_MODIFIED; }

    bool IsReadOnly() const { return !!(m_Flags & CVAR_READONLY); }

    bool IsNoSave() const { return !!(m_Flags & CVAR_NOSAVE); }

    bool IsCheat() const { return !!(m_Flags & CVAR_CHEAT); }

    bool IsServerOnly() const { return !!(m_Flags & CVAR_SERVERONLY); }

    bool IsNoInGame() const { return !!(m_Flags & CVAR_NOINGAME); }

    void SetString(StringView _String);

    void SetBool(bool _Bool);

    void SetInteger(int32_t _Integer);

    void SetFloat(float _Float);

    void ForceString(StringView _String);

    void ForceBool(bool _Bool);

    void ForceInteger(int32_t _Integer);

    void ForceFloat(float _Float);

    void SetLatched();

    void Print();

    ConsoleVar const& operator=(StringView s)
    {
        SetString(s);
        return *this;
    }
    ConsoleVar const& operator=(bool b)
    {
        SetBool(b);
        return *this;
    }
    template <typename T, std::enable_if_t<std::is_integral<T>::value, bool> = true>
    ConsoleVar const& operator=(T i)
    {
        SetInteger(i);
        return *this;
    }
    template <typename T, std::enable_if_t<std::is_floating_point<T>::value, bool> = true>
    ConsoleVar const& operator=(T f)
    {
        SetFloat(f);
        return *this;
    }
    operator bool() const { return GetBool(); }

    ConsoleVar* GetNext() { return m_Next; }

    static ConsoleVar* GlobalVariableList();

    static ConsoleVar* FindVariable(StringView _Name);

    // Internal
    static void AllocateVariables();
    static void FreeVariables();

private:
    char const* const m_Name;
    char const* const m_DefaultValue;
    char const* const m_Comment;
    String m_Value;
    String m_LatchedValue;
    int32_t m_I32{};
    float m_F32{};
    uint16_t m_Flags{};
    ConsoleVar* m_Next;
};

HK_NAMESPACE_END
