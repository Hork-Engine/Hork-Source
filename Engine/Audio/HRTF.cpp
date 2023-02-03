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

#include "HRTF.h"

#include <Engine/Core/Platform/Logger.h>
#include <Engine/Core/Platform/Platform.h>
#include <Engine/Geometry/BV/BvIntersect.h>
#include <Engine/Core/ConsoleVar.h>
#include <Engine/Runtime/EmbeddedResources.h>

// Use miniaudio for resampling
#include <miniaudio/miniaudio.h>

#include <muFFT/fft.h>

HK_NAMESPACE_BEGIN

#define FILTER_SIZE_POW2

ConsoleVar Snd_LerpHRTF("Snd_LerpHRTF"s, "1"s);

AudioHRTF::AudioHRTF(int SampleRate)
{
    File f = File::OpenRead("HRTF/IRC_1002_C.bin", Runtime::GetEmbeddedResources());
    if (!f)
    {
        // An error occurred...
        CriticalError("Failed to open HRTF data\n");
    }

    /*
    | Field        | Size | Type     | Value |
    |--------------|------|----------|-------|
    | magic        | 4    | uint32_t | HRIR  |
    | sample_rate  | 4    | uint32_t |       |
    | length       | 4    | uint32_t |       |
    | vertex_count | 4    | uint32_t |       |
    | index_count  | 4    | uint32_t |       |
    */

    uint32_t magic = f.ReadUInt32();
    const char* MAGIC = "HRIR";
    if (std::memcmp(&magic, MAGIC, sizeof(uint32_t)) != 0)
    {
        CriticalError("Invalid HRTF data\n");
    }

    int sampleRateHRIR = (int)f.ReadUInt32();
    m_FrameCount = (int)f.ReadUInt32();

    uint32_t vertexCount = f.ReadUInt32();
    uint32_t indexCount = f.ReadUInt32();

    if (indexCount % 3)
    {
        CriticalError("Invalid index count for HRTF geometry\n");
    }

    /*
    | Field   | Size            | Type     |
    |---------|-----------------|----------|
    | Indices | 4 * index_count | uint32_t |
    */

    m_Indices.Resize(indexCount);
    f.ReadWords<uint32_t>(m_Indices.ToPtr(), m_Indices.Size());

    /*
    Vertex format

    | Field      | Size       | Type  |
    |------------|------------|-------|
    | X          | 4          | float |
    | Y          | 4          | float |
    | Z          | 4          | float |
    | Left HRIR  | 4 * length | float |
    | Right HRIR | 4 * length | float |
    */

    m_Vertices.Resize(vertexCount);

    if (sampleRateHRIR == SampleRate)
    {
        // There is no need for resampling, so we just read it as is

        TVector<float> framesIn;
        framesIn.Resize(m_FrameCount);

        m_FilterSize = m_FrameCount - 1 + HRTF_BLOCK_LENGTH; // M - 1 + L
#ifdef FILTER_SIZE_POW2
        m_FilterSize = Math::ToGreaterPowerOfTwo(m_FilterSize);
#endif
        m_ForwardFFT = mufft_create_plan_1d_c2c(m_FilterSize, MUFFT_FORWARD, 0);
        m_InverseFFT = mufft_create_plan_1d_c2c(m_FilterSize, MUFFT_INVERSE, 0);

        m_hrtfL.Resize(vertexCount * m_FilterSize);
        m_hrtfR.Resize(vertexCount * m_FilterSize);

        for (auto i = 0; i < vertexCount; i++)
        {
            f.ReadObject(m_Vertices[i]);
            m_Vertices[i].X = -m_Vertices[i].X;

            f.ReadFloats(framesIn.ToPtr(), framesIn.Size());
            GenerateHRTF(framesIn.ToPtr(), framesIn.Size(), m_hrtfL.ToPtr() + i * m_FilterSize);

            f.ReadFloats(framesIn.ToPtr(), framesIn.Size());
            GenerateHRTF(framesIn.ToPtr(), framesIn.Size(), m_hrtfR.ToPtr() + i * m_FilterSize);
        }
    }
    else
    {
        ma_resampler_config config = ma_resampler_config_init(ma_format_f32, 1, sampleRateHRIR, SampleRate, ma_resample_algorithm_linear);
        ma_resampler resampler;
        ma_result result = ma_resampler_init(&config, &resampler);
        if (result != MA_SUCCESS)
        {
            CriticalError("Failed to resample HRTF data\n");
        }

        ma_uint64 frameCountIn = m_FrameCount;
        ma_uint64 frameCountOut = ma_resampler_get_expected_output_frame_count(&resampler, m_FrameCount);

        TVector<float> framesIn;
        framesIn.Resize(frameCountIn);

        TVector<float> framesOut;
        framesOut.Resize(frameCountOut);

        m_FrameCount = frameCountOut;
        m_FilterSize = m_FrameCount - 1 + HRTF_BLOCK_LENGTH; // M - 1 + L
#ifdef FILTER_SIZE_POW2
        m_FilterSize = Math::ToGreaterPowerOfTwo(m_FilterSize);
#endif
        m_ForwardFFT = mufft_create_plan_1d_c2c(m_FilterSize, MUFFT_FORWARD, 0);
        m_InverseFFT = mufft_create_plan_1d_c2c(m_FilterSize, MUFFT_INVERSE, 0);

        m_hrtfL.Resize(vertexCount * m_FilterSize);
        m_hrtfR.Resize(vertexCount * m_FilterSize);

        for (auto i = 0; i < vertexCount; i++)
        {
            f.ReadObject(m_Vertices[i]);
            m_Vertices[i].X = -m_Vertices[i].X;

            f.ReadFloats(framesIn.ToPtr(), framesIn.Size());

            // ma_resampler_process_pcm_frames overwrite frameCountIn and frameCountOut, so we restore them before each call
            frameCountIn = framesIn.Size();
            frameCountOut = framesOut.Size();
            result = ma_resampler_process_pcm_frames(&resampler, framesIn.ToPtr(), &frameCountIn, framesOut.ToPtr(), &frameCountOut);
            if (result != MA_SUCCESS)
            {
                CriticalError("Failed to resample HRTF data\n");
            }
            HK_ASSERT(frameCountOut <= framesOut.Size());

            GenerateHRTF(framesOut.ToPtr(), frameCountOut, m_hrtfL.ToPtr() + i * m_FilterSize);

            f.ReadFloats(framesIn.ToPtr(), framesIn.Size());

            // ma_resampler_process_pcm_frames overwrite frameCountIn and frameCountOut, so we restore them before each call
            frameCountIn = framesIn.Size();
            frameCountOut = framesOut.Size();
            result = ma_resampler_process_pcm_frames(&resampler, framesIn.ToPtr(), &frameCountIn, framesOut.ToPtr(), &frameCountOut);
            if (result != MA_SUCCESS)
            {
                CriticalError("Failed to resample HRTF data\n");
            }
            HK_ASSERT(frameCountOut <= framesOut.Size());

            GenerateHRTF(framesOut.ToPtr(), frameCountOut, m_hrtfR.ToPtr() + i * m_FilterSize);
        }

        ma_resampler_uninit(&resampler);
    }

    m_pFramesSourceFFT = (Complex*)mufft_calloc(m_FilterSize * sizeof(Complex));
    m_pFramesFreqFFT = (Complex*)mufft_alloc(m_FilterSize * sizeof(Complex));
    m_pFramesFreqLeftFFT = (Complex*)mufft_alloc(m_FilterSize * sizeof(Complex));
    m_pFramesFreqRightFFT = (Complex*)mufft_alloc(m_FilterSize * sizeof(Complex));
    m_pFramesTimeLeftFFT = (Complex*)mufft_alloc(m_FilterSize * sizeof(Complex));
    m_pFramesTimeRightFFT = (Complex*)mufft_alloc(m_FilterSize * sizeof(Complex));

    for (int i = 0; i < 4; i++)
    {
        m_pHRTFs[i] = (Complex*)mufft_alloc(sizeof(Complex) * m_FilterSize);
    }
}

