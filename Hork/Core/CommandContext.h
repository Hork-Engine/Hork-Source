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

#include <Hork/Core/CommandProcessor.h>
#include <Hork/Core/ConsoleVar.h>
#include <Hork/Core/Delegate.h>
#include <Hork/Core/Containers/Vector.h>

HK_NAMESPACE_BEGIN

class CommandContext : public ICommandContext, public Noncopyable
{
public:
    CommandContext();
    ~CommandContext();

    void AddCommand(GlobalStringView name, Delegate<void(CommandProcessor const&)> const& callback, GlobalStringView comment = ""_s);

    void RemoveCommand(StringView name);

    void RemoveCommands();

    int CompleteString(StringView str, String& result);

    void Print(StringView str);

protected:
    //
    // ICommandContext implementation
    //

    void ExecuteCommand(CommandProcessor const& proc) override;

private:
    class RuntimeCommand
    {
    public:
        RuntimeCommand(GlobalStringView name, Delegate<void(CommandProcessor const&)> const& callback, GlobalStringView comment) :
            Name(name.CStr()), Comment(comment.CStr()), Callback(callback)
        {}

        void Override(Delegate<void(CommandProcessor const&)> const& callback, GlobalStringView comment)
        {
            Comment  = comment.CStr();
            Callback = callback;
        }

        const char* GetName() const { return Name; }

        const char* GetComment() const { return Comment; }

        void Execute(CommandProcessor const& proc) { Callback.Invoke(proc); }

    private:
        const char*                             Name;
        const char*                             Comment;
        Delegate<void(CommandProcessor const&)> Callback;
    };

    Vector<RuntimeCommand> Commands;
};

HK_NAMESPACE_END
