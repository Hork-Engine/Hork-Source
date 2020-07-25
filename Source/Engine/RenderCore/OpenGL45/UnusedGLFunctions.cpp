
/*

// Set texture parameter
glTexParameterf                     glTextureParameterf
glTexParameterfv                    glTextureParameterfv
glTexParameteri                     glTextureParameteri
glTexParameterIiv                   glTextureParameterIiv
glTexParameterIuiv                  glTextureParameterIuiv
glTexParameteriv                    glTextureParameteriv

// Get texture parameter
glGetTexParameterfv                 glGetTextureParameterfv
glGetTexParameterIiv                glGetTextureParameterIiv
glGetTexParameterIuiv               glGetTextureParameterIuiv

// Framebuffer
glFramebufferRenderbuffer               glNamedFramebufferRenderbuffer
glGetFramebufferAttachmentParameteriv   glGetNamedFramebufferAttachmentParameteriv
glGetFramebufferParameteriv             glGetNamedFramebufferParameteriv

// glIs###
glIsProgram
glIsProgramPipeline
glIsShader
glIsVertexArray
glIsQuery
glIsTransformFeedback
glIsRenderbuffer
glIsSync

// Vertex arrays
glGetVertexArrayIndexed64iv
glGetVertexArrayIndexediv
glGetVertexArrayiv
glDisableVertexArrayAttrib

// Programs/Shaders UNUSED:
glUseProgram                        -- UNUSED
glGetShaderSource                   -- UNUSED
glGetAttachedShaders                -- UNUSED
glReleaseShaderCompiler             -- UNUSED
glActiveShaderProgram               -- UNUSED
glGetShaderPrecisionFormat          -- UNUSED
glGetShaderInfoLog                  -- UNUSED
glGetShaderiv                       -- UNUSED
glGetProgramInterfaceiv             -- UNUSED
glGetProgramStageiv                 -- UNUSED
glGetProgramResourceIndex           -- UNUSED
glGetProgramResourceiv              -- UNUSED
glGetProgramResourceLocation
glGetProgramResourceLocationIndex
glGetProgramResourceName
glUniformBlockBinding
glUniformSubroutinesuiv
glUniform1f             glProgramUniform1f
glUniform1fv            glProgramUniform1fv
glUniform1i             glProgramUniform1i
glUniform1iv            glProgramUniform1iv
glUniform1ui            glProgramUniform1ui
glUniform1uiv           glProgramUniform1uiv
glUniform2f             glProgramUniform2f
glUniform2fv            glProgramUniform2fv
glUniform2i             glProgramUniform2i
glUniform2iv            glProgramUniform2iv
glUniform2ui            glProgramUniform2ui
glUniform2uiv           glProgramUniform2uiv
glUniform3f             glProgramUniform3f
glUniform3fv            glProgramUniform3fv
glUniform3i             glProgramUniform3i
glUniform3iv            glProgramUniform3iv
glUniform3ui            glProgramUniform3ui
glUniform3uiv           glProgramUniform3uiv
glUniform4f             glProgramUniform4f
glUniform4fv            glProgramUniform4fv
glUniform4i             glProgramUniform4i
glUniform4iv            glProgramUniform4iv
glUniform4ui            glProgramUniform4ui
glUniform4uiv           glProgramUniform4uiv
glUniformMatrix2fv      glProgramUniformMatrix2fv
glUniformMatrix2x3fv    glProgramUniformMatrix2x3fv
glUniformMatrix2x4fv    glProgramUniformMatrix2x4fv
glUniformMatrix3fv      glProgramUniformMatrix3fv
glUniformMatrix3x2fv    glProgramUniformMatrix3x2fv
glUniformMatrix3x4fv    glProgramUniformMatrix3x4fv
glUniformMatrix4fv      glProgramUniformMatrix4fv
glUniformMatrix4x2fv    glProgramUniformMatrix4x2fv
glUniformMatrix4x3fv    glProgramUniformMatrix4x3fv
glGetnUniformdv
glGetnUniformfv
glGetnUniformiv
glGetnUniformuiv
glGetSubroutineUniformLocation
glGetActiveSubroutineUniformiv
glGetActiveSubroutineUniformName
glGetActiveUniform
glGetActiveUniformBlockiv
glGetActiveUniformBlockName
glGetActiveUniformName
glGetActiveUniformsiv
glGetUniformBlockIndex
glGetUniformdv
glGetUniformfv
glGetUniformIndices
glGetUniformiv
glGetUniformLocation
glGetUniformSubroutineuiv
glGetUniformuiv
glGetFragDataIndex
glGetFragDataLocation
glGetActiveAttrib
glGetActiveSubroutineName
glGetSubroutineIndex
glShaderStorageBlockBinding
glGetActiveAtomicCounterBufferiv

// Generic Vertex Attrib
glVertexAttrib1d
glVertexAttrib1dv
glVertexAttrib1f
glVertexAttrib1fv
glVertexAttrib1s
glVertexAttrib1sv
glVertexAttrib2d
glVertexAttrib2dv
glVertexAttrib2f
glVertexAttrib2fv
glVertexAttrib2s
glVertexAttrib2sv
glVertexAttrib3d
glVertexAttrib3dv
glVertexAttrib3f
glVertexAttrib3fv
glVertexAttrib3s
glVertexAttrib3sv
glVertexAttrib4bv
glVertexAttrib4d
glVertexAttrib4dv
glVertexAttrib4f
glVertexAttrib4fv
glVertexAttrib4iv
glVertexAttrib4Nbv
glVertexAttrib4Niv
glVertexAttrib4Nsv
glVertexAttrib4Nub
glVertexAttrib4Nubv
glVertexAttrib4Nuiv
glVertexAttrib4Nusv
glVertexAttrib4s
glVertexAttrib4sv
glVertexAttrib4ubv
glVertexAttrib4uiv
glVertexAttrib4usv
glVertexAttribI1i
glVertexAttribI1iv
glVertexAttribI1ui
glVertexAttribI1uiv
glVertexAttribI2i
glVertexAttribI2iv
glVertexAttribI2ui
glVertexAttribI2uiv
glVertexAttribI3i
glVertexAttribI3iv
glVertexAttribI3ui
glVertexAttribI3uiv
glVertexAttribI4bv
glVertexAttribI4i
glVertexAttribI4iv
glVertexAttribI4sv
glVertexAttribI4ubv
glVertexAttribI4ui
glVertexAttribI4uiv
glVertexAttribI4usv
glVertexAttribL1d
glVertexAttribL1dv
glVertexAttribL2d
glVertexAttribL2dv
glVertexAttribL3d
glVertexAttribL3dv
glVertexAttribL4d
glVertexAttribL4dv
glVertexAttribP1ui
glVertexAttribP2ui
glVertexAttribP3ui
glVertexAttribP4ui
glGetVertexAttribdv
glGetVertexAttribfv
glGetVertexAttribIiv
glGetVertexAttribIuiv
glGetVertexAttribiv
glGetVertexAttribLdv
glGetVertexAttribPointerv

// Query
glQueryCounter
glGetQueryIndexediv
glGetQueryiv
glGetQueryObjecti64v
glGetQueryObjectiv

// Transform feedback
glGetTransformFeedbacki64_v
glGetTransformFeedbacki_v
glGetTransformFeedbackiv
glGetTransformFeedbackVarying
glTransformFeedbackBufferBase
glTransformFeedbackBufferRange
glTransformFeedbackVaryings

// Renderbuffer
glCreateRenderbuffers
glDeleteRenderbuffers
glBindRenderbuffer
glGetRenderbufferParameteriv            glGetNamedRenderbufferParameteriv
glRenderbufferStorage                   glNamedRenderbufferStorage
glRenderbufferStorageMultisample        glNamedRenderbufferStorageMultisample

// Debugging
glDebugMessageCallback
glDebugMessageControl
glDebugMessageInsert
glGetDebugMessageLog
glPushDebugGroup
glPopDebugGroup
glGetPointerv
glObjectLabel
glObjectPtrLabel
glGetObjectLabel
glGetObjectPtrLabel

// Stencil
glStencilMaskSeparate   UNUSED

// Sync
glFinish

// Draw*
glDrawRangeElements
glDrawRangeElementsBaseVertex

glPrimitiveRestartIndex

glGetGraphicsResetStatus
glGetInternalformati64v
glGetInternalformativ

glPointParameterf
glPointParameterfv
glPointParameteri
glPointParameteriv
glPointSize

glProvokingVertex

glMinSampleShading
glSampleCoverage
glGetMultisamplefv

*/