AudioHRTF::~AudioHRTF()
{
    for (int i = 0; i < 4; i++)
    {
        mufft_free(m_pHRTFs[i]);
    }

    mufft_free(m_pFramesSourceFFT);
    mufft_free(m_pFramesFreqFFT);
    mufft_free(m_pFramesFreqLeftFFT);
    mufft_free(m_pFramesFreqRightFFT);
    mufft_free(m_pFramesTimeLeftFFT);
    mufft_free(m_pFramesTimeRightFFT);

    mufft_free_plan_1d((mufft_plan_1d*)m_ForwardFFT);
    mufft_free_plan_1d((mufft_plan_1d*)m_InverseFFT);
}

void AudioHRTF::FFT(Complex const* pIn, Complex* pOut)
{
    mufft_execute_plan_1d((mufft_plan_1d*)m_ForwardFFT, pOut, pIn);
}

void AudioHRTF::IFFT(Complex const* pIn, Complex* pOut)
{
    mufft_execute_plan_1d((mufft_plan_1d*)m_InverseFFT, pOut, pIn);
}

void AudioHRTF::GenerateHRTF(const float* pFrames, int InFrameCount, Complex* pHRTF)
{
    // MuFFT requires 64-bit aligned data. Therefore, we must use it's own allocators.
    Complex* hrirComplex = (Complex*)mufft_alloc(sizeof(Complex) * m_FilterSize);
    Complex* temp = (Complex*)mufft_alloc(sizeof(Complex) * m_FilterSize);

    for (int i = 0; i < m_FilterSize; i++)
    {
        if (i < InFrameCount)
        {
            hrirComplex[i].R = pFrames[i];
        }
        else
        {
            hrirComplex[i].R = 0;
        }
        hrirComplex[i].I = 0;
    }

    FFT(hrirComplex, temp);

    Platform::Memcpy(pHRTF, temp, sizeof(Complex) * m_FilterSize);

    mufft_free(temp);
    mufft_free(hrirComplex);
}

