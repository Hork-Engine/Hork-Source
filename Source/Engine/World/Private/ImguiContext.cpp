/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include <Engine/World/Public/ImguiContext.h>
#include <Engine/World/Public/GameMaster.h>
#include <Engine/Runtime/Public/InputDefs.h>

#include <Engine/imgui/imgui.h>

AN_CLASS_META_NO_ATTRIBS( FImguiContext )

FImguiContext::FImguiContext() {
    GUIContext = ImGui::CreateContext();

    ImGuiIO & IO = ImGui::GetIO();
    IO.Fonts = NULL;//&FontAtlas;
    IO.SetClipboardTextFn = NULL;// []( void *, const char * _Text ) { GPlatformPort->SetClipboard( _Text ); };
    IO.GetClipboardTextFn = NULL;// []( void * ) { return GPlatformPort->GetClipboard(); };
    IO.ClipboardUserData = NULL;
    //IO.UserData = this;
    IO.ImeWindowHandle = NULL;

    IO.KeyMap[ImGuiKey_Tab] = KEY_TAB;
    IO.KeyMap[ImGuiKey_LeftArrow] = KEY_LEFT;
    IO.KeyMap[ImGuiKey_RightArrow] = KEY_RIGHT;
    IO.KeyMap[ImGuiKey_UpArrow] = KEY_UP;
    IO.KeyMap[ImGuiKey_DownArrow] = KEY_DOWN;
    IO.KeyMap[ImGuiKey_PageUp] = KEY_PAGE_UP;
    IO.KeyMap[ImGuiKey_PageDown] = KEY_PAGE_DOWN;
    IO.KeyMap[ImGuiKey_Home] = KEY_HOME;
    IO.KeyMap[ImGuiKey_End] = KEY_END;
    IO.KeyMap[ImGuiKey_Delete] = KEY_DELETE;
    IO.KeyMap[ImGuiKey_Backspace] = KEY_BACKSPACE;
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

    ImGui::StyleColorsLight( &ImGui::GetStyle() );

    //ImGui::NewFrame();
}

FImguiContext::~FImguiContext() {
    ImGui::GetIO().Fonts = NULL;

    //ImGui::Shutdown();

    ImGui::DestroyContext( GUIContext );
    GUIContext = NULL;
}

void FImguiContext::OnKeyEvent( FKeyEvent const & _Event ) {
    ImGuiIO & IO = ImGui::GetIO();

    if ( _Event.Key >= 0 && _Event.Key < AN_ARRAY_LENGTH(IO.KeysDown) ) {
        IO.KeysDown[_Event.Key] = ( _Event.Action != IE_Release );
    }

    IO.KeyCtrl = IO.KeysDown[ KEY_LEFT_CONTROL ] || IO.KeysDown[ KEY_RIGHT_CONTROL ];
    IO.KeyShift = IO.KeysDown[ KEY_LEFT_SHIFT ] || IO.KeysDown[ KEY_RIGHT_SHIFT ];
    IO.KeyAlt = IO.KeysDown[ KEY_LEFT_ALT ] || IO.KeysDown[ KEY_RIGHT_ALT ];
}

void FImguiContext::OnCharEvent( FCharEvent const & _Event ) {
    ImGuiIO & IO = ImGui::GetIO();

    IO.AddInputCharacter( _Event.UnicodeCharacter );
}

void FImguiContext::OnMouseButtonEvent( FMouseButtonEvent const & _Event ) {
    ImGuiIO & IO = ImGui::GetIO();

    if ( _Event.Button < 5 ) {
        IO.MouseDown[ _Event.Button ] = ( _Event.Action != IE_Release );
    }
}

void FImguiContext::OnMouseWheelEvent( FMouseWheelEvent const & _Event ) {
    ImGuiIO & IO = ImGui::GetIO();

    if ( _Event.WheelY < 0.0 ) {
        IO.MouseWheel -= 1.0f;
    } else if ( _Event.WheelY > 0.0 ) {
        IO.MouseWheel += 1.0f;
    }
}

void FImguiContext::SetFontAtlas( ImFontAtlas * _Atlas ) {
    ImGuiIO & IO = ImGui::GetIO();

    IO.Fonts = _Atlas;
}

void FImguiContext::BeginFrame( float _TimeStep ) {
    FVideoMode const & VideoMode = GGameMaster.GetVideoMode();
    Float2 const & CursorPosition = GGameMaster.GetCursorPosition();

    ImGuiIO & IO = ImGui::GetIO();

    IO.DisplaySize.x = VideoMode.Width;
    IO.DisplaySize.y = VideoMode.Height;
    IO.DisplayFramebufferScale = GGameMaster.GetRetinaScale();
    IO.DeltaTime = _TimeStep;
    IO.MousePos.x = CursorPosition.X;
    IO.MousePos.y = CursorPosition.Y;
    IO.MouseDrawCursor = true;

    ImGui::NewFrame();
}

void FImguiContext::EndFrame() {
    ImGui::ShowDemoWindow();

    ImGui::Render();

    ImDrawData * drawData = ImGui::GetDrawData();
    if ( drawData->CmdListsCount > 0 ) {
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fb_width = drawData->DisplaySize.x * drawData->FramebufferScale.x;
        int fb_height = drawData->DisplaySize.y * drawData->FramebufferScale.y;
        if ( fb_width == 0 || fb_height == 0 ) {
            return;
        }

        if ( drawData->FramebufferScale.x != 1.0f || drawData->FramebufferScale.y != 1.0f ) {
            drawData->ScaleClipRects( drawData->FramebufferScale );
        }

        for ( int n = 0; n < drawData->CmdListsCount ; n++ ) {
//            GRenderFrontend.WriteDrawList( drawData->CmdLists[ n ] );
        }
    }
}
