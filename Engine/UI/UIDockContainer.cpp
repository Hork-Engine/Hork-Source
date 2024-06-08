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

#include "UIDockContainer.h"
#include "UIManager.h"
#include "UIViewport.h"

#include <Engine/Geometry/BV/BvIntersect.h>

#include <Engine/GameApplication/FrameLoop.h>

HK_NAMESPACE_BEGIN

UIDockContainer::UIDockContainer() :
    UIDockContainer("Default")
{}

UIDockContainer::UIDockContainer(StringView containerName) :
    m_ContainerName(containerName)
{
    m_Root = UINew(UIDockNode);
    Layout = UINew(DockLayout, this);

    //Padding = UIPadding(10.0f);
    Padding = UIPadding(0);
}

UIDockNode* UIDockContainer::TraceLeaf(float x, float y)
{
    x -= m_Geometry.PaddedMins.X;
    y -= m_Geometry.PaddedMins.Y;

    return m_Root->TraceLeaf(x, y);
}

UIDockPlacement UIDockContainer::GetPlacement(float x, float y)
{
    x -= m_Geometry.PaddedMins.X;
    y -= m_Geometry.PaddedMins.Y;

    UIDockNode* leaf = m_Root->TraceLeaf(x, y);
    if (!leaf)
        return {};

    //if (leaf == m_Root)
    //{
    //    UIDockPlacement placement;
    //    placement.Leaf = leaf;
    //    placement.Zone = DOCK_ZONE_CENTER;
    //    placement.PolygonVerts[0] = Geometry.PaddedMins + leaf->m_Mins;
    //    placement.PolygonVerts[1] = Geometry.PaddedMins + Float2(leaf->m_Maxs.X, leaf->m_Mins.Y);
    //    placement.PolygonVerts[2] = Geometry.PaddedMins + leaf->m_Maxs;
    //    placement.PolygonVerts[3] = Geometry.PaddedMins + Float2(leaf->m_Mins.X, leaf->m_Maxs.Y);
    //    return placement;
    //}

    // Translate X, Y to normalized window space 0..1

    x -= leaf->m_Mins.X;
    y -= leaf->m_Mins.Y;

    float w = leaf->m_Maxs.X - leaf->m_Mins.X;
    float h = leaf->m_Maxs.Y - leaf->m_Mins.Y;

    x /= w;
    y /= h;

    float areaWidth = Math::Min(w, h) * 0.3f / w;

    float aspect = w / h;

    float xmin = areaWidth;
    float xmax = 1.0f - xmin;

    float ymin = areaWidth * aspect;
    float ymax = 1.0f - ymin;

    UIDockPlacement placement;
    placement.Leaf = leaf;

    while (1)
    {
        placement.PolygonVerts[0] = {0, 0};
        placement.PolygonVerts[1] = {xmin, ymin};
        placement.PolygonVerts[2] = {xmin, ymax};
        placement.PolygonVerts[3] = {0, 1};

        if (BvPointInPoly2D(placement.PolygonVerts.ToPtr(), 4, x, y))
        {
            placement.Zone = DOCK_ZONE_LEFT;
            break;
        }

        placement.PolygonVerts[0] = {1, 0};
        placement.PolygonVerts[1] = {1, 1};
        placement.PolygonVerts[2] = {xmax, ymax};
        placement.PolygonVerts[3] = {xmax, ymin};

        if (BvPointInPoly2D(placement.PolygonVerts.ToPtr(), 4, x, y))
        {
            placement.Zone = DOCK_ZONE_RIGHT;
            break;
        }

        placement.PolygonVerts[0] = {0, 0};
        placement.PolygonVerts[1] = {1, 0};
        placement.PolygonVerts[2] = {xmax, ymin};
        placement.PolygonVerts[3] = {xmin, ymin};

        if (BvPointInPoly2D(placement.PolygonVerts.ToPtr(), 4, x, y))
        {
            placement.Zone = DOCK_ZONE_TOP;
            break;
        }

        placement.PolygonVerts[0] = {xmin, ymax};
        placement.PolygonVerts[1] = {xmax, ymax};
        placement.PolygonVerts[2] = {1, 1};
        placement.PolygonVerts[3] = {0, 1};

        if (BvPointInPoly2D(placement.PolygonVerts.ToPtr(), 4, x, y))
        {
            placement.Zone = DOCK_ZONE_BOTTOM;
            break;
        }

        placement.PolygonVerts[0] = {0, 0};
        placement.PolygonVerts[1] = {1, 0};
        placement.PolygonVerts[2] = {1, 1};
        placement.PolygonVerts[3] = {0, 1};
        placement.Zone = DOCK_ZONE_CENTER;
        break;
    }

    // Convert polygon vertices from normalized coordinates
    for (Float2& v : placement.PolygonVerts)
    {
        v *= {w, h};
        v += m_Geometry.PaddedMins + leaf->m_Mins;
    }

    return placement;
}

