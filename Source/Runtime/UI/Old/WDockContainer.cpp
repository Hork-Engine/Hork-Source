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

#include "WDockContainer.h"
#include "WDesktop.h"

#include <Runtime/FrameLoop.h>

HK_CLASS_META(WDockNode)
HK_CLASS_META(WDockContainer)

WDockContainer::WDockContainer() :
    WDockContainer("Default")
{}

WDockContainer::WDockContainer(AStringView containerName) :
    m_ContainerName(containerName)
{
    m_Root = CreateInstanceOf<WDockNode>();

    SetMargin(0, 0, 0, 0);

    // Call to update leaf size
    UpdateDocks();
}

WDockNode* WDockContainer::TraceLeaf(float x, float y)
{
    return m_Root->TraceLeaf(x, y);
}

static bool IsPointInRect(float x, float y, Float2 const& mins, Float2 const& maxs)
{
    if (x < mins.X || y < mins.Y || x > maxs.X || y > maxs.Y)
        return false;
    return true;
}


WDockPlacement WDockContainer::GetPlacement(float x, float y)
{
    WDockNode* leaf = TraceLeaf(x, y);
    if (!leaf)
        return {};

    if (leaf == m_Root)
    {
        WDockPlacement placement;
        placement.Leaf = leaf;
        placement.Zone = DOCK_ZONE_CENTER;
        placement.PolygonVerts[0] = leaf->m_Mins;
        placement.PolygonVerts[1] = Float2(leaf->m_Maxs.X, leaf->m_Mins.Y);
        placement.PolygonVerts[2] = leaf->m_Maxs;
        placement.PolygonVerts[3] = Float2(leaf->m_Mins.X, leaf->m_Maxs.Y);
        return placement;
    }

    // Translate X, Y to normalized window space 0..1

    x -= leaf->m_Mins.X;
    y -= leaf->m_Mins.Y;

    float w = leaf->m_Maxs.X - leaf->m_Mins.X;
    float h = leaf->m_Maxs.Y - leaf->m_Mins.Y;

    x /= w;
    y /= h;

    float aspect = w / h;

    float xmin = 0.2f;
    float xmax = 1.0f - xmin;

    float ymin = 0.2f * aspect;
    float ymax = 1.0f - ymin;

    //y *= aspect;

    // Find zone

    DOCK_ZONE zone;

    if (x > y)
    {
        if (1.0f - x < y)
            zone = x > xmax ? DOCK_ZONE_RIGHT : DOCK_ZONE_CENTER;
        else
            zone = y < ymin ? DOCK_ZONE_TOP : DOCK_ZONE_CENTER;
    }
    else
    {
        if (1.0f - x > y)
            zone = x < xmin ? DOCK_ZONE_LEFT : DOCK_ZONE_CENTER;
        else
            zone = y > ymax ? DOCK_ZONE_BOTTOM : DOCK_ZONE_CENTER;
    }

    WDockPlacement placement;
    placement.Leaf = leaf;
    placement.Zone = zone;

    switch (zone)
    {
        case DOCK_ZONE_LEFT:
            placement.PolygonVerts[0] = {0, 0};
            placement.PolygonVerts[1] = {xmin, ymin};
            placement.PolygonVerts[2] = {xmin, ymax};
            placement.PolygonVerts[3] = {0, 1};
            break;
        case DOCK_ZONE_RIGHT:
            placement.PolygonVerts[0] = {1, 0};
            placement.PolygonVerts[1] = {1, 1};
            placement.PolygonVerts[2] = {xmax, ymax};
            placement.PolygonVerts[3] = {xmax, ymin};
            break;
        case DOCK_ZONE_TOP:
            placement.PolygonVerts[0] = {0, 0};
            placement.PolygonVerts[1] = {1, 0};
            placement.PolygonVerts[2] = {xmax, ymin};
            placement.PolygonVerts[3] = {xmin, ymin};
            break;
        case DOCK_ZONE_BOTTOM:
            placement.PolygonVerts[0] = {xmin, ymax};
            placement.PolygonVerts[1] = {xmax, ymax};
            placement.PolygonVerts[2] = {1, 1};
            placement.PolygonVerts[3] = {0, 1};
            break;
        default:
            placement.PolygonVerts[0] = {0, 0};
            placement.PolygonVerts[1] = {1, 0};
            placement.PolygonVerts[2] = {1, 1};
            placement.PolygonVerts[3] = {0, 1};
            break;
    }

    // Convert polygon vertices from normalized coordinates
    for (Float2& v : placement.PolygonVerts)
    {
        v *= {w, h};
        v += leaf->m_Mins;
    }

    return placement;
}

