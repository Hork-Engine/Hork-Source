/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include <Engine/Audio/Public/AudioSystem.h>

#define AL_NO_PROTOTYPES
#include <AL/al.h>

#ifndef AN_NO_OPENAL
#define AL_SAFE( Instruction ) {Instruction;AL_CheckError( AN_STRINGIFY( Instruction ) );}
#define ALC_SAFE( Instruction ) {Instruction;ALC_CheckError( AN_STRINGIFY( Instruction ) );}
#else
#define AL_SAFE( Instruction )
#define ALC_SAFE( Instruction )
#endif

extern LPALENABLE alEnable;
extern LPALDISABLE alDisable;
extern LPALISENABLED alIsEnabled;
extern LPALGETSTRING alGetString;
extern LPALGETBOOLEANV alGetBooleanv;
extern LPALGETINTEGERV alGetIntegerv;
extern LPALGETFLOATV alGetFloatv;
extern LPALGETDOUBLEV alGetDoublev;
extern LPALGETBOOLEAN alGetBoolean;
extern LPALGETINTEGER alGetInteger;
extern LPALGETFLOAT alGetFloat;
extern LPALGETDOUBLE alGetDouble;
extern LPALGETERROR alGetError;
extern LPALISEXTENSIONPRESENT alIsExtensionPresent;
extern LPALGETPROCADDRESS alGetProcAddress;
extern LPALGETENUMVALUE alGetEnumValue;
extern LPALLISTENERF alListenerf;
extern LPALLISTENER3F alListener3f;
extern LPALLISTENERFV alListenerfv;
extern LPALLISTENERI alListeneri;
extern LPALLISTENER3I alListener3i;
extern LPALLISTENERIV alListeneriv;
extern LPALGETLISTENERF alGetListenerf;
extern LPALGETLISTENER3F alGetListener3f;
extern LPALGETLISTENERFV alGetListenerfv;
extern LPALGETLISTENERI alGetListeneri;
extern LPALGETLISTENER3I alGetListener3i;
extern LPALGETLISTENERIV alGetListeneriv;
extern LPALGENSOURCES alGenSources;
extern LPALDELETESOURCES alDeleteSources;
extern LPALISSOURCE alIsSource;
extern LPALSOURCEF alSourcef;
extern LPALSOURCE3F alSource3f;
extern LPALSOURCEFV alSourcefv;
extern LPALSOURCEI alSourcei;
extern LPALSOURCE3I alSource3i;
extern LPALSOURCEIV alSourceiv;
extern LPALGETSOURCEF alGetSourcef;
extern LPALGETSOURCE3F alGetSource3f;
extern LPALGETSOURCEFV alGetSourcefv;
extern LPALGETSOURCEI alGetSourcei;
extern LPALGETSOURCE3I alGetSource3i;
extern LPALGETSOURCEIV alGetSourceiv;
extern LPALSOURCEPLAYV alSourcePlayv;
extern LPALSOURCESTOPV alSourceStopv;
extern LPALSOURCEREWINDV alSourceRewindv;
extern LPALSOURCEPAUSEV alSourcePausev;
extern LPALSOURCEPLAY alSourcePlay;
extern LPALSOURCESTOP alSourceStop;
extern LPALSOURCEREWIND alSourceRewind;
extern LPALSOURCEPAUSE alSourcePause;
extern LPALSOURCEQUEUEBUFFERS alSourceQueueBuffers;
extern LPALSOURCEUNQUEUEBUFFERS alSourceUnqueueBuffers;
extern LPALGENBUFFERS alGenBuffers;
extern LPALDELETEBUFFERS alDeleteBuffers;
extern LPALISBUFFER alIsBuffer;
extern LPALBUFFERDATA alBufferData;
extern LPALBUFFERF alBufferf;
extern LPALBUFFER3F alBuffer3f;
extern LPALBUFFERFV alBufferfv;
extern LPALBUFFERI alBufferi;
extern LPALBUFFER3I alBuffer3i;
extern LPALBUFFERIV alBufferiv;
extern LPALGETBUFFERF alGetBufferf;
extern LPALGETBUFFER3F alGetBuffer3f;
extern LPALGETBUFFERFV alGetBufferfv;
extern LPALGETBUFFERI alGetBufferi;
extern LPALGETBUFFER3I alGetBuffer3i;
extern LPALGETBUFFERIV alGetBufferiv;
extern LPALDOPPLERFACTOR alDopplerFactor;
extern LPALDOPPLERVELOCITY alDopplerVelocity;
extern LPALSPEEDOFSOUND alSpeedOfSound;
extern LPALDISTANCEMODEL alDistanceModel;

void AL_CheckError( const char * _Text );
unsigned int AL_CreateBuffer();
void AL_DeleteBuffer( unsigned int _Id );
void AL_UploadBuffer( unsigned int _Id, int _Format, void * _Data, size_t _Size, int _Frequency );
