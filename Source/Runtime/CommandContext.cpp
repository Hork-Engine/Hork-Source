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


#include "CommandContext.h"
#include <Platform/Logger.h>

ACommandContext::ACommandContext()
{
}

ACommandContext::~ACommandContext()
{
}

void ACommandContext::ExecuteCommand(ACommandProcessor const& _Proc)
{
    HK_ASSERT(_Proc.GetArgsCount() > 0);

    const char* name = _Proc.GetArg(0);

    for (ARuntimeCommand& cmd : Commands)
    {
        if (!cmd.GetName().Icmp(name))
        {
            cmd.Execute(_Proc);
            return;
        }
    }

    AConsoleVar* var;
    if (nullptr != (var = AConsoleVar::FindVariable(name)))
    {
        if (_Proc.GetArgsCount() < 2)
        {
            var->Print();
        }
        else
        {
            var->SetString(_Proc.GetArg(1));
        }
        return;
    }

    GLogger.Printf("Unknown command \"%s\"\n", name);
}

void ACommandContext::AddCommand(const char* _Name, TCallback<void(ACommandProcessor const&)> const& _Callback, const char* _Comment)
{
    if (!ACommandProcessor::IsValidCommandName(_Name))
    {
        GLogger.Printf("ACommandContext::AddCommand: invalid command name\n");
        return;
    }

    if (AConsoleVar::FindVariable(_Name))
    {
        GLogger.Printf("Name conflict: %s already registered as variable\n", _Name);
        return;
    }

    for (ARuntimeCommand& cmd : Commands)
    {
        if (!cmd.GetName().Icmp(_Name))
        {
            GLogger.Printf("Overriding %s command\n", _Name);

            cmd.Override(_Callback, _Comment);
            return;
        }
    }

    // Register new command
    Commands.emplace_back(_Name, _Callback, _Comment);
}

void ACommandContext::RemoveCommand(const char* _Name)
{
    for (TStdVector<ARuntimeCommand>::iterator it = Commands.begin(); it != Commands.end(); it++)
    {
        ARuntimeCommand& command = *it;

        if (!command.GetName().Icmp(_Name))
        {
            Commands.erase(it);
            return;
        }
    }
}

void ACommandContext::RemoveCommands()
{
    Commands.Clear();
}

namespace
{

HK_FORCEINLINE bool CompareChar(char _Ch1, char _Ch2)
{
    return tolower(_Ch1) == tolower(_Ch2);
}

} // namespace

int ACommandContext::CompleteString(const char* _Str, int _StrLen, AString& _Result)
{
    int count = 0;

    _Result.Clear();

    // skip whitespaces
    while (((*_Str < 32 && *_Str > 0) || *_Str == ' ' || *_Str == '\t') && _StrLen > 0)
    {
        _Str++;
        _StrLen--;
    }

    if (_StrLen <= 0 || !*_Str)
    {
        return 0;
    }

    for (ARuntimeCommand const& cmd : Commands)
    {
        if (!cmd.GetName().IcmpN(_Str, _StrLen))
        {
            int n;
            int minLen = Math::Min(cmd.GetName().Length(), _Result.Length());
            for (n = 0; n < minLen && CompareChar(cmd.GetName()[n], _Result[n]); n++) {}
            if (n == 0)
            {
                _Result = cmd.GetName();
            }
            else
            {
                _Result.Resize(n);
            }
            count++;
        }
    }

    for (AConsoleVar* var = AConsoleVar::GlobalVariableList(); var; var = var->GetNext())
    {
        if (!Platform::StricmpN(var->GetName(), _Str, _StrLen))
        {
            int n;
            int minLen = Math::Min(Platform::Strlen(var->GetName()), _Result.Length());
            for (n = 0; n < minLen && CompareChar(var->GetName()[n], _Result[n]); n++) {}
            if (n == 0)
            {
                _Result = var->GetName();
            }
            else
            {
                _Result.Resize(n);
            }
            count++;
        }
    }

    return count;
}

void ACommandContext::Print(const char* _Str, int _StrLen)
{
    TPodVector<ARuntimeCommand*>  cmds;
    TPodVector<AConsoleVar*> vars;

    if (_StrLen > 0)
    {
        for (ARuntimeCommand& cmd : Commands)
        {
            if (!cmd.GetName().IcmpN(_Str, _StrLen))
            {
                cmds.Append(&cmd);
            }
        }

        struct
        {
            bool operator()(ARuntimeCommand const* _A, ARuntimeCommand* _B)
            {
                return _A->GetName().Icmp(_B->GetName()) < 0;
            }
        } CmdSortFunction;

        std::sort(cmds.Begin(), cmds.End(), CmdSortFunction);

        for (AConsoleVar* var = AConsoleVar::GlobalVariableList(); var; var = var->GetNext())
        {
            if (!Platform::StricmpN(var->GetName(), _Str, _StrLen))
            {
                vars.Append(var);
            }
        }

        struct
        {
            bool operator()(AConsoleVar const* _A, AConsoleVar* _B)
            {
                return Platform::Stricmp(_A->GetName(), _B->GetName()) < 0;
            }
        } VarSortFunction;

        std::sort(vars.Begin(), vars.End(), VarSortFunction);

        GLogger.Printf("Total commands found: %d\n"
                       "Total variables found: %d\n",
                       cmds.Size(), vars.Size());
        for (ARuntimeCommand* cmd : cmds)
        {
            if (!cmd->GetComment().IsEmpty())
            {
                GLogger.Printf("    %s (%s)\n", cmd->GetName().CStr(), cmd->GetComment().CStr());
            }
            else
            {
                GLogger.Printf("    %s\n", cmd->GetName().CStr());
            }
        }
        for (AConsoleVar* var : vars)
        {
            if (*var->GetComment())
            {
                GLogger.Printf("    %s \"%s\" (%s)\n", var->GetName(), var->GetValue().CStr(), var->GetComment());
            }
            else
            {
                GLogger.Printf("    %s \"%s\"\n", var->GetName(), var->GetValue().CStr());
            }
        }
    }
}
