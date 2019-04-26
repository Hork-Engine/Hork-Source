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

#include "Factory.h"
#include "BaseObject.h"
#include <Engine/Core/Public/Document.h>
#include <Engine/Runtime/Public/GameModuleCallback.h>

class FWorld;
class FImguiContext;
struct ImFont;

struct FWorldSpawnParameters {
    FWorldSpawnParameters() = delete;

    FWorldSpawnParameters( FClassMeta const * _WorldTypeClassMeta )
        : Template( nullptr )
        , WorldTypeClassMeta( _WorldTypeClassMeta )
    {
    }

    // FIXME: This might be useful from scripts
#if 0
    FWorldSpawnParameters( uint64_t _WorldClassId )
        : FWorldSpawnParameters( FWorld::Factory().LookupClass( _WorldClassId ) )
    {
    }

    FWorldSpawnParameters( const char * _WorldClassName )
        : FWorldSpawnParameters( FWorld::Factory().LookupClass( _WorldClassName ) )
    {
    }
#endif

    void SetTemplate( FWorld const * _Template );

    FClassMeta const * WorldClassMeta() const { return WorldTypeClassMeta; }
    FWorld const * GetTemplate() const { return Template; }

    // TODO: here world spawn parameters

protected:
    FWorld const * Template;    // template type meta must match WorldTypeClassMeta
    FClassMeta const * WorldTypeClassMeta;
};

template< typename WorldType >
struct TWorldSpawnParameters : FWorldSpawnParameters {
    TWorldSpawnParameters()
        : FWorldSpawnParameters( &WorldType::ClassMeta() )
    {
    }
};

struct FVideoMode {
    unsigned short Width;
    unsigned short Height;
    unsigned short PhysicalMonitor;
    byte RefreshRate;
    bool bFullscreen;
    char Backend[32];
};

class ANGIE_API FGameMaster {
    AN_SINGLETON( FGameMaster )

    friend class FWorld;

public:
    bool bQuitOnEscape = true;
    bool bToggleFullscreenAltEnter = true;
    bool bAllowConsole = true;
//    int GameHertz = 120;//30;//60;
    int PhysicsHertz = 24;//30;
    bool bEnablePhysicsInterpolation = true;
    bool bContactSolverSplitImpulse = false; // disabled for performance
    int NumContactSolverIterations = 10;
    float MouseSensitivity = 1.0f;

    // Spawn a new world
    FWorld * SpawnWorld( FWorldSpawnParameters const & _SpawnParameters );

    // Spawn a new world
    template< typename WorldType >
    WorldType * SpawnWorld( TWorldSpawnParameters< WorldType > const & _SpawnParameters ) {
        FWorldSpawnParameters const & spawnParameters = _SpawnParameters;
        return static_cast< WorldType * >( SpawnWorld( spawnParameters ) );
    }

    // Spawn a new world
    template< typename WorldType >
    WorldType * SpawnWorld() {
        TWorldSpawnParameters< WorldType > spawnParameters;
        return static_cast< WorldType * >( SpawnWorld( spawnParameters ) );
    }

    // Load world from document data
    FWorld * LoadWorld( FDocument const & _Document, int _FieldsHead );

    // Destroy all existing worlds
    void DestroyWorlds();

    // Get all worlds
    TPodArray< FWorld * > const & GetWorlds() const { return Worlds; }

    // Get current tick number
    int GetTickNumber() const { return TickNumber; }

    // Get current frame update number
    int GetFrameNumber() const { return FrameNumber; }

    // Get current frame update duration
    float GetFrameDurationInSeconds() { return FrameDurationInSeconds; }

    // System time at frame start
    int64_t GetFrameTimeStamp() const { return FrameTimeStamp; }

    // System time at tick start
    int64_t GetTickTimeStamp() const { return TickTimeStamp; }

    // Game virtual time based on frame step
    int64_t GetRunningTimeMicro() const { return GameRunningTimeMicro; }

    // Gameplay virtual time based on frame step, running when unpaused
    int64_t GetGameplayTimeMicro() const { return GameplayTimeMicro; }

    // Pause the game. Freezes world and actor ticking since the next game tick.
    void SetGamePaused( bool _Paused );

    // Returns current pause state
    bool IsGamePaused() const;

    void ResetGameplayTimer();

    // Stops the game
    void Stop();

    //bool IsGameThreadRunning() const;

    void SetVideoMode( unsigned short _Width, unsigned short _Height, unsigned short _PhysicalMonitor, byte _RefreshRate, bool _Fullscreen, const char * _Backend );
    void SetVideoMode( FVideoMode const & _VideoMode );
    void ResetVideoMode();

