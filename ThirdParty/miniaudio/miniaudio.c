#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.h"

#define MINIAUDIO_IMPLEMENTATION
//#define MA_PREFER_SSE2
#include "miniaudio.h"

// The stb_vorbis implementation must come after the implementation of miniaudio.
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4245 )
#pragma warning( disable : 4456 )
#pragma warning( disable : 4457 )
#pragma warning( disable : 4701 )
#endif

#undef STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.h"

#ifdef _MSC_VER
#pragma warning( pop )
#endif
