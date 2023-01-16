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

#pragma once

#include <Platform/BaseTypes.h>
#include <Core/HeapBlob.h>

HK_NAMESPACE_BEGIN

#define freeverb_Undenormalise(sample) \
    if (((*(unsigned int*)&sample) & 0x7f800000) == 0) sample = 0.0f

struct Freeverb_FilterComb
{
    float* pBuffer;
    int BufSize;
    int BufIdx;
    float Feedback;
    float FilterStore;
    float Damp1;
    float Damp2;

    void SetDamp(float val)
    {
        Damp1 = val;
        Damp2 = 1 - val;
    }

    float GetDamp() const
    {
        return Damp1;
    }

    void SetFeedback(float val)
    {
        Feedback = val;
    }

    float GetFeedback() const
    {
        return Feedback;
    }

    inline float Process(float input)
    {
        float output;

        output = pBuffer[BufIdx];
        freeverb_Undenormalise(output);

        FilterStore = (output * Damp2) + (FilterStore * Damp1);
        freeverb_Undenormalise(FilterStore);

        pBuffer[BufIdx] = input + (FilterStore * Feedback);

        if (++BufIdx >= BufSize) BufIdx = 0;

        return output;
    }
};

struct Freeverb_FilterAllPass
{
    float* pBuffer;
    int BufSize;
    int BufIdx;

    inline float Process(float input)
    {
        float output;
        float bufout;

        const float feedback = 0.5f;

        bufout = pBuffer[BufIdx];
        freeverb_Undenormalise(bufout);

        output = -input + bufout;
        pBuffer[BufIdx] = input + (bufout * feedback);

        if (++BufIdx >= BufSize) BufIdx = 0;

        return output;
    }
};

#undef freeverb_Undenormalise

class Freeverb
{
    HK_FORBID_COPY(Freeverb)

public:
    const float MutedGain = 0;
    const float FixedGain = 0.015f;
    const float ScaleWet = 3;
    const float ScaleDry = 2;
    const float ScaleDamp = 0.4f;
    const float ScaleRoom = 0.28f;
    const float OffsetRoom = 0.7f;
    const float InitialRoom = 0.5f;
    const float InitialDamp = 0.5f;
    const float InitialWet = 1 / ScaleWet;
    const float InitialDry = 0;
    const float InitialWidth = 1;

    Freeverb(int SampleRate);
    virtual ~Freeverb() = default;

    void Mute();

    void SetRoomSize(float InRoomSize);
    float GetRoomSize() const;

    void SetDamp(float InDamp);
    float GetDamp() const;

    void SetWet(float InWet);
    float GetWet() const;

    void SetDry(float InDry);
    float GetDry() const;

    void SetWidth(float InWidth);
    float GetWidth() const;

    void SetFreeze(bool InbFreeze);
    bool IsFreeze() const;

    void ProcessMix(float* inputL, float* inputR, float* outputL, float* outputR, int frameCount, int skip);
    void ProcessReplace(float* inputL, float* inputR, float* outputL, float* outputR, int frameCount, int skip);

private:
    void Update();

    float Gain;
    float RoomSize;
    float RoomSize1;
    float Damp;
    float Damp1;
    float Wet;
    float Wet1;
    float Wet2;
    float Dry;
    float Width;
    bool bFreeze;

    enum
    {
        NUM_COMBs = 8
    };
    enum
    {
        NUM_ALL_PASSES = 4
    };

    // Comb filters
    Freeverb_FilterComb CombL[NUM_COMBs];
    Freeverb_FilterComb CombR[NUM_COMBs];

    // Allpass filters
    Freeverb_FilterAllPass AllPassL[NUM_ALL_PASSES];
    Freeverb_FilterAllPass AllPassR[NUM_ALL_PASSES];

    HeapBlob MemoryBlob;
};

HK_NAMESPACE_END
