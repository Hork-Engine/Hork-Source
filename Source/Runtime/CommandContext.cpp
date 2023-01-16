/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

HK_NAMESPACE_BEGIN

CommandContext::CommandContext()
{
}

CommandContext::~CommandContext()
{
}

void CommandContext::ExecuteCommand(CommandProcessor const& _Proc)
{
    HK_ASSERT(_Proc.GetArgsCount() > 0);

    const char* name = _Proc.GetArg(0);

    for (RuntimeCommand& cmd : Commands)
    {
        if (!Platform::Stricmp(cmd.GetName(), name))
        {
            cmd.Execute(_Proc);
            return;
        }
    }

    ConsoleVar* var;
    if (nullptr != (var = ConsoleVar::FindVariable(name)))
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

    LOG("Unknown command \"{}\"\n", name);
}

void CommandContext::AddCommand(GlobalStringView _Name, TCallback<void(CommandProcessor const&)> const& _Callback, GlobalStringView _Comment)
{
    if (!CommandProcessor::IsValidCommandName(_Name.CStr()))
    {
        LOG("CommandContext::AddCommand: invalid command name\n");
        return;
    }

    if (ConsoleVar::FindVariable(_Name))
    {
        LOG("Name conflict: {} already registered as variable\n", _Name);
        return;
    }

    for (RuntimeCommand& cmd : Commands)
    {
        if (!Platform::Stricmp(cmd.GetName(), _Name.CStr()))
        {
            LOG("Overriding {} command\n", _Name);

            cmd.Override(_Callback, _Comment);
            return;
        }
    }

    // Register new command
    Commands.EmplaceBack(_Name, _Callback, _Comment);
}

void CommandContext::RemoveCommand(StringView _Name)
{
    for (TVector<RuntimeCommand>::Iterator it = Commands.begin(); it != Commands.end(); it++)
    {
        RuntimeCommand& command = *it;

        if (!_Name.Icmp(command.GetName()))
        {
            Commands.Erase(it);
            return;
        }
    }
}

void CommandContext::RemoveCommands()
{
    Commands.Clear();
}

namespace
{

HK_FORCEINLINE bool CompareChar(char _Ch1, char _Ch2)
{
    return ToLower(_Ch1) == ToLower(_Ch2);
}

} // namespace

int CommandContext::CompleteString(StringView Str, String& _Result)
{
    int count = 0;
    const char* s     = Str.Begin();
    auto len   = Str.Size();

    _Result.Clear();

    // skip whitespaces
    while (((*s < 32 && *s > 0) || *s == ' ' || *s == '\t') && len > 0)
    {
        s++;
        len--;
    }

    if (len == 0)
    {
        return 0;
    }

    for (RuntimeCommand const& cmd : Commands)
    {
        if (!Platform::StricmpN(cmd.GetName(), s, len))
        {
            int n;
            int minLen = Math::Min(StringLength(cmd.GetName()), _Result.Length());
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

    for (ConsoleVar* var = ConsoleVar::GlobalVariableList(); var; var = var->GetNext())
    {
        if (!Platform::StricmpN(var->GetName(), s, len))
        {
            int n;
            int minLen = Math::Min(StringLength(var->GetName()), _Result.Length());
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

void CommandContext::Print(StringView Str)
{
    TPodVector<RuntimeCommand*>  cmds;
    TPodVector<ConsoleVar*> vars;

    if (!Str.IsEmpty())
    {
        for (RuntimeCommand& cmd : Commands)
        {
            if (!Str.IcmpN(cmd.GetName(), Str.Size()))
            {
                cmds.Add(&cmd);
            }
        }

        struct
        {
            bool operator()(RuntimeCommand const* _A, RuntimeCommand* _B)
            {
                return Platform::Stricmp(_A->GetName(), _B->GetName()) < 0;
            }
        } CmdSortFunction;

        std::sort(cmds.Begin(), cmds.End(), CmdSortFunction);

        for (ConsoleVar* var = ConsoleVar::GlobalVariableList(); var; var = var->GetNext())
        {
            if (!Str.IcmpN(var->GetName(), Str.Size()))
            {
                vars.Add(var);
            }
        }

        struct
        {
            bool operator()(ConsoleVar const* _A, ConsoleVar* _B)
            {
                return Platform::Stricmp(_A->GetName(), _B->GetName()) < 0;
            }
        } VarSortFunction;

        std::sort(vars.Begin(), vars.End(), VarSortFunction);

        LOG("Total commands found: {}\n"
            "Total variables found: {}\n",
            cmds.Size(), vars.Size());
        for (RuntimeCommand* cmd : cmds)
        {
            if (*cmd->GetComment())
            {
                LOG("    {} ({})\n", cmd->GetName(), cmd->GetComment());
            }
            else
            {
                LOG("    {}\n", cmd->GetName());
            }
        }
        for (ConsoleVar* var : vars)
        {
            if (*var->GetComment())
            {
                LOG("    {} \"{}\" ({})\n", var->GetName(), var->GetValue(), var->GetComment());
            }
            else
            {
                LOG("    {} \"{}\"\n", var->GetName(), var->GetValue());
            }
        }
    }
}

HK_NAMESPACE_END
