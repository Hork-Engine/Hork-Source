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

#include <World/Public/Base/BaseObject.h>

class AWorld;

class ATimer : public ABaseObject
{
    AN_CLASS( ATimer, ABaseObject )

    friend class AWorld;

public:
    float FirstDelay = 0.0f;

    float SleepDelay = 0.0f;

    float PulseTime = 0.0f;

    int MaxPulses = 0;

    /** Pause the timer */
    bool bPaused = false;

    /** Tick timer even when game is paused */
    bool bTickEvenWhenPaused = false;

    TCallback< void() > Callback;

    /** Register timer in a world. Call this in/after BeginPlay */
    void Register( AWorld * InOwnerWorld );

    /** Unregister timer if you want to stop timer ticking */
    void Unregister();

    void Restart();

    void Stop();

    bool IsStopped() const;

    float GetElapsedTime() const { return ElapsedTime; }

    int GetPulseIndex() const { return NumPulses - 1; }

protected:
    ATimer();
    ~ATimer();

private:
    AWorld * pOwnerWorld = nullptr;
    ATimer * Next = nullptr;      // List inside a world
    ATimer * Prev = nullptr;      // List inside a world
    int State = 0;
    int NumPulses = 0;
    float ElapsedTime = 0.0f;

    /** The tick function is called by owner world */
    void Tick( float InTimeStep );

    void Trigger();
};
