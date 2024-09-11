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

#include "UIViewport.h"
#include "UIManager.h"
#include <Hork/Runtime/GameApplication/FrameLoop.h>
#include <Hork/Runtime/GameApplication/GameApplication.h>
#include <Hork/Runtime/World/World.h>

HK_NAMESPACE_BEGIN

ConsoleVar rt_UseWideScreenCorrection("rt_UseWideScreenCorrection"_s, "0"_s);

bool GUILockViewportScaling = false;

UIViewport::UIViewport()
{}

UIViewport& UIViewport::SetWorldRenderView(WorldRenderView* worldRenderView)
{
    m_WorldRenderView = worldRenderView;
    return *this;
}

UIViewport& UIViewport::WithRounding(RoundingDesc const& rounding)
{
    Rounding = rounding;
    return *this;
}

UIViewport& UIViewport::WithTint(Color4 const& tintColor)
{
    TintColor = tintColor;
    return *this;
}

UIViewport& UIViewport::WithComposite(CANVAS_COMPOSITE composite)
{
    Composite = composite;
    return *this;
}

void UIViewport::OnKeyEvent(KeyEvent const& event)
{
    VirtualKey virtualKey = VirtualKey(event.Key);
    if (event.Action == InputAction::Pressed)
        GameApplication::sGetInputSystem().SetKeyState(virtualKey, InputEvent::OnPress, event.ModMask);
    else if (event.Action == InputAction::Released)
        GameApplication::sGetInputSystem().SetKeyState(virtualKey, InputEvent::OnRelease, event.ModMask);
}

void UIViewport::OnMouseButtonEvent(MouseButtonEvent const& event)
{
    if (event.Action == InputAction::Pressed)
        GameApplication::sGetInputSystem().SetKeyState(event.Button, InputEvent::OnPress, event.ModMask);
    else if (event.Action == InputAction::Released)
        GameApplication::sGetInputSystem().SetKeyState(event.Button, InputEvent::OnRelease, event.ModMask);
}

void UIViewport::OnMouseWheelEvent(MouseWheelEvent const& event)
{
}

void UIViewport::OnMouseMoveEvent(MouseMoveEvent const& event)
{
    GameApplication::sGetInputSystem().SetMouseAxisState(event.X, event.Y);

    UpdateViewSize();

    Float2 const& pos  = m_Geometry.Mins;
    Float2 size = m_Geometry.Maxs - m_Geometry.Mins;

    //GameApplication::sGetInputSystem().SetCursorPosition((GUIManager->CursorPosition - pos) / size * Float2(m_ViewWidth, m_ViewHeight));
    GameApplication::sGetInputSystem().SetCursorPosition((GUIManager->CursorPosition - pos) / size);
}

void UIViewport::OnGamepadButtonEvent(GamepadKeyEvent const& event)
{
    if (event.Action == InputAction::Pressed)
        GameApplication::sGetInputSystem().SetGamepadButtonState(event.Key, InputEvent::OnPress, PlayerController(event.AssignedPlayerIndex));
    else if (event.Action == InputAction::Released)
        GameApplication::sGetInputSystem().SetGamepadButtonState(event.Key, InputEvent::OnRelease, PlayerController(event.AssignedPlayerIndex));
}

void UIViewport::OnGamepadAxisMotionEvent(GamepadAxisMotionEvent const& event)
{
    GameApplication::sGetInputSystem().SetGamepadAxis(event.Axis, event.Value, PlayerController(event.AssignedPlayerIndex));
}

void UIViewport::OnCharEvent(CharEvent const& event)
{
    GameApplication::sGetInputSystem().AddCharacter(event.UnicodeCharacter, event.ModMask);
}

void UIViewport::OnFocusLost()
{
    GameApplication::sGetInputSystem().ResetKeyState();
}

void UIViewport::OnFocusReceive()
{
}

void UIViewport::UpdateViewSize() // TODO: PostGeometryUpdate?
{
    if (!GUILockViewportScaling)
    {
        m_ViewWidth  = static_cast<int>(Math::Max(0.0f, m_Geometry.Maxs.X - Math::Floor(m_Geometry.Mins.X)));
        m_ViewHeight = static_cast<int>(Math::Max(0.0f, m_Geometry.Maxs.Y - Math::Floor(m_Geometry.Mins.Y)));
    }
}

void UIViewport::Clear(Canvas& canvas)
{
    CANVAS_COMPOSITE currentComposite = canvas.CompositeOperation(Composite);
    canvas.DrawRectFilled(m_Geometry.Mins, m_Geometry.Maxs, Color4::sBlack());
    canvas.CompositeOperation(currentComposite);
}

void UIViewport::Draw(Canvas& canvas)
{
    UpdateViewSize();

    if (!m_WorldRenderView)
    {
        Clear(canvas);
        return;
    }

    Float2 size = m_Geometry.Maxs - m_Geometry.Mins;
    if (size.X < 1 && size.Y < 1)
    {
        Clear(canvas);
        return;
    }

    //m_WorldRenderView->SetViewport(size.X, size.Y);
    m_WorldRenderView->SetViewport(m_ViewWidth, m_ViewHeight);

    auto world = m_WorldRenderView->GetWorld();
    if (!world)
    {
        Clear(canvas);
        return;
    }

    auto camera = world->GetComponent(m_WorldRenderView->GetCamera());
    if (!camera || !camera->IsInitialized())
    {
        Clear(canvas);
        return;
    }

    float aspectScale = 1;
    if (rt_UseWideScreenCorrection)
        aspectScale = GUIManager->GetGenericWindow()->GetWideScreenCorrection();

    camera->SetViewportPosition(m_Geometry.Mins);
    camera->SetViewportSize({static_cast<float>(m_ViewWidth), static_cast<float>(m_ViewHeight)}, aspectScale);

    GameApplication::sGetFrameLoop().RegisterView(m_WorldRenderView);

    m_WorldRenderView->AcquireRenderTarget();

    DrawTextureDesc desc;
    desc.TexHandle = m_WorldRenderView->GetTextureHandle();
    desc.X = m_Geometry.Mins.X;
    desc.Y = m_Geometry.Mins.Y;
    desc.W = size.X;
    desc.H = size.Y;
    desc.Rounding = Rounding;
    desc.Angle = 0;
    desc.TintColor = TintColor;
    desc.Composite = Composite;
    desc.bFlipY = true;
    canvas.DrawTexture(desc);
}

HK_NAMESPACE_END
