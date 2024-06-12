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
#include <Engine/GameApplication/FrameLoop.h>
#include <Engine/GameApplication/GameApplication.h>
#include <Engine/World/World.h>

HK_NAMESPACE_BEGIN

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
        GameApplication::GetInputSystem().SetKeyState(virtualKey, InputEvent::OnPress, event.ModMask);
    else if (event.Action == InputAction::Released)
        GameApplication::GetInputSystem().SetKeyState(virtualKey, InputEvent::OnRelease, event.ModMask);
}

void UIViewport::OnMouseButtonEvent(MouseButtonEvent const& event)
{
    if (event.Action == InputAction::Pressed)
        GameApplication::GetInputSystem().SetKeyState(event.Button, InputEvent::OnPress, event.ModMask);
    else if (event.Action == InputAction::Released)
        GameApplication::GetInputSystem().SetKeyState(event.Button, InputEvent::OnRelease, event.ModMask);
}

void UIViewport::OnMouseWheelEvent(MouseWheelEvent const& event)
{
}

void UIViewport::OnMouseMoveEvent(MouseMoveEvent const& event)
{
    GameApplication::GetInputSystem().SetMouseAxisState(event.X, event.Y);

    UpdateViewSize();

    Float2 const& pos  = m_Geometry.Mins;
    Float2 size = m_Geometry.Maxs - m_Geometry.Mins;

    //GameApplication::GetInputSystem().SetCursorPosition((GUIManager->CursorPosition - pos) / size * Float2(m_ViewWidth, m_ViewHeight));
    GameApplication::GetInputSystem().SetCursorPosition((GUIManager->CursorPosition - pos) / size);
}

void UIViewport::OnJoystickButtonEvent(JoystickButtonEvent const& event)
{
    // TODO
}

void UIViewport::OnJoystickAxisEvent(JoystickAxisEvent const& event)
{
    // TODO
}

void UIViewport::OnCharEvent(CharEvent const& event)
{
    GameApplication::GetInputSystem().NotifyUnicodeCharacter(event.UnicodeCharacter, event.ModMask);
}

void UIViewport::OnFocusLost()
{
    GameApplication::GetInputSystem().ResetKeyState();
}

void UIViewport::OnFocusReceive()
{
}

void UIViewport::UpdateViewSize() // TODO: PostGeometryUpdate?
{
    if (!GUILockViewportScaling)
    {
        int x = m_Geometry.Mins.X;
        int y = m_Geometry.Mins.Y;

        m_ViewWidth  = Math::Max(0.0f, m_Geometry.Maxs.X - x);
        m_ViewHeight = Math::Max(0.0f, m_Geometry.Maxs.Y - y);
    }
}

void UIViewport::Draw(Canvas& canvas)
{
    UpdateViewSize();

    if (!m_WorldRenderView)
        return;

    Float2 const& pos = m_Geometry.Mins;
    Float2 size = m_Geometry.Maxs - m_Geometry.Mins;

    if (size.X >= 1 && size.Y >= 1)
    {
        m_WorldRenderView->SetViewport(size.X, size.Y);

        if (auto* world = m_WorldRenderView->GetWorld())
        {
            if (auto* cameraComponent = world->GetComponent(m_WorldRenderView->GetCamera()))
            {
                float aspectRatio;
                if (m_ViewWidth > 0 && m_ViewHeight > 0)
                    aspectRatio = (float)m_ViewWidth / m_ViewHeight;
                else
                    aspectRatio = 1;

                DisplayVideoMode const& vidMode = GameApplication::GetVideoMode();
                cameraComponent->SetAspectRatio(aspectRatio * vidMode.AspectScale);

                GameApplication::GetFrameLoop().RegisterView(m_WorldRenderView);

                DrawTextureDesc desc;
                desc.TexHandle = m_WorldRenderView->GetTextureHandle();
                desc.X = pos.X;
                desc.Y = pos.Y;
                desc.W = size.X;
                desc.H = size.Y;
                desc.Rounding = Rounding;
                desc.Angle = 0;
                desc.TintColor = TintColor;
                desc.Composite = Composite;
                desc.bFlipY = true;

                canvas.DrawTexture(desc);
            }
        }
        //    hud->DrawHUD(canvas, pos.X, pos.Y, size.X, size.Y);
    }
}

HK_NAMESPACE_END