UIDockNode* UIDockContainer::AttachWidget(UIDockWidget* dockWidget, UIDockNode* leaf, DOCK_ZONE zone, float splitDistance)
{
    if (!dockWidget)
        return {};

    // Assignment to this dock container is not allowed.
    if (dockWidget->m_Container != this)
        return {};

    if (dockWidget->m_Leaf)
    {
        LOG("Dock widget already assigned to dock container\n");
        return {};
    }

    if (!leaf)
        return {};

    // We can add widget only to leafs
    if (leaf->m_NodeType != UIDockNode::NODE_LEAF)
        return {};

    // Reset margin. Dock widget should not have margin.
    //dockWidget->Padding = UIPadding(0);

    if (zone == DOCK_ZONE_CENTER || leaf->m_LeafWidgets.IsEmpty())
    {
        // Just assign new widget to leaf

        leaf->m_LeafWidgets.Add(TRef<UIDockWidget>(dockWidget));
        leaf->m_WidgetNum = leaf->m_LeafWidgets.Size() - 1;

        dockWidget->m_Leaf        = leaf;
        dockWidget->m_ContainerId = Id;

        AddWidget(dockWidget);

        //leaf->m_LeafWidget->Position = leaf->m_Mins;
        //leaf->m_LeafWidget->Size = leaf->m_Maxs - leaf->m_Mins;
        return leaf;
    }

    UIDockNode* node = leaf;

    node->m_Child[0] = UINew(UIDockNode);
    node->m_Child[1] = UINew(UIDockNode);

    node->m_Child[0]->m_NodeType = UIDockNode::NODE_LEAF;
    node->m_Child[1]->m_NodeType = UIDockNode::NODE_LEAF;

    node->m_SplitDistance = splitDistance;

    node->m_NodeType = (UIDockNode::NODE_TYPE)(((int)zone >> 1) & 1);

    int child0 = ((int)zone) & 1;
    int child1 = ((int)zone + 1) & 1;

    leaf = node->m_Child[child0];
    leaf->m_LeafWidgets.Add(TRef<UIDockWidget>(dockWidget));
    leaf->m_WidgetNum = leaf->m_LeafWidgets.Size() - 1;

    dockWidget->m_Leaf = leaf;
    dockWidget->m_ContainerId = Id;
    AddWidget(dockWidget);

    node->m_Child[child1]->m_LeafWidgets = std::move(node->m_LeafWidgets);
    node->m_Child[child1]->m_WidgetNum   = node->m_WidgetNum;
    for (TRef<UIDockWidget>& w : node->m_Child[child1]->m_LeafWidgets)
        w->m_Leaf = node->m_Child[child1];

    //node->UpdateRecursive(node->m_Mins, node->m_Maxs);

    return leaf;
}

bool UIDockContainer::DetachWidget(UIDockWidget* dockWidget)
{
    if (!dockWidget)
        return false;

    // Widget is not attached to this dock container
    if (dockWidget->m_ContainerId != Id)
        return false;

    if (!dockWidget->m_Leaf)
        return false;

    int index = dockWidget->m_Leaf->m_LeafWidgets.IndexOf(TRef<UIDockWidget>(dockWidget));

    return DetachWidget(dockWidget->m_Leaf, index) != nullptr;
}