/*
Buffer usage:

Vertex Buffer
Index Buffer
Uniform Buffer
Texture Buffer
Shader Storage Buffer
Dispatch Indirect Buffer
Draw Indirect Buffer
Atomic Counter Buffer
Query Buffer
Transform Feedback Buffer
*/

/*
Есть ли аналоги этих ф-ий у DX?
glDrawRangeElements // 2.0
glDrawRangeElementsBaseVertex // 3.2
*/

#if 0
static const char * GetFramebufferStatusString( unsigned int _Status ) {
    switch ( _Status ) {
    case GL_FRAMEBUFFER_COMPLETE:
        return "complete";
    case GL_FRAMEBUFFER_UNSUPPORTED:
        return "unsupported";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        return "incomplete attachment";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        return "incomplete missing attachment";
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        return "incomplete draw buffer";
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        return "incomplete read buffer";
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        return "incomplete multisample";
    };
    return "Unknown status";
}
#endif


/*

glEnable[i],glDisable[i]

GL_CLIP_DISTANCE i
If enabled, clip geometry against user-defined half space i.

GL_DEBUG_OUTPUT
If enabled, debug messages are produced by a debug context. When disabled, the debug message log is silenced. Note that in a non-debug context, very few, if any messages might be produced, even when GL_DEBUG_OUTPUT is enabled.

GL_DEBUG_OUTPUT_SYNCHRONOUS
If enabled, debug messages are produced synchronously by a debug context. If disabled, debug messages may be produced asynchronously. In particular, they may be delayed relative to the execution of GL commands, and the debug callback function may be called from a thread other than that in which the commands are executed. See glDebugMessageCallback.

GL_DITHER
If enabled, dither color components or indices before they are written to the color buffer.

GL_PRIMITIVE_RESTART
Enables primitive restarting. If enabled, any one of the draw commands which transfers a set of generic attribute array elements to the GL will restart the primitive when the index of the vertex is equal to the primitive restart index. See glPrimitiveRestartIndex.

GL_SAMPLE_ALPHA_TO_ONE
If enabled, each sample alpha value is replaced by the maximum representable alpha value.

GL_SAMPLE_COVERAGE
If enabled, the fragment's coverage is ANDed with the temporary coverage value. If GL_SAMPLE_COVERAGE_INVERT is set to GL_TRUE, invert the coverage value. See glSampleCoverage.

GL_SAMPLE_SHADING
If enabled, the active fragment shader is run once for each covered sample, or at fraction of this rate as determined by the current value of GL_MIN_SAMPLE_SHADING_VALUE. See glMinSampleShading.

GL_PROGRAM_POINT_SIZE
If enabled and a vertex or geometry shader is active, then the derived point size is taken from the (potentially clipped) shader builtin gl_PointSize and clamped to the implementation-dependent point size range.

*/

/*
glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
glDepthFunc(GL_GREATER);
glClearDepth(0.0);

//Reversed Infinite Projection Matrix
const float zNear = 0.001f;
const double viewAngleVertical = 90.0f;
const float f = 1.0 / tan(viewAngleVertical / 2.0); // 1.0 / tan == cotangent
const float aspect = float(resolutionX) / float(resolutionY);
mat4 projectionMatrix =
{
f/aspect, 0.0f, 0.0f, 0.0f,
0.0f, f, 0.0f, 0.0f,
0.0f, 0.0f, 0.0f, -1.0f,
0.0f, 0.0f, 2*zNear, 0.0f
};
*/


// glValidateProgram
// glGetProgramPipelineInfoLog
// glGetProgramPipelineiv
