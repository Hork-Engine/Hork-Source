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

#include <Engine/Geometry/VectorMath.h>
#include <Engine/Geometry/Complex.h>
#include <Engine/Core/Containers/Vector.h>

HK_NAMESPACE_BEGIN

constexpr int HRTF_BLOCK_LENGTH = 128; // Keep it to a power of two

class AudioHRTF
{
    HK_FORBID_COPY(AudioHRTF)

public:
    AudioHRTF(int SampleRate);
    virtual ~AudioHRTF();

    /** Gets a bilinearly interpolated HRTF */
    void SampleHRTF(Float3 const& Dir, Complex* pLeftHRTF, Complex* pRightHRTF) const;

    /** Applies HRTF to input frames. Frames must also contain GetFrameCount()-1 of the previous frames.
    FrameCount must be multiples of HRTF_BLOCK_LENGTH */
    void ApplyHRTF(Float3 const& CurDir, Float3 const& NewDir, const float* pFrames, int FrameCount, float* pStream, Float3& Dir);

    /** Sphere geometry vertics */
    TVector<Float3> const& GetVertices() const { return m_Vertices; }

    /** Sphere geometry indices */
    TVector<uint32_t> const& GetIndices() const { return m_Indices; }

    /** Length of Head-Related Impulse Response (HRIR) */
    int GetFrameCount() const
    {
        return m_FrameCount;
    }

    /** HRTF FFT filter size in frames */
    int GetFilterSize() const
    {
        // Computed as power of two of FrameCount - 1 + HRTF_BLOCK_LENGTH
        return m_FilterSize;
    }

private:
    void GenerateHRTF(const float* pFrames, int InFrameCount, Complex* pHRTF);

    /** Fast fourier transform (forward) */
    void FFT(Complex const* pIn, Complex* pOut);

    /** Fast fourier transform (inverse) */
    void IFFT(Complex const* pIn, Complex* pOut);

    /** Length of Head-Related Impulse Response (HRIR) */
    int m_FrameCount = 0;

    /** HRTF FFT filter size in frames */
    int m_FilterSize = 0;

    TVector<uint32_t> m_Indices;
    TVector<Float3> m_Vertices;
    TVector<Complex> m_hrtfL;
    TVector<Complex> m_hrtfR;

    void* m_ForwardFFT = nullptr;
    void* m_InverseFFT = nullptr;

    // Storage for processing frames, time domain
    Complex* m_pFramesSourceFFT = nullptr;
    // Processing frames, freq domain
    Complex* m_pFramesFreqFFT = nullptr;
    // Frames for left ear, freq domain
    Complex* m_pFramesFreqLeftFFT = nullptr;
    // Frames for right ear, freq domain
    Complex* m_pFramesFreqRightFFT = nullptr;
    // Frames for left ear, time domain
    Complex* m_pFramesTimeLeftFFT = nullptr;
    // Frames for right ear, time domain
    Complex* m_pFramesTimeRightFFT = nullptr;

    Complex* m_pHRTFs[4] = {nullptr, nullptr, nullptr, nullptr};
};

HK_NAMESPACE_END
