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
    for (ConsoleVar* var = GlobalVars; var; var = var->Next)
    {
        var->Value = var->DefaultValue;
        var->F32   = Core::ParseCvar(var->Value);
        var->I32   = static_cast<int32_t>(var->F32);
    }
    GVariableAllocated = true;
}

void ConsoleVar::FreeVariables()
{
    for (ConsoleVar* var = GlobalVars; var; var = var->Next)
    {
        var->Value.Free();
        var->LatchedValue.Free();
    }
    GlobalVars = nullptr;
}

ConsoleVar::ConsoleVar(GlobalStringView _Name, GlobalStringView _Value, uint16_t _Flags, GlobalStringView _Comment) :
    Name(_Name.CStr()), DefaultValue(_Value.CStr()), Comment(_Comment.CStr()), Flags(_Flags)
{
    HK_ASSERT(!GVariableAllocated);
    HK_ASSERT(CommandProcessor::IsValidCommandName(Name));

    ConsoleVar* head = GlobalVars;
    Next              = head;
    GlobalVars        = this;
}

ConsoleVar::~ConsoleVar()
{
    ConsoleVar* prev = nullptr;
    for (ConsoleVar* var = GlobalVars; var; var = var->Next)
    {
        if (var == this)
        {
            if (prev)
            {
                prev->Next = var->Next;
            }
            else
            {
                GlobalVars = var->Next;
            }
            break;
        }
        prev = var;
    }
}

bool ConsoleVar::CanChangeValue() const
{
    if (Flags & CVAR_READONLY)
    {
        LOG("{} is readonly\n", Name);
        return false;
    }

    if ((Flags & CVAR_CHEAT) && !(EnvironmentFlags & CVAR_CHEATS_ALLOWED))
    {
        LOG("{} is cheat protected\n", Name);
        return false;
    }

    if ((Flags & CVAR_SERVERONLY) && !(EnvironmentFlags & CVAR_SERVER_ACTIVE))
    {
        LOG("{} can be changed by server only\n", Name);
        return false;
    }

    if ((Flags & CVAR_NOINGAME) && (EnvironmentFlags & CVAR_INGAME_STATUS))
    {
        LOG("{} can't be changed in game\n", Name);
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

    bool bApply = Value.Cmp(_String) != 0;
    if (!bApply)
    {
        // Value is not changed
        return;
    }

    if (Flags & CVAR_LATCHED)
    {
        LOG("{} restart required to change value\n", Name);

        LatchedValue = _String;
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
    Value = _String;
    F32   = Core::ParseCvar(Value);
    I32   = static_cast<int32_t>(F32);
    LatchedValue.Clear();
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
    if (!(Flags & CVAR_LATCHED))
    {
        return;
    }

    if (LatchedValue.IsEmpty())
    {
        return;
    }

    if (!CanChangeValue())
    {
        return;
    }

    ForceString(LatchedValue);
}

void ConsoleVar::Print()
{
    LOG("    {}", Name);

    if (*Comment)
    {
        LOG(" ({})", Comment);
    }

    LOG("\n        [CURRENT \"{}\"]  [DEFAULT \"{}\"]", Value, DefaultValue);

    if ((Flags & CVAR_LATCHED) && !LatchedValue.IsEmpty())
    {
        LOG("  [LATCHED \"{}\"]\n", LatchedValue);
    }
    else
    {
        LOG("\n");
    }

    if (Flags & (CVAR_LATCHED | CVAR_READONLY | CVAR_NOSAVE | CVAR_CHEAT | CVAR_SERVERONLY | CVAR_NOINGAME))
    {
        LOG("        [FLAGS");
        if (Flags & CVAR_LATCHED) { LOG(" LATCHED"); }
        if (Flags & CVAR_READONLY) { LOG(" READONLY"); }
        if (Flags & CVAR_NOSAVE) { LOG(" NOSAVE"); }
        if (Flags & CVAR_CHEAT) { LOG(" CHEAT"); }
        if (Flags & CVAR_SERVERONLY) { LOG(" SERVERONLY"); }
        if (Flags & CVAR_NOINGAME) { LOG(" NOINGAME"); }
        LOG("]\n");
    }
}
