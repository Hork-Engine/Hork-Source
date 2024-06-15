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

/*

Небольшой набросок на тему Behavior Tree.

*/

#include <Engine/Core/Ref.h>
#include <Engine/Core/Random.h>
#include <Engine/Core/Containers/Vector.h>

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
    Vector<BehaviorTreeNode*> m_Children;
    bool m_bRandom{};
};

class CompositeNode : public BehaviorTreeNode
{
    using Super = BehaviorTreeNode;

protected:
    Vector<BehaviorTreeNode*> m_Children;

private:
    Vector<int> m_Order;

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
