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

#include <Engine/Base/Public/Factory.h>
#include <Engine/Base/Public/BaseObject.h>
#include <Engine/Core/Public/Document.h>
#include <Engine/Runtime/Public/ImportExport.h>
#include <Engine/Resource/Public/FontAtlas.h>

class FWorld;
class FImguiContext;

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

class ANGIE_API FGameEngine : public IGameEngine {
    AN_SINGLETON( FGameEngine )

    friend class FWorld;

public:
    bool bQuitOnEscape = true;

    bool bToggleFullscreenAltEnter = true;

    bool bAllowConsole = true;

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

    // void SetHearableWorld( FWorld * _World ); TODO?
    // FWorld * GetHearableWorld()

    // Get current frame update number
    int GetFrameNumber() const { return FrameNumber; }

    // Stops the game
    void Stop();

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

private:
    // IGameEngine interface
    void Initialize( FCreateGameModuleCallback _CreateGameModuleCallback ) override;
    void Deinitialize() override;
    void BuildFrame() override;
    void UpdateFrame() override;
    bool IsStopped() override;
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

    void InitializeDefaultFont();
    void DeinitializeDefaultFont();

    // All existing worlds
    // FWorld friend
    TPodArray< FWorld * > Worlds;

    bool bStopRequest;

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

    // Frame update number
    int FrameNumber = 0;

    // Frame update duration
    float FrameDurationInSeconds = 0;

    // Frame duration in microseconds
    int64_t FrameDuration;

    // System time at frame start
    int64_t FrameTimeStamp;

    IGameModule * GameModule;

    TRef< FFontAtlas > DefaultFontAtlas;
    FFont * DefaultFont;
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

extern ANGIE_API FGameEngine & GGameEngine;
