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

#include <Platform/Public/Logger.h>
#include <Platform/Public/Platform.h>
#include <Geometry/Public/BV/BvIntersect.h>

#include <Runtime/Public/RuntimeVariable.h>
#include <Runtime/Public/Runtime.h>

#include <Audio/Public/HRTF.h>

// Use miniaudio for resampling
#include <miniaudio/miniaudio.h>

#include <muFFT/fft.h>

#define FILTER_SIZE_POW2

ARuntimeVariable Snd_LerpHRTF( _CTS("Snd_LerpHRTF"), _CTS("1") );

AAudioHRTF::AAudioHRTF( int SampleRate )
{
    AMemoryStream f;
    if ( !f.OpenRead( "HRTF/IRC_1002_C.bin", GRuntime->GetEmbeddedResources() ) ) {
        // An error occurred...
        CriticalError( "Failed to open HRTF data\n" );
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
    const char * MAGIC = "HRIR";
    if (std::memcmp(&magic, MAGIC, sizeof(uint32_t)) != 0)
    {
        CriticalError( "Invalid HRTF data\n" );
    }

    int sampleRateHRIR = (int)f.ReadUInt32();
    FrameCount = (int)f.ReadUInt32();

    uint32_t vertexCount = f.ReadUInt32();
    uint32_t indexCount = f.ReadUInt32();

    if ( indexCount % 3 ) {
        CriticalError( "Invalid index count for HRTF geometry\n" );
    }

    /*
    | Field   | Size            | Type     |
    |---------|-----------------|----------|
    | Indices | 4 * index_count | uint32_t |
    */

    Indices.Resize( indexCount );
    f.ReadUInt32ToBuffer( Indices.ToPtr(), Indices.Size() );

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

    Vertices.Resize( vertexCount );

    if ( sampleRateHRIR == SampleRate ) {
        // There is no need for resampling, so we just read it as is

        TPodVectorHeap< float > framesIn;
        framesIn.Resize( FrameCount );

        FilterSize = FrameCount - 1 + HRTF_BLOCK_LENGTH; // M - 1 + L
#ifdef FILTER_SIZE_POW2
        FilterSize = Math::ToGreaterPowerOfTwo( FilterSize );
#endif
        ForwardFFT = mufft_create_plan_1d_c2c( FilterSize, MUFFT_FORWARD, 0 );
        InverseFFT = mufft_create_plan_1d_c2c( FilterSize, MUFFT_INVERSE, 0 );

        hrtfL.Resize( vertexCount * FilterSize );
        hrtfR.Resize( vertexCount * FilterSize );

        for ( auto i = 0 ; i < vertexCount ; i++ ) {
            f.ReadObject( Vertices[i] );
            Vertices[i].X = -Vertices[i].X;

            f.ReadFloatToBuffer( framesIn.ToPtr(), framesIn.Size() );
            GenerateHRTF( framesIn.ToPtr(), framesIn.Size(), hrtfL.ToPtr() + i * FilterSize );

            f.ReadFloatToBuffer( framesIn.ToPtr(), framesIn.Size() );
            GenerateHRTF( framesIn.ToPtr(), framesIn.Size(), hrtfR.ToPtr() + i * FilterSize );
        }
    }
    else {
        ma_resampler_config config = ma_resampler_config_init( ma_format_f32, 1, sampleRateHRIR, SampleRate, ma_resample_algorithm_linear );
        ma_resampler resampler;
        ma_result result = ma_resampler_init( &config, &resampler );
        if ( result != MA_SUCCESS ) {
            CriticalError( "Failed to resample HRTF data\n" );
        }

        ma_uint64 frameCountIn = FrameCount;
        ma_uint64 frameCountOut = ma_resampler_get_expected_output_frame_count( &resampler, FrameCount );

        TPodVectorHeap< float > framesIn;
        framesIn.Resize( frameCountIn );

        TPodVectorHeap< float > framesOut;
        framesOut.Resize( frameCountOut );

        FrameCount = frameCountOut;
        FilterSize = FrameCount - 1 + HRTF_BLOCK_LENGTH; // M - 1 + L
#ifdef FILTER_SIZE_POW2
        FilterSize = Math::ToGreaterPowerOfTwo( FilterSize );
#endif
        ForwardFFT = mufft_create_plan_1d_c2c( FilterSize, MUFFT_FORWARD, 0 );
        InverseFFT = mufft_create_plan_1d_c2c( FilterSize, MUFFT_INVERSE, 0 );

        hrtfL.Resize( vertexCount * FilterSize );
        hrtfR.Resize( vertexCount * FilterSize );

        for ( auto i = 0 ; i < vertexCount ; i++ ) {
            f.ReadObject( Vertices[i] );
            Vertices[i].X = -Vertices[i].X;

            f.ReadFloatToBuffer( framesIn.ToPtr(), framesIn.Size() );

            // ma_resampler_process_pcm_frames overwrite frameCountIn and frameCountOut, so we restore them before each call
            frameCountIn = framesIn.Size();
            frameCountOut = framesOut.Size();
            result = ma_resampler_process_pcm_frames( &resampler, framesIn.ToPtr(), &frameCountIn, framesOut.ToPtr(), &frameCountOut );
            if ( result != MA_SUCCESS ) {
                CriticalError( "Failed to resample HRTF data\n" );
            }
            AN_ASSERT( frameCountOut <= framesOut.Size() );

            GenerateHRTF( framesOut.ToPtr(), frameCountOut, hrtfL.ToPtr() + i * FilterSize );

            f.ReadFloatToBuffer( framesIn.ToPtr(), framesIn.Size() );

            // ma_resampler_process_pcm_frames overwrite frameCountIn and frameCountOut, so we restore them before each call
            frameCountIn = framesIn.Size();
            frameCountOut = framesOut.Size();
            result = ma_resampler_process_pcm_frames( &resampler, framesIn.ToPtr(), &frameCountIn, framesOut.ToPtr(), &frameCountOut );
            if ( result != MA_SUCCESS ) {
                CriticalError( "Failed to resample HRTF data\n" );
            }
            AN_ASSERT( frameCountOut <= framesOut.Size() );

            GenerateHRTF( framesOut.ToPtr(), frameCountOut, hrtfR.ToPtr() + i * FilterSize );
        }

        ma_resampler_uninit( &resampler );
    }

    pFramesSourceFFT = (SComplex *)mufft_calloc( FilterSize * sizeof( SComplex ) );
    pFramesFreqFFT = (SComplex *)mufft_alloc( FilterSize * sizeof( SComplex ) );
    pFramesFreqLeftFFT = (SComplex *)mufft_alloc( FilterSize * sizeof( SComplex ) );
    pFramesFreqRightFFT = (SComplex *)mufft_alloc( FilterSize * sizeof( SComplex ) );
    pFramesTimeLeftFFT = (SComplex *)mufft_alloc( FilterSize * sizeof( SComplex ) );
    pFramesTimeRightFFT = (SComplex *)mufft_alloc( FilterSize * sizeof( SComplex ) );

    for ( int i = 0 ; i < 4 ; i++ ) {
        pHRTFs[i] = (SComplex *)mufft_alloc( sizeof( SComplex ) * FilterSize );
    }
}

AAudioHRTF::~AAudioHRTF()
{
    for ( int i = 0 ; i < 4 ; i++ ) {
        mufft_free( pHRTFs[i] );
    }

    mufft_free( pFramesSourceFFT );
    mufft_free( pFramesFreqFFT );
    mufft_free( pFramesFreqLeftFFT );
    mufft_free( pFramesFreqRightFFT );
    mufft_free( pFramesTimeLeftFFT );
    mufft_free( pFramesTimeRightFFT );

    mufft_free_plan_1d( (mufft_plan_1d *)ForwardFFT );
    mufft_free_plan_1d( (mufft_plan_1d *)InverseFFT );
}

void AAudioHRTF::FFT( SComplex const * pIn, SComplex * pOut )
{
    mufft_execute_plan_1d( (mufft_plan_1d *)ForwardFFT, pOut, pIn );
}

void AAudioHRTF::IFFT( SComplex const * pIn, SComplex * pOut )
{
    mufft_execute_plan_1d( (mufft_plan_1d *)InverseFFT, pOut, pIn );
}

void AAudioHRTF::GenerateHRTF( const float * pFrames, int InFrameCount, SComplex * pHRTF )
{
    // MuFFT requires 64-bit aligned data. Therefore, we must use it's own allocators.
    SComplex * hrirComplex = (SComplex *)mufft_alloc( sizeof( SComplex ) * FilterSize );
    SComplex * temp = (SComplex *)mufft_alloc( sizeof( SComplex ) * FilterSize );

    for ( int i = 0; i < FilterSize; i++ ) {
        if ( i < InFrameCount ) {
            hrirComplex[i].R = pFrames[i];
        }
        else {
            hrirComplex[i].R = 0;
        }
        hrirComplex[i].I = 0;
    }

    FFT( hrirComplex, temp );

    Core::Memcpy( pHRTF, temp, sizeof( SComplex ) * FilterSize );

    mufft_free( temp );
    mufft_free( hrirComplex );
}

void AAudioHRTF::SampleHRTF( Float3 const & Dir, SComplex * pLeftHRTF, SComplex * pRightHRTF ) const
{
    // TODO: optimize: create sphere with regular grid. Find sphere segment by azimuth and pitch. Perform raycast between two triangles of the segment.

    float d, u, v;

    for ( int i = 0 ; i < Indices.Size() ; i += 3 ) {
        uint32_t index0 = Indices[i+0];
        uint32_t index1 = Indices[i+1];
        uint32_t index2 = Indices[i+2];

        Float3 const & a = Vertices[index0];
        Float3 const & b = Vertices[index1];
        Float3 const & c = Vertices[index2];

        if ( BvRayIntersectTriangle( Float3(0.0f), Dir, a, b, c, d, u, v ) ) {
            float w = 1.0f - u - v;

            if ( w < 0.0f ) w = 0.0f; // fix rounding issues

            SComplex const * a_left  = hrtfL.ToPtr() + index0 * FilterSize;
            SComplex const * a_right = hrtfR.ToPtr() + index0 * FilterSize;

            SComplex const * b_left  = hrtfL.ToPtr() + index1 * FilterSize;
            SComplex const * b_right = hrtfR.ToPtr() + index1 * FilterSize;

            SComplex const * c_left  = hrtfL.ToPtr() + index2 * FilterSize;
            SComplex const * c_right = hrtfR.ToPtr() + index2 * FilterSize;

            for ( int n = 0 ; n < FilterSize ; n++ ) {
                pLeftHRTF[n].R = a_left[n].R * u + b_left[n].R * v + c_left[n].R * w;
                pLeftHRTF[n].I = a_left[n].I * u + b_left[n].I * v + c_left[n].I * w;
                pRightHRTF[n].R = a_right[n].R * u + b_right[n].R * v + c_right[n].R * w;
                pRightHRTF[n].I = a_right[n].I * u + b_right[n].I * v + c_right[n].I * w;
            }

            return;
        }
    }

    Core::ZeroMem( pLeftHRTF, FilterSize * sizeof( SComplex ) );
    Core::ZeroMem( pRightHRTF, FilterSize * sizeof( SComplex ) );
}

void AAudioHRTF::ApplyHRTF( Float3 const & CurDir, Float3 const & NewDir, const float * pFrames, int InFrameCount, float * pStream, Float3 & Dir )
{
    AN_ASSERT( InFrameCount > 0 );
    AN_ASSERT( ( InFrameCount % HRTF_BLOCK_LENGTH ) == 0 );

    const int numBlocks = InFrameCount / HRTF_BLOCK_LENGTH;
    const int hrtfLen = FrameCount - 1;

    SComplex * filterL[2] = {
        pHRTFs[0],
        pHRTFs[1]
    };
    SComplex * filterR[2] = {
        pHRTFs[2],
        pHRTFs[3]
    };

    SComplex * pFramesLeft = pFramesTimeLeftFFT + hrtfLen;
    SComplex * pFramesRight = pFramesTimeRightFFT + hrtfLen;

    int curIndex = 1;
    int newIndex = 0;

    bool bNoLerp = CurDir.LengthSqr() < 0.1f || !Snd_LerpHRTF;
    Dir = bNoLerp ? NewDir : CurDir;

    SampleHRTF( Dir, filterL[curIndex], filterR[curIndex] );

    const float * history = pFrames;
    const float * frames = pFrames + hrtfLen;

    for ( int blockNum = 0 ; blockNum < numBlocks ; blockNum++ ) {
        // Copy frames to pFramesSourceFFT
        int n = 0, frameNum = 0;
        while ( n < hrtfLen ) {
            // Restore previous frames from channel
            pFramesSourceFFT[n].R = history[n];
            n++;
        }
        while ( frameNum < HRTF_BLOCK_LENGTH ) {
            pFramesSourceFFT[n].R = frames[frameNum];
            frameNum++;
            n++;
        }

        // Perform FFT on source frames
        FFT( pFramesSourceFFT, pFramesFreqFFT );

        // Apply HRTF
        for ( n = 0; n < FilterSize ; n++ ) {
            pFramesFreqLeftFFT[n] = pFramesFreqFFT[n] * filterL[curIndex][n];
            pFramesFreqRightFFT[n] = pFramesFreqFFT[n] * filterR[curIndex][n];
        }

        // Perform inverse FFT to convert result in time domain
        IFFT( pFramesFreqLeftFFT, pFramesTimeLeftFFT );
        IFFT( pFramesFreqRightFFT, pFramesTimeRightFFT );

        float * blockStart = pStream;

        // Save block in output stream
        for ( n = 0 ; n < HRTF_BLOCK_LENGTH ; n++ ) {
            pStream[0] = pFramesLeft[n].R;
            pStream[1] = pFramesRight[n].R;
            pStream += 2;
        }

        if ( !bNoLerp ) {
            Dir = Math::Lerp( CurDir, NewDir, (blockNum + 1) * 1.0f / numBlocks );
            Dir.NormalizeSelf();

            SampleHRTF( Dir, filterL[newIndex], filterR[newIndex] );

            // Apply HRTF
            for ( n = 0; n < FilterSize ; n++ ) {
                pFramesFreqLeftFFT[n] = pFramesFreqFFT[n] * filterL[newIndex][n];
                pFramesFreqRightFFT[n] = pFramesFreqFFT[n] * filterR[newIndex][n];
            }

            // Perform inverse FFT to convert result in time domain
            IFFT( pFramesFreqLeftFFT, pFramesTimeLeftFFT );
            IFFT( pFramesFreqRightFFT, pFramesTimeRightFFT );

            pStream = blockStart;

            // Save block in output stream
            const float scale = 1.0f / HRTF_BLOCK_LENGTH;
            for ( n = 0 ; n < HRTF_BLOCK_LENGTH ; n++ ) {
                float lerp = (float)n * scale;
                pStream[0] = Math::Lerp( pStream[0], pFramesLeft[n].R, lerp );
                pStream[1] = Math::Lerp( pStream[1], pFramesRight[n].R, lerp );
                pStream += 2;
            }

            std::swap( curIndex, newIndex );
        }

        frames += HRTF_BLOCK_LENGTH;
        history += HRTF_BLOCK_LENGTH;
    }
}

#if 0
#include <World/Public/Base/DebugRenderer.h>
void DrawHRTF( ADebugRenderer * InRenderer )
{
    static bool binit = false;
    static TPodVectorHeap< Float3 > sphereVerts;
    static TPodVectorHeap< uint32_t > sphereIndices;
    if ( !binit ) {
        binit = true;

        SHrtfSphere hrir( 44100 );

        sphereVerts = hrir.GetVertices();
        for ( Float3 & v : sphereVerts ) {
            v *= 10.0f;
        }
        sphereIndices = hrir.GetIndices();
    }

    InRenderer->DrawTriangleSoupWireframe( sphereVerts.ToPtr(), sizeof( Float3 ), sphereIndices.ToPtr(), sphereIndices.Size() );
}
#endif
