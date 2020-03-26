/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include "ImguiContext.h"

#include <GameThread/Public/EngineInstance.h>
#include <Runtime/Public/InputDefs.h>
#include <World/Public/Components/InputComponent.h>

#include <imgui/imgui.h>

AN_CLASS_META( AImguiContext )

static void SetClipboardText( void *, const char * _Text ) {
    GRuntime.SetClipboard( _Text );
}

static const char * GetClipboardText( void * ) {
    return GRuntime.GetClipboard();
}

AImguiContext::AImguiContext() {
    GUIContext = ImGui::CreateContext();

    ImGuiIO & IO = ImGui::GetIO();
    IO.Fonts = NULL;
    IO.SetClipboardTextFn = SetClipboardText;
    IO.GetClipboardTextFn = GetClipboardText;
    IO.ClipboardUserData = NULL;
    //IO.UserData = this;
    IO.ImeWindowHandle = NULL;
    IO.MouseDrawCursor = false;

    IO.KeyMap[ImGuiKey_Tab] = KEY_TAB;
    IO.KeyMap[ImGuiKey_LeftArrow] = KEY_LEFT;
    IO.KeyMap[ImGuiKey_RightArrow] = KEY_RIGHT;
    IO.KeyMap[ImGuiKey_UpArrow] = KEY_UP;
    IO.KeyMap[ImGuiKey_DownArrow] = KEY_DOWN;
    IO.KeyMap[ImGuiKey_PageUp] = KEY_PAGE_UP;
    IO.KeyMap[ImGuiKey_PageDown] = KEY_PAGE_DOWN;
    IO.KeyMap[ImGuiKey_Home] = KEY_HOME;
    IO.KeyMap[ImGuiKey_End] = KEY_END;
    IO.KeyMap[ImGuiKey_Insert] = KEY_INSERT;
    IO.KeyMap[ImGuiKey_Delete] = KEY_DELETE;
    IO.KeyMap[ImGuiKey_Backspace] = KEY_BACKSPACE;
    IO.KeyMap[ImGuiKey_Space] = KEY_SPACE;
    IO.KeyMap[ImGuiKey_Enter] = KEY_ENTER;
    IO.KeyMap[ImGuiKey_Escape] = KEY_ESCAPE;
    IO.KeyMap[ImGuiKey_A] = KEY_A;
    IO.KeyMap[ImGuiKey_C] = KEY_C;
    IO.KeyMap[ImGuiKey_V] = KEY_V;
    IO.KeyMap[ImGuiKey_X] = KEY_X;
    IO.KeyMap[ImGuiKey_Y] = KEY_Y;
    IO.KeyMap[ImGuiKey_Z] = KEY_Z;

    ImVec2 FramebufferSize(640,480);
    IO.DisplaySize.x = 640;
    IO.DisplaySize.y = 480;
    IO.DisplayFramebufferScale = Float2((float)FramebufferSize.x / IO.DisplaySize.x, (float)FramebufferSize.y / IO.DisplaySize.y);
    IO.DeltaTime = 1.0f / 60.0f;
    IO.MousePos.x = -1;
    IO.MousePos.y = -1;
    for ( int i = 0 ; i < 5 ; i++ ) {
        IO.MouseDown[i] = false;
    }
    IO.MouseWheel = 0;
    IO.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableSetMousePos;

    ImGui::StyleColorsDark( &ImGui::GetStyle() );
}

AImguiContext::~AImguiContext() {
    ImGui::GetIO().Fonts = NULL;

    ImGui::DestroyContext( GUIContext );
}

void AImguiContext::OnKeyEvent( SKeyEvent const & _Event ) {
    ImGuiIO & IO = ImGui::GetIO();

    if ( _Event.Key >= 0 && _Event.Key < AN_ARRAY_SIZE(IO.KeysDown) ) {
        IO.KeysDown[_Event.Key] = ( _Event.Action != IA_Release );
    }

    IO.KeyCtrl = IO.KeysDown[ KEY_LEFT_CONTROL ] || IO.KeysDown[ KEY_RIGHT_CONTROL ];
    IO.KeyShift = IO.KeysDown[ KEY_LEFT_SHIFT ] || IO.KeysDown[ KEY_RIGHT_SHIFT ];
    IO.KeyAlt = IO.KeysDown[ KEY_LEFT_ALT ] || IO.KeysDown[ KEY_RIGHT_ALT ];
    IO.KeySuper = IO.KeysDown[ KEY_LEFT_SUPER ] || IO.KeysDown[ KEY_RIGHT_SUPER ];
}

void AImguiContext::OnCharEvent( SCharEvent const & _Event ) {
    ImGuiIO & IO = ImGui::GetIO();

    IO.AddInputCharacter( _Event.UnicodeCharacter );
}

void AImguiContext::OnMouseButtonEvent( SMouseButtonEvent const & _Event ) {
    ImGuiIO & IO = ImGui::GetIO();

    if ( _Event.Button < 5 ) {
        IO.MouseDown[ _Event.Button ] = ( _Event.Action != IA_Release );
    }
}

void AImguiContext::OnMouseWheelEvent( SMouseWheelEvent const & _Event ) {
    ImGuiIO & IO = ImGui::GetIO();

    if ( _Event.WheelY < 0.0 ) {
        IO.MouseWheel -= 1.0f;
    } else if ( _Event.WheelY > 0.0 ) {
        IO.MouseWheel += 1.0f;
    }
}

void AImguiContext::SetFont( AFont * _Font ) {
    ImGuiIO & IO = ImGui::GetIO();

    IO.Fonts = (ImFontAtlas *)_Font->GetImguiFontAtlas();
}

void AImguiContext::BeginFrame( float _TimeStep ) {
    SVideoMode const & videoMode = GRuntime.GetVideoMode();
    //Float2 const & cursorPosition = AInputComponent::GetCursorPosition();
    Float2 cursorPosition(0); // TODO: = Desktop->GetCursorPosition();

    ImGuiIO & IO = ImGui::GetIO();

    IO.DisplaySize.x = videoMode.Width;
    IO.DisplaySize.y = videoMode.Height;
    IO.DisplayFramebufferScale = GEngine.GetRetinaScale();
    IO.DeltaTime = _TimeStep;
    IO.MousePos.x = cursorPosition.X;
    IO.MousePos.y = cursorPosition.Y;

    ImGui::NewFrame();

    if ( IO.WantSetMousePos ) {
        //AInputComponent::SetCursorPosition( IO.MousePos.x, IO.MousePos.y );
        // TODO: Desktop->SetCursorPosition( IO.MousePos.x, IO.MousePos.y );
    }
}

void AImguiContext::EndFrame() {
    ImGui::Render();
}
