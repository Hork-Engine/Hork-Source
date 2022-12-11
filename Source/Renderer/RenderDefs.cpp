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

#include "RenderDefs.h"

// TODO: this can be computed at compile-time
float FRUSTUM_SLICE_SCALE = -(MAX_FRUSTUM_CLUSTERS_Z + FRUSTUM_SLICE_OFFSET) / std::log2((double)FRUSTUM_CLUSTER_ZFAR / FRUSTUM_CLUSTER_ZNEAR);
float FRUSTUM_SLICE_BIAS  = std::log2((double)FRUSTUM_CLUSTER_ZFAR) * (MAX_FRUSTUM_CLUSTERS_Z + FRUSTUM_SLICE_OFFSET) / std::log2((double)FRUSTUM_CLUSTER_ZFAR / FRUSTUM_CLUSTER_ZNEAR) - FRUSTUM_SLICE_OFFSET;
float FRUSTUM_SLICE_ZCLIP[MAX_FRUSTUM_CLUSTERS_Z + 1];

struct FrustumSliceZClipInitializer
{
    FrustumSliceZClipInitializer()
    {
        FRUSTUM_SLICE_ZCLIP[0] = 1; // extended near cluster

        for (int SliceIndex = 1; SliceIndex < MAX_FRUSTUM_CLUSTERS_Z + 1; SliceIndex++)
        {
            //float sliceZ = FRUSTUM_CLUSTER_ZNEAR * Math::Pow( ( FRUSTUM_CLUSTER_ZFAR / FRUSTUM_CLUSTER_ZNEAR ), ( float )sliceIndex / MAX_FRUSTUM_CLUSTERS_Z ); // linear depth
            //FRUSTUM_SLICE_ZCLIP[ sliceIndex ] = ( FRUSTUM_CLUSTER_ZFAR * FRUSTUM_CLUSTER_ZNEAR / sliceZ - FRUSTUM_CLUSTER_ZNEAR ) / FRUSTUM_CLUSTER_ZRANGE; // to ndc

            FRUSTUM_SLICE_ZCLIP[SliceIndex] = (FRUSTUM_CLUSTER_ZFAR / Math::Pow((double)FRUSTUM_CLUSTER_ZFAR / FRUSTUM_CLUSTER_ZNEAR, (double)(SliceIndex + FRUSTUM_SLICE_OFFSET) / (MAX_FRUSTUM_CLUSTERS_Z + FRUSTUM_SLICE_OFFSET)) - FRUSTUM_CLUSTER_ZNEAR) / FRUSTUM_CLUSTER_ZRANGE; // to ndc
        }
    }
};

static FrustumSliceZClipInitializer FrustumSliceZClipInitializer;

