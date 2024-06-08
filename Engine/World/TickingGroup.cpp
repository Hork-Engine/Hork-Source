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

#include "TickingGroup.h"
#include "WorldTick.h"

HK_NAMESPACE_BEGIN

void TickingGroup::AddFunction(TickFunction const& f)
{
    Function& function = m_FunctionList.EmplaceBack();

    function.Desc = f.Desc;
    function.Delegate = f.Delegate;
    function.OwnerTypeID = f.OwnerTypeID;
    function.TraverseID = m_TraverseID;

    m_RebuildRequired = true;
}

void TickingGroup::Dispatch(WorldTick& tick)
{
    if (m_RebuildRequired)
        Rebuild();

    //LOG("---------------------------------------\n");

    for (uint32_t index = 0; index < m_ExecutionOrder.Size(); ++index)
    {
        auto& f = m_FunctionList[m_ExecutionOrder[index]];
        if (!tick.IsPaused || f.Desc.TickEvenWhenPaused)
        {
            //LOG("EXECUTE: {}\n", f.Function.Desc.Name);
            f.Delegate.Invoke();
        }
    }
}

void TickingGroup::Rebuild()
{
    struct DepthFirstTranversal
    {
        TVector<Function>&  FunctionList;
        TVector<uint32_t>&  ExecutionOrder;
        uint32_t            TraverseID;

        DepthFirstTranversal(TVector<Function>& functionList,
            TVector<uint32_t>& executionOrder) :
            FunctionList(functionList),
            ExecutionOrder(executionOrder)
        {}


        void Traverse(uint32_t index)
        {
            Function& function = FunctionList[index];

            if (function.TraverseID == TraverseID)
                return;

            function.TraverseID = TraverseID;

            for (auto prerequisite : function.Desc.Prerequisites)
            {
                for (uint32_t index2 = 0; index2 < FunctionList.Size(); ++index2)
                    if (FunctionList[index2].OwnerTypeID == prerequisite)
                        Traverse(index2);
            }

            ExecutionOrder.Add(index);
        }

        void Traverse()
        {
            uint32_t functionCount = FunctionList.Size();
            uint32_t index = 0;

            ExecutionOrder.Clear();
            ExecutionOrder.Reserve(functionCount);
            while (ExecutionOrder.Size() < functionCount)
            {
                Traverse(index++);
            }
        }
    };

    DepthFirstTranversal traverse(m_FunctionList, m_ExecutionOrder);
    traverse.TraverseID = ++m_TraverseID;
    traverse.Traverse();

    m_RebuildRequired = false;
}

HK_NAMESPACE_END
