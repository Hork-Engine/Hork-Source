/*

Angie Engine Source Code

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

#pragma once

#include <RenderCore/TransformFeedback.h>

namespace RenderCore {

class ADeviceGLImpl;

class ATransformFeedbackGLImpl final : public ITransformFeedback
{
    friend class ADeviceGLImpl;

public:
    ATransformFeedbackGLImpl( ADeviceGLImpl * _Device, STransformFeedbackCreateInfo const & _CreateInfo );
    ~ATransformFeedbackGLImpl();

private:
    ADeviceGLImpl * pDevice;
};

#if 0

//glGetTransformFeedbacki64_v
//glGetTransformFeedbacki_v
//glGetTransformFeedbackiv
//glGetTransformFeedbackVarying

struct ShaderTransformFeedbackSlot {
    uint16_t SlotIndex;
    //BUFFER_TYPE BufferType;
    TransformFeedback * pTransformFeedback;
    Buffer const * pBuffer;
    size_t BindingOffset;
    size_t BindingSize;
};

void SetTransformFeedbackBuffer( ShaderTransformFeedbackSlot const & _Slot ) {
    if ( _Slot.BindingOffset > 0 ) {
        glTransformFeedbackBufferRange( (size_t)pTransformFeedback->GetHandle(), _Slot.SlotIndex, (size_t)_Slot.pBuffer->GetHandle(), _Slot.BindingOffset, _Slot.BindingSize ); // 4.5
    } else {
        glTransformFeedbackBufferBase( (size_t)pTransformFeedback->GetHandle(), _Slot.SlotIndex, (size_t)_Slot.pBuffer->GetHandle() ); // 4.5
    }
}

//glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const* varyings, GLenum bufferMode);

#endif


}
