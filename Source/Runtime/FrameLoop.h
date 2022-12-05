/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#pragma once

#include <Platform/Memory/LinearAllocator.h>
#include <Platform/Utf8.h>
#include <RenderCore/VertexMemoryGPU.h>

#include "InputDefs.h"

class AFontStash;

//struct SJoystick
//{
//    int NumAxes;
//    int NumButtons;
//    bool bGamePad;
//    bool bConnected;
//    int Id;
//};

struct SKeyEvent
{
    int Key;
    int Scancode; // Not used, reserved for future
    int ModMask;
    int Action; // EInputAction
};

struct SMouseButtonEvent
{
    int Button;
    int ModMask;
    int Action; // EInputAction
};

struct SMouseWheelEvent
{
    double WheelX;
    double WheelY;
};

struct SMouseMoveEvent
{
    float X;
    float Y;
};

struct SJoystickAxisEvent
{
    int   Joystick;
    int   Axis;
    float Value;
};

struct SJoystickButtonEvent
{
    int Joystick;
    int Button;
    int Action; // EInputAction
};

//struct SJoystickStateEvent
//{
//    int Joystick;
//    int NumAxes;
//    int NumButtons;
//    bool bGamePad;
//    bool bConnected;
//};

struct SCharEvent
{
    WideChar UnicodeCharacter;
    int       ModMask;
};

class IEventListener
{
public:
    virtual ~IEventListener() {}

    virtual void OnKeyEvent(struct SKeyEvent const& _Event, double _TimeStamp) = 0;

    virtual void OnMouseButtonEvent(struct SMouseButtonEvent const& _Event, double _TimeStamp) = 0;

    virtual void OnMouseWheelEvent(struct SMouseWheelEvent const& _Event, double _TimeStamp) = 0;

    virtual void OnMouseMoveEvent(struct SMouseMoveEvent const& _Event, double _TimeStamp) = 0;

    virtual void OnJoystickAxisEvent(struct SJoystickAxisEvent const& _Event, double _TimeStamp) = 0;

    virtual void OnJoystickButtonEvent(struct SJoystickButtonEvent const& _Event, double _TimeStamp) = 0;

    virtual void OnCharEvent(struct SCharEvent const& _Event, double _TimeStamp) = 0;

    virtual void OnWindowVisible(bool _Visible) = 0;

    virtual void OnCloseEvent() = 0;

    virtual void OnResize() = 0;
};

class WorldRenderView;

class AFrameLoop : public ARefCounted
{
public:
    AFrameLoop(RenderCore::IDevice* RenderDevice);
    virtual ~AFrameLoop();

    /** Allocate frame memory */
    void* AllocFrameMem(size_t _SizeInBytes);

    template <typename T>
    T* AllocFrameMem()
    {
        return FrameMemory.Allocate<T>();
    }

    /** Return frame memory size in bytes */
    size_t GetFrameMemorySize() const;

    /** Return used frame memory in bytes */
    size_t GetFrameMemoryUsed() const;

    /** Return used frame memory on previous frame, in bytes */
    size_t GetFrameMemoryUsedPrev() const;

    /** Return max frame memory usage since application start */
    size_t GetMaxFrameMemoryUsage() const;

    /** Get time stamp at beggining of the frame */
    int64_t SysFrameTimeStamp();

    /** Get frame duration in microseconds */
    int64_t SysFrameDuration();

    /** Get current frame update number */
    int SysFrameNumber() const;

    /** Begin a new frame */
    void NewFrame(TPodVector<RenderCore::ISwapChain*> const& SwapChains, int SwapInterval);

    /** Poll runtime events */
    void PollEvents(IEventListener* Listener);

    void RegisterView(WorldRenderView* pView);

    TVector<WorldRenderView*> const& GetRenderViews() { return m_Views; }

    AStreamedMemoryGPU* GetStreamedMemoryGPU() { return StreamedMemoryGPU; }

private:
    void ClearViews();
    void ClearJoystickAxes(IEventListener* Listener, int _JoystickNum, double _TimeStamp);
    void UnpressKeysAndButtons(IEventListener* Listener);
    void UnpressJoystickButtons(IEventListener* Listener, int _JoystickNum, double _TimeStamp);

    int64_t FrameTimeStamp;
    int64_t FrameDuration;
    int     FrameNumber;

    TLinearAllocator<>& FrameMemory;
    size_t             FrameMemoryUsedPrev = 0;
    size_t             MaxFrameMemoryUsage = 0;

    TRef<class AGPUSync> GPUSync;
    TRef<AStreamedMemoryGPU> StreamedMemoryGPU;

    TRef<RenderCore::IDevice> RenderDevice;

    TArray<int, KEY_LAST + 1>                                                PressedKeys;
    TArray<bool, MOUSE_BUTTON_8 + 1>                                         PressedMouseButtons;
    TArray<TArray<unsigned char, MAX_JOYSTICK_BUTTONS>, MAX_JOYSTICKS_COUNT> JoystickButtonState;
    TArray<TArray<short, MAX_JOYSTICK_AXES>, MAX_JOYSTICKS_COUNT>            JoystickAxisState;
    TArray<bool, MAX_JOYSTICKS_COUNT>                                        JoystickAdded;

    TVector<WorldRenderView*> m_Views;
    TRef<AFontStash>          m_FontStash;
};
