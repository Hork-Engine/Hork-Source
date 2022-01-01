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

#include <Geometry/Public/VectorMath.h>
#include <Geometry/Public/Complex.h>
#include <Containers/Public/PodVector.h>

constexpr int HRTF_BLOCK_LENGTH = 128; // Keep it to a power of two

class AAudioHRTF
{
    AN_FORBID_COPY( AAudioHRTF )

public:
    AAudioHRTF( int SampleRate );
    virtual ~AAudioHRTF();

    /** Gets a bilinearly interpolated HRTF */
    void SampleHRTF( Float3 const & Dir, SComplex * pLeftHRTF, SComplex * pRightHRTF ) const;

    /** Applies HRTF to input frames. Frames must also contain GetFrameCount()-1 of the previous frames.
    FrameCount must be multiples of HRTF_BLOCK_LENGTH */
    void ApplyHRTF( Float3 const & CurDir, Float3 const & NewDir, const float * pFrames, int FrameCount, float * pStream, Float3 & Dir );

    /** Sphere geometry vertics */
    TPodVectorHeap< Float3 > const & GetVertices() const { return Vertices; }

    /** Sphere geometry indices */
    TPodVectorHeap< uint32_t > const & GetIndices() const { return Indices; }

    /** Length of Head-Related Impulse Response (HRIR) */
    int GetFrameCount() const
    {
        return FrameCount;
    }

    /** HRTF FFT filter size in frames */
    int GetFilterSize() const
    {
        // Computed as power of two of FrameCount - 1 + HRTF_BLOCK_LENGTH
        return FilterSize;
    }

private:
    void GenerateHRTF( const float * pFrames, int InFrameCount, SComplex * pHRTF );

    /** Fast fourier transform (forward) */
    void FFT( SComplex const * pIn, SComplex * pOut );

    /** Fast fourier transform (inverse) */
    void IFFT( SComplex const * pIn, SComplex * pOut );

    /** Length of Head-Related Impulse Response (HRIR) */
    int FrameCount = 0;

    /** HRTF FFT filter size in frames */
    int FilterSize = 0;

    TPodVectorHeap< uint32_t > Indices;
    TPodVectorHeap< Float3 > Vertices;
    TPodVectorHeap< SComplex > hrtfL;
    TPodVectorHeap< SComplex > hrtfR;

    void * ForwardFFT = nullptr;
    void * InverseFFT = nullptr;

    // Storage for processing frames, time domain
    SComplex * pFramesSourceFFT = nullptr;
    // Processing frames, freq domain
    SComplex * pFramesFreqFFT = nullptr;
    // Frames for left ear, freq domain
    SComplex * pFramesFreqLeftFFT = nullptr;
    // Frames for right ear, freq domain
    SComplex * pFramesFreqRightFFT = nullptr;
    // Frames for left ear, time domain
    SComplex * pFramesTimeLeftFFT = nullptr;
    // Frames for right ear, time domain
    SComplex * pFramesTimeRightFFT = nullptr;

    SComplex * pHRTFs[4] = { nullptr,nullptr,nullptr,nullptr };
};
