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

#include <World/Public/Base/GameModuleInterface.h>
#include <Runtime/Public/EngineInterface.h>
#include <Runtime/Public/RuntimeCommandProcessor.h>
#include <World/Public/Resource/FontAtlas.h>
#include <World/Public/Widgets/WDesktop.h>
#include <World/Public/World.h>

struct SVideoMode
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

class ANGIE_API AEngineInstance : public IEngineInterface
{
    AN_SINGLETON( AEngineInstance )

public:
    /** Quit when user press ESCAPE */
    bool bQuitOnEscape = true;

    /** Toggle fullscreen on ALT+ENTER */
    bool bToggleFullscreenAltEnter = true;

    /** Allow to drop down the console */
    bool bAllowConsole = true;

    /** Global mouse sensitivity */
    float MouseSensitivity = 1.0f;

    ACanvas Canvas;

    /** Helper. Create a new world */
    AWorld * CreateWorld() { return AWorld::CreateWorld(); }

    /** Helper. Destroy all existing worlds */
    void DestroyWorlds() { return AWorld::DestroyWorlds(); }

    /** Helper. Get all existing worlds */
    TPodArray< AWorld * > const & GetWorld() { return AWorld::GetWorlds(); }

    /** Get current frame update number */
    int GetFrameNumber() const { return FrameNumber; }

    /** Change a video mode */
    void SetVideoMode( unsigned short _Width, unsigned short _Height, unsigned short _PhysicalMonitor, byte _RefreshRate, bool _Fullscreen, const char * _Backend );

    /** Change a video mode */
    void SetVideoMode( SVideoMode const & _VideoMode );

    /** Get current video mode */
    SVideoMode const & GetVideoMode() const;

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

    /** Set game module */
    IGameModule * GetGameModule() { return GameModule; }

    /** Set hud desktop */
    void SetDesktop( WDesktop * _Desktop );

    /** Get hud desktop */
    WDesktop * GetDesktop() { return Desktop; }

    ARuntimeCommandProcessor & GetCommandProcessor() { return CommandProcessor; }

private:
    /** IEngineInterface interface. Initialize the engine */
    void Initialize( ACreateGameModuleCallback _CreateGameModuleCallback ) override;

    /** IEngineInterface interface. Shutdown the engine */
    void Deinitialize() override;

    /** IEngineInterface interface. Prepare a frame for rendering */
    void PrepareFrame() override;

    /** IEngineInterface interface. Update game frame */
    void UpdateFrame() override;

    /** IEngineInterface interface. Message print callback. This must be a thread-safe function. */
    void Print( const char * _Message ) override;

    // Process runtime event
    void ProcessEvent( struct SEvent const & _Event );
    void ProcessEvents();

    // Send event to runtime
    SEvent & SendEvent();

    void OnKeyEvent( struct SKeyEvent const & _Event, double _TimeStamp );
    void OnMouseButtonEvent( struct SMouseButtonEvent const & _Event, double _TimeStamp );
    void OnMouseWheelEvent( struct SMouseWheelEvent const & _Event, double _TimeStamp );
    void OnMouseMoveEvent( struct SMouseMoveEvent const & _Event, double _TimeStamp );
    void OnCharEvent( struct SCharEvent const & _Event, double _TimeStamp );
    void OnChangedVideoModeEvent( struct SChangedVideoModeEvent const & _Event );

    public:void UpdateInputAxes( float _Fract );private: // !!! Temp solution !!!

    // Used to debug some features. Must be removed from release.
    void DeveloperKeys( struct SKeyEvent const & _Event );

    // Tick the game
    void UpdateWorlds();

    void KickoffPendingKillWorlds();

    void DrawCanvas();

    void UpdateImgui();

    void ResetVideoMode();

    void ShowStats();

    SVideoMode VideoMode;
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

    //class AImguiContext * ImguiContext;

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

    ARuntimeCommandProcessor CommandProcessor;

    float AxesFractAvg = 1;
    float AxesFract = 1;
};

extern ANGIE_API AEngineInstance & GEngine;
