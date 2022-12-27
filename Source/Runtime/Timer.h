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

#include "BaseObject.h"

HK_NAMESPACE_BEGIN

class World;

class WorldTimer : public BaseObject
{
    HK_CLASS(WorldTimer, BaseObject)

public:
    WorldTimer();
    ~WorldTimer();

    float FirstDelay = 0.0f;

    float SleepDelay = 0.0f;

    float PulseTime = 0.0f;

    int MaxPulses = 0;

    /** Pause the timer */
    bool bPaused = false;

    /** Tick timer even when game is paused */
    bool bTickEvenWhenPaused = false;

    TCallback<void()> Callback;

    void Restart();

    void Stop();

    bool IsStopped() const;

    float GetElapsedTime() const { return m_ElapsedTime; }

    int GetPulseIndex() const { return m_NumPulses - 1; }

    void Tick(World* World, float TimeStep);

private:
    // Allow a world to register actors
    friend class World;
    WorldTimer* m_NextInActor = {};
    WorldTimer* m_PrevInActor = {};

    // Allow an actor to keep a list of timers
    friend class Actor;
    WorldTimer* m_NextInWorld = {};
    WorldTimer* m_PrevInWorld = {};

    int m_State = 0;
    int m_NumPulses = 0;
    float m_ElapsedTime = 0.0f;

    void Trigger();
};

HK_NAMESPACE_END
