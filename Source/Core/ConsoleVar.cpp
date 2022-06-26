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

#include <Platform/Logger.h>

static AConsoleVar* GlobalVars         = nullptr;
static bool         GVariableAllocated = false;

int AConsoleVar::EnvironmentFlags = CVAR_CHEATS_ALLOWED;

AConsoleVar* AConsoleVar::GlobalVariableList()
{
    return GlobalVars;
}

AConsoleVar* AConsoleVar::FindVariable(AStringView _Name)
{
    for (AConsoleVar* var = GlobalVars; var; var = var->GetNext())
    {
        if (!_Name.Icmp(var->GetName()))
        {
            return var;
        }
    }
    return nullptr;
}

void AConsoleVar::AllocateVariables()
{
    for (AConsoleVar* var = GlobalVars; var; var = var->Next)
    {
        var->Value = var->DefaultValue;
        var->I32   = Core::ParseInt32(var->Value);
        var->F32   = Core::ParseFloat(var->Value);
    }
    GVariableAllocated = true;
}

void AConsoleVar::FreeVariables()
{
    for (AConsoleVar* var = GlobalVars; var; var = var->Next)
    {
        var->Value.Free();
        var->LatchedValue.Free();
    }
    GlobalVars = nullptr;
}

AConsoleVar::AConsoleVar(AGlobalStringView _Name, AGlobalStringView _Value, uint16_t _Flags, AGlobalStringView _Comment) :
    Name(_Name.CStr()), DefaultValue(_Value.CStr()), Comment(_Comment.CStr()), Flags(_Flags)
{
    HK_ASSERT(!GVariableAllocated);
    HK_ASSERT(ACommandProcessor::IsValidCommandName(Name));

    AConsoleVar* head = GlobalVars;
    Next              = head;
    GlobalVars        = this;
}

AConsoleVar::~AConsoleVar()
{
    AConsoleVar* prev = nullptr;
    for (AConsoleVar* var = GlobalVars; var; var = var->Next)
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

bool AConsoleVar::CanChangeValue() const
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

void AConsoleVar::SetString(AStringView _String)
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

void AConsoleVar::SetBool(bool _Bool)
{
    SetString(_Bool ? "1" : "0");
}

void AConsoleVar::SetInteger(int32_t _Integer)
{
    SetString(Core::ToString(_Integer));
}

void AConsoleVar::SetFloat(float _Float)
{
    SetString(Core::ToString(_Float));
}

void AConsoleVar::ForceString(AStringView _String)
{
    Value = _String;
    I32   = Core::ParseInt32(Value);
    F32   = Core::ParseFloat(Value);
    LatchedValue.Clear();
    MarkModified();
}

void AConsoleVar::ForceBool(bool _Bool)
{
    ForceString(_Bool ? "1" : "0");
}

void AConsoleVar::ForceInteger(int32_t _Integer)
{
    ForceString(Core::ToString(_Integer));
}

void AConsoleVar::ForceFloat(float _Float)
{
    ForceString(Core::ToString(_Float));
}

void AConsoleVar::SetLatched()
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

void AConsoleVar::Print()
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