WDockNode* WDockContainer::AttachWidget(WDockWidget* dockWidget, WDockNode* leaf, DOCK_ZONE zone, float splitDistance)
{
    if (!dockWidget)
        return {};

    // Assignment to this dock container is not allowed.
    if (dockWidget->m_ContainerName != m_ContainerName)
        return {};

    if (dockWidget->m_Leaf)
    {
        LOG("Dock widget already assigned to dock container\n");
        return {};
    }

    if (!leaf)
        return {};

    // We can add widget only to leafs
    if (leaf->m_NodeType != WDockNode::NODE_LEAF)
        return {};

    // Reset margin. Dock widget should not have margin.
    dockWidget->SetMargin(0, 0, 0, 0);

    if (zone == DOCK_ZONE_CENTER || !leaf->m_LeafWidget)
    {
        // Just assign new widget to leaf

        if (leaf->m_LeafWidget)
        {
            leaf->m_LeafWidget->m_Leaf.Reset();
            leaf->m_LeafWidget->m_ContainerId = 0;
        }

        leaf->m_LeafWidget = dockWidget;
        leaf->m_LeafWidget->m_Leaf = leaf;
        leaf->m_LeafWidget->m_ContainerId = Id;

        leaf->m_LeafWidget->SetParent(this);

        leaf->m_LeafWidget->SetDektopPosition(leaf->m_Mins);
        leaf->m_LeafWidget->SetSize(leaf->m_Maxs - leaf->m_Mins);
        return leaf;
    }

    WDockNode* node = leaf;

    node->m_Child[0] = CreateInstanceOf<WDockNode>();
    node->m_Child[1] = CreateInstanceOf<WDockNode>();

    node->m_Child[0]->m_NodeType = WDockNode::NODE_LEAF;
    node->m_Child[1]->m_NodeType = WDockNode::NODE_LEAF;

    node->m_SplitDistance = splitDistance;

    node->m_NodeType = (WDockNode::NODE_TYPE)(((int)zone >> 1) & 1);

    int child0 = ((int)zone) & 1;
    int child1 = ((int)zone + 1) & 1;

    leaf = node->m_Child[child0];
    leaf->m_LeafWidget = dockWidget;
    dockWidget->m_Leaf = leaf;
    dockWidget->m_ContainerId = Id;
    dockWidget->SetParent(this);

    node->m_Child[child1]->m_LeafWidget = std::move(node->m_LeafWidget);
    node->m_Child[child1]->m_LeafWidget->m_Leaf = node->m_Child[child1];

    node->UpdateRecursive(node->m_Mins, node->m_Maxs);

    return leaf;
}

bool WDockContainer::DetachWidget(WDockWidget* dockWidget)
{
    if (!dockWidget)
        return false;

    // Widget is not attached to this dock container
    if (dockWidget->m_ContainerId != Id)
        return false;

    if (!dockWidget->m_Leaf)
        return false;

    return DetachWidget(dockWidget->m_Leaf) != nullptr;
}

WDockWidget* WDockContainer::DetachWidget(WDockNode* leaf)
{
    // Expect leaf node
    if (leaf->m_NodeType != WDockNode::NODE_LEAF)
        return {};

    TRef<WDockWidget> detachedWidget = leaf->m_LeafWidget;
    if (detachedWidget)
    {
        detachedWidget->m_Leaf.Reset();
        detachedWidget->m_ContainerId = 0;
        detachedWidget->Unparent();
    }

    WDockNode* parent = FindParent(leaf);
    if (parent)
    {
        TRef<WDockNode> neighborNode;
        if (leaf == parent->m_Child[0])
            neighborNode = parent->m_Child[1];
        else
            neighborNode = parent->m_Child[0];

        parent->m_NodeType = neighborNode->m_NodeType;
        parent->m_LeafWidget = neighborNode->m_LeafWidget;
        parent->m_SplitDistance = neighborNode->m_SplitDistance;
        parent->m_Child[0] = neighborNode->m_Child[0];
        parent->m_Child[1] = neighborNode->m_Child[1];

        if (parent->m_LeafWidget)
            parent->m_LeafWidget->m_Leaf = parent;

        parent->UpdateRecursive(parent->m_Mins, parent->m_Maxs);
    }
    else
    {
        leaf->m_LeafWidget.Reset();
    }

    return detachedWidget;
}

