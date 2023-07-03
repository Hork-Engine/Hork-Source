#pragma once

#include <Engine/ECS/ECS.h>
#include <Engine/Core/Ref.h>
#include <Engine/Core/Random.h>

HK_NAMESPACE_BEGIN

namespace BT
{

enum STATUS
{
    UNDEFINED,
    RUNNING,
    SUCCESS,
    FAILURE
};

class BehaviorTreeContext
{
public:
    float TimeStep{};
    ECS::CommandBuffer* CommandBuf{};
    MersenneTwisterRand* RandomGenerator{};
};

class BehaviorTreeNode
{
public:
    // TODO: override new/delete to use custom allocator

    virtual ~BehaviorTreeNode() = default;

    virtual void Start(BehaviorTreeContext& context);
    virtual void Update(BehaviorTreeContext& context);

protected:
    void SetStatus(STATUS status);

public:
    STATUS Status() const;

private:
    STATUS m_Status{UNDEFINED};
};

template <typename TCompositeNode>
class CompositeNodeBuilder final
{
public:
    ~CompositeNodeBuilder()
    {
        for (BehaviorTreeNode* children : m_Children)
            delete children;
    }

    CompositeNodeBuilder& AddChildren(BehaviorTreeNode* node)
    {
        m_Children.push_back(node);
        return *this;
    }

    template <typename T, typename... Args>
    CompositeNodeBuilder& AddChildren(Args&&... args)
    {
        m_Children.push_back(new T(std::forward<Args>(args)...));
        return *this;
    }

    CompositeNodeBuilder& SetRandom(bool bRandom)
    {
        m_bRandom = bRandom;
        return *this;
    }

    [[nodiscard]] TCompositeNode* Build()
    {
        return new TCompositeNode(*this);
    }

private:
    TVector<BehaviorTreeNode*> m_Children;
    bool m_bRandom{};
};

class CompositeNode : public BehaviorTreeNode
{
    using Super = BehaviorTreeNode;

protected:
    TVector<BehaviorTreeNode*> m_Children;

private:
    TVector<int> m_Order;

protected:
    int m_Current{};

private:
    mutable int m_Iterator{};
    bool bRandom{};

protected:
    template <typename T>
    CompositeNode(CompositeNodeBuilder<T>& builder) :
        m_Children(std::move(builder.m_Children)),
        bRandom(builder.m_bRandom)
    {}

    virtual ~CompositeNode();

    void Start(BehaviorTreeContext& context) override;
    void Update(BehaviorTreeContext& context) override;

protected:
    int GetFirst() const;
    bool HasNext() const;
    int GetNext() const;
};

class Sequence : public CompositeNode
{
    using Super = CompositeNode;

public:
    template <typename T>
    Sequence(CompositeNodeBuilder<T>& builder) :
        CompositeNode(builder)
    {}

    void Start(BehaviorTreeContext& context) override;
    void Update(BehaviorTreeContext& context) override;
};

class ParallelSequence : public CompositeNode
{
    using Super = CompositeNode;

public:
    template <typename T>
    ParallelSequence(CompositeNodeBuilder<T>& builder) :
        CompositeNode(builder)
    {}

    void Start(BehaviorTreeContext& context) override;
    void Update(BehaviorTreeContext& context) override;
};

class Selector : public CompositeNode
{
    using Super = CompositeNode;

public:
    template <typename T>
    Selector(CompositeNodeBuilder<T>& builder) :
        CompositeNode(builder)
    {}

    void Start(BehaviorTreeContext& context) override;
    void Update(BehaviorTreeContext& context) override;
};

class DecoratorNode : public BehaviorTreeNode
{
    using Super = BehaviorTreeNode;

protected:
    BehaviorTreeNode* m_Children;

    DecoratorNode(BehaviorTreeNode* children);
    ~DecoratorNode();

    void Start(BehaviorTreeContext& context) override;
    void Update(BehaviorTreeContext& context) override;
};

class Inverter : public DecoratorNode
{
    using Super = DecoratorNode;

public:
    Inverter(BehaviorTreeNode* children);

    void Start(BehaviorTreeContext& context) override;
    void Update(BehaviorTreeContext& context) override;
};

class Succeeder : public DecoratorNode
{
    using Super = DecoratorNode;

public:
    Succeeder(BehaviorTreeNode* children);

    void Start(BehaviorTreeContext& context) override;
    void Update(BehaviorTreeContext& context) override;
};

class Repeater : public DecoratorNode
{
    using Super = DecoratorNode;

    int m_MaxRepeates{};
    int m_NumRepeates{};

public:
    Repeater(BehaviorTreeNode* children, int maxRepeats = 0);

    void Start(BehaviorTreeContext& context) override;
    void Update(BehaviorTreeContext& context) override;
};

class RepeatUntilFail : public DecoratorNode
{
    using Super = DecoratorNode;

public:
    RepeatUntilFail(BehaviorTreeNode* children);

    void Start(BehaviorTreeContext& context) override;
    void Update(BehaviorTreeContext& context) override;
};

template <typename T>
class ECSNode : public BehaviorTreeNode
{
    using Super = BehaviorTreeNode;

    ECS::EntityHandle m_Entity;
    STATUS m_UpdateStatus{UNDEFINED};

public:
    ECSNode(ECS::EntityHandle entity) :
        m_Entity(entity)
    {}

    void SetSuccess()
    {
        m_UpdateStatus = SUCCESS;
    }

    void SetFailure()
    {
        m_UpdateStatus = FAILURE;
    }

    void Start(BehaviorTreeContext& context) override
    {
        Super::Start(context);

        if (context.CommandBuf)
        {
            T& stateComponent = context.CommandBuf->AddComponent<T>(m_Entity);
            stateComponent.BT_Leaf = this;
        }

        m_UpdateStatus = RUNNING;
    }

    void Update(BehaviorTreeContext& context) override
    {
        Super::Update(context);

        if (m_UpdateStatus != RUNNING)
        {
            if (context.CommandBuf)
                context.CommandBuf->RemoveComponent<T>(m_Entity);

            SetStatus(m_UpdateStatus);
        }
    }
};

class BehaviorTree : public RefCounted
{
    BehaviorTreeNode* m_Root;

public:
    BehaviorTree(BehaviorTreeNode* root);
    ~BehaviorTree();

    STATUS Status() const;

    void Start(BehaviorTreeContext& context);
    void Update(BehaviorTreeContext& context);    
};

} // namespace BT

HK_NAMESPACE_END
