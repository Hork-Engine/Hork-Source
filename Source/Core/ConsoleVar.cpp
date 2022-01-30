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

AConsoleVar* AConsoleVar::FindVariable(const char* _Name)
{
    for (AConsoleVar* var = GlobalVars; var; var = var->GetNext())
    {
        if (!Platform::Stricmp(var->GetName(), _Name))
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
        var->I32   = Math::ToInt<int32_t>(var->Value);
        var->F32   = Math::ToFloat(var->Value);
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

AConsoleVar::AConsoleVar(const char* _Name,
                         const char* _Value,
                         uint16_t    _Flags,
                         const char* _Comment) :
    Name(_Name ? _Name : ""), DefaultValue(_Value ? _Value : ""), Comment(_Comment ? _Comment : ""), Flags(_Flags)
{
    AN_ASSERT(!GVariableAllocated);
    AN_ASSERT(ACommandProcessor::IsValidCommandName(Name));

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
        GLogger.Printf("%s is readonly\n", Name);
        return false;
    }

    if ((Flags & CVAR_CHEAT) && !(EnvironmentFlags & CVAR_CHEATS_ALLOWED))
    {
        GLogger.Printf("%s is cheat protected\n", Name);
        return false;
    }

    if ((Flags & CVAR_SERVERONLY) && !(EnvironmentFlags & CVAR_SERVER_ACTIVE))
    {
        GLogger.Printf("%s can be changed by server only\n", Name);
        return false;
    }

    if ((Flags & CVAR_NOINGAME) && (EnvironmentFlags & CVAR_INGAME_STATUS))
    {
        GLogger.Printf("%s can't be changed in game\n", Name);
        return false;
    }

    return true;
}

void AConsoleVar::SetString(const char* _String)
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
        GLogger.Printf("%s restart required to change value\n", Name);

        LatchedValue = _String;
    }
    else
    {
        ForceString(_String);
    }
}

void AConsoleVar::SetString(AString const& _String)
{
    SetString(_String.CStr());
}

void AConsoleVar::SetBool(bool _Bool)
{
    SetString(_Bool ? "1" : "0");
}

void AConsoleVar::SetInteger(int32_t _Integer)
{
    SetString(Math::ToString(_Integer));
}

void AConsoleVar::SetFloat(float _Float)
{
    SetString(Math::ToString(_Float));
}

void AConsoleVar::ForceString(const char* _String)
{
    Value = _String;
    I32   = Math::ToInt<int32_t>(Value);
    F32   = Math::ToFloat(Value);
    LatchedValue.Clear();
    MarkModified();
}

void AConsoleVar::ForceString(AString const& _String)
{
    ForceString(_String.CStr());
}

void AConsoleVar::ForceBool(bool _Bool)
{
    ForceString(_Bool ? "1" : "0");
}

void AConsoleVar::ForceInteger(int32_t _Integer)
{
    ForceString(Math::ToString(_Integer));
}

void AConsoleVar::ForceFloat(float _Float)
{
    ForceString(Math::ToString(_Float));
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
    GLogger.Printf("    %s", Name);

    if (*Comment)
    {
        GLogger.Printf(" (%s)", Comment);
    }

    GLogger.Printf("\n        [CURRENT \"%s\"]  [DEFAULT \"%s\"]", Value.CStr(), DefaultValue);

    if ((Flags & CVAR_LATCHED) && !LatchedValue.IsEmpty())
    {
        GLogger.Printf("  [LATCHED \"%s\"]\n", LatchedValue.CStr());
    }
    else
    {
        GLogger.Print("\n");
    }

    if (Flags & (CVAR_LATCHED | CVAR_READONLY | CVAR_NOSAVE | CVAR_CHEAT | CVAR_SERVERONLY | CVAR_NOINGAME))
    {
        GLogger.Print("        [FLAGS");
        if (Flags & CVAR_LATCHED) { GLogger.Print(" LATCHED"); }
        if (Flags & CVAR_READONLY) { GLogger.Print(" READONLY"); }
        if (Flags & CVAR_NOSAVE) { GLogger.Print(" NOSAVE"); }
        if (Flags & CVAR_CHEAT) { GLogger.Print(" CHEAT"); }
        if (Flags & CVAR_SERVERONLY) { GLogger.Print(" SERVERONLY"); }
        if (Flags & CVAR_NOINGAME) { GLogger.Print(" NOINGAME"); }
        GLogger.Print("]\n");
    }
}
