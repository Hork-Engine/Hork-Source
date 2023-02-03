/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "UIDockWidget.h"

HK_NAMESPACE_BEGIN

enum DOCK_ZONE : int
{
    DOCK_ZONE_LEFT,
    DOCK_ZONE_RIGHT,
    DOCK_ZONE_TOP,
    DOCK_ZONE_BOTTOM,
    DOCK_ZONE_CENTER
};

class UIDockNode : public UIObject
{
    UI_CLASS(UIDockNode, UIObject)

public:
    UIDockNode();

    UIDockNode* TraceLeaf(float x, float y);

    UIDockNode* TraceSeparator(float x, float y);

    void UpdateRecursive(Float2 const& mins, Float2 const& maxs);
    void UpdateRecursive(float x, float y, float w, float h);

    UIDockNode* FindParent(UIDockNode* node);

    void GetSplitterBounds(Float2& bmins, Float2& bmaxs, float splitterWidth) const;

    void GetWidgets(TVector<TRef<UIDockWidget>>& widgetList) const;

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
    TRef<UIDockNode>  m_Child[2];
    TVector<TRef<UIDockWidget>> m_LeafWidgets;
    int                         m_WidgetNum = 0;// TODO

    Float2 m_Mins;
    Float2 m_Maxs;

    // Vertical or horizontal split distance 0..1
    float m_SplitDistance = 0.5f;

    friend class UIDockContainer;
};

struct UIDockPlacement
{
    TRef<UIDockNode>  Leaf;
    DOCK_ZONE         Zone;
    TArray<Float2, 4> PolygonVerts;

    operator bool() const
    {
        return Leaf != nullptr;
    }
};

// Widget containing docks.
class UIDockContainer : public UIWidget
{
    UI_CLASS(UIDockContainer, UIWidget)

public:
    UIDockContainer();
    UIDockContainer(StringView containerName);

    UIDockNode* TraceLeaf(float x, float y);

    UIDockPlacement GetPlacement(float x, float y);

    String const& GetContainerName() const { return m_ContainerName; }

    bool AttachWidgetAt(UIDockWidget* widget, float x, float y);

    // Returns leaf where widget was placed (on success).
    UIDockNode* AttachWidget(UIDockWidget* dockWidget, UIDockNode* leaf, DOCK_ZONE zone, float splitDistance = 0.5f);

    // Removes widget from dock dock container.
    bool DetachWidget(UIDockWidget* dockWidget);

    // Removes widget from dock container. Returns widget pointer on success.
    UIDockWidget* DetachWidget(UIDockNode* leaf, int index);

    TVector<TRef<UIDockWidget>> GetWidgets() const;

    UIDockNode* FindParent(UIDockNode* node);

    UIDockNode* GetRoot() { return m_Root; }

    bool bDrawPlacement = false;
    TRef<UIDockWidget> DragWidget;

protected:
    void PostDraw(Canvas& canvas) override;

    bool OnChildrenMouseButtonEvent(struct MouseButtonEvent const& event, double timeStamp) override;

    void OnMouseMoveEvent(struct MouseMoveEvent const& event, double timeStamp) override;

    void OnFocusLost() override;

private:
    struct DockLayout : UIBaseLayout
    {
        UIDockContainer* Self;

        DockLayout(UIDockContainer* self);

        Float2 MeasureLayout(UIWidget* self, bool bAutoWidth, bool bAutoHeight, Float2 const& size) override;

        void ArrangeChildren(UIWidget* self, bool bAutoWidth, bool bAutoHeight) override;

        void ArrangeChildren(UIDockNode* node);
    };

    String m_ContainerName;
    TRef<UIDockNode>    m_Root;
    TWeakRef<UIDockNode> m_DragSplitter;
    Float2 m_DragPos;
    float m_StartSplitPos;
};

HK_NAMESPACE_END
