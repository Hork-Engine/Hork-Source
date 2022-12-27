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

#include "Timer.h"
#include "World.h"

HK_NAMESPACE_BEGIN

enum TIMER_STATE
{
    TIMER_STATE_FINISHED                 = 1,
    TIMER_STATE_PULSE                    = 2,
    TIMER_STATE_TRIGGERED_ON_FIRST_DELAY = 4
};

HK_CLASS_META(WorldTimer)

WorldTimer::WorldTimer()
{}

WorldTimer::~WorldTimer()
{}

void WorldTimer::Trigger()
{
    Callback();
}

void WorldTimer::Restart()
{
    m_State = 0;
    m_NumPulses = 0;
    m_ElapsedTime = 0;
}

void WorldTimer::Stop()
{
    m_State = TIMER_STATE_FINISHED;
}

bool WorldTimer::IsStopped() const
{
    return !!(m_State & TIMER_STATE_FINISHED);
}

void WorldTimer::Tick(World* World, float TimeStep)
{
    if (m_State & TIMER_STATE_FINISHED)
    {
        return;
    }

    if (bPaused)
    {
        return;
    }

    if (World->IsPaused() && !bTickEvenWhenPaused)
    {
        return;
    }

    if (m_State & TIMER_STATE_PULSE)
    {
        if (m_ElapsedTime >= PulseTime)
        {
            m_ElapsedTime = 0;
            if (MaxPulses > 0 && m_NumPulses == MaxPulses)
            {
                m_State = TIMER_STATE_FINISHED;
                return;
            }
            m_State &= ~TIMER_STATE_PULSE;
        }
        else
        {
            Trigger();
            if (m_State & TIMER_STATE_FINISHED)
            {
                // Called Stop() in trigger function
                return;
            }
            m_ElapsedTime += TimeStep;
            return;
        }
    }

    if (!(m_State & TIMER_STATE_TRIGGERED_ON_FIRST_DELAY))
    {
        if (m_ElapsedTime >= FirstDelay)
        {
            m_State |= TIMER_STATE_TRIGGERED_ON_FIRST_DELAY | TIMER_STATE_PULSE;

            m_NumPulses++;
            Trigger();
            if (m_State & TIMER_STATE_FINISHED)
            {
                // Called Stop() in trigger function
                return;
            }
            if (m_State == 0)
            {
                // Called Restart() in trigger function
                m_ElapsedTime += TimeStep;
                return;
            }
            m_ElapsedTime = 0;

            if (PulseTime <= 0)
            {
                if (MaxPulses > 0 && m_NumPulses == MaxPulses)
                {
                    m_State = TIMER_STATE_FINISHED;
                    return;
                }
                m_State &= ~TIMER_STATE_PULSE;
            }
        }
        else
        {
            m_ElapsedTime += TimeStep;
            return;
        }
    }

    if (m_ElapsedTime >= SleepDelay)
    {
        m_State |= TIMER_STATE_PULSE;
        m_NumPulses++;
        Trigger();
        if (m_State & TIMER_STATE_FINISHED)
        {
            // Called Stop() in trigger function
            return;
        }
        if (m_State == 0)
        {
            // Called Restart() in trigger function
            m_ElapsedTime += TimeStep;
            return;
        }
        m_ElapsedTime = 0;
        if (MaxPulses > 0 && m_NumPulses == MaxPulses)
        {
            m_State = TIMER_STATE_FINISHED;
            return;
        }
        if (PulseTime <= 0)
        {
            m_State &= ~TIMER_STATE_PULSE;
        }
    }
    m_ElapsedTime += TimeStep;
}

HK_NAMESPACE_END