TVector<TRef<WDockWidget>> WDockContainer::GetWidgets() const
{
    TVector<TRef<WDockWidget>> widgetList;
    m_Root->GetWidgets(widgetList);
    return widgetList;
}

WDockNode* WDockContainer::FindParent(WDockNode* node)
{
    if (node == m_Root)
        return nullptr;
    
    return m_Root->FindParent(node);
}

void WDockContainer::UpdateDocks()
{
    Float2 mins, maxs;
    GetDesktopRect(mins, maxs, false);

    m_Root->UpdateRecursive(mins, maxs);
}

void WDockContainer::OnPostDrawEvent(ACanvas& canvas)
{
    Super::OnPostDrawEvent(canvas);



    if (m_DragSplitter)
    {
        Float2 cursorPos = GetDesktop()->GetCursorPosition();
        Float2 dragDelta = cursorPos - m_DragPos;

        if (m_DragSplitter->m_NodeType == WDockNode::NODE_SPLIT_VERTICAL)
        {
            float split = m_StartSplitPos + dragDelta.X;
            split -= m_DragSplitter->m_Mins.X;
            split /= m_DragSplitter->m_Maxs.X - m_DragSplitter->m_Mins.X;

            m_DragSplitter->m_SplitDistance = split;
        }
        else if (m_DragSplitter->m_NodeType == WDockNode::NODE_SPLIT_HORIZONTAL)
        {
            float split = m_StartSplitPos + dragDelta.Y;
            split -= m_DragSplitter->m_Mins.Y;
            split /= m_DragSplitter->m_Maxs.Y - m_DragSplitter->m_Mins.Y;

            m_DragSplitter->m_SplitDistance = split;
        }

        m_DragSplitter->UpdateRecursive(m_DragSplitter->m_Mins, m_DragSplitter->m_Maxs);
    }
    Float2 cursorPos = GetDesktop()->GetCursorPosition();
    WDockPlacement placement = GetPlacement(cursorPos.X, cursorPos.Y);
    if (placement)
    {
        canvas.BeginPath();
        canvas.MoveTo(placement.PolygonVerts[0]);
        canvas.LineTo(placement.PolygonVerts[1]);
        canvas.LineTo(placement.PolygonVerts[2]);
        canvas.LineTo(placement.PolygonVerts[3]);
        canvas.LineTo(placement.PolygonVerts[0]);
        canvas.StrokeWidth(2);
        canvas.StrokeColor(Color4::Orange());
        canvas.Stroke();
    }

    WDockNode* node = m_Root->TraceSeparator(cursorPos.X, cursorPos.Y);
    if (node)
    {
        Float2 bmins, bmaxs;
        const float splitterWidth = 4;

        node->GetSplitterBounds(bmins, bmaxs, splitterWidth);

        canvas.DrawRectFilled(bmins, bmaxs, Color4::Orange());
    }
}

void WDockContainer::OnMouseMoveEvent(SMouseMoveEvent const& event, double timeStamp)
{
    //if (m_DragSplitter)
    //{
    //    LOG("Move splitter {} -> {}\n", m_DragPos, Float2(event.X, event.Y));
    //    Float2 dragDelta = Float2(event.X, event.Y) - m_DragPos;

    //    if (m_DragSplitter->m_NodeType == WDockNode::NODE_SPLIT_VERTICAL)
    //    {
    //        float split = m_StartSplitPos + dragDelta.X;
    //        split -= m_DragSplitter->m_Mins.X;
    //        split /= m_DragSplitter->m_Maxs.X - m_DragSplitter->m_Mins.X;

    //        m_DragSplitter->m_SplitDistance = split;
    //    }
    //    else if (m_DragSplitter->m_NodeType == WDockNode::NODE_SPLIT_HORIZONTAL)
    //    {
    //        float split = m_StartSplitPos + dragDelta.Y;
    //        split -= m_DragSplitter->m_Mins.Y;
    //        split /= m_DragSplitter->m_Maxs.Y - m_DragSplitter->m_Mins.Y;

    //        m_DragSplitter->m_SplitDistance = split;
    //    }

    //    m_DragSplitter->UpdateRecursive(m_DragSplitter->m_Mins, m_DragSplitter->m_Maxs);
    //}
}