UIDockWidget* UIDockContainer::DetachWidget(UIDockNode* leaf, int index)
{
    // Expect leaf node
    if (leaf->m_NodeType != UIDockNode::NODE_LEAF)
        return {};

    TRef<UIDockWidget> detachedWidget = leaf->m_LeafWidgets[index];
    if (detachedWidget)
    {
        detachedWidget->m_Leaf.Reset();
        detachedWidget->m_ContainerId = 0;
        detachedWidget->Detach();
    }
    leaf->m_LeafWidgets.Remove(index);
    leaf->m_WidgetNum = Math::Max(0, index - 1);

    if (leaf->m_LeafWidgets.IsEmpty())
    {
        UIDockNode* parent = FindParent(leaf);
        if (parent)
        {
            TRef<UIDockNode> neighborNode;
            if (leaf == parent->m_Child[0])
                neighborNode = parent->m_Child[1];
            else
                neighborNode = parent->m_Child[0];

            parent->m_NodeType = neighborNode->m_NodeType;
            parent->m_LeafWidgets = std::move(neighborNode->m_LeafWidgets);
            parent->m_WidgetNum = neighborNode->m_WidgetNum;
            parent->m_SplitDistance = neighborNode->m_SplitDistance;
            parent->m_Child[0] = std::move(neighborNode->m_Child[0]);
            parent->m_Child[1] = std::move(neighborNode->m_Child[1]);

            for (auto& w : parent->m_LeafWidgets)
                w->m_Leaf = parent;

            //parent->UpdateRecursive(parent->m_Mins, parent->m_Maxs);
        }
    }

    return detachedWidget;
}

TVector<TRef<UIDockWidget>> UIDockContainer::GetWidgets() const
{
    TVector<TRef<UIDockWidget>> widgetList;
    m_Root->GetWidgets(widgetList);
    return widgetList;
}

UIDockNode* UIDockContainer::FindParent(UIDockNode* node)
{
    if (node == m_Root)
        return nullptr;
    
    return m_Root->FindParent(node);
}

