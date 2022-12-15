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

#include "ConsoleVar.h"
#include "CommandProcessor.h"
#include "BaseMath.h"
#include "Parse.h"

#include <Platform/Logger.h>

static ConsoleVar* GlobalVars         = nullptr;
static bool         GVariableAllocated = false;

int ConsoleVar::EnvironmentFlags = CVAR_CHEATS_ALLOWED;

ConsoleVar* ConsoleVar::GlobalVariableList()
{
    return GlobalVars;
}

ConsoleVar* ConsoleVar::FindVariable(StringView _Name)
{
    for (ConsoleVar* var = GlobalVars; var; var = var->GetNext())
    {
        if (!_Name.Icmp(var->GetName()))
        {
            return var;
        }
    }
    return nullptr;
}

void ConsoleVar::AllocateVariables()
{
    for (ConsoleVar* var = GlobalVars; var; var = var->m_Next)
    {
        var->m_Value = var->m_DefaultValue;
        var->m_F32   = Core::ParseCvar(var->m_Value);
        var->m_I32 = static_cast<int32_t>(var->m_F32);
    }
    GVariableAllocated = true;
}

void ConsoleVar::FreeVariables()
{
    for (ConsoleVar* var = GlobalVars; var; var = var->m_Next)
    {
        var->m_Value.Free();
        var->m_LatchedValue.Free();
    }
    GlobalVars = nullptr;
}

ConsoleVar::ConsoleVar(GlobalStringView _Name, GlobalStringView _Value, uint16_t _Flags, GlobalStringView _Comment) :
    m_Name(_Name.CStr()), m_DefaultValue(_Value.CStr()), m_Comment(_Comment.CStr()), m_Flags(_Flags)
{
    HK_ASSERT(!GVariableAllocated);
    HK_ASSERT(CommandProcessor::IsValidCommandName(m_Name));

    ConsoleVar* head = GlobalVars;
    m_Next = head;
    GlobalVars        = this;
}

ConsoleVar::~ConsoleVar()
{
    ConsoleVar* prev = nullptr;
    for (ConsoleVar* var = GlobalVars; var; var = var->m_Next)
    {
        if (var == this)
        {
            if (prev)
            {
                prev->m_Next = var->m_Next;
            }
            else
            {
                GlobalVars = var->m_Next;
            }
            break;
        }
        prev = var;
    }
}

bool ConsoleVar::CanChangeValue() const
{
    if (m_Flags & CVAR_READONLY)
    {
        LOG("{} is readonly\n", m_Name);
        return false;
    }

    if ((m_Flags & CVAR_CHEAT) && !(EnvironmentFlags & CVAR_CHEATS_ALLOWED))
    {
        LOG("{} is cheat protected\n", m_Name);
        return false;
    }

    if ((m_Flags & CVAR_SERVERONLY) && !(EnvironmentFlags & CVAR_SERVER_ACTIVE))
    {
        LOG("{} can be changed by server only\n", m_Name);
        return false;
    }

    if ((m_Flags & CVAR_NOINGAME) && (EnvironmentFlags & CVAR_INGAME_STATUS))
    {
        LOG("{} can't be changed in game\n", m_Name);
        return false;
    }

    return true;
}

void ConsoleVar::SetString(StringView _String)
{
    if (!CanChangeValue())
    {
        return;
    }

    bool bApply = m_Value.Cmp(_String) != 0;
    if (!bApply)
    {
        // Value is not changed
        return;
    }

    if (m_Flags & CVAR_LATCHED)
    {
        LOG("{} restart required to change value\n", m_Name);

        m_LatchedValue = _String;
    }
    else
    {
        ForceString(_String);
    }
}

void ConsoleVar::SetBool(bool _Bool)
{
    SetString(_Bool ? "1" : "0");
}

void ConsoleVar::SetInteger(int32_t _Integer)
{
    SetString(Core::ToString(_Integer));
}

void ConsoleVar::SetFloat(float _Float)
{
    SetString(Core::ToString(_Float));
}

void ConsoleVar::ForceString(StringView _String)
{
    m_Value = _String;
    m_F32 = Core::ParseCvar(m_Value);
    m_I32 = static_cast<int32_t>(m_F32);
    m_LatchedValue.Clear();
    MarkModified();
}

void ConsoleVar::ForceBool(bool _Bool)
{
    ForceString(_Bool ? "1" : "0");
}

void ConsoleVar::ForceInteger(int32_t _Integer)
{
    ForceString(Core::ToString(_Integer));
}

void ConsoleVar::ForceFloat(float _Float)
{
    ForceString(Core::ToString(_Float));
}

void ConsoleVar::SetLatched()
{
    if (!(m_Flags & CVAR_LATCHED))
    {
        return;
    }

    if (m_LatchedValue.IsEmpty())
    {
        return;
    }

    if (!CanChangeValue())
    {
        return;
    }

    ForceString(m_LatchedValue);
}

void ConsoleVar::Print()
{
    LOG("    {}", m_Name);

    if (*m_Comment)
    {
        LOG(" ({})", m_Comment);
    }

    LOG("\n        [CURRENT \"{}\"]  [DEFAULT \"{}\"]", m_Value, m_DefaultValue);

    if ((m_Flags & CVAR_LATCHED) && !m_LatchedValue.IsEmpty())
    {
        LOG("  [LATCHED \"{}\"]\n", m_LatchedValue);
    }
    else
    {
        LOG("\n");
    }

    if (m_Flags & (CVAR_LATCHED | CVAR_READONLY | CVAR_NOSAVE | CVAR_CHEAT | CVAR_SERVERONLY | CVAR_NOINGAME))
    {
        LOG("        [FLAGS");
        if (m_Flags & CVAR_LATCHED) { LOG(" LATCHED"); }
        if (m_Flags & CVAR_READONLY) { LOG(" READONLY"); }
        if (m_Flags & CVAR_NOSAVE) { LOG(" NOSAVE"); }
        if (m_Flags & CVAR_CHEAT) { LOG(" CHEAT"); }
        if (m_Flags & CVAR_SERVERONLY) { LOG(" SERVERONLY"); }
        if (m_Flags & CVAR_NOINGAME) { LOG(" NOINGAME"); }
        LOG("]\n");
    }
}
