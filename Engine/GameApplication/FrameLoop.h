/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include <Engine/Core/Allocators/LinearAllocator.h>
#include <Engine/Core/Containers/ArrayView.h>
#include <Engine/RenderCore/VertexMemoryGPU.h>

#include "InputDefs.h"

HK_NAMESPACE_BEGIN

//struct Joystick
//{
//    int NumAxes;
//    int NumButtons;
//    bool bGamePad;
//    bool bConnected;
//    int Id;
//};

struct KeyEvent
{
    int Key;
    int Scancode; // Not used, reserved for future
    int ModMask;
    int Action; // INPUT_ACTION
};

struct MouseButtonEvent
{
    int Button;
    int ModMask;
    int Action; // INPUT_ACTION
};

struct MouseWheelEvent
{
    double WheelX;
    double WheelY;
};

struct MouseMoveEvent
{
    float X;
    float Y;
};

struct JoystickAxisEvent
{
    int Joystick;
    int Axis;
    float Value;
};

struct JoystickButtonEvent
{
    int Joystick;
    int Button;
    int Action; // INPUT_ACTION
};

//struct JoystickStateEvent
//{
//    int Joystick;
//    int NumAxes;
//    int NumButtons;
//    bool bGamePad;
//    bool bConnected;
//};

struct CharEvent
{
    WideChar UnicodeCharacter;
    int ModMask;
};

class IEventListener
{
public:
    virtual ~IEventListener() {}

    virtual void OnKeyEvent(struct KeyEvent const& event) = 0;

    virtual void OnMouseButtonEvent(struct MouseButtonEvent const& event) = 0;

    virtual void OnMouseWheelEvent(struct MouseWheelEvent const& event) = 0;

    virtual void OnMouseMoveEvent(struct MouseMoveEvent const& event) = 0;

    virtual void OnJoystickAxisEvent(struct JoystickAxisEvent const& event) = 0;

    virtual void OnJoystickButtonEvent(struct JoystickButtonEvent const& event) = 0;

    virtual void OnCharEvent(struct CharEvent const& event) = 0;

    virtual void OnWindowVisible(bool bVisible) = 0;

    virtual void OnCloseEvent() = 0;

    virtual void OnResize() = 0;
};

class WorldRenderView;

class FrameLoop : public RefCounted
{
public:
                    FrameLoop(RenderCore::IDevice* RenderDevice);
    virtual         ~FrameLoop();

    /** Allocate frame memory */
    void*           AllocFrameMem(size_t _SizeInBytes);

    template <typename T>
    T*              AllocFrameMem() { return m_FrameMemory.Allocate<T>(); }

    /** Return frame memory size in bytes */
    size_t          GetFrameMemorySize() const;

    /** Return used frame memory in bytes */
    size_t          GetFrameMemoryUsed() const;

    /** Return used frame memory on previous frame, in bytes */
    size_t          GetFrameMemoryUsedPrev() const;

    /** Return max frame memory usage since application start */
    size_t          GetMaxFrameMemoryUsage() const;

    /** Get time stamp at beggining of the frame */
    int64_t         SysFrameTimeStamp();

    /** Get frame duration in microseconds */
    int64_t         SysFrameDuration();

    /** Get current frame number */
    int             SysFrameNumber() const;

    void            SetGenerateInputEvents(bool bShouldGenerateInputEvents);

    /** Begin a new frame */
    void            NewFrame(TArrayView<RenderCore::ISwapChain*> SwapChains, int SwapInterval, class ResourceManager* resourceManager);

    /** Poll runtime events */
    void            PollEvents(IEventListener* Listener);

    void            RegisterView(WorldRenderView* pView);

    TVector<WorldRenderView*> const&    GetRenderViews() { return m_Views; }

    StreamedMemoryGPU*                  GetStreamedMemoryGPU() { return m_StreamedMemoryGPU; }

private:
    void            ClearViews();
    void            ClearJoystickAxes(IEventListener* Listener, int _JoystickNum);
    void            UnpressKeysAndButtons(IEventListener* Listener);
    void            UnpressJoystickButtons(IEventListener* Listener, int _JoystickNum);
        
    int64_t m_FrameTimeStamp;
    int64_t m_FrameDuration;
    int     m_FrameNumber;

    TLinearAllocator<>& m_FrameMemory;
    size_t              m_FrameMemoryUsedPrev = 0;
    size_t              m_MaxFrameMemoryUsage = 0;

    TRef<class GPUSync>     m_GPUSync;
    TRef<StreamedMemoryGPU> m_StreamedMemoryGPU;

    TRef<RenderCore::IDevice> m_RenderDevice;

    TArray<int, KEY_LAST + 1>           m_PressedKeys;
    TArray<bool, MOUSE_BUTTON_8 + 1>    m_PressedMouseButtons;
    TArray<TArray<unsigned char, MAX_JOYSTICK_BUTTONS>, MAX_JOYSTICKS_COUNT>    m_JoystickButtonState;
    TArray<TArray<short, MAX_JOYSTICK_AXES>, MAX_JOYSTICKS_COUNT>               m_JoystickAxisState;
    TArray<bool, MAX_JOYSTICKS_COUNT>   m_JoystickAdded;
    bool                                m_bShouldGenerateInputEvents{true};

    TVector<WorldRenderView*>   m_Views;
    TRef<class FontStash>       m_FontStash;
};

HK_NAMESPACE_END