void UIDockContainer::PostDraw(Canvas& canvas)
{
    Super::PostDraw(canvas);

    if (bDrawPlacement && DragWidget)
    {
        //auto geometry = DragWidget->Geometry;

        //UIDockPlacement placement = GetPlacement(GUIManager->CursorPosition.X, GUIManager->CursorPosition.Y);
        //if (placement)
        //{
        //    float splitDistance = 0.5f;

        //    float leafWidth  = placement.Leaf->m_Maxs.X - placement.Leaf->m_Mins.X;
        //    float leafHeight = placement.Leaf->m_Maxs.Y - placement.Leaf->m_Mins.Y;

        //    switch (placement.Zone)
        //    {
        //        case DOCK_ZONE_LEFT:
        //            if (DragWidget->m_DockSize.X < leafWidth)
        //                splitDistance = DragWidget->m_DockSize.X / leafWidth;

        //            DragWidget->Geometry.Mins = placement.Leaf->m_Mins;
        //            DragWidget->Geometry.Maxs.X = Math::Lerp(placement.Leaf->m_Mins.X, placement.Leaf->m_Maxs.X, splitDistance);
        //            DragWidget->Geometry.Maxs.Y = placement.Leaf->m_Maxs.Y;
        //            break;
        //        case DOCK_ZONE_RIGHT:
        //            if (DragWidget->m_DockSize.X < leafWidth)
        //                splitDistance = 1.0f - DragWidget->m_DockSize.X / leafWidth;

        //            DragWidget->Geometry.Mins.X = Math::Lerp(placement.Leaf->m_Mins.X, placement.Leaf->m_Maxs.X, splitDistance);
        //            DragWidget->Geometry.Mins.Y = placement.Leaf->m_Mins.Y;
        //            DragWidget->Geometry.Maxs   = placement.Leaf->m_Maxs;
        //            break;
        //        case DOCK_ZONE_TOP:
        //            if (DragWidget->m_DockSize.Y < leafHeight)
        //                splitDistance = DragWidget->m_DockSize.Y / leafHeight;
        //            break;
        //        case DOCK_ZONE_BOTTOM:
        //            if (DragWidget->m_DockSize.Y < leafHeight)
        //                splitDistance = 1.0f - DragWidget->m_DockSize.Y / leafHeight;
        //            break;
        //        case DOCK_ZONE_CENTER:
        //            break;
        //    }

        //    DragWidget->Geometry.Mins += Geometry.PaddedMins;
        //    DragWidget->Geometry.Maxs += Geometry.PaddedMins;
        //    DragWidget->Geometry.UpdatePadding(DragWidget->Padding);
        //    
        //    DragWidget->Draw(canvas, Float2(0), Float2(4096), 1);
        //    //DragWidget->Draw(canvas);
        //    DragWidget->Geometry = geometry;            
        //}

        Float2         cursorPos = GUIManager->CursorPosition;
        UIDockPlacement placement = GetPlacement(cursorPos.X, cursorPos.Y);
        if (placement)
        {
            #if 1
            canvas.BeginPath();
            canvas.MoveTo(placement.PolygonVerts[0]);
            canvas.LineTo(placement.PolygonVerts[1]);
            canvas.LineTo(placement.PolygonVerts[2]);
            canvas.LineTo(placement.PolygonVerts[3]);
            Color4 fillColor = Color4::Orange();
            fillColor.A      = 0.2f;
            canvas.FillColor(fillColor);
            canvas.Fill();

            canvas.BeginPath();
            canvas.MoveTo(placement.PolygonVerts[0]);
            canvas.LineTo(placement.PolygonVerts[1]);
            canvas.LineTo(placement.PolygonVerts[2]);
            canvas.LineTo(placement.PolygonVerts[3]);
            canvas.LineTo(placement.PolygonVerts[0]);
            canvas.StrokeWidth(2);
            canvas.StrokeColor(Color4::Orange());
            canvas.Stroke();
            #else

            float splitDistance = 0.5f;

            float leafWidth  = placement.Leaf->m_Maxs.X - placement.Leaf->m_Mins.X;
            float leafHeight = placement.Leaf->m_Maxs.Y - placement.Leaf->m_Mins.Y;
            
            Float2 rectMins = placement.Leaf->m_Mins;
            Float2 rectMaxs = placement.Leaf->m_Maxs;

            switch (placement.Zone)
            {
                case DOCK_ZONE_LEFT:
                    if (DragWidget->m_DockSize.X < leafWidth)
                        splitDistance = DragWidget->m_DockSize.X / leafWidth;

                    rectMaxs.X = Math::Lerp(placement.Leaf->m_Mins.X, placement.Leaf->m_Maxs.X, splitDistance);
                    break;
                case DOCK_ZONE_RIGHT:
                    if (DragWidget->m_DockSize.X < leafWidth)
                        splitDistance = 1.0f - DragWidget->m_DockSize.X / leafWidth;

                    rectMins.X = Math::Lerp(placement.Leaf->m_Mins.X, placement.Leaf->m_Maxs.X, splitDistance);
                    break;
                case DOCK_ZONE_TOP:
                    if (DragWidget->m_DockSize.Y < leafHeight)
                        splitDistance = DragWidget->m_DockSize.Y / leafHeight;

                    rectMaxs.Y = Math::Lerp(placement.Leaf->m_Mins.Y, placement.Leaf->m_Maxs.Y, splitDistance);
                    break;
                case DOCK_ZONE_BOTTOM:
                    if (DragWidget->m_DockSize.Y < leafHeight)
                        splitDistance = 1.0f - DragWidget->m_DockSize.Y / leafHeight;

                    rectMins.Y = Math::Lerp(placement.Leaf->m_Mins.Y, placement.Leaf->m_Maxs.Y, splitDistance);
                    break;
                case DOCK_ZONE_CENTER:
                    break;
            }

            rectMins += Geometry.Mins;
            rectMaxs += Geometry.Mins;

            fillColor.A = 0.2f;
            canvas.DrawRectFilled(rectMins, rectMaxs, fillColor);

            fillColor.A = 1.0f;
            canvas.DrawRect(rectMins, rectMaxs, fillColor, 2);
            #endif
        }
    }
    else if (m_DragSplitter)
    {
        Float2 cursorPos = GUIManager->CursorPosition;
        Float2 dragDelta = cursorPos - m_DragPos;

        if (m_DragSplitter->m_NodeType == UIDockNode::NODE_SPLIT_VERTICAL)
        {
            float w     = m_DragSplitter->m_Maxs.X - m_DragSplitter->m_Mins.X;
            float split = m_StartSplitPos + dragDelta.X;
            split -= m_DragSplitter->m_Mins.X;
            split = Math::Floor(split);
            split = Math::Clamp(split, 1.0f, w - 1.0f);
            split /= w;

            m_DragSplitter->m_SplitDistance = split;
        }
        else if (m_DragSplitter->m_NodeType == UIDockNode::NODE_SPLIT_HORIZONTAL)
        {
            float h     = m_DragSplitter->m_Maxs.Y - m_DragSplitter->m_Mins.Y;
            float split = m_StartSplitPos + dragDelta.Y;
            split -= m_DragSplitter->m_Mins.Y;
            split = Math::Floor(split);
            split = Math::Clamp(split, 1.0f, h - 1.0f);
            split /= h;

            m_DragSplitter->m_SplitDistance = split;
        }

        //m_DragSplitter->UpdateRecursive(m_DragSplitter->m_Mins, m_DragSplitter->m_Maxs);
    }
    else
    {
        Float2 cursorPos = GUIManager->CursorPosition;
        cursorPos -= m_Geometry.PaddedMins;

        UIDockNode* node = m_Root->TraceSeparator(cursorPos.X, cursorPos.Y);
        if (node)
        {
            Float2 bmins, bmaxs;
            const float splitterWidth = 4;

            node->GetSplitterBounds(bmins, bmaxs, splitterWidth);

            bmins += m_Geometry.PaddedMins;
            bmaxs += m_Geometry.PaddedMins;

            canvas.DrawRectFilled(bmins, bmaxs, Color4::Orange());
        }
    }
}

