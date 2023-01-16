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

// Freeverb is a free and open-source Schroeder reverberator originally implemented in C++ by Jezar at Dreampoint.

#include "Freeverb.h"

#include <Platform/Memory/Memory.h>

HK_NAMESPACE_BEGIN

Freeverb::Freeverb(int SampleRate)
{
    // These values assume 44.1KHz sample rate
    // they will probably be OK for 48KHz sample rate
    // but would need scaling for 96KHz (or other) sample rates.
    // The values were obtained by listening tests.
    int combLengths[NUM_COMBs] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
    int allpassLengths[NUM_ALL_PASSES] = {556, 441, 341, 225};

    // Scale for device sample rate
    if (SampleRate != 44100)
    {
        double ratio = (double)SampleRate / 44100.0;

        for (int i = 0; i < NUM_COMBs; i++)
        {
            combLengths[i] = (int)floor(ratio * combLengths[i]);
        }
        for (int i = 0; i < NUM_ALL_PASSES; i++)
        {
            allpassLengths[i] = (int)floor(ratio * allpassLengths[i]);
        }
    }

    const int stereoSpread = 23;

    // calculate capacity
    int capacity = 0;
    for (int i = 0; i < NUM_COMBs; i++)
    {
        capacity += combLengths[i];
        capacity += combLengths[i] + stereoSpread;
    }
    for (int i = 0; i < NUM_ALL_PASSES; i++)
    {
        capacity += allpassLengths[i];
        capacity += allpassLengths[i] + stereoSpread;
    }

    // Allocate memory for buffers
    MemoryBlob.Reset(capacity * sizeof(float), nullptr, MALLOC_ZERO);

    float* buffer = (float*)MemoryBlob.GetData();
    for (int i = 0; i < NUM_COMBs; i++)
    {
        CombL[i].pBuffer = buffer;
        CombL[i].BufSize = combLengths[i];
        CombL[i].BufIdx = 0;
        CombL[i].FilterStore = 0;
        buffer += combLengths[i];

        CombR[i].pBuffer = buffer;
        CombR[i].BufSize = combLengths[i] + stereoSpread;
        CombR[i].BufIdx = 0;
        CombR[i].FilterStore = 0;
        buffer += combLengths[i] + stereoSpread;
    }

    for (int i = 0; i < NUM_ALL_PASSES; i++)
    {
        AllPassL[i].pBuffer = buffer;
        AllPassL[i].BufSize = allpassLengths[i];
        AllPassL[i].BufIdx = 0;
        buffer += allpassLengths[i];

        AllPassR[i].pBuffer = buffer;
        AllPassR[i].BufSize = allpassLengths[i] + stereoSpread;
        AllPassR[i].BufIdx = 0;
        buffer += allpassLengths[i] + stereoSpread;
    }

    // Set default values
    SetWet(InitialWet);
    SetRoomSize(InitialRoom);
    SetDry(InitialDry);
    SetDamp(InitialDamp);
    SetWidth(InitialWidth);
    SetFreeze(false);
}

void Freeverb::Mute()
{
    if (bFreeze)
    {
        return;
    }

    MemoryBlob.ZeroMem();
}

void Freeverb::ProcessReplace(float* inputL, float* inputR, float* outputL, float* outputR, int frameCount, int skip)
{
    float outL, outR, input;

    while (frameCount-- > 0)
    {
        outL = outR = 0;
        input = (*inputL + *inputR) * Gain;

        // Accumulate comb filters in parallel
        for (int i = 0; i < NUM_COMBs; i++)
        {
            outL += CombL[i].Process(input);
            outR += CombR[i].Process(input);
        }

        // Feed through allpasses in series
        for (int i = 0; i < NUM_ALL_PASSES; i++)
        {
            outL = AllPassL[i].Process(outL);
            outR = AllPassR[i].Process(outR);
        }

        // Calculate output REPLACING anything already there
        *outputL = outL * Wet1 + outR * Wet2 + *inputL * Dry;
        *outputR = outR * Wet1 + outL * Wet2 + *inputR * Dry;

        // Increment sample pointers, allowing for interleave (if any)
        inputL += skip;
        inputR += skip;
        outputL += skip;
        outputR += skip;
    }
}

void Freeverb::ProcessMix(float* inputL, float* inputR, float* outputL, float* outputR, int frameCount, int skip)
{
    float outL, outR, input;

    while (frameCount-- > 0)
    {
        outL = outR = 0;
        input = (*inputL + *inputR) * Gain;

        // Accumulate comb filters in parallel
        for (int i = 0; i < NUM_COMBs; i++)
        {
            outL += CombL[i].Process(input);
            outR += CombR[i].Process(input);
        }

        // Feed through allpasses in series
        for (int i = 0; i < NUM_ALL_PASSES; i++)
        {
            outL = AllPassL[i].Process(outL);
            outR = AllPassR[i].Process(outR);
        }

        // Calculate output MIXING with anything already there
        *outputL += outL * Wet1 + outR * Wet2 + *inputL * Dry;
        *outputR += outR * Wet1 + outL * Wet2 + *inputR * Dry;

        // Increment sample pointers, allowing for interleave (if any)
        inputL += skip;
        inputR += skip;
        outputL += skip;
        outputR += skip;
    }
}

void Freeverb::Update()
{
    // Recalculate internal values after parameter change

    Wet1 = Wet * (Width * 0.5f + 0.5f);
    Wet2 = Wet * (0.5f - Width * 0.5f);

    if (bFreeze)
    {
        RoomSize1 = 1;
        Damp1 = 0;
        Gain = MutedGain;
    }
    else
    {
        RoomSize1 = RoomSize;
        Damp1 = Damp;
        Gain = FixedGain;
    }

    for (int i = 0; i < NUM_COMBs; i++)
    {
        CombL[i].SetFeedback(RoomSize1);
        CombR[i].SetFeedback(RoomSize1);

        CombL[i].SetDamp(Damp1);
        CombR[i].SetDamp(Damp1);
    }
}

// The following get/set functions are not inlined, because
// speed is never an issue when calling them, and also
// because as you develop the reverb model, you may
// wish to take dynamic action when they are called.

void Freeverb::SetRoomSize(float InRoomSize)
{
    RoomSize = (InRoomSize * ScaleRoom) + OffsetRoom;
    Update();
}

float Freeverb::GetRoomSize() const
{
    return (RoomSize - OffsetRoom) / ScaleRoom;
}

void Freeverb::SetDamp(float InDamp)
{
    Damp = InDamp * ScaleDamp;
    Update();
}

float Freeverb::GetDamp() const
{
    return Damp / ScaleDamp;
}

void Freeverb::SetWet(float InWet)
{
    Wet = InWet * ScaleWet;
    Update();
}

float Freeverb::GetWet() const
{
    return Wet / ScaleWet;
}

void Freeverb::SetDry(float InDry)
{
    Dry = InDry * ScaleDry;
}

float Freeverb::GetDry() const
{
    return Dry / ScaleDry;
}

void Freeverb::SetWidth(float InWidth)
{
    Width = InWidth;
    Update();
}

float Freeverb::GetWidth() const
{
    return Width;
}

void Freeverb::SetFreeze(bool InbFreeze)
{
    bFreeze = InbFreeze;
    Update();
}

bool Freeverb::IsFreeze() const
{
    return bFreeze;
}

HK_NAMESPACE_END
