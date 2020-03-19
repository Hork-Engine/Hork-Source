/*

Graphics Hardware Interface (GHI) is part of Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

#include "GHITransformFeedback.h"
#include "GHIDevice.h"
#include "GHIState.h"
#include "LUT.h"

#include "GL/glew.h"

namespace GHI {

TransformFeedback::TransformFeedback() {
    pDevice = nullptr;
    Handle = nullptr;
}

TransformFeedback::~TransformFeedback() {
    Deinitialize();
}

void TransformFeedback::Initialize( TransformFeedbackCreateInfo const & _CreateInfo ) {
    State * state = GetCurrentState();

    Deinitialize();

    GLuint id;
    glCreateTransformFeedbacks( 1, &id ); // 4.5

    pDevice = state->GetDevice();
    Handle = ( void * )( size_t )id;

    state->TotalTransformFeedbacks++;

}

void TransformFeedback::Deinitialize() {
    if ( !Handle ) {
        return;
    }

    State * state = GetCurrentState();

    GLuint id = GL_HANDLE( Handle );
    glDeleteTransformFeedbacks( 1, &id );
    state->TotalTransformFeedbacks--;

    pDevice = nullptr;
    Handle = nullptr;
}

}