WDockNode::WDockNode()
{
}

WDockNode* WDockNode::TraceLeaf(float x, float y)
{
    if (IsPointInRect(x, y, m_Mins, m_Maxs))
    {
        if (m_NodeType == NODE_LEAF)
            return this;

        for (int i = 0; i < 2; ++i)
        {
            WDockNode* leaf = m_Child[i]->TraceLeaf(x, y);
            if (leaf)
                return leaf;
        }
    }

    return nullptr;
}

void WDockNode::GetSplitterBounds(Float2& bmins, Float2& bmaxs, float splitterWidth) const
{
    float splitHalfWidth = splitterWidth * 0.5f;

    if (m_NodeType == NODE_SPLIT_VERTICAL)
    {
        float d = Math::Lerp(m_Mins.X, m_Maxs.X, m_SplitDistance);

        bmins.X = d - splitHalfWidth;
        bmaxs.X = d + splitHalfWidth;

        bmins.Y = m_Mins.Y;
        bmaxs.Y = m_Maxs.Y;
    }
    else if (m_NodeType == NODE_SPLIT_HORIZONTAL)
    {
        float d = Math::Lerp(m_Mins.Y, m_Maxs.Y, m_SplitDistance);

        bmins.Y = d - splitHalfWidth;
        bmaxs.Y = d + splitHalfWidth;

        bmins.X = m_Mins.X;
        bmaxs.X = m_Maxs.X;
    }
    else
    {
        bmins.Clear();
        bmaxs.Clear();
    }
}

WDockNode* WDockNode::TraceSeparator(float x, float y)
{
    if (m_NodeType == NODE_LEAF)
    {
        return nullptr;
    }

    if (!IsPointInRect(x, y, m_Mins, m_Maxs))
    {
        return nullptr;
    }

    {
        const float splitterWidth = 8;

        Float2 bmins, bmaxs;
        GetSplitterBounds(bmins, bmaxs, splitterWidth);

        if (IsPointInRect(x, y, bmins, bmaxs))
        {
            return this;
        }
    }

    WDockNode* node = m_Child[0]->TraceSeparator(x, y);
    if (node)
        return node;

    return m_Child[1]->TraceSeparator(x, y);
}

void WDockNode::UpdateRecursive(Float2 const& mins, Float2 const& maxs)
{
    UpdateRecursive(mins.X, mins.Y, maxs.X - mins.X, maxs.Y - mins.Y);
}

void WDockNode::UpdateRecursive(float x, float y, float w, float h)
{
    m_Mins = {x, y};
    m_Maxs = {x + w, y + h};

    if (m_NodeType == NODE_LEAF)
    {
        if (m_LeafWidget)
        {
            m_LeafWidget->SetDektopPosition(x, y);
            m_LeafWidget->SetSize(w, h);
        }
        return;
    }

    if (m_NodeType == NODE_SPLIT_VERTICAL)
    {
        float d = m_SplitDistance * w;

        // Left
        m_Child[0]->UpdateRecursive(x, y, d, h);

        // Right
        m_Child[1]->UpdateRecursive(x + d, y, w - d, h);
        return;
    }

    if (m_NodeType == NODE_SPLIT_HORIZONTAL)
    {
        float d = m_SplitDistance * h;

        // Top
        m_Child[0]->UpdateRecursive(x, y, w, d);

        // Bottom
        m_Child[1]->UpdateRecursive(x, y + d, w, h - d);
        return;
    }
}

WDockNode* WDockNode::FindParent(WDockNode* node)
{
    if (m_NodeType == NODE_LEAF)
        return nullptr;

    for (int i = 0; i < 2; ++i)
        if (m_Child[i] == node)
            return this;
    
    WDockNode* n = m_Child[0]->FindParent(node);
    if (n)
        return n;

    return m_Child[1]->FindParent(node);
}

void WDockNode::GetWidgets(TVector<TRef<WDockWidget>>& widgetList) const
{
    if (m_NodeType == NODE_LEAF)
    {
        if (m_LeafWidget)
            widgetList.Add(m_LeafWidget);

        return;
    }

    m_Child[0]->GetWidgets(widgetList);
    m_Child[1]->GetWidgets(widgetList);
}
