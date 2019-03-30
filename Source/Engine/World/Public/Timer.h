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

#include <Engine/Core/Public/BaseTypes.h>

class ANGIE_API FTimer final {
    AN_FORBID_COPY( FTimer )

    friend class FWorld;
    friend class FActor;

public:
    float FirstDelay = 0;
    float SleepDelay = 0;
    float PulseTime = 0;
    bool bPaused = false;
    bool bTickEvenWhenPaused = false;
    int MaxPulses = 0;

    FTimer() {}
    void Restart();
    void Stop();
    bool IsStopped() const;

    template< typename T >
    void SetCallback( T * _Object, void (T::*_Method)() ) {
        SetCallback( { _Object, _Method } );
    }

    void SetCallback( TCallback< void() > const & _Callback ) {
        Callback = _Callback;
    }

    float GetElapsedTime() const { return ElapsedTime; }
    int GetPulseIndex() const { return NumPulses - 1; }

private:
    int State = 0;
    int NumPulses = 0;
    float ElapsedTime = 0;
    TCallback< void() > Callback;
    FTimer * P;         // List inside actor
    FTimer * Next;      // List inside world
    FTimer * Prev;      // List inside world

    void Tick( float _TimeStep );
    void Trigger();
};
