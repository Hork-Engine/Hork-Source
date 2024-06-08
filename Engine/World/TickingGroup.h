#pragma once

#include "TickFunction.h"

HK_NAMESPACE_BEGIN

struct TickingGroup
{
private:
    struct Function
    {
        TickFunctionDesc    Desc;
        Delegate<void()>    Delegate;
        uint32_t            OwnerTypeID;
        uint32_t            TraverseID;
    };

    TVector<Function>       m_FunctionList;
    TVector<uint32_t>       m_ExecutionOrder;
    bool                    m_RebuildRequired{};
    uint32_t                m_TraverseID{};

public:
    void                    AddFunction(TickFunction const& f);
    void                    Dispatch(struct WorldTick& tick);
    void                    Rebuild();
};

HK_NAMESPACE_END
