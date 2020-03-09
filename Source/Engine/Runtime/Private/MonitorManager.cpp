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

#include "MonitorManager.h"
#include "RuntimeEvents.h"

#include <GLFW/glfw3.h>

AMonitorManager & GMonitorManager = AMonitorManager::Inst();

static APhysicalMonitorArray PhysicalMonitors;
static SPhysicalMonitor * PrimaryMonitor = nullptr;

static void RegisterMonitor( GLFWmonitor * _Monitor );
static void UnregisterMonitor( GLFWmonitor * _Monitor );

AMonitorManager::AMonitorManager() {

}

void AMonitorManager::Initialize() {
    int monitorsCount = 0;
    GLFWmonitor ** monitors;

    monitors = glfwGetMonitors( &monitorsCount );
    for ( int i = 0; i < monitorsCount; i++ ) {
        RegisterMonitor( monitors[i] );
    }

    glfwSetMonitorCallback( []( GLFWmonitor * _Monitor, int _ConnectStatus ) {
        if ( _ConnectStatus == GLFW_CONNECTED ) {
            RegisterMonitor( _Monitor );
        } else {
            UnregisterMonitor( _Monitor );
        }
    } );
}

static void UpdatePrimaryMonitor() {
    GLFWmonitor * monitor = glfwGetPrimaryMonitor();
    if ( !monitor ) {
        return;
    }
    int primaryMonitorHandle = (size_t)glfwGetMonitorUserPointer( monitor );
    if ( primaryMonitorHandle == -1 ) {
        return;
    }
    PrimaryMonitor = PhysicalMonitors[ primaryMonitorHandle ];
}

static void RegisterMonitor( GLFWmonitor * _Monitor ) {
    int videoModesCount = 0;
    const GLFWvidmode * videoModesGLFW = glfwGetVideoModes( _Monitor, &videoModesCount );

    if ( videoModesCount == 0 ) {
        glfwSetMonitorUserPointer( _Monitor, ( void * )( size_t )-1 );
        return;
    }

    int physMonitorSizeOf = sizeof( SPhysicalMonitor ) + sizeof( SMonitorVideoMode ) * (videoModesCount - 1);
    SPhysicalMonitor * physMonitor = (SPhysicalMonitor *)GZoneMemory.ClearedAlloc( physMonitorSizeOf );

    PhysicalMonitors.Append( physMonitor );

    int handle = PhysicalMonitors.Size() - 1;
    glfwSetMonitorUserPointer( _Monitor, ( void * )( size_t )handle );

    AString::CopySafe( physMonitor->MonitorName, sizeof( physMonitor->MonitorName ), glfwGetMonitorName( _Monitor ) );
    glfwGetMonitorPos( _Monitor, &physMonitor->PositionX, &physMonitor->PositionY );
    glfwGetMonitorPhysicalSize( _Monitor, &physMonitor->PhysicalWidthMM, &physMonitor->PhysicalHeightMM );

    const GLFWvidmode * videoMode = glfwGetVideoMode( _Monitor );

    const float MM_To_Inch = 0.0393701f;
    physMonitor->DPI_X = (float)videoMode->width / (physMonitor->PhysicalWidthMM*MM_To_Inch);
    physMonitor->DPI_Y = (float)videoMode->height / (physMonitor->PhysicalHeightMM*MM_To_Inch);

    physMonitor->Internal.Pointer = _Monitor;
    physMonitor->VideoModesCount = videoModesCount;

    for ( int mode = 0; mode < videoModesCount; mode++ ) {
        SMonitorVideoMode * dst = &physMonitor->VideoModes[mode];
        const GLFWvidmode * src = &videoModesGLFW[mode];
        dst->Width = src->width;
        dst->Height = src->height;
        dst->RedBits = src->redBits;
        dst->GreenBits = src->greenBits;
        dst->BlueBits = src->blueBits;
        dst->RefreshRate = src->refreshRate;
    }

    const GLFWgammaramp * gammaRamp = glfwGetGammaRamp( _Monitor );
    AN_ASSERT( gammaRamp && gammaRamp->size <= GAMMA_RAMP_SIZE );
    memcpy( &physMonitor->Internal.InitialGammaRamp[0], gammaRamp->red, gammaRamp->size * sizeof( unsigned short ) );
    memcpy( &physMonitor->Internal.InitialGammaRamp[gammaRamp->size], gammaRamp->green, gammaRamp->size * sizeof( unsigned short ) );
    memcpy( &physMonitor->Internal.InitialGammaRamp[gammaRamp->size * 2], gammaRamp->blue, gammaRamp->size * sizeof( unsigned short ) );
    memcpy( physMonitor->Internal.GammaRamp, physMonitor->Internal.InitialGammaRamp, sizeof( unsigned short ) * gammaRamp->size * 3 );
    physMonitor->GammaRampSize = gammaRamp->size;
    physMonitor->Internal.bGammaRampDirty = false;

    UpdatePrimaryMonitor();

    SEvent * event = GRuntimeEvents.Push();
    event->Type = ET_MonitorConnectionEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();   // in seconds
    event->Data.MonitorConnectionEvent.Handle = handle;
    event->Data.MonitorConnectionEvent.bConnected = true;

    GLogger.Printf( "Monitor connected: %s\n", glfwGetMonitorName( _Monitor ) );
}

