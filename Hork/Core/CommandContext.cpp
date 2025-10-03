/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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
#include "Logger.h"

HK_NAMESPACE_BEGIN

CommandContext::CommandContext()
{
}

CommandContext::~CommandContext()
{
}

void CommandContext::ExecuteCommand(CommandProcessor const& proc)
{
    HK_ASSERT(proc.GetArgsCount() > 0);

    const char* name = proc.GetArg(0);

    for (RuntimeCommand& cmd : Commands)
    {
        if (!Core::Stricmp(cmd.GetName(), name))
        {
            cmd.Execute(proc);
            return;
        }
    }

    ConsoleVar* var;
    if (nullptr != (var = ConsoleVar::sFindVariable(name)))
    {
        if (proc.GetArgsCount() < 2)
        {
            var->Print();
        }
        else
        {
            var->SetString(proc.GetArg(1));
        }
        return;
    }

    LOG("Unknown command \"{}\"\n", name);
}

void CommandContext::AddCommand(GlobalStringView name, Delegate<void(CommandProcessor const&)> const& callback, GlobalStringView comment)
{
    if (!CommandProcessor::sIsValidCommandName(name.CStr()))
    {
        LOG("CommandContext::AddCommand: invalid command name\n");
        return;
    }

    if (ConsoleVar::sFindVariable(name))
    {
        LOG("Name conflict: {} already registered as variable\n", name);
        return;
    }

    for (RuntimeCommand& cmd : Commands)
    {
        if (!Core::Stricmp(cmd.GetName(), name.CStr()))
        {
            LOG("Overriding {} command\n", name);

            cmd.Override(callback, comment);
            return;
        }
    }

    // Register new command
    Commands.EmplaceBack(name, callback, comment);
}

void CommandContext::RemoveCommand(StringView name)
{
    for (Vector<RuntimeCommand>::Iterator it = Commands.begin(); it != Commands.end(); it++)
    {
        RuntimeCommand& command = *it;

        if (!name.Icmp(command.GetName()))
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

int CommandContext::CompleteString(StringView str, String& result)
{
    int count = 0;
    const char* s = str.Begin();
    auto len = str.Size();

    result.Clear();

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
        if (!Core::StricmpN(cmd.GetName(), s, len))
        {
            int n;
            int minLen = Math::Min(StringLength(cmd.GetName()), result.Length());
            for (n = 0; n < minLen && CompareChar(cmd.GetName()[n], result[n]); n++) {}
            if (n == 0)
            {
                result = cmd.GetName();
            }
            else
            {
                result.Resize(n);
            }
            count++;
        }
    }

    for (ConsoleVar* var = ConsoleVar::sGlobalVariableList(); var; var = var->GetNext())
    {
        if (!Core::StricmpN(var->GetName(), s, len))
        {
            int n;
            int minLen = Math::Min(StringLength(var->GetName()), result.Length());
            for (n = 0; n < minLen && CompareChar(var->GetName()[n], result[n]); n++) {}
            if (n == 0)
            {
                result = var->GetName();
            }
            else
            {
                result.Resize(n);
            }
            count++;
        }
    }

    return count;
}

void CommandContext::Print(StringView str)
{
    Vector<RuntimeCommand*>  cmds;
    Vector<ConsoleVar*> vars;

    if (!str.IsEmpty())
    {
        for (RuntimeCommand& cmd : Commands)
        {
            if (!str.IcmpN(cmd.GetName(), str.Size()))
            {
                cmds.Add(&cmd);
            }
        }

        struct
        {
            bool operator()(RuntimeCommand const* _A, RuntimeCommand* _B)
            {
                return Core::Stricmp(_A->GetName(), _B->GetName()) < 0;
            }
        } CmdSortFunction;

        std::sort(cmds.Begin(), cmds.End(), CmdSortFunction);

        for (ConsoleVar* var = ConsoleVar::sGlobalVariableList(); var; var = var->GetNext())
        {
            if (!str.IcmpN(var->GetName(), str.Size()))
            {
                vars.Add(var);
            }
        }

        struct
        {
            bool operator()(ConsoleVar const* _A, ConsoleVar* _B)
            {
                return Core::Stricmp(_A->GetName(), _B->GetName()) < 0;
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
                LOG("    {} \"{}\" ({})\n", var->GetName(), var->GetString(), var->GetComment());
            }
            else
            {
                LOG("    {} \"{}\"\n", var->GetName(), var->GetString());
            }
        }
    }
}

HK_NAMESPACE_END