bool UIDockContainer::OnChildrenMouseButtonEvent(MouseButtonEvent const& event)
{
    bool handled = false;

    GUILockViewportScaling = false;

    if (event.Action == IA_PRESS && event.Button == 0)
    {
        m_DragPos = GUIManager->CursorPosition;

        m_DragSplitter = m_Root->TraceSeparator(m_DragPos.X - m_Geometry.PaddedMins.X, m_DragPos.Y - m_Geometry.PaddedMins.Y);
        if (m_DragSplitter)
        {
            if (m_DragSplitter->m_NodeType == UIDockNode::NODE_SPLIT_VERTICAL)
                m_StartSplitPos = Math::Lerp(m_DragSplitter->m_Mins.X, m_DragSplitter->m_Maxs.X, m_DragSplitter->m_SplitDistance);
            else
                m_StartSplitPos = Math::Lerp(m_DragSplitter->m_Mins.Y, m_DragSplitter->m_Maxs.Y, m_DragSplitter->m_SplitDistance);

            GUILockViewportScaling = true;

            handled = true;
        }
    }
    else
        m_DragSplitter.Reset();

    return handled;
}

void UIDockContainer::OnMouseMoveEvent(MouseMoveEvent const& event)
{
}

void UIDockContainer::OnFocusLost()
{
    m_DragSplitter.Reset();
    GUILockViewportScaling = false;
}

UIDockNode::UIDockNode()
{
}

UIDockNode* UIDockNode::TraceLeaf(float x, float y)
{
    if (BvPointInRect(m_Mins, m_Maxs, x, y))
    {
        if (m_NodeType == NODE_LEAF)
            return this;

        for (int i = 0; i < 2; ++i)
        {
            UIDockNode* leaf = m_Child[i]->TraceLeaf(x, y);
            if (leaf)
                return leaf;
        }
    }

    return nullptr;
}

void UIDockNode::GetSplitterBounds(Float2& bmins, Float2& bmaxs, float splitterWidth) const
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

UIDockNode* UIDockNode::TraceSeparator(float x, float y)
{
    if (m_NodeType == NODE_LEAF)
    {
        return nullptr;
    }

    if (!BvPointInRect(m_Mins, m_Maxs, x, y))
    {
        return nullptr;
    }

    {
        const float splitterWidth = 8;

        Float2 bmins, bmaxs;
        GetSplitterBounds(bmins, bmaxs, splitterWidth);

        if (BvPointInRect(bmins, bmaxs, x, y))
        {
            return this;
        }
    }

    UIDockNode* node = m_Child[0]->TraceSeparator(x, y);
    if (node)
        return node;

    return m_Child[1]->TraceSeparator(x, y);
}

void UIDockNode::UpdateRecursive(Float2 const& mins, Float2 const& maxs)
{
    UpdateRecursive(mins.X, mins.Y, maxs.X - mins.X, maxs.Y - mins.Y);
}

void UIDockNode::UpdateRecursive(float x, float y, float w, float h)
{
    m_Mins = {x, y};
    m_Maxs = {x + w, y + h};

    if (m_NodeType == NODE_LEAF)
    {
        if (!m_LeafWidgets.IsEmpty())
        {
            UIDockWidget* dockWidget = m_LeafWidgets[m_WidgetNum];

            dockWidget->m_DockPosition = {x, y};
            dockWidget->m_DockSize     = {w, h};

            //w -= dockWidget->Padding.Left + dockWidget->Padding.Right;
            //h -= dockWidget->Padding.Top + dockWidget->Padding.Bottom;

            //if (w < 0)
            //    w = 0;
            //if (h < 0)
            //    h = 0;

            dockWidget->MeasureLayout(false, false, {w, h});
        }
        return;
    }

    if (m_NodeType == NODE_SPLIT_VERTICAL)
    {
        float d = Math::Floor(m_SplitDistance * w);

        // Left
        m_Child[0]->UpdateRecursive(x, y, d, h);

        // Right
        m_Child[1]->UpdateRecursive(x + d, y, w - d, h);
        return;
    }

    if (m_NodeType == NODE_SPLIT_HORIZONTAL)
    {
        float d = Math::Floor(m_SplitDistance * h);

        // Top
        m_Child[0]->UpdateRecursive(x, y, w, d);

        // Bottom
        m_Child[1]->UpdateRecursive(x, y + d, w, h - d);
        return;
    }
}

