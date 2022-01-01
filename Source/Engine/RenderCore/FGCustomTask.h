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

#include "FGRenderTask.h"

namespace RenderCore
{

class ACustomTask;

using ATaskFunction = std::function<void(ACustomTask const&)>;

class ACustomTask : public FGRenderTask<ACustomTask>
{
    using Super = FGRenderTask<ACustomTask>;

public:
    ACustomTask(AFrameGraph* pFrameGraph, const char* Name) :
        FGRenderTask(pFrameGraph, Name, FG_RENDER_TASK_PROXY_TYPE_CUSTOM)
    {
    }

    ACustomTask(ACustomTask const&) = delete;
    ACustomTask(ACustomTask&&)      = default;
    ACustomTask& operator=(ACustomTask const&) = delete;
    ACustomTask& operator=(ACustomTask&&) = default;

    ACustomTask& SetFunction(ATaskFunction RecordFunction)
    {
        Function = RecordFunction;
        return *this;
    }

    //private:
    ATaskFunction Function;
};

} // namespace RenderCore