void AudioHRTF::SampleHRTF(Float3 const& Dir, Complex* pLeftHRTF, Complex* pRightHRTF) const
{
    // TODO: optimize: create sphere with regular grid. Find sphere segment by azimuth and pitch. Perform raycast between two triangles of the segment.

    float d, u, v;

    for (int i = 0; i < m_Indices.Size(); i += 3)
    {
        uint32_t index0 = m_Indices[i + 0];
        uint32_t index1 = m_Indices[i + 1];
        uint32_t index2 = m_Indices[i + 2];

        Float3 const& a = m_Vertices[index0];
        Float3 const& b = m_Vertices[index1];
        Float3 const& c = m_Vertices[index2];

        if (BvRayIntersectTriangle(Float3(0.0f), Dir, a, b, c, d, u, v))
        {
            float w = 1.0f - u - v;

            if (w < 0.0f) w = 0.0f; // fix rounding issues

            Complex const* a_left = m_hrtfL.ToPtr() + index0 * m_FilterSize;
            Complex const* a_right = m_hrtfR.ToPtr() + index0 * m_FilterSize;

            Complex const* b_left = m_hrtfL.ToPtr() + index1 * m_FilterSize;
            Complex const* b_right = m_hrtfR.ToPtr() + index1 * m_FilterSize;

            Complex const* c_left = m_hrtfL.ToPtr() + index2 * m_FilterSize;
            Complex const* c_right = m_hrtfR.ToPtr() + index2 * m_FilterSize;

            for (int n = 0; n < m_FilterSize; n++)
            {
                pLeftHRTF[n].R = a_left[n].R * u + b_left[n].R * v + c_left[n].R * w;
                pLeftHRTF[n].I = a_left[n].I * u + b_left[n].I * v + c_left[n].I * w;
                pRightHRTF[n].R = a_right[n].R * u + b_right[n].R * v + c_right[n].R * w;
                pRightHRTF[n].I = a_right[n].I * u + b_right[n].I * v + c_right[n].I * w;
            }

            return;
        }
    }

    Platform::ZeroMem(pLeftHRTF, m_FilterSize * sizeof(Complex));
    Platform::ZeroMem(pRightHRTF, m_FilterSize * sizeof(Complex));
}