static void UnregisterMonitor( GLFWmonitor * _Monitor ) {
    int handle = (size_t)glfwGetMonitorUserPointer( _Monitor );
    if ( handle == -1 ) {
        return;
    }

    SPhysicalMonitor * physMonitor = PhysicalMonitors[ handle ];

    physMonitor->Internal.Pointer = nullptr;

    UpdatePrimaryMonitor();

    SEvent * event = GRuntimeEvents.Push();
    event->Type = ET_MonitorConnectionEvent;
    event->TimeStamp = GRuntime.SysSeconds_d();   // in seconds
    event->Data.MonitorConnectionEvent.Handle = handle;
    event->Data.MonitorConnectionEvent.bConnected = false;

    GLogger.Printf( "Monitor disconnected %s\n", glfwGetMonitorName( _Monitor ) );
}

void AMonitorManager::Deinitialize() {
    glfwSetMonitorCallback( nullptr );

    for ( SPhysicalMonitor * physMonitor : PhysicalMonitors ) {
        GZoneMemory.Dealloc( physMonitor );
    }
    PhysicalMonitors.Free();
}

static void UpdateMonitorGamma( SPhysicalMonitor * _PhysMonitor ) {
    GLFWgammaramp ramp;
    ramp.size = _PhysMonitor->GammaRampSize;
    ramp.red = _PhysMonitor->Internal.GammaRamp;
    ramp.green = ramp.red + ramp.size;
    ramp.blue = ramp.green + ramp.size;
    glfwSetGammaRamp( ( GLFWmonitor * )_PhysMonitor->Internal.Pointer, &ramp );
    _PhysMonitor->Internal.bGammaRampDirty = false;
}

void AMonitorManager::UpdateMonitors() {
    for ( SPhysicalMonitor * physMonitor : PhysicalMonitors ) {
        if ( !physMonitor->Internal.Pointer ) {
            // not connected
            continue;
        }
        if ( physMonitor->Internal.bGammaRampDirty ) {
            UpdateMonitorGamma( physMonitor );
        }
    }
}

SPhysicalMonitor * AMonitorManager::FindMonitor( const char * _MonitorName ) {
    for ( SPhysicalMonitor * physMonitor : PhysicalMonitors ) {
        if ( !AString::Cmp( physMonitor->MonitorName, _MonitorName ) ) {
            return physMonitor;
        }
    }
    return nullptr;
}

APhysicalMonitorArray const & AMonitorManager::GetMonitors() {
    return PhysicalMonitors;
}

SPhysicalMonitor * AMonitorManager::GetPrimaryMonitor() {
    return PrimaryMonitor;
}