UIDockNode* UIDockNode::FindParent(UIDockNode* node)
{
    if (m_NodeType == NODE_LEAF)
        return nullptr;

    for (int i = 0; i < 2; ++i)
        if (m_Child[i] == node)
            return this;
    
    UIDockNode* n = m_Child[0]->FindParent(node);
    if (n)
        return n;

    return m_Child[1]->FindParent(node);
}

void UIDockNode::GetWidgets(TVector<TRef<UIDockWidget>>& widgetList) const
{
    if (m_NodeType == NODE_LEAF)
    {
        for (auto& w : m_LeafWidgets)
            widgetList.Add(w);

        return;
    }

    m_Child[0]->GetWidgets(widgetList);
    m_Child[1]->GetWidgets(widgetList);
}

UIDockContainer::DockLayout::DockLayout(UIDockContainer* self) :
    Self(self)
{
}

Float2 UIDockContainer::DockLayout::MeasureLayout(UIWidget* self, bool, bool, Float2 const& size)
{
    Float2 paddedSize(Math::Max(0.0f, size.X - self->Padding.Left - self->Padding.Right),
                      Math::Max(0.0f, size.Y - self->Padding.Top - self->Padding.Bottom));

    Self->m_Root->UpdateRecursive(Float2(0.0f), paddedSize);

    return paddedSize;
}

void UIDockContainer::DockLayout::ArrangeChildren(UIWidget*, bool, bool)
{
    ArrangeChildren(Self->m_Root);
}

void UIDockContainer::DockLayout::ArrangeChildren(UIDockNode* node)
{
    if (node->m_NodeType == UIDockNode::NODE_LEAF)
    {
        if (!node->m_LeafWidgets.IsEmpty())
        {
            UIWidgetGeometry const& geometry   = Self->m_Geometry;
            UIDockWidget* dockWidget = node->m_LeafWidgets[node->m_WidgetNum];

            dockWidget->m_Geometry.Mins = geometry.PaddedMins + dockWidget->m_DockPosition;
            dockWidget->m_Geometry.Maxs = dockWidget->m_Geometry.Mins + dockWidget->m_DockSize;

            //if ((dockWidget->Geometry.Mins.X < geometry.PaddedMaxs.X) && (dockWidget->Geometry.Mins.Y < geometry.PaddedMaxs.Y))
            {
                dockWidget->ArrangeChildren(false, false);
            }
        }
        return;
    }

    ArrangeChildren(node->m_Child[0]);
    ArrangeChildren(node->m_Child[1]);
}

bool UIDockContainer::AttachWidgetAt(UIDockWidget* widget, float x, float y)
{
    if (!widget)
        return false;

    UIDockPlacement placement = GetPlacement(x, y);
    if (placement)
    {
        float splitDistance = 0.5f;

        float leafWidth  = placement.Leaf->m_Maxs.X - placement.Leaf->m_Mins.X;
        float leafHeight = placement.Leaf->m_Maxs.Y - placement.Leaf->m_Mins.Y;

        switch (placement.Zone)
        {
            case DOCK_ZONE_LEFT:
                if (widget->m_DockSize.X < leafWidth)
                    splitDistance = widget->m_DockSize.X / leafWidth;
                break;
            case DOCK_ZONE_RIGHT:
                if (widget->m_DockSize.X < leafWidth)
                    splitDistance = 1.0f - widget->m_DockSize.X / leafWidth;
                break;
            case DOCK_ZONE_TOP:
                if (widget->m_DockSize.Y < leafHeight)
                    splitDistance = widget->m_DockSize.Y / leafHeight;
                break;
            case DOCK_ZONE_BOTTOM:
                if (widget->m_DockSize.Y < leafHeight)
                    splitDistance = 1.0f - widget->m_DockSize.Y / leafHeight;
                break;
            case DOCK_ZONE_CENTER:
                break;
        }

        if (AttachWidget(widget, placement.Leaf, placement.Zone, splitDistance))
        {
            return true;
        }
    }

    return false;
}

HK_NAMESPACE_END
