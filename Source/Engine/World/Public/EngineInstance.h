/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include <Runtime/Public/EngineInterface.h>
#include <Runtime/Public/RuntimeCommandProcessor.h>
#include <World/Public/Base/GameModuleInterface.h>
#include <World/Public/Resource/FontAtlas.h>
#include <World/Public/Widgets/WDesktop.h>
#include <World/Public/World.h>
#include <World/Public/Render/RenderFrontend.h>
#include <Renderer/RenderBackend.h>

class AEngineInstance : public IEngineInterface
{
    AN_FORBID_COPY( AEngineInstance )

public:
    ACanvas Canvas;

    AEngineInstance();

    /** Helper. Create a new world */
    AWorld * CreateWorld() { return AWorld::CreateWorld(); }

    /** Helper. Destroy all existing worlds */
    void DestroyWorlds() { return AWorld::DestroyWorlds(); }

    /** Helper. Get all existing worlds */
    TPodArray< AWorld * > const & GetWorld() { return AWorld::GetWorlds(); }

    /** Get scale for Retina displays */
    Float2 const & GetRetinaScale() const { return RetinaScale; }

    /** Get window visible status */
    bool IsWindowVisible() const { return bIsWindowVisible; }

    /** Map coordinate from window space to monitor space */
    void MapWindowCoordinate( float & InOutX, float & InOutY ) const;

    /** Map coordinate from monitor space to window space */
    void UnmapWindowCoordinate( float & InOutX, float & InOutY ) const;

    /** Set hud desktop */
    void SetDesktop( WDesktop * _Desktop );

    /** Get hud desktop */
    WDesktop * GetDesktop() { return Desktop; }

    ARuntimeCommandProcessor & GetCommandProcessor() { return CommandProcessor; }

    ARenderBackend * GetRenderBackend() { return RenderBackend; }

private:
    /** IEngineInterface interface. Run the engine */
    void Run( SEntryDecl const & _EntryDecl ) override;

    /** IEngineInterface interface. Message print callback. This must be a thread-safe function. */
    void Print( const char * _Message ) override;

    /** IEngineInterface interface. */
    void OnKeyEvent( struct SKeyEvent const & _Event, double _TimeStamp ) override;

    /** IEngineInterface interface. */
    void OnMouseButtonEvent( struct SMouseButtonEvent const & _Event, double _TimeStamp ) override;

    /** IEngineInterface interface. */
    void OnMouseWheelEvent( struct SMouseWheelEvent const & _Event, double _TimeStamp ) override;

    /** IEngineInterface interface. */
    void OnMouseMoveEvent( struct SMouseMoveEvent const & _Event, double _TimeStamp ) override;

    /** IEngineInterface interface. */
    void OnJoystickAxisEvent( struct SJoystickAxisEvent const & _Event, double _TimeStamp ) override;

    /** IEngineInterface interface. */
    void OnJoystickButtonEvent( struct SJoystickButtonEvent const & _Event, double _TimeStamp );

    /** IEngineInterface interface. */
    void OnCharEvent( struct SCharEvent const & _Event, double _TimeStamp ) override;

    /** IEngineInterface interface. */
    void OnWindowVisible( bool _Visible ) override;

    /** IEngineInterface interface. */
    void OnCloseEvent() override;

    /** IEngineInterface interface. */
    void OnResize() override;

    /** Update input */
    void UpdateInput();

    /** Used to debug some features. Must be removed from release. */
    void DeveloperKeys( struct SKeyEvent const & _Event );

    void DrawCanvas();

    //void UpdateImgui();

    void ShowStats();

    /** scale coordinates for Retina displays */
    Float2 RetinaScale;

    bool bIsWindowVisible = false;

    //class AImguiContext * ImguiContext;

    /** Frame update duration */
    float FrameDurationInSeconds = 0;

    IGameModule * GameModule;

    TRef< WDesktop > Desktop;

    ARuntimeCommandProcessor CommandProcessor;

    TRef< ARenderFrontend > Renderer;
    TRef< ARenderBackend > RenderBackend;

    TUniqueRef< AResourceManager > ResourceManager;

    bool bAllowInputEvents = false;
};

extern AEngineInstance * GEngine;
