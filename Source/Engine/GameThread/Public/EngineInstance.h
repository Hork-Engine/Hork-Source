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

#pragma once

#include <Engine/Base/Public/GameModuleInterface.h>
#include <Engine/Runtime/Public/EngineInterface.h>
#include <Engine/Runtime/Public/RuntimeCommandProcessor.h>
#include <Engine/Resource/Public/FontAtlas.h>
#include <Engine/Widgets/Public/WDesktop.h>
#include <Engine/World/Public/World.h>

struct FVideoMode
{
    /** Horizontal display resolution */
    unsigned short Width;
    /** Vertical display resolution */
    unsigned short Height;
    /** Phyisical monitor */
    unsigned short PhysicalMonitor;
    /** Display refresh rate */
    byte RefreshRate;
    /** Fullscreen or Windowed mode */
    bool bFullscreen;
    /** Render backend name */
    char Backend[32];
};

class ANGIE_API FEngineInstance : public IEngineInterface
{
    AN_SINGLETON( FEngineInstance )

public:
    /** Quit when user press ESCAPE */
    bool bQuitOnEscape = true;

    /** Toggle fullscreen on ALT+ENTER */
    bool bToggleFullscreenAltEnter = true;

    /** Allow to drop down the console */
    bool bAllowConsole = true;

    /** Global mouse sensitivity */
    float MouseSensitivity = 1.0f;

    /** Helper. Create a new world */
    FWorld * CreateWorld() { return FWorld::CreateWorld(); }

    /** Helper. Destroy all existing worlds */
    void DestroyWorlds() { return FWorld::DestroyWorlds(); }

    /** Helper. Get all existing worlds */
    TPodArray< FWorld * > const & GetWorld() { return FWorld::GetWorlds(); }

    /** Get current frame update number */
    int GetFrameNumber() const { return FrameNumber; }

    /** Change a video mode */
    void SetVideoMode( unsigned short _Width, unsigned short _Height, unsigned short _PhysicalMonitor, byte _RefreshRate, bool _Fullscreen, const char * _Backend );

    /** Change a video mode */
    void SetVideoMode( FVideoMode const & _VideoMode );

    /** Get current video mode */
    FVideoMode const & GetVideoMode() const;

    /** Get video mode aspect ratio */
    float GetVideoAspectRatio() const { return VideoAspectRatio; }

    /** Get framebuffer size */
    float GetFramebufferWidth() const { return FramebufferWidth; }

    /** Get framebuffer size */
    float GetFramebufferHeight() const { return FramebufferHeight; }

    /** Get scale for retina displays */
    Float2 const & GetRetinaScale() const { return RetinaScale; }

    /** Get dots per inch for current video mode */
    float GetDPIX() const { return DPI_X; }

    /** Get dots per inch for current video mode */
    float GetDPIY() const { return DPI_Y; }

    /** Change window parameters */
    void SetWindowDefs( float _Opacity, bool _Decorated, bool _AutoIconify, bool _Floating, const char * _Title );

    /** Change window position */
    void SetWindowPos( int _X, int _Y );

    /** Get current window position */
    void GetWindowPos( int & _X, int & _Y );

    /** Set window in focus */
    void SetInputFocus();

    /** Is window in focus */
    bool IsInputFocus() const { return bInputFocus; }

    /** Get window visible status */
    bool IsWindowVisible() const { return bIsWindowVisible; }

    void SetCursorEnabled( bool _Enabled );    

    /** Map coordinate from window space to monitor space */
    void MapWindowCoordinate( float & InOutX, float & InOutY ) const;

    /** Map coordinate from monitor space to window space */
    void UnmapWindowCoordinate( float & InOutX, float & InOutY ) const;

    /** Set cursor position */
    void SetCursorPosition( float _X, float _Y ) { CursorPosition.X = _X; CursorPosition.Y = _Y; }

    /** Get cursor position */
    Float2 const & GetCursorPosition() const { return CursorPosition; }

    /** Set game module */
    IGameModule * GetGameModule() { return GameModule; }

    /** Set hud desktop */
    void SetDesktop( WDesktop * _Desktop );

    /** Get hud desktop */
    WDesktop * GetDesktop() { return Desktop; }

private:
    /** IEngineInterface interface. Initialize the engine */
    void Initialize( FCreateGameModuleCallback _CreateGameModuleCallback ) override;

    /** IEngineInterface interface. Shutdown the engine */
    void Deinitialize() override;

    /** IEngineInterface interface. Prepare a frame for rendering */
    void PrepareFrame() override;

    /** IEngineInterface interface. Update game frame */
    void UpdateFrame() override;

    /** IEngineInterface interface. Print callback */
    void Print( const char * _Message ) override;

    // Process runtime event
    void ProcessEvent( struct FEvent const & _Event );
    void ProcessEvents();

    // Send event to runtime
    FEvent & SendEvent();

    void OnKeyEvent( struct FKeyEvent const & _Event, double _TimeStamp );
    void OnMouseButtonEvent( struct FMouseButtonEvent const & _Event, double _TimeStamp );
    void OnMouseWheelEvent( struct FMouseWheelEvent const & _Event, double _TimeStamp );
    void OnMouseMoveEvent( struct FMouseMoveEvent const & _Event, double _TimeStamp );
    void OnCharEvent( struct FCharEvent const & _Event, double _TimeStamp );
    void OnChangedVideoModeEvent( struct FChangedVideoModeEvent const & _Event );

    public:void UpdateInputAxes( float _Fract );private: // !!! Temp solution !!!

    // Used to debug some features. Must be removed from release.
    void DeveloperKeys( struct FKeyEvent const & _Event );

    // Tick the game
    void UpdateWorlds();

    void KickoffPendingKillWorlds();

    void DrawCanvas();

    void UpdateImgui();

    void ResetVideoMode();

    FVideoMode VideoMode;
    float VideoAspectRatio = 4.0f/3.0f;
    float FramebufferWidth;
    float FramebufferHeight;
    Float2 RetinaScale; // scale coordinates for retina displays
    bool bInputFocus;
    bool bIsWindowVisible;
    int WindowPosX;
    int WindowPosY;

    float DPI_X;
    float DPI_Y;

    Float2 CursorPosition;

    //class FImguiContext * ImguiContext;

    // Frame update number
    int FrameNumber = 0;

    // Frame update duration
    float FrameDurationInSeconds = 0;

    // Frame duration in microseconds
    int64_t FrameDuration;

    // System time at frame start
    int64_t FrameTimeStamp;

    IGameModule * GameModule;

    TRef< WDesktop > Desktop;

    FRuntimeCommandProcessor CommandProcessor;
};

extern ANGIE_API FEngineInstance & GEngine;
