/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2018 Alexander Samusev.

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

#include <Engine/Core/Public/FastRand.h>
//#include <stdlib.h>

#define FAST_RAND_THREAD_SAFE

#ifdef FAST_RAND_THREAD_SAFE
#define THREAD_LOCAL thread_local
#else
#define THREAD_LOCAL
#endif

static float LookupTable[65536];
THREAD_LOCAL static unsigned short FastRandIterator = 0;

namespace FCore {

// Call FastRandInit from program main
void FastRandInit( unsigned int _InitialSeed ) {
    srand( _InitialSeed );
    for ( int i = 0 ; i < 65536 ; i++ ) {
        LookupTable[i] = (float)rand() / RAND_MAX;
    }
    FastRandIterator = 0;
}

float FastRand( float _From, float _To ) {
    return _From + ( _To - _From ) * LookupTable[++FastRandIterator];
}

float FastRand() {
    return LookupTable[++FastRandIterator];
}

float FastSignedRand() {
    return LookupTable[++FastRandIterator]*2.0f - 1.0f;
}

void FastRandSeed( unsigned int _Seed ) {
    FastRandIterator = _Seed & 65535;
}

}