    FVideoMode const & GetVideoMode() const;

    float GetVideoAspectRatio() const { return VideoAspectRatio; }

    float GetFramebufferWidth() const { return FramebufferWidth; }
    float GetFramebufferHeight() const { return FramebufferHeight; }
    Float2 const & GetRetinaScale() const { return RetinaScale; }

    float GetDPIX() const { return DPI_X; }
    float GetDPIY() const { return DPI_Y; }

    void SetWindowDefs( float _Opacity, bool _Decorated, bool _AutoIconify, bool _Floating, const char * _Title );

    void SetWindowPos( int _X, int _Y );
    void GetWindowPos( int & _X, int & _Y );

    void SetInputFocus();
    bool IsInputFocus() const { return bInputFocus; }

    void SetRenderFeatures( int _VSyncMode );

    void SetCursorEnabled( bool _Enabled );

    bool IsWindowVisible() const { return bIsWindowVisible; }

    // From window to monitor coordinate
    void MapWindowCoordinate( float & InOutX, float & InOutY );

    // From monitor to window coordinate
    void UnmapWindowCoordinate( float & InOutX, float & InOutY );

    void SetCursorPosition( float _X, float _Y ) { CursorPosition.X = _X; CursorPosition.Y = _Y; }
    Float2 const & GetCursorPosition() const { return CursorPosition; }

    IGameModule * GetGameModule() { return GameModule; }

    // !!! Temp solution !!!
    void InitializeGame();
    void DeinitializeGame();

private:
    // Game thread callback
    friend void _GameThreadMain();

    void Run();

    // Process any events
    public:void ProcessEvents();private: // !!! Temp solution !!!

    // Process any event
    void ProcessEvent( struct FEvent const & _Event );

    // Send event to runtime
    FEvent & SendEvent();

    void OnKeyEvent( struct FKeyEvent const & _Event, double _TimeStamp );
    void OnMouseButtonEvent( struct FMouseButtonEvent const & _Event, double _TimeStamp );
    void OnMouseWheelEvent( struct FMouseWheelEvent const & _Event, double _TimeStamp );
    void OnMouseMoveEvent( struct FMouseMoveEvent const & _Event, double _TimeStamp );
    void OnCharEvent( struct FCharEvent const & _Event, double _TimeStamp );
    void OnChangedVideoModeEvent( struct FChangedVideoModeEvent const & _Event );

    public:void UpdateInputAxes( float _Fract );private:

    // Used to debug some features. Must be removed from release.
    void DeveloperKeys( struct FKeyEvent const & _Event );

    // Tick the game
    void Tick( float _TimeStep );

    void UpdateGameplayTimer( int64_t _TimeStep );

    void KickoffPendingKillWorlds();

    void DrawCanvas();

    void UpdateImgui();

    // System time at frame start
    int64_t FrameTimeStamp;

    // System time at tick start
    int64_t TickTimeStamp;

    // Game virtual time based on frame step
    int64_t GameRunningTimeMicro;
    int64_t GameRunningTimeMicroAfterTick;

    // Gameplay virtual time based on frame step, running when unpaused
    int64_t GameplayTimeMicro;
    int64_t GameplayTimeMicroAfterTick;

    float TimeScale;

    // All existing worlds
    // FWorld friend
    TPodArray< FWorld * > Worlds;

    FWorld * PendingKillWorlds = nullptr;

    bool bStopRequest;

    bool bGamePauseRequest = false;
    bool bGameUnpauseRequest = false;
    bool bGamePaused = false;
    bool bResetGameplayTimer = false;

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

    FImguiContext * ImguiContext;

    // Game tick number
    int TickNumber = 0;

    // Frame update number
    int FrameNumber = 0;

    // Frame update duration
    float FrameDurationInSeconds = 0;

    IGameModule * GameModule;
};

class FCanvas;

class IGameModule : public FBaseObject {
    AN_CLASS( IGameModule, FBaseObject )

public:
    IGameModule() {}

    virtual void OnGameStart() {}
    virtual void OnGameEnd() {}
    virtual void OnPreGameTick( float _TimeStep ) {}
    virtual void OnPostGameTick( float _TimeStep ) {}
    virtual void OnGameClose();
    virtual void DrawCanvas( FCanvas * _Canvas ) {}

    template< typename T >
    static IGameModule * CreateGameModule() {
        return NewObject< T >();
    }
};

extern ANGIE_API FGameMaster & GGameMaster;
extern ImFont * GAngieFont;