void AudioHRTF::ApplyHRTF(Float3 const& CurDir, Float3 const& NewDir, const float* pFrames, int InFrameCount, float* pStream, Float3& Dir)
{
    HK_ASSERT(InFrameCount > 0);
    HK_ASSERT((InFrameCount % HRTF_BLOCK_LENGTH) == 0);

    const int numBlocks = InFrameCount / HRTF_BLOCK_LENGTH;
    const int hrtfLen = m_FrameCount - 1;

    Complex* filterL[2] = {
        m_pHRTFs[0],
        m_pHRTFs[1]};
    Complex* filterR[2] = {
        m_pHRTFs[2],
        m_pHRTFs[3]};

    Complex* pFramesLeft = m_pFramesTimeLeftFFT + hrtfLen;
    Complex* pFramesRight = m_pFramesTimeRightFFT + hrtfLen;

    int curIndex = 1;
    int newIndex = 0;

    bool bNoLerp = CurDir.LengthSqr() < 0.1f || !Snd_LerpHRTF;
    Dir = bNoLerp ? NewDir : CurDir;

    SampleHRTF(Dir, filterL[curIndex], filterR[curIndex]);

    const float* history = pFrames;
    const float* frames = pFrames + hrtfLen;

    for (int blockNum = 0; blockNum < numBlocks; blockNum++)
    {
        // Copy frames to m_pFramesSourceFFT
        int n = 0, frameNum = 0;
        while (n < hrtfLen)
        {
            // Restore previous frames from channel
            m_pFramesSourceFFT[n].R = history[n];
            n++;
        }
        while (frameNum < HRTF_BLOCK_LENGTH)
        {
            m_pFramesSourceFFT[n].R = frames[frameNum];
            frameNum++;
            n++;
        }

        // Perform FFT on source frames
        FFT(m_pFramesSourceFFT, m_pFramesFreqFFT);

        // Apply HRTF
        for (n = 0; n < m_FilterSize; n++)
        {
            m_pFramesFreqLeftFFT[n] = m_pFramesFreqFFT[n] * filterL[curIndex][n];
            m_pFramesFreqRightFFT[n] = m_pFramesFreqFFT[n] * filterR[curIndex][n];
        }

        // Perform inverse FFT to convert result in time domain
        IFFT(m_pFramesFreqLeftFFT, m_pFramesTimeLeftFFT);
        IFFT(m_pFramesFreqRightFFT, m_pFramesTimeRightFFT);

        float* blockStart = pStream;

        // Save block in output stream
        for (n = 0; n < HRTF_BLOCK_LENGTH; n++)
        {
            pStream[0] = pFramesLeft[n].R;
            pStream[1] = pFramesRight[n].R;
            pStream += 2;
        }

        if (!bNoLerp)
        {
            Dir = Math::Lerp(CurDir, NewDir, (blockNum + 1) * 1.0f / numBlocks);
            Dir.NormalizeSelf();

            SampleHRTF(Dir, filterL[newIndex], filterR[newIndex]);

            // Apply HRTF
            for (n = 0; n < m_FilterSize; n++)
            {
                m_pFramesFreqLeftFFT[n] = m_pFramesFreqFFT[n] * filterL[newIndex][n];
                m_pFramesFreqRightFFT[n] = m_pFramesFreqFFT[n] * filterR[newIndex][n];
            }

            // Perform inverse FFT to convert result in time domain
            IFFT(m_pFramesFreqLeftFFT, m_pFramesTimeLeftFFT);
            IFFT(m_pFramesFreqRightFFT, m_pFramesTimeRightFFT);

            pStream = blockStart;

            // Save block in output stream
            const float scale = 1.0f / HRTF_BLOCK_LENGTH;
            for (n = 0; n < HRTF_BLOCK_LENGTH; n++)
            {
                float lerp = (float)n * scale;
                pStream[0] = Math::Lerp(pStream[0], pFramesLeft[n].R, lerp);
                pStream[1] = Math::Lerp(pStream[1], pFramesRight[n].R, lerp);
                pStream += 2;
            }

            std::swap(curIndex, newIndex);
        }

        frames += HRTF_BLOCK_LENGTH;
        history += HRTF_BLOCK_LENGTH;
    }
}

#if 0
void DrawHRTF( DebugRenderer * InRenderer )
{
    static bool binit = false;
    static TVector< Float3 > sphereVerts;
    static TVector< uint32_t > sphereIndices;
    if ( !binit ) {
        binit = true;

        SHrtfSphere hrir( 44100 );

        sphereVerts = hrir.GetVertices();
        for ( Float3 & v : sphereVerts ) {
            v *= 10.0f;
        }
        sphereIndices = hrir.GetIndices();
    }

    InRenderer->DrawTriangleSoupWireframe( sphereVerts, sphereIndices );
}
#endif

HK_NAMESPACE_END
