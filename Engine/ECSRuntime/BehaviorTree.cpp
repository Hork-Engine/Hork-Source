#include "BehaviorTree.h"

HK_NAMESPACE_BEGIN

namespace BT
{

/////// BehaviorTreeNode //////////////////////////////////////////////////////////////////////

void BehaviorTreeNode::Start(BehaviorTreeContext& context)
{
    m_Status = RUNNING;
}

void BehaviorTreeNode::Update(BehaviorTreeContext& context)
{
    assert(m_Status == RUNNING);
}

void BehaviorTreeNode::SetStatus(STATUS status)
{
    m_Status = status;
}

STATUS BehaviorTreeNode::Status() const
{
    return m_Status;
}

/////// CompositeNode //////////////////////////////////////////////////////////////////////

CompositeNode::~CompositeNode()
{
    for (BehaviorTreeNode* children : m_Children)
        delete children;
}

void CompositeNode::Start(BehaviorTreeContext& context)
{
    Super::Start(context);

    if (bRandom)
    {
        m_Order.Resize(m_Children.Size());
        int index{};
        for (auto& i : m_Order)
            i = index++;

        if (context.RandomGenerator)
        {
            for (int i = 0; i < m_Order.Size(); i++)
            {
                index = context.RandomGenerator->Get() % m_Order.Size();

                if (i != index)
                    std::swap(m_Order[i], m_Order[index]);
            }
        }
    }
}

void CompositeNode::Update(BehaviorTreeContext& context)
{
    Super::Update(context);
}

int CompositeNode::GetFirst() const
{
    m_Iterator = 0;
    return bRandom ? m_Order[0] : 0;
}

bool CompositeNode::HasNext() const
{
    return m_Iterator + 1 < m_Children.Size();
}

int CompositeNode::GetNext() const
{
    ++m_Iterator;
    return bRandom ? m_Order[m_Iterator] : m_Iterator;
}

/////// Sequence //////////////////////////////////////////////////////////////////////

void Sequence::Start(BehaviorTreeContext& context)
{
    Super::Start(context);

    m_Current = GetFirst();
    m_Children[m_Current]->Start(context);
}

void Sequence::Update(BehaviorTreeContext& context)
{
    Super::Update(context);

    m_Children[m_Current]->Update(context);
    switch (m_Children[m_Current]->Status())
    {
        case RUNNING:
            break;
        case SUCCESS:
            if (HasNext())
            {
                m_Current = GetNext();
                m_Children[m_Current]->Start(context);
            }
            else
            {
                SetStatus(SUCCESS);
            }
            break;
        case FAILURE:
            SetStatus(FAILURE);
            break;
        case UNDEFINED:
            assert(0);
            break;
    }
}

/////// ParallelSequence //////////////////////////////////////////////////////////////////////

void ParallelSequence::Start(BehaviorTreeContext& context)
{
    Super::Start(context);

    for (auto* node : m_Children)
        node->Start(context);
}

void ParallelSequence::Update(BehaviorTreeContext& context)
{
    Super::Update(context);

    int numSuccess{};
    int numFailure{};
    for (auto* node : m_Children)
    {
        if (node->Status() == RUNNING)
        {
            node->Update(context);
            switch (node->Status())
            {
                case RUNNING:
                    break;
                case SUCCESS:
                    numSuccess++;
                    break;
                case FAILURE:
                    numFailure++;
                    break;
                case UNDEFINED:
                    assert(0);
                    break;
            }
        }
        else if (node->Status() == SUCCESS)
            numSuccess++;
        else if (node->Status() == FAILURE)
            numFailure++;
    }

    if (numFailure + numSuccess == m_Children.Size())
    {
        if (numFailure > 0)
            SetStatus(FAILURE);
        else
            SetStatus(SUCCESS);
    }
}

/////// Selector //////////////////////////////////////////////////////////////////////

void Selector::Start(BehaviorTreeContext& context)
{
    Super::Start(context);

    m_Current = GetFirst();
    m_Children[m_Current]->Start(context);
}

void Selector::Update(BehaviorTreeContext& context)
{
    Super::Update(context);

    m_Children[m_Current]->Update(context);
    switch (m_Children[m_Current]->Status())
    {
        case RUNNING:
            break;
        case SUCCESS:
            SetStatus(SUCCESS);
            break;
        case FAILURE:
            if (HasNext())
            {
                m_Current = GetNext();
                m_Children[m_Current]->Start(context);
            }
            else
            {
                SetStatus(FAILURE);
            }
            break;
        case UNDEFINED:
            assert(0);
            break;
    }
}

/////// DecoratorNode //////////////////////////////////////////////////////////////////////

DecoratorNode::DecoratorNode(BehaviorTreeNode* children) :
    m_Children(children)
{}

DecoratorNode::~DecoratorNode()
{
    delete m_Children;
}

void DecoratorNode::Start(BehaviorTreeContext& context)
{
    Super::Start(context);
}

void DecoratorNode::Update(BehaviorTreeContext& context)
{
    Super::Update(context);
}

/////// Inverter //////////////////////////////////////////////////////////////////////

Inverter::Inverter(BehaviorTreeNode* children) :
    DecoratorNode(children)
{}

void Inverter::Start(BehaviorTreeContext& context)
{
    Super::Start(context);

    m_Children->Start(context);
}

void Inverter::Update(BehaviorTreeContext& context)
{
    Super::Update(context);

    m_Children->Update(context);
    switch (m_Children->Status())
    {
        case RUNNING:
            break;
        case SUCCESS:
            SetStatus(FAILURE);
            break;
        case FAILURE:
            SetStatus(SUCCESS);
            break;
        case UNDEFINED:
            assert(0);
            break;
    }
}

/////// Succeeder //////////////////////////////////////////////////////////////////////

Succeeder::Succeeder(BehaviorTreeNode* children) :
    DecoratorNode(children)
{}

void Succeeder::Start(BehaviorTreeContext& context)
{
    Super::Start(context);

    m_Children->Start(context);
}

void Succeeder::Update(BehaviorTreeContext& context)
{
    Super::Update(context);

    m_Children->Update(context);
    switch (m_Children->Status())
    {
        case RUNNING:
            break;
        case SUCCESS:
        case FAILURE:
            SetStatus(SUCCESS);
            break;
        case UNDEFINED:
            assert(0);
            break;
    }
}

/////// Repeater //////////////////////////////////////////////////////////////////////

Repeater::Repeater(BehaviorTreeNode* children, int maxRepeats) :
    DecoratorNode(children),
    m_MaxRepeates(maxRepeats)
{}

void Repeater::Start(BehaviorTreeContext& context)
{
    Super::Start(context);

    m_Children->Start(context);
    m_NumRepeates = 0;
}

void Repeater::Update(BehaviorTreeContext& context)
{
    Super::Update(context);

    m_Children->Update(context);
    switch (m_Children->Status())
    {
        case RUNNING:
            break;
        case SUCCESS:
        case FAILURE:
            if (m_MaxRepeates > 0)
            {
                m_NumRepeates++;
                if (m_NumRepeates == m_MaxRepeates)
                    SetStatus(SUCCESS);
                else
                    m_Children->Start(context);
            }
            else
                m_Children->Start(context);
            break;
        case UNDEFINED:
            assert(0);
            break;
    }
}

/////// RepeatUntilFail //////////////////////////////////////////////////////////////////////

RepeatUntilFail::RepeatUntilFail(BehaviorTreeNode* children) :
    DecoratorNode(children)
{}

void RepeatUntilFail::Start(BehaviorTreeContext& context)
{
    Super::Start(context);

    m_Children->Start(context);
}

void RepeatUntilFail::Update(BehaviorTreeContext& context)
{
    Super::Update(context);

    m_Children->Update(context);
    switch (m_Children->Status())
    {
        case RUNNING:
            break;
        case SUCCESS:
            m_Children->Start(context);
            break;
        case FAILURE:
            SetStatus(SUCCESS);
            break;
        case UNDEFINED:
            assert(0);
            break;
    }
}

/////// BehaviorTree //////////////////////////////////////////////////////////////////////

BehaviorTree::BehaviorTree(BehaviorTreeNode* root) :
    m_Root(root)
{}

BehaviorTree::~BehaviorTree()
{
    delete m_Root;
}

STATUS BehaviorTree::Status() const
{
    return m_Root->Status();
}

void BehaviorTree::Start(BehaviorTreeContext& context)
{
    m_Root->Start(context);
}

void BehaviorTree::Update(BehaviorTreeContext& context)
{
    m_Root->Update(context);
}

}

HK_NAMESPACE_END
