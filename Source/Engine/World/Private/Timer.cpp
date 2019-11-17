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

#include <Engine/World/Public/Timer.h>
#include <Engine/World/Public/World.h>

enum ETimerState {
    TS_Finished = 1,
    TS_Pulse = 2,
    TS_TriggeredOnFirstDelay = 4
};

void ATimer::Trigger() {
    Callback();
}

void ATimer::Restart() {
    State = 0;
    NumPulses = 0;
    ElapsedTime = 0;
}

void ATimer::Stop() {
    State = TS_Finished;
}

bool ATimer::IsStopped() const {
    return !!(State & TS_Finished);
}

void ATimer::Tick( AWorld * _World, float _TimeStep ) {
    if ( State & TS_Finished ) {
        return;
    }

    if ( bPaused ) {
        return;
    }

    if ( _World->IsPaused() && !bTickEvenWhenPaused ) {
        return;
    }

    if ( State & TS_Pulse ) {
        if ( ElapsedTime >= PulseTime ) {
            ElapsedTime = 0;
            if ( MaxPulses > 0 && NumPulses == MaxPulses ) {
                State = TS_Finished;
                return;
            }
            State &= ~TS_Pulse;
        } else {
            Trigger();
            if ( State & TS_Finished ) {
                // Called Stop() in trigger function
                return;
            }
            ElapsedTime += _TimeStep;
            return;
        }
    }

    if ( !(State & TS_TriggeredOnFirstDelay) ) {
        if ( ElapsedTime >= FirstDelay ) {
            State |= TS_TriggeredOnFirstDelay | TS_Pulse;

            NumPulses++;
            Trigger();
            if ( State & TS_Finished ) {
                // Called Stop() in trigger function
                return;
            }
            if ( State == 0 ) {
                // Called Restart() in trigger function
                ElapsedTime += _TimeStep;
                return;
            }
            ElapsedTime = 0;

            if ( PulseTime <= 0 ) {
                if ( MaxPulses > 0 && NumPulses == MaxPulses ) {
                    State = TS_Finished;
                    return;
                }
                State &= ~TS_Pulse;
            }
        } else {
            ElapsedTime += _TimeStep;
            return;
        }
    }

    if ( ElapsedTime >= SleepDelay ) {
        State |= TS_Pulse;
        NumPulses++;
        Trigger();
        if ( State & TS_Finished ) {
            // Called Stop() in trigger function
            return;
        }
        if ( State == 0 ) {
            // Called Restart() in trigger function
            ElapsedTime += _TimeStep;
            return;
        }
        ElapsedTime = 0;
        if ( MaxPulses > 0 && NumPulses == MaxPulses ) {
            State = TS_Finished;
            return;
        }
        if ( PulseTime <= 0 ) {
            State &= ~TS_Pulse;
        }
    }
    ElapsedTime += _TimeStep;
}
