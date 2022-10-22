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

#pragma once

#include "WDockWidget.h"

enum DOCK_ZONE : int
{
    DOCK_ZONE_LEFT,
    DOCK_ZONE_RIGHT,
    DOCK_ZONE_TOP,
    DOCK_ZONE_BOTTOM,
    DOCK_ZONE_CENTER
};

class WDockNode : public ABaseObject
{
    HK_CLASS(WDockNode, ABaseObject)

public:
    WDockNode();

    WDockNode* TraceLeaf(float x, float y);

    WDockNode* TraceSeparator(float x, float y);

    void UpdateRecursive(Float2 const& mins, Float2 const& maxs);
    void UpdateRecursive(float x, float y, float w, float h);

    WDockNode* FindParent(WDockNode* node);

    void GetSplitterBounds(Float2& bmins, Float2& bmaxs, float splitterWidth) const;

    void GetWidgets(TVector<TRef<WDockWidget>>& widgetList) const;

private:
    enum NODE_TYPE : int
    {
        NODE_SPLIT_VERTICAL,
        NODE_SPLIT_HORIZONTAL,
        NODE_LEAF        
    };

    NODE_TYPE m_NodeType = NODE_LEAF;

    // [left, right] if NODE_SPLIT_VERTICAL
    // [top, bottom] if NODE_SPLIT_HORIZONTAL
    // [null,  null] if NODE_LEAF
    TRef<WDockNode> m_Child[2];
    TRef<WDockWidget> m_LeafWidget;

    Float2 m_Mins;
    Float2 m_Maxs;

    // Vertical or horizontal split distance 0..1
    float m_SplitDistance = 0.5f;

    friend class WDockContainer;
};

struct WDockPlacement
{
    TRef<WDockNode>   Leaf;
    DOCK_ZONE         Zone;
    TArray<Float2, 4> PolygonVerts;

    operator bool() const
    {
        return Leaf != nullptr;
    }
};

// Widget containing docks.
class WDockContainer : public WWidget
{
    HK_CLASS(WDockContainer, WWidget)

public:
    WDockContainer();
    WDockContainer(AStringView containerName);

    WDockNode* TraceLeaf(float x, float y);

    WDockPlacement GetPlacement(float x, float y);

    AString const& GetContainerName() const { return m_ContainerName; }

    // Returns leaf where widget was placed (on success).
    WDockNode* AttachWidget(WDockWidget* dockWidget, WDockNode* leaf, DOCK_ZONE zone, float splitDistance = 0.5f);

    // Removes widget from dock dock container.
    bool DetachWidget(WDockWidget* dockWidget);

    // Removes widget from dock container. Returns widget pointer on success.
    WDockWidget* DetachWidget(WDockNode* leaf);

    TVector<TRef<WDockWidget>> GetWidgets() const;

    WDockNode* FindParent(WDockNode* node);

    WDockNode* GetRoot() { return m_Root; }

    void UpdateDocks();

protected:
    // FIXME
    void OnTransformDirty() override
    {
        Super::OnTransformDirty();

        UpdateDocks();
    }

    void OnPostDrawEvent(ACanvas& canvas) override;

    void OnMouseMoveEvent(struct SMouseMoveEvent const& event, double timeStamp) override;

private:
    AString m_ContainerName;
    TRef<WDockNode> m_Root;
    TWeakRef<WDockNode> m_DragSplitter;
    Float2 m_DragPos;
    float m_StartSplitPos;
};
