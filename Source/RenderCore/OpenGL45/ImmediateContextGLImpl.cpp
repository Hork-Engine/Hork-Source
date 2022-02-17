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

#include "ImmediateContextGLImpl.h"
#include "DeviceGLImpl.h"
#include "BufferGLImpl.h"
#include "TextureGLImpl.h"
#include "PipelineGLImpl.h"
#include "TransformFeedbackGLImpl.h"
#include "QueryGLImpl.h"
#include "LUT.h"
#include "GenericWindowGLImpl.h"

#include "../FrameGraph.h"

#include <Platform/Platform.h>
#include <RenderCore/GenericWindow.h>

#include "GL/glew.h"

#include <SDL.h>

#define DEFAULT_STENCIL_REF 0

#define VerifyContext() HK_ASSERT(AImmediateContextGLImpl::Current == this);

namespace RenderCore
{

AImmediateContextGLImpl* AImmediateContextGLImpl::Current = nullptr;

AResourceTableGLImpl::AResourceTableGLImpl(ADeviceGLImpl* pDevice, bool bIsRoot) :
    IResourceTable(pDevice, bIsRoot)
{
    Platform::ZeroMem(TextureBindings, sizeof(TextureBindings));
    Platform::ZeroMem(TextureBindingUIDs, sizeof(TextureBindingUIDs));
    Platform::ZeroMem(ImageBindings, sizeof(ImageBindings));
    Platform::ZeroMem(ImageBindingUIDs, sizeof(ImageBindingUIDs));
    Platform::ZeroMem(ImageMipLevel, sizeof(ImageMipLevel));
    Platform::ZeroMem(ImageLayerIndex, sizeof(ImageLayerIndex));
    Platform::ZeroMem(ImageLayered, sizeof(ImageLayered));
    Platform::ZeroMem(BufferBindings, sizeof(BufferBindings));
    Platform::ZeroMem(BufferBindingUIDs, sizeof(BufferBindingUIDs));
    Platform::ZeroMem(BufferBindingOffsets, sizeof(BufferBindingOffsets));
    Platform::ZeroMem(BufferBindingSizes, sizeof(BufferBindingSizes));
}

AResourceTableGLImpl::~AResourceTableGLImpl()
{
}

void AResourceTableGLImpl::BindTexture(unsigned int Slot, ITextureView* pShaderResourceView)
{
    HK_ASSERT(Slot < MAX_SAMPLER_SLOTS);

    // Slot must be < pDevice->MaxCombinedTextureImageUnits

    if (pShaderResourceView)
    {
        HK_ASSERT(pShaderResourceView->GetDesc().ViewType == TEXTURE_VIEW_SHADER_RESOURCE);
        TextureBindings[Slot]    = pShaderResourceView->GetHandleNativeGL();
        TextureBindingUIDs[Slot] = pShaderResourceView->GetUID();
    }
    else
    {
        TextureBindings[Slot]    = 0;
        TextureBindingUIDs[Slot] = 0;
    }
}

void AResourceTableGLImpl::BindTexture(unsigned int Slot, IBufferView* pShaderResourceView)
{
    HK_ASSERT(Slot < MAX_SAMPLER_SLOTS);

    // Slot must be < pDevice->MaxCombinedTextureImageUnits

    if (pShaderResourceView)
    {
        TextureBindings[Slot]    = pShaderResourceView->GetHandleNativeGL();
        TextureBindingUIDs[Slot] = pShaderResourceView->GetUID();
    }
    else
    {
        TextureBindings[Slot]    = 0;
        TextureBindingUIDs[Slot] = 0;
    }
}

 void AResourceTableGLImpl::BindImage(unsigned int Slot, ITextureView* pUnorderedAccessView)
{
    HK_ASSERT(Slot < MAX_IMAGE_SLOTS);

    // _Slot must be < pDevice->MaxCombinedTextureImageUnits

    if (pUnorderedAccessView)
    {
        STextureViewDesc const& desc = pUnorderedAccessView->GetDesc();

        HK_ASSERT(desc.ViewType == TEXTURE_VIEW_UNORDERED_ACCESS);

        bool bLayered = desc.FirstSlice != 0 || desc.NumSlices != pUnorderedAccessView->GetTexture()->GetSliceCount(desc.FirstMipLevel);

        ImageBindings[Slot]    = pUnorderedAccessView->GetHandleNativeGL();
        ImageBindingUIDs[Slot] = pUnorderedAccessView->GetUID();
        ImageMipLevel[Slot]    = desc.FirstMipLevel;
        ImageLayerIndex[Slot]  = desc.FirstSlice;
        ImageLayered[Slot]     = bLayered;
    }
    else
    {
        ImageBindings[Slot]    = 0;
        ImageBindingUIDs[Slot] = 0;
        ImageMipLevel[Slot]    = 0;
        ImageLayerIndex[Slot]  = 0;
        ImageLayered[Slot]     = false;
    }
}

void AResourceTableGLImpl::BindBuffer(int Slot, IBuffer const* pBuffer, size_t Offset, size_t Size)
{
    HK_ASSERT(Slot < MAX_BUFFER_SLOTS);

    if (pBuffer)
    {
        BufferBindings[Slot]       = pBuffer->GetHandleNativeGL();
        BufferBindingUIDs[Slot]    = pBuffer->GetUID();
        BufferBindingOffsets[Slot] = Offset;
        BufferBindingSizes[Slot]   = Size;
    }
    else
    {
        BufferBindings[Slot]       = 0;
        BufferBindingUIDs[Slot]    = 0;
        BufferBindingOffsets[Slot] = 0;
        BufferBindingSizes[Slot]   = 0;
    }
}



void AFramebufferCacheGL::CleanupOutdatedFramebuffers()
{
    // Remove outdated framebuffers
    for (int i = 0; i < FramebufferCache.Size();)
    {
        if (FramebufferCache[i]->IsAttachmentsOutdated())
        {
            FramebufferHash.RemoveIndex(FramebufferCache[i]->GetHash(), i);
            FramebufferCache.erase(FramebufferCache.begin() + i);
            continue;
        }
        i++;
    }
}

//static int MakeHash(TArray<ITextureView*, MAX_COLOR_ATTACHMENTS> const& ColorAttachments,
//                    int                                                 NumColorAttachments,
//                    ITextureView*                                       pDepthStencilAttachment)
//{
//    uint32_t hash{0};
//    for (int a = 0 ; a < NumColorAttachments ; a++)
//    {
//        hash = Core::MurMur3Hash32(ColorAttachments[a]->GetUID(), hash);
//    }
//    if (pDepthStencilAttachment)
//    {
//        hash = Core::MurMur3Hash32(pDepthStencilAttachment->GetUID(), hash);
//    }
//    return hash;
//}

AFramebufferGL* AFramebufferCacheGL::GetFramebuffer(const char*                     RenderPassName,
                                                    TStdVector<STextureAttachment>& ColorAttachments,
                                                    STextureAttachment*             pDepthStencilAttachment)
{
    SFramebufferDescGL                           framebufferDesc;
    TArray<ITextureView*, MAX_COLOR_ATTACHMENTS> colorAttachments;
 
    HK_ASSERT(ColorAttachments.Size() <= MAX_COLOR_ATTACHMENTS);

    framebufferDesc.NumColorAttachments = ColorAttachments.Size();
    framebufferDesc.pColorAttachments   = colorAttachments.ToPtr();

    STextureViewDesc viewDesc;
    viewDesc.ViewType      = TEXTURE_VIEW_RENDER_TARGET;
    viewDesc.Type          = TEXTURE_2D;
    viewDesc.Format        = TEXTURE_FORMAT_RGBA8;
    viewDesc.FirstMipLevel = 0;
    viewDesc.NumMipLevels  = 1;
    viewDesc.FirstSlice    = 0;
    viewDesc.NumSlices     = 0;

    uint32_t hash{0};

    int rt = 0;
    for (STextureAttachment& attachment : ColorAttachments)
    {
        ITexture* texture = attachment.GetTexture();

        viewDesc.Type          = texture->GetDesc().Type;
        viewDesc.Format        = texture->GetDesc().Format;
        viewDesc.FirstMipLevel = attachment.MipLevel;

        if (attachment.bSingleSlice)
        {
            viewDesc.FirstSlice = attachment.SliceNum;
            viewDesc.NumSlices = 1;
        }
        else
        {
            viewDesc.FirstSlice = 0;
            viewDesc.NumSlices  = texture->GetSliceCount(attachment.MipLevel);
        }

        ITextureView* textureView = texture->GetTextureView(viewDesc);

        if (rt == 0)
        {
            framebufferDesc.Width = textureView->GetWidth();
            framebufferDesc.Height = textureView->GetHeight();
        }
        else
        {
            HK_ASSERT(framebufferDesc.Width == textureView->GetWidth());
            HK_ASSERT(framebufferDesc.Height == textureView->GetHeight());
        }

        colorAttachments[rt++] = textureView;

        hash = Core::MurMur3Hash32(textureView->GetUID(), hash);
    }

    if (pDepthStencilAttachment)
    {
        ITexture* texture = pDepthStencilAttachment->GetTexture();

        viewDesc.ViewType      = TEXTURE_VIEW_DEPTH_STENCIL;
        viewDesc.Type          = texture->GetDesc().Type;
        viewDesc.Format        = texture->GetDesc().Format;
        viewDesc.FirstMipLevel = pDepthStencilAttachment->MipLevel;

        if (pDepthStencilAttachment->bSingleSlice)
        {
            viewDesc.FirstSlice = pDepthStencilAttachment->SliceNum;
            viewDesc.NumSlices  = 1;
        }
        else
        {
            viewDesc.FirstSlice = 0;
            viewDesc.NumSlices  = texture->GetSliceCount(pDepthStencilAttachment->MipLevel);
        }

        ITextureView* textureView = texture->GetTextureView(viewDesc);

        if (rt == 0)
        {
            framebufferDesc.Width  = textureView->GetWidth();
            framebufferDesc.Height = textureView->GetHeight();
        }
        else
        {
            HK_ASSERT(framebufferDesc.Width == textureView->GetWidth());
            HK_ASSERT(framebufferDesc.Height == textureView->GetHeight());
        }

        framebufferDesc.pDepthStencilAttachment = textureView;

        hash = Core::MurMur3Hash32(textureView->GetUID(), hash);
    }

    //int hash = MakeHash(ColorAttachments, pDepthStencilAttachment);

    for (int i = FramebufferHash.First(hash); i != -1; i = FramebufferHash.Next(i))
    {
        AFramebufferGL* framebuffer = FramebufferCache[i].get();

        if (framebuffer->CompareWith(framebufferDesc))
        {
            return framebuffer;
        }
    }

    // create new framebuffer

    std::unique_ptr<AFramebufferGL> framebuffer = std::make_unique<AFramebufferGL>(framebufferDesc, hash);

    FramebufferHash.Insert(framebuffer->GetHash(), FramebufferCache.Size());
    FramebufferCache.emplace_back(std::move(framebuffer));

    //LOG( "Total framebuffers {} for {} hash {:08x}\n", FramebufferCache.Size(), RenderPassName, hash );

    return FramebufferCache.back().get();
}

AImmediateContextGLImpl::AImmediateContextGLImpl(ADeviceGLImpl* pDevice, AWindowPoolGL::SWindowGL Window, bool bMainContext) :
    IImmediateContext(pDevice),
    Window(Window),
    pContextGL(Window.GLContext),
    bMainContext(bMainContext)
{
    SScopedContextGL scopedContext(this);

    Platform::ZeroMem(BufferBindingUIDs, sizeof(BufferBindingUIDs));
    Platform::ZeroMem(BufferBindingOffsets, sizeof(BufferBindingOffsets));
    Platform::ZeroMem(BufferBindingSizes, sizeof(BufferBindingSizes));

    CurrentPipeline       = nullptr;
    CurrentVertexLayout   = nullptr;
    CurrentVAO            = nullptr;
    NumPatchVertices      = 0;
    IndexBufferType       = 0;
    IndexBufferTypeSizeOf = 0;
    IndexBufferOffset     = 0;
    IndexBufferUID        = 0;
    IndexBufferHandle     = 0;
    Platform::ZeroMem(VertexBufferUIDs, sizeof(VertexBufferUIDs));
    Platform::ZeroMem(VertexBufferHandles, sizeof(VertexBufferHandles));
    Platform::ZeroMem(VertexBufferOffsets, sizeof(VertexBufferOffsets));

    //CurrentQueryTarget = 0;
    //CurrentQueryObject = 0;
    Platform::ZeroMem(CurrentQueryUID, sizeof(CurrentQueryUID));

    // GL_NICEST, GL_FASTEST and GL_DONT_CARE

    // Sampling quality of antialiased lines during rasterization stage
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    // Sampling quality of antialiased polygons during rasterization stage
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    // Quality and performance of the compressing texture images
    glHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);

    // Accuracy of the derivative calculation for the GLSL fragment processing built-in functions: dFdx, dFdy, and fwidth.
    glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_NICEST);

    // If enabled, cubemap textures are sampled such that when linearly sampling from the border between two adjacent faces,
    // texels from both faces are used to generate the final sample value.
    // When disabled, texels from only a single face are used to construct the final sample value.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    PixelStore.PackAlignment = 4;
    glPixelStorei(GL_PACK_ALIGNMENT, PixelStore.PackAlignment);
    PixelStore.UnpackAlignment = 4;
    glPixelStorei(GL_UNPACK_ALIGNMENT, PixelStore.UnpackAlignment);

    Platform::ZeroMem(&Binding, sizeof(Binding));

    // Init default blending state
    bLogicOpEnabled = false;
    glColorMask(1, 1, 1, 1);
    glDisable(GL_BLEND);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glBlendFunc(GL_ONE, GL_ZERO);
    glBlendEquation(GL_FUNC_ADD);
    glBlendColor(0, 0, 0, 0);
    glDisable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_COPY);
    Platform::ZeroMem(BlendColor, sizeof(BlendColor));

    GLint maxSampleMaskWords = 0;
    glGetIntegerv(GL_MAX_SAMPLE_MASK_WORDS, &maxSampleMaskWords);
    if (maxSampleMaskWords > 4)
    {
        maxSampleMaskWords = 4;
    }
    SampleMask[0] = 0xffffffff;
    SampleMask[1] = 0;
    SampleMask[2] = 0;
    SampleMask[3] = 0;
    for (int i = 0; i < maxSampleMaskWords; i++)
    {
        glSampleMaski(i, SampleMask[i]);
    }
    bSampleMaskEnabled = false;
    glDisable(GL_SAMPLE_MASK);

    // Init default rasterizer state
    glDisable(GL_POLYGON_OFFSET_FILL);
    PolygonOffsetClampSafe(0, 0, 0);
    glDisable(GL_DEPTH_CLAMP);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_RASTERIZER_DISCARD);
    glDisable(GL_MULTISAMPLE);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_CULL_FACE);
    CullFace = GL_BACK;
    glCullFace(CullFace);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glFrontFace(GL_CCW);
    // GL_POLYGON_SMOOTH
    // If enabled, draw polygons with proper filtering. Otherwise, draw aliased polygons.
    // For correct antialiased polygons, an alpha buffer is needed and the polygons must be sorted front to back.
    glDisable(GL_POLYGON_SMOOTH); // Smooth polygons have some artifacts
    bPolygonOffsetEnabled = false;

    // Init default depth-stencil state
    StencilRef = DEFAULT_STENCIL_REF;
    glEnable(GL_DEPTH_TEST);
    glDepthMask(1);
    glDepthFunc(GL_LESS);
    glDisable(GL_STENCIL_TEST);
    glStencilMask(DEFAULT_STENCIL_WRITE_MASK);
    glStencilOpSeparate(GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_ALWAYS, StencilRef, DEFAULT_STENCIL_READ_MASK);

    // Enable clip distances by default
    // FIXME: make it as pipeline state?
    glEnable(GL_CLIP_DISTANCE0);
    glEnable(GL_CLIP_DISTANCE1);
    glEnable(GL_CLIP_DISTANCE2);
    glEnable(GL_CLIP_DISTANCE3);
    glEnable(GL_CLIP_DISTANCE4);
    glEnable(GL_CLIP_DISTANCE5);

    ColorClamp = COLOR_CLAMP_OFF;
    glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);

    glEnable(GL_FRAMEBUFFER_SRGB);

    // GL_PRIMITIVE_RESTART_FIXED_INDEX if from GL_ARB_ES3_compatibility
    // Enables primitive restarting with a fixed index.
    // If enabled, any one of the draw commands which transfers a set of generic attribute array elements
    // to the GL will restart the primitive when the index of the vertex is equal to the fixed primitive index
    // for the specified index type.
    // The fixed index is equal to 2n−1 where n is equal to 8 for GL_UNSIGNED_BYTE,
    // 16 for GL_UNSIGNED_SHORT and 32 for GL_UNSIGNED_INT.
    glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

    CurrentRenderPass  = nullptr;
    CurrentFramebuffer = nullptr;

    CurrentViewport[0] = Math::MaxValue<float>();
    CurrentViewport[1] = Math::MaxValue<float>();
    CurrentViewport[2] = Math::MaxValue<float>();
    CurrentViewport[3] = Math::MaxValue<float>();

    CurrentDepthRange[0] = 0;
    CurrentDepthRange[1] = 1;
    glDepthRangef(CurrentDepthRange[0], CurrentDepthRange[1]); // Since GL v4.1

    CurrentScissor.X      = 0;
    CurrentScissor.Y      = 0;
    CurrentScissor.Width  = 0;
    CurrentScissor.Height = 0;

    // OpenGL Classic ndc_z -1,-1, lower-left corner
    //glClipControl( GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE ); // Zw = ( ( f - n ) / 2 ) * Zd + ( n + f ) / 2

    // DirectX ndc_z 0,1, upper-left corner
    glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE); // Zw = ( f - n ) * Zd + n

    Binding.ReadFramebuffer = ~0u;
    Binding.DrawFramebuffer = ~0u;

    RootResourceTable = MakeRef<AResourceTableGLImpl>(pDevice, true);
    RootResourceTable->SetDebugName("Root");

    CurrentResourceTable = static_cast<AResourceTableGLImpl*>(RootResourceTable.GetObject());

    pFramebufferCache = MakeRef<AFramebufferCacheGL>();
}

void AImmediateContextGLImpl::MakeCurrent(AImmediateContextGLImpl* pContext)
{
    if (pContext)
    {
        SDL_GL_MakeCurrent(pContext->Window.Handle, pContext->Window.GLContext);
    }
    else
    {
        SDL_GL_MakeCurrent(nullptr, nullptr);
    }
    Current = pContext;
}

AImmediateContextGLImpl::~AImmediateContextGLImpl()
{
    {
        SScopedContextGL scopedContext(this);

        pFramebufferCache.Reset();
        CurrentResourceTable.Reset();
        RootResourceTable.Reset();

        glBindVertexArray(0);

        for (AVertexLayoutGL* pVertexLayout : static_cast<ADeviceGLImpl*>(GetDevice())->GetVertexLayouts())
        {
            pVertexLayout->DestroyVAO(this);
        }

        for (auto& pair : ProgramPipelines)
        {
            GLuint pipelineId = pair.second;
            glDeleteProgramPipelines(1, &pipelineId);
        }
    }

    if (Current == this)
    {
        MakeCurrent(nullptr);
    }
}

void AImmediateContextGLImpl::PolygonOffsetClampSafe(float _Slope, int _Bias, float _Clamp)
{
    VerifyContext();

    const float DepthBiasTolerance = 0.00001f;

    if (std::fabs(_Slope) < DepthBiasTolerance && std::fabs(_Clamp) < DepthBiasTolerance && _Bias == 0)
    {

        // FIXME: GL_POLYGON_OFFSET_LINE,GL_POLYGON_OFFSET_POINT тоже enable/disable?

        if (bPolygonOffsetEnabled)
        {
            glDisable(GL_POLYGON_OFFSET_FILL);
            bPolygonOffsetEnabled = false;
        }
    }
    else
    {
        if (!bPolygonOffsetEnabled)
        {
            glEnable(GL_POLYGON_OFFSET_FILL);
            bPolygonOffsetEnabled = true;
        }
    }

    if (glPolygonOffsetClampEXT)
    {
        glPolygonOffsetClampEXT(_Slope, static_cast<float>(_Bias), _Clamp);
    }
    else
    {
        glPolygonOffset(_Slope, static_cast<float>(_Bias));
    }
}

void AImmediateContextGLImpl::PackAlignment(unsigned int _Alignment)
{
    VerifyContext();

    if (PixelStore.PackAlignment != _Alignment)
    {
        glPixelStorei(GL_PACK_ALIGNMENT, _Alignment);
        PixelStore.PackAlignment = _Alignment;
    }
}

void AImmediateContextGLImpl::UnpackAlignment(unsigned int _Alignment)
{
    VerifyContext();

    if (PixelStore.UnpackAlignment != _Alignment)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, _Alignment);
        PixelStore.UnpackAlignment = _Alignment;
    }
}

void AImmediateContextGLImpl::ClampReadColor(COLOR_CLAMP _ColorClamp)
{
    VerifyContext();

    if (ColorClamp != _ColorClamp)
    {
        glClampColor(GL_CLAMP_READ_COLOR, ColorClampLUT[_ColorClamp]);
        ColorClamp = _ColorClamp;
    }
}

static GLenum Attachments[MAX_COLOR_ATTACHMENTS];

static bool BlendCompareEquation(SRenderTargetBlendingInfo::Operation const& _Mode1,
                                 SRenderTargetBlendingInfo::Operation const& _Mode2)
{
    return _Mode1.ColorRGB == _Mode2.ColorRGB && _Mode1.Alpha == _Mode2.Alpha;
}

static bool BlendCompareFunction(SRenderTargetBlendingInfo::Function const& _Func1,
                                 SRenderTargetBlendingInfo::Function const& _Func2)
{
    return _Func1.SrcFactorRGB == _Func2.SrcFactorRGB && _Func1.DstFactorRGB == _Func2.DstFactorRGB && _Func1.SrcFactorAlpha == _Func2.SrcFactorAlpha && _Func1.DstFactorAlpha == _Func2.DstFactorAlpha;
}

static bool BlendCompareColor(const float _Color1[4], const float _Color2[4])
{
    return _Color1[0] == _Color2[0] && _Color1[1] == _Color2[1] && _Color1[2] == _Color2[2] && _Color1[3] == _Color2[3];
}

// Compare render target blending at specified slot and change if different
static void SetRenderTargetSlotBlending(int                              _Slot,
                                        SRenderTargetBlendingInfo const& _CurrentState,
                                        SRenderTargetBlendingInfo const& _RequiredState)
{

    bool isEquationChanged = !BlendCompareEquation(_RequiredState.Op, _CurrentState.Op);
    bool isFunctionChanged = !BlendCompareFunction(_RequiredState.Func, _CurrentState.Func);

    // Change only modified blending states

    if (_CurrentState.bBlendEnable != _RequiredState.bBlendEnable)
    {
        if (_RequiredState.bBlendEnable)
        {
            glEnablei(GL_BLEND, _Slot);
        }
        else
        {
            glDisablei(GL_BLEND, _Slot);
        }
    }

    if (_CurrentState.ColorWriteMask != _RequiredState.ColorWriteMask)
    {
        if (_RequiredState.ColorWriteMask == COLOR_WRITE_RGBA)
        {
            glColorMaski(_Slot, 1, 1, 1, 1);
        }
        else if (_RequiredState.ColorWriteMask == COLOR_WRITE_DISABLED)
        {
            glColorMaski(_Slot, 0, 0, 0, 0);
        }
        else
        {
            glColorMaski(_Slot,
                         !!(_RequiredState.ColorWriteMask & COLOR_WRITE_R_BIT),
                         !!(_RequiredState.ColorWriteMask & COLOR_WRITE_G_BIT),
                         !!(_RequiredState.ColorWriteMask & COLOR_WRITE_B_BIT),
                         !!(_RequiredState.ColorWriteMask & COLOR_WRITE_A_BIT));
        }
    }

    if (isEquationChanged)
    {

        bool equationSeparate = _RequiredState.Op.ColorRGB != _RequiredState.Op.Alpha;

        if (equationSeparate)
        {
            glBlendEquationSeparatei(_Slot,
                                     BlendEquationConvertionLUT[_RequiredState.Op.ColorRGB],
                                     BlendEquationConvertionLUT[_RequiredState.Op.Alpha]);
        }
        else
        {
            glBlendEquationi(_Slot,
                             BlendEquationConvertionLUT[_RequiredState.Op.ColorRGB]);
        }
    }

    if (isFunctionChanged)
    {

        bool funcSeparate = _RequiredState.Func.SrcFactorRGB != _RequiredState.Func.SrcFactorAlpha || _RequiredState.Func.DstFactorRGB != _RequiredState.Func.DstFactorAlpha;

        if (funcSeparate)
        {
            glBlendFuncSeparatei(_Slot,
                                 BlendFuncConvertionLUT[_RequiredState.Func.SrcFactorRGB],
                                 BlendFuncConvertionLUT[_RequiredState.Func.DstFactorRGB],
                                 BlendFuncConvertionLUT[_RequiredState.Func.SrcFactorAlpha],
                                 BlendFuncConvertionLUT[_RequiredState.Func.DstFactorAlpha]);
        }
        else
        {
            glBlendFunci(_Slot,
                         BlendFuncConvertionLUT[_RequiredState.Func.SrcFactorRGB],
                         BlendFuncConvertionLUT[_RequiredState.Func.DstFactorRGB]);
        }
    }
}

// Compare render target blending and change all slots if different
static void SetRenderTargetSlotsBlending(SRenderTargetBlendingInfo const& _CurrentState,
                                         SRenderTargetBlendingInfo const& _RequiredState,
                                         bool                             _NeedReset)
{
    bool isEquationChanged = _NeedReset || !BlendCompareEquation(_RequiredState.Op, _CurrentState.Op);
    bool isFunctionChanged = _NeedReset || !BlendCompareFunction(_RequiredState.Func, _CurrentState.Func);

    // Change only modified blending states

    if (_NeedReset || _CurrentState.bBlendEnable != _RequiredState.bBlendEnable)
    {
        if (_RequiredState.bBlendEnable)
        {
            glEnable(GL_BLEND);
        }
        else
        {
            glDisable(GL_BLEND);
        }
    }

    if (_NeedReset || _CurrentState.ColorWriteMask != _RequiredState.ColorWriteMask)
    {
        if (_RequiredState.ColorWriteMask == COLOR_WRITE_RGBA)
        {
            glColorMask(1, 1, 1, 1);
        }
        else if (_RequiredState.ColorWriteMask == COLOR_WRITE_DISABLED)
        {
            glColorMask(0, 0, 0, 0);
        }
        else
        {
            glColorMask(!!(_RequiredState.ColorWriteMask & COLOR_WRITE_R_BIT),
                        !!(_RequiredState.ColorWriteMask & COLOR_WRITE_G_BIT),
                        !!(_RequiredState.ColorWriteMask & COLOR_WRITE_B_BIT),
                        !!(_RequiredState.ColorWriteMask & COLOR_WRITE_A_BIT));
        }
    }

    if (isEquationChanged)
    {

        bool equationSeparate = _RequiredState.Op.ColorRGB != _RequiredState.Op.Alpha;

        if (equationSeparate)
        {
            glBlendEquationSeparate(BlendEquationConvertionLUT[_RequiredState.Op.ColorRGB],
                                    BlendEquationConvertionLUT[_RequiredState.Op.Alpha]);
        }
        else
        {
            glBlendEquation(BlendEquationConvertionLUT[_RequiredState.Op.ColorRGB]);
        }
    }

    if (isFunctionChanged)
    {

        bool funcSeparate = _RequiredState.Func.SrcFactorRGB != _RequiredState.Func.SrcFactorAlpha || _RequiredState.Func.DstFactorRGB != _RequiredState.Func.DstFactorAlpha;

        if (funcSeparate)
        {
            glBlendFuncSeparate(BlendFuncConvertionLUT[_RequiredState.Func.SrcFactorRGB],
                                BlendFuncConvertionLUT[_RequiredState.Func.DstFactorRGB],
                                BlendFuncConvertionLUT[_RequiredState.Func.SrcFactorAlpha],
                                BlendFuncConvertionLUT[_RequiredState.Func.DstFactorAlpha]);
        }
        else
        {
            glBlendFunc(BlendFuncConvertionLUT[_RequiredState.Func.SrcFactorRGB],
                        BlendFuncConvertionLUT[_RequiredState.Func.DstFactorRGB]);
        }
    }
}

void AImmediateContextGLImpl::BindPipeline(IPipeline* _Pipeline)
{
    VerifyContext();

    HK_ASSERT(_Pipeline != nullptr);

    if (CurrentPipeline == _Pipeline)
    {
        return;
    }

    CurrentPipeline = static_cast<APipelineGLImpl*>(_Pipeline);

    GLuint pipelineId = GetProgramPipeline(CurrentPipeline); //CurrentPipeline->GetHandleNativeGL();

    glBindProgramPipeline(pipelineId);

    if (CurrentPipeline->pVertexLayout != CurrentVertexLayout)
    {
        CurrentVertexLayout = CurrentPipeline->pVertexLayout;
        CurrentVAO = CurrentVertexLayout->GetVAO(this);
        glBindVertexArray(CurrentVAO->HandleGL);
        //LOG( "Binding vao {}\n", CurrentVAO->HandleGL );
    }
    else
    {
        //LOG( "caching vao binding {}\n", CurrentVAO->HandleGL );
    }

    //
    // Set input assembly
    //

    if (CurrentPipeline->PrimitiveTopology == GL_PATCHES)
    {
        if (NumPatchVertices != CurrentPipeline->NumPatchVertices)
        {
            glPatchParameteri(GL_PATCH_VERTICES, CurrentPipeline->NumPatchVertices); // Sinse GL v4.0
            NumPatchVertices = (uint8_t)CurrentPipeline->NumPatchVertices;
        }
    }

    //
    // Set blending state
    //

    // Compare blending states
    if (Binding.BlendState != CurrentPipeline->BlendingState)
    {
        SBlendingStateInfo const& desc = *CurrentPipeline->BlendingState;

        if (desc.bIndependentBlendEnable)
        {
            for (int i = 0; i < MAX_COLOR_ATTACHMENTS; i++)
            {
                SRenderTargetBlendingInfo const& rtDesc = desc.RenderTargetSlots[i];
                SetRenderTargetSlotBlending(i, BlendState.RenderTargetSlots[i], rtDesc);
                Platform::Memcpy(&BlendState.RenderTargetSlots[i], &rtDesc, sizeof(rtDesc));
            }
        }
        else
        {
            SRenderTargetBlendingInfo const& rtDesc    = desc.RenderTargetSlots[0];
            bool                             needReset = BlendState.bIndependentBlendEnable;
            SetRenderTargetSlotsBlending(BlendState.RenderTargetSlots[0], rtDesc, needReset);
            for (int i = 0; i < MAX_COLOR_ATTACHMENTS; i++)
            {
                Platform::Memcpy(&BlendState.RenderTargetSlots[i], &rtDesc, sizeof(rtDesc));
            }
        }

        BlendState.bIndependentBlendEnable = desc.bIndependentBlendEnable;

        if (BlendState.bSampleAlphaToCoverage != desc.bSampleAlphaToCoverage)
        {
            if (desc.bSampleAlphaToCoverage)
            {
                glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
            }
            else
            {
                glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
            }

            BlendState.bSampleAlphaToCoverage = desc.bSampleAlphaToCoverage;
        }

        if (BlendState.LogicOp != desc.LogicOp)
        {
            if (desc.LogicOp == LOGIC_OP_COPY)
            {
                if (bLogicOpEnabled)
                {
                    glDisable(GL_COLOR_LOGIC_OP);
                    bLogicOpEnabled = false;
                }
            }
            else
            {
                if (!bLogicOpEnabled)
                {
                    glEnable(GL_COLOR_LOGIC_OP);
                    bLogicOpEnabled = true;
                }
                glLogicOp(LogicOpLUT[desc.LogicOp]);
            }

            BlendState.LogicOp = desc.LogicOp;
        }

        Binding.BlendState = CurrentPipeline->BlendingState;
    }

    //
    // Set rasterizer state
    //

    if (Binding.RasterizerState != CurrentPipeline->RasterizerState)
    {
        SRasterizerStateInfo const& desc = *CurrentPipeline->RasterizerState;

        if (RasterizerState.FillMode != desc.FillMode)
        {
            glPolygonMode(GL_FRONT_AND_BACK, FillModeLUT[desc.FillMode]);
            RasterizerState.FillMode = desc.FillMode;
        }

        if (RasterizerState.CullMode != desc.CullMode)
        {
            if (desc.CullMode == POLYGON_CULL_DISABLED)
            {
                glDisable(GL_CULL_FACE);
            }
            else
            {
                if (RasterizerState.CullMode == POLYGON_CULL_DISABLED)
                {
                    glEnable(GL_CULL_FACE);
                }
                if (CullFace != CullModeLUT[desc.CullMode])
                {
                    CullFace = CullModeLUT[desc.CullMode];
                    glCullFace(CullFace);
                }
            }
            RasterizerState.CullMode = desc.CullMode;
        }

        if (RasterizerState.bScissorEnable != desc.bScissorEnable)
        {
            if (desc.bScissorEnable)
            {
                glEnable(GL_SCISSOR_TEST);
            }
            else
            {
                glDisable(GL_SCISSOR_TEST);
            }
            RasterizerState.bScissorEnable = desc.bScissorEnable;
        }

        if (RasterizerState.bMultisampleEnable != desc.bMultisampleEnable)
        {
            if (desc.bMultisampleEnable)
            {
                glEnable(GL_MULTISAMPLE);
            }
            else
            {
                glDisable(GL_MULTISAMPLE);
            }
            RasterizerState.bMultisampleEnable = desc.bMultisampleEnable;
        }

        if (RasterizerState.bRasterizerDiscard != desc.bRasterizerDiscard)
        {
            if (desc.bRasterizerDiscard)
            {
                glEnable(GL_RASTERIZER_DISCARD);
            }
            else
            {
                glDisable(GL_RASTERIZER_DISCARD);
            }
            RasterizerState.bRasterizerDiscard = desc.bRasterizerDiscard;
        }

        if (RasterizerState.bAntialiasedLineEnable != desc.bAntialiasedLineEnable)
        {
            if (desc.bAntialiasedLineEnable)
            {
                glEnable(GL_LINE_SMOOTH);
            }
            else
            {
                glDisable(GL_LINE_SMOOTH);
            }
            RasterizerState.bAntialiasedLineEnable = desc.bAntialiasedLineEnable;
        }

        if (RasterizerState.bDepthClampEnable != desc.bDepthClampEnable)
        {
            if (desc.bDepthClampEnable)
            {
                glEnable(GL_DEPTH_CLAMP);
            }
            else
            {
                glDisable(GL_DEPTH_CLAMP);
            }
            RasterizerState.bDepthClampEnable = desc.bDepthClampEnable;
        }

        if (RasterizerState.DepthOffset.Slope != desc.DepthOffset.Slope || RasterizerState.DepthOffset.Bias != desc.DepthOffset.Bias || RasterizerState.DepthOffset.Clamp != desc.DepthOffset.Clamp)
        {

            PolygonOffsetClampSafe(desc.DepthOffset.Slope, desc.DepthOffset.Bias, desc.DepthOffset.Clamp);

            RasterizerState.DepthOffset.Slope = desc.DepthOffset.Slope;
            RasterizerState.DepthOffset.Bias  = desc.DepthOffset.Bias;
            RasterizerState.DepthOffset.Clamp = desc.DepthOffset.Clamp;
        }

        if (RasterizerState.bFrontClockwise != desc.bFrontClockwise)
        {
            glFrontFace(desc.bFrontClockwise ? GL_CW : GL_CCW);
            RasterizerState.bFrontClockwise = desc.bFrontClockwise;
        }

        Binding.RasterizerState = CurrentPipeline->RasterizerState;
    }

    //
    // Set depth stencil state
    //
#if 0
    if ( Binding.DepthStencilState == CurrentPipeline->DepthStencilState ) {

        if ( StencilRef != _StencilRef ) {
            // Update stencil ref

            DepthStencilStateInfo const & desc = CurrentPipeline->DepthStencilState;

            if ( desc.FrontFace.StencilFunc == desc.BackFace.StencilFunc ) {
                glStencilFuncSeparate( GL_FRONT_AND_BACK,
                                       ComparisonFuncLUT[desc.FrontFace.StencilFunc],
                                       _StencilRef,
                                       desc.StencilReadMask );
            } else {
                glStencilFuncSeparate( GL_FRONT,
                                       ComparisonFuncLUT[desc.FrontFace.StencilFunc],
                                       _StencilRef,
                                       desc.StencilReadMask );

                glStencilFuncSeparate( GL_BACK,
                                       ComparisonFuncLUT[desc.BackFace.StencilFunc],
                                       _StencilRef,
                                       desc.StencilReadMask );
            }
        }
    }
#endif

    if (Binding.DepthStencilState != CurrentPipeline->DepthStencilState)
    {
        SDepthStencilStateInfo const& desc = *CurrentPipeline->DepthStencilState;

        if (DepthStencilState.bDepthEnable != desc.bDepthEnable)
        {
            if (desc.bDepthEnable)
            {
                glEnable(GL_DEPTH_TEST);
            }
            else
            {
                glDisable(GL_DEPTH_TEST);
            }

            DepthStencilState.bDepthEnable = desc.bDepthEnable;
        }

        if (DepthStencilState.bDepthWrite != desc.bDepthWrite)
        {
            glDepthMask(desc.bDepthWrite);
            DepthStencilState.bDepthWrite = desc.bDepthWrite;
        }

        if (DepthStencilState.DepthFunc != desc.DepthFunc)
        {
            glDepthFunc(ComparisonFuncLUT[desc.DepthFunc]);
            DepthStencilState.DepthFunc = desc.DepthFunc;
        }

        if (DepthStencilState.bStencilEnable != desc.bStencilEnable)
        {
            if (desc.bStencilEnable)
            {
                glEnable(GL_STENCIL_TEST);
            }
            else
            {
                glDisable(GL_STENCIL_TEST);
            }

            DepthStencilState.bStencilEnable = desc.bStencilEnable;
        }

        if (DepthStencilState.StencilWriteMask != desc.StencilWriteMask)
        {
            glStencilMask(desc.StencilWriteMask);
            DepthStencilState.StencilWriteMask = desc.StencilWriteMask;
        }

        if (DepthStencilState.StencilReadMask != desc.StencilReadMask || DepthStencilState.FrontFace.StencilFunc != desc.FrontFace.StencilFunc || DepthStencilState.BackFace.StencilFunc != desc.BackFace.StencilFunc
            /*|| StencilRef != _StencilRef*/)
        {

#if 0
            if ( desc.FrontFace.StencilFunc == desc.BackFace.StencilFunc ) {
                glStencilFunc( ComparisonFuncLUT[ desc.FrontFace.StencilFunc ],
                               StencilRef,//_StencilRef,
                               desc.StencilReadMask );
            } else
#endif
            {
                if (desc.FrontFace.StencilFunc == desc.BackFace.StencilFunc)
                {
                    glStencilFuncSeparate(GL_FRONT_AND_BACK,
                                          ComparisonFuncLUT[desc.FrontFace.StencilFunc],
                                          StencilRef, //_StencilRef,
                                          desc.StencilReadMask);
                }
                else
                {
                    glStencilFuncSeparate(GL_FRONT,
                                          ComparisonFuncLUT[desc.FrontFace.StencilFunc],
                                          StencilRef, //_StencilRef,
                                          desc.StencilReadMask);

                    glStencilFuncSeparate(GL_BACK,
                                          ComparisonFuncLUT[desc.BackFace.StencilFunc],
                                          StencilRef, //_StencilRef,
                                          desc.StencilReadMask);
                }
            }

            DepthStencilState.StencilReadMask       = desc.StencilReadMask;
            DepthStencilState.FrontFace.StencilFunc = desc.FrontFace.StencilFunc;
            DepthStencilState.BackFace.StencilFunc  = desc.BackFace.StencilFunc;
            //StencilRef = _StencilRef;
        }

        bool frontStencilChanged = (DepthStencilState.FrontFace.StencilFailOp != desc.FrontFace.StencilFailOp || DepthStencilState.FrontFace.DepthFailOp != desc.FrontFace.DepthFailOp || DepthStencilState.FrontFace.DepthPassOp != desc.FrontFace.DepthPassOp);

        bool backStencilChanged = (DepthStencilState.BackFace.StencilFailOp != desc.BackFace.StencilFailOp || DepthStencilState.BackFace.DepthFailOp != desc.BackFace.DepthFailOp || DepthStencilState.BackFace.DepthPassOp != desc.BackFace.DepthPassOp);

        if (frontStencilChanged || backStencilChanged)
        {
            bool isSame = desc.FrontFace.StencilFailOp == desc.BackFace.StencilFailOp && desc.FrontFace.DepthFailOp == desc.BackFace.DepthFailOp && desc.FrontFace.DepthPassOp == desc.BackFace.DepthPassOp;

            if (isSame)
            {
                glStencilOpSeparate(GL_FRONT_AND_BACK,
                                    StencilOpLUT[desc.FrontFace.StencilFailOp],
                                    StencilOpLUT[desc.FrontFace.DepthFailOp],
                                    StencilOpLUT[desc.FrontFace.DepthPassOp]);

                Platform::Memcpy(&DepthStencilState.FrontFace, &desc.FrontFace, sizeof(desc.FrontFace));
                Platform::Memcpy(&DepthStencilState.BackFace, &desc.BackFace, sizeof(desc.FrontFace));
            }
            else
            {

                if (frontStencilChanged)
                {
                    glStencilOpSeparate(GL_FRONT,
                                        StencilOpLUT[desc.FrontFace.StencilFailOp],
                                        StencilOpLUT[desc.FrontFace.DepthFailOp],
                                        StencilOpLUT[desc.FrontFace.DepthPassOp]);

                    Platform::Memcpy(&DepthStencilState.FrontFace, &desc.FrontFace, sizeof(desc.FrontFace));
                }

                if (backStencilChanged)
                {
                    glStencilOpSeparate(GL_BACK,
                                        StencilOpLUT[desc.BackFace.StencilFailOp],
                                        StencilOpLUT[desc.BackFace.DepthFailOp],
                                        StencilOpLUT[desc.BackFace.DepthPassOp]);

                    Platform::Memcpy(&DepthStencilState.BackFace, &desc.BackFace, sizeof(desc.FrontFace));
                }
            }
        }

        Binding.DepthStencilState = CurrentPipeline->DepthStencilState;
    }

    //
    // Set sampler state
    //

#if 1
    if (CurrentPipeline->NumSamplerObjects > 0)
    {
        glBindSamplers(0, CurrentPipeline->NumSamplerObjects, CurrentPipeline->SamplerObjects); // 4.4 or GL_ARB_multi_bind
    }
#else
    for (int i = 0; i < CurrentPipeline->NumSamplerObjects; i++)
    {
        glBindSampler(i, CurrentPipeline->SamplerObjects[i]); // 3.2 or GL_ARB_sampler_objects
    }
#endif
}

void AImmediateContextGLImpl::BindVertexBuffer(unsigned int   _InputSlot,
                                               IBuffer const* _VertexBuffer,
                                               unsigned int   _Offset)
{
    HK_ASSERT(_InputSlot < MAX_VERTEX_BUFFER_SLOTS);

    VertexBufferUIDs[_InputSlot]    = _VertexBuffer ? _VertexBuffer->GetUID() : 0;
    VertexBufferHandles[_InputSlot] = _VertexBuffer ? _VertexBuffer->GetHandleNativeGL() : 0;
    VertexBufferOffsets[_InputSlot] = _Offset;
}

void AImmediateContextGLImpl::BindVertexBuffers(unsigned int    _StartSlot,
                                                unsigned int    _NumBuffers,
                                                IBuffer* const* _VertexBuffers,
                                                uint32_t const* _Offsets)
{
    HK_ASSERT(_StartSlot + _NumBuffers <= MAX_VERTEX_BUFFER_SLOTS);

    if (_VertexBuffers)
    {
        for (int i = 0; i < _NumBuffers; i++)
        {
            int slot = _StartSlot + i;

            VertexBufferUIDs[slot]    = _VertexBuffers[i] ? _VertexBuffers[i]->GetUID() : 0;
            VertexBufferHandles[slot] = _VertexBuffers[i] ? _VertexBuffers[i]->GetHandleNativeGL() : 0;
            VertexBufferOffsets[slot] = _Offsets ? _Offsets[i] : 0;
        }
    }
    else
    {
        for (int i = 0; i < _NumBuffers; i++)
        {
            int slot = _StartSlot + i;

            VertexBufferUIDs[slot]    = 0;
            VertexBufferHandles[slot] = 0;
            VertexBufferOffsets[slot] = 0;
        }
    }
}

#if 0




void AImmediateContextGLImpl::BindVertexBuffers( unsigned int _StartSlot,
                                                 unsigned int _NumBuffers,
                                                 IBuffer * const * _VertexBuffers,
                                                 uint32_t const * _Offsets )
{
    VerifyContext();

    HK_ASSERT( CurrentVAO != nullptr );

    GLuint id = CurrentVAO->Handle;

    static_assert( sizeof( CurrentVAO->VertexBindingsStrides[0] ) == sizeof( GLsizei ), "Wrong type size" );

    if ( _StartSlot + _NumBuffers > GetDevice()->MaxVertexBufferSlots ) {
        LOG( "BindVertexBuffers: StartSlot + NumBuffers > MaxVertexBufferSlots\n" );
        return;
    }

    bool bModified = false;

    if ( _VertexBuffers ) {

        for ( unsigned int i = 0 ; i < _NumBuffers ; i++ ) {
            unsigned int slot = _StartSlot + i;

            uint32_t uid = _VertexBuffers[i] ? _VertexBuffers[i]->GetUID() : 0;
            uint32_t offset = _Offsets ? _Offsets[i] : 0;

            bModified = CurrentVAO->VertexBufferUIDs[ slot ] != uid || CurrentVAO->VertexBufferOffsets[ slot ] != offset;

            CurrentVAO->VertexBufferUIDs[ slot ] = uid;
            CurrentVAO->VertexBufferOffsets[ slot ] = offset;
        }

        if ( !bModified ) {
            return;
        }

        if ( _NumBuffers == 1 )
        {
            GLuint vertexBufferId = _VertexBuffers[0] ? _VertexBuffers[0]->GetHandleNativeGL() : 0;
            glVertexArrayVertexBuffer( id, _StartSlot, vertexBufferId, CurrentVAO->VertexBufferOffsets[ _StartSlot ], CurrentVAO->VertexBindingsStrides[ _StartSlot ] );
        }
        else
        {
            // Convert input parameters to OpenGL format
            for ( unsigned int i = 0 ; i < _NumBuffers ; i++ ) {
                TmpHandles[ i ] = _VertexBuffers[i] ? _VertexBuffers[i]->GetHandleNativeGL : 0;
                TmpPointers[ i ] = CurrentVAO->VertexBufferOffsets[ _StartSlot + i ];
            }
            glVertexArrayVertexBuffers( id, _StartSlot, _NumBuffers, TmpHandles, TmpPointers, reinterpret_cast< const GLsizei * >( &CurrentVAO->VertexBindingsStrides[ _StartSlot ] ) );
        }
    } else {

        for ( unsigned int i = 0 ; i < _NumBuffers ; i++ ) {
            unsigned int slot = _StartSlot + i;

            uint32_t uid = 0;
            uint32_t offset = 0;

            bModified = CurrentVAO->VertexBufferUIDs[ slot ] != uid || CurrentVAO->VertexBufferOffsets[ slot ] != offset;

            CurrentVAO->VertexBufferUIDs[ slot ] = uid;
            CurrentVAO->VertexBufferOffsets[ slot ] = offset;
        }

        if ( !bModified ) {
            return;
        }

        if ( _NumBuffers == 1 ) {
            glVertexArrayVertexBuffer( id, _StartSlot, 0, 0, 16 ); // From OpenGL specification
        } else {
            glVertexArrayVertexBuffers( id, _StartSlot, _NumBuffers, nullptr, nullptr, nullptr );
        }
    }
}
#endif

void AImmediateContextGLImpl::BindIndexBuffer(IBuffer const* _IndexBuffer,
                                              INDEX_TYPE     _Type,
                                              unsigned int   _Offset)
{
    IndexBufferType       = IndexTypeLUT[_Type];
    IndexBufferOffset     = _Offset;
    IndexBufferTypeSizeOf = IndexTypeSizeOfLUT[_Type];
    IndexBufferUID        = _IndexBuffer ? _IndexBuffer->GetUID() : 0;
    IndexBufferHandle     = _IndexBuffer ? _IndexBuffer->GetHandleNativeGL() : 0;
}

IResourceTable* AImmediateContextGLImpl::GetRootResourceTable()
{
    return RootResourceTable;
}

void AImmediateContextGLImpl::BindResourceTable(IResourceTable* _ResourceTable)
{
    IResourceTable* tbl = _ResourceTable ? _ResourceTable : RootResourceTable.GetObject();

    CurrentResourceTable = static_cast<AResourceTableGLImpl*>(tbl);
}

#if 0
void AImmediateContextGLImpl::BindResourceTable( SResourceTable const * _ResourceTable )
{
    // TODO: Cache resource tables

    VerifyContext();

    SResourceBufferBinding const * buffers = _ResourceTable->GetBuffers();
    //SResourceTextureBinding const * textures = _ResourceTable->GetTextures();
    //SResourceImageBinding const * images = _ResourceTable->GetImages();
    int numBuffers = _ResourceTable->GetNumBuffers();
    //int numTextures = _ResourceTable->GetNumTextures();
    //int numImages = _ResourceTable->GetNumImages();
    int slot;

    slot = 0;
    for ( SResourceBufferBinding const * binding = buffers ; binding < &buffers[numBuffers] ; binding++, slot++ ) {
        GLenum target = BufferTargetLUT[ binding->BufferType ].Target;

        IDeviceObject const * native = binding->pBuffer;

        uint32_t bufferUid = native ? native->GetUID() : 0;

        if ( BufferBindingUIDs[ slot ] != bufferUid || binding->BindingSize > 0 ) {
            BufferBindingUIDs[ slot ] = bufferUid;

            if ( bufferUid && binding->BindingSize > 0 ) {
                glBindBufferRange( target, slot, native->GetHandleNativeGL(), binding->BindingOffset, binding->BindingSize ); // 3.0 or GL_ARB_uniform_buffer_object
            } else {
                glBindBufferBase( target, slot, native->GetHandleNativeGL() ); // 3.0 or GL_ARB_uniform_buffer_object
            }
        }
    }
#    if 0
    slot = 0;
    for ( SResourceTextureBinding const * binding = textures ; binding < &textures[numTextures] ; binding++, slot++ ) {
        GLuint textureId;
        uint32_t textureUid;

        IDeviceObject const * native = binding->pTexture;

        if ( native ) {
            textureId = native->GetHandleNativeGL();
            textureUid = native->GetUID();
        } else {
            textureId = 0;
            textureUid = 0;
        }

        if ( TextureBindings[ slot ] != textureUid ) {
            TextureBindings[ slot ] = textureUid;

            glBindTextureUnit( slot, textureId ); // 4.5
        }
    }

    slot = 0;
    for ( SResourceImageBinding const * binding = images ; binding < &images[numImages] ; binding++, slot++ ) {
        IDeviceObject const * native = binding->pTexture;

        GLuint id = native ? native->GetHandleNativeGL() : 0;

        glBindImageTexture( slot,
                            id,
                            binding->MipLevel,
                            binding->bLayered,
                            binding->LayerIndex,
                            ImageAccessModeLUT[ binding->AccessMode ],
                            InternalFormatLUT[ binding->TextureFormat ].InternalFormat ); // 4.2
    }
#    endif
}
#endif

#if 0
void ImmediateContext::SetBuffers( BUFFER_TYPE _BufferType,
                                unsigned int _StartSlot,
                                unsigned int _NumBuffers,
                                IBuffer * const * _Buffers,
                                unsigned int * _RangeOffsets,
                                unsigned int * _RangeSizes )
{
    VerifyContext();

    // _StartSlot + _NumBuffers must be < storage->MaxBufferBindings[_BufferType]

    GLenum target = BufferTargetLUT[ _BufferType ].Target;

    if ( !_Buffers ) {
        glBindBuffersBase( target, _StartSlot, _NumBuffers, nullptr ); // 4.4 or GL_ARB_multi_bind
        return;
    }

    if ( _RangeOffsets && _RangeSizes ) {
        if ( _NumBuffers == 1 ) {
            glBindBufferRange( target, _StartSlot, ( size_t )_Buffers[0]->GetHandle(), _RangeOffsets[0], _RangeSizes[0] ); // 3.0 or GL_ARB_uniform_buffer_object
        } else {
            for ( unsigned int i = 0 ; i < _NumBuffers ; i++ ) {
                TmpHandles[ i ] = ( size_t )_Buffers[ i ]->GetHandle();
                TmpPointers[ i ] = _RangeOffsets[ i ];
                TmpPointers2[ i ] = _RangeSizes[ i ];
            }
            glBindBuffersRange( target, _StartSlot, _NumBuffers, TmpHandles, TmpPointers, TmpPointers2 ); // 4.4 or GL_ARB_multi_bind

            // Or  Since 3.0 or GL_ARB_uniform_buffer_object
            //if ( _Buffers != nullptr) {
            //    for ( int i = 0 ; i < _NumBuffers ; i++ ) {
            //        glBindBufferRange( target, _StartSlot + i, _Buffers[i]->GetHandleNativeGL(), _RangeOffsets​[i], _RangeSizes[i] );
            //    }
            //} else {
            //    for ( int i = 0 ; i < _NumBuffers ; i++ ) {
            //        glBindBufferBase( target, _StartSlot + i, 0 );
            //    }
            //}
        }
    } else {
        if ( _NumBuffers == 1 ) {
            glBindBufferBase( target, _StartSlot, ( size_t )_Buffers[0]->GetHandle() ); // 3.0 or GL_ARB_uniform_buffer_object
        } else {
            for ( unsigned int i = 0 ; i < _NumBuffers ; i++ ) {
                TmpHandles[ i ] = ( size_t )_Buffers[i]->GetHandle();
            }
            glBindBuffersBase( target, _StartSlot, _NumBuffers, TmpHandles ); // 4.4 or GL_ARB_multi_bind

            // Or  Since 3.0 or GL_ARB_uniform_buffer_object
            //if ( _Buffers != nullptr) {
            //    for ( int i = 0 ; i < _NumBuffers ; i++ ) {
            //        glBindBufferBase( target, _StartSlot + i, _Buffers[i]->GetHandleNativeGL() );
            //    }
            //} else {
            //    for ( int i = 0 ; i < _NumBuffers ; i++ ) {
            //        glBindBufferBase( target, _StartSlot + i, 0 );
            //    }
            //}
        }
    }
}

void ImmediateContext::SetTextureUnit( unsigned int _Unit, Texture const * _Texture )
{
    VerifyContext();

    // Unit must be < MaxCombinedTextureImageUnits

    glBindTextureUnit( _Unit, ( size_t )_Texture->GetHandle() ); // 4.5

    /*
    // Other path
    GLuint texture = ( size_t )_Texture->GetHandle();
    glBindTextures( _Unit, 1, &texture ); // 4.4
    */

    /*
    // Other path
    glActiveTexture( GL_TEXTURE0 + _Unit );
    if ( texture ) {
        glBindTexture( target, ( size_t )_Texture->GetHandle() );
    } else {
        for (target in all supported targets) {
            glBindTexture( target, 0 );
        }
    }
    */
}

void ImmediateContext::SetTextureUnits( unsigned int _FirstUnit,
                                        unsigned int _NumUnits,
                                        Texture * const * _Textures )
{
    VerifyContext();

    // _FirstUnit + _NumUnits must be < MaxCombinedTextureImageUnits

    if ( _Textures ) {
        for ( unsigned int i = 0 ; i < _NumUnits ; i++ ) {
            TmpHandles[ i ] = ( size_t )_Textures[ i ]->GetHandle();
        }

        glBindTextures(	_FirstUnit, _NumUnits, TmpHandles ); // 4.4
    } else {
        glBindTextures(	_FirstUnit, _NumUnits, nullptr ); // 4.4
    }


    /*
    // Other path
    for (i = 0; i < _NumUnits; i++) {
        GLuint texture;
        if (textures == nullptr) {
            texture = 0;
        } else {
            texture = textures[i];
        }
        glActiveTexture(GL_TEXTURE0 + _FirstUnit + i);
        if (texture != 0) {
            GLenum target = // target of textures[i] ;
            glBindTexture(target, textures[i]);
        } else {
            for (target in all supported targets) {
                glBindTexture(target, 0);
            }
        }
    }
    */
}

bool ImmediateContext::SetImageUnit( unsigned int _Unit,
                                  Texture * _Texture,
                                  unsigned int _MipLevel,
                                  bool _AllLayers,
                                  unsigned int _LayerIndex,  // Array index for texture arrays, depth for 3D textures or cube face for cubemaps
                                  // For cubemap arrays: arrayLength = floor( _LayerIndex / 6 ), face = _LayerIndex % 6
                                  IMAGE_ACCESS_MODE _AccessMode,
                                  INTERNAL_PIXEL_FORMAT _InternalFormat )
{
    VerifyContext();

    if ( !*InternalFormatLUT[ _InternalFormat ].ShaderImageFormatQualifier ) {
        return false;
    }

    // Unit must be < ResourceStoragesPtr->MaxImageUnits
    glBindImageTexture( _Unit,
                        ( size_t )_Texture->GetHandle(),
                        _MipLevel,
                        _AllLayers,
                        _LayerIndex,
                        ImageAccessModeLUT[ _AccessMode ],
                        InternalFormatLUT[ _InternalFormat ].InternalFormat ); // 4.2

    return true;
}

void ImmediateContext::SetImageUnits( unsigned int _FirstUnit,
                                      unsigned int _NumUnits,
                                      Texture * const * _Textures )
{
    VerifyContext();

    // _FirstUnit + _NumUnits must be < ResourceStoragesPtr->MaxImageUnits

    if ( _Textures ) {
        for ( unsigned int i = 0 ; i < _NumUnits ; i++ ) {
            TmpHandles[ i ] = ( size_t )_Textures[ i ]->GetHandle();
        }

        glBindImageTextures( _FirstUnit, _NumUnits, TmpHandles ); // 4.4
    } else {
        glBindImageTextures( _FirstUnit, _NumUnits, nullptr ); // 4.4
    }

    /*
    // Other path:
    for (i = 0; i < _NumUnits; i++) {
        if (textures == nullptr || textures[i] = 0) {
            glBindImageTexture(_FirstUnit + i, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8); // 4.2
        } else {
            glBindImageTexture(_FirstUnit + i, textures[i], 0, GL_TRUE, 0, GL_READ_WRITE, lookupInternalFormat(textures[i])); // 4.2
        }
    }
    */
}

#endif

#define INVERT_VIEWPORT_Y(Viewport) (Binding.DrawFramebufferHeight - (Viewport)->Y - (Viewport)->Height)

void AImmediateContextGLImpl::SetViewport(SViewport const& _Viewport)
{
    VerifyContext();

    HK_ASSERT(CurrentRenderPass != nullptr);

    if (std::memcmp(CurrentViewport, &_Viewport, sizeof(CurrentViewport)) != 0)
    {
        glViewport((GLint)_Viewport.X,
                   (GLint)INVERT_VIEWPORT_Y(&_Viewport),
                   (GLsizei)_Viewport.Width,
                   (GLsizei)_Viewport.Height);
        Platform::Memcpy(CurrentViewport, &_Viewport, sizeof(CurrentViewport));
    }

    if (std::memcmp(CurrentDepthRange, &_Viewport.MinDepth, sizeof(CurrentDepthRange)) != 0)
    {
        glDepthRangef(_Viewport.MinDepth, _Viewport.MaxDepth); // Since GL v4.1

        Platform::Memcpy(CurrentDepthRange, &_Viewport.MinDepth, sizeof(CurrentDepthRange));
    }
}

void AImmediateContextGLImpl::SetViewportArray(uint32_t _NumViewports, SViewport const* _Viewports)
{
    SetViewportArray(0, _NumViewports, _Viewports);
}

void AImmediateContextGLImpl::SetViewportArray(uint32_t _FirstIndex, uint32_t _NumViewports, SViewport const* _Viewports)
{
    VerifyContext();

    HK_ASSERT(CurrentRenderPass != nullptr);

#define MAX_VIEWPORT_DATA 1024
    static_assert(sizeof(float) * 2 == sizeof(double), "ImmediateContext::SetViewportArray type check");
    constexpr uint32_t MAX_VIEWPORTS = MAX_VIEWPORT_DATA >> 2;
    float              viewportData[MAX_VIEWPORT_DATA];

    _NumViewports = std::min(MAX_VIEWPORTS, _NumViewports);

    const bool bInvertY = true;

    float* pViewportData = viewportData;
    for (SViewport const* viewport = _Viewports; viewport < &_Viewports[_NumViewports]; viewport++, pViewportData += 4)
    {
        pViewportData[0] = viewport->X;
        pViewportData[1] = bInvertY ? INVERT_VIEWPORT_Y(viewport) : viewport->Y;
        pViewportData[2] = viewport->Width;
        pViewportData[3] = viewport->Height;
    }
    glViewportArrayv(_FirstIndex, _NumViewports, viewportData);

    double* pDepthRangeData = reinterpret_cast<double*>(&viewportData[0]);
    for (SViewport const* viewport = _Viewports; viewport < &_Viewports[_NumViewports]; viewport++, pDepthRangeData += 2)
    {
        pDepthRangeData[0] = viewport->MinDepth;
        pDepthRangeData[1] = viewport->MaxDepth;
    }
    glDepthRangeArrayv(_FirstIndex, _NumViewports, reinterpret_cast<double*>(&viewportData[0]));
}

void AImmediateContextGLImpl::SetViewportIndexed(uint32_t _Index, SViewport const& _Viewport)
{
    VerifyContext();

    HK_ASSERT(CurrentRenderPass != nullptr);

    const bool bInvertY        = true;
    float      viewportData[4] = {_Viewport.X, bInvertY ? INVERT_VIEWPORT_Y(&_Viewport) : _Viewport.Y, _Viewport.Width, _Viewport.Height};
    glViewportIndexedfv(_Index, viewportData);
    glDepthRangeIndexed(_Index, _Viewport.MinDepth, _Viewport.MaxDepth);
}

void AImmediateContextGLImpl::SetScissor(SRect2D const& _Scissor)
{
    VerifyContext();

    CurrentScissor = _Scissor;

    const bool bInvertY = true;

    glScissor(CurrentScissor.X,
              bInvertY ? Binding.DrawFramebufferHeight - CurrentScissor.Y - CurrentScissor.Height : CurrentScissor.Y,
              CurrentScissor.Width,
              CurrentScissor.Height);
}

void AImmediateContextGLImpl::SetScissorArray(uint32_t _NumScissors, SRect2D const* _Scissors)
{
    SetScissorArray(0, _NumScissors, _Scissors);
}

void AImmediateContextGLImpl::SetScissorArray(uint32_t _FirstIndex, uint32_t _NumScissors, SRect2D const* _Scissors)
{
    VerifyContext();

    HK_ASSERT(CurrentRenderPass != nullptr);

#define MAX_SCISSOR_DATA 1024
    constexpr uint32_t MAX_SCISSORS = MAX_VIEWPORT_DATA >> 2;
    GLint              scissorData[MAX_SCISSOR_DATA];

    _NumScissors = std::min(MAX_SCISSORS, _NumScissors);

    const bool bInvertY = true;

    GLint* pScissorData = scissorData;
    for (SRect2D const* scissor = _Scissors; scissor < &_Scissors[_NumScissors]; scissor++, pScissorData += 4)
    {
        pScissorData[0] = scissor->X;
        pScissorData[1] = bInvertY ? INVERT_VIEWPORT_Y(scissor) : scissor->Y;
        pScissorData[2] = scissor->Width;
        pScissorData[3] = scissor->Height;
    }
    glScissorArrayv(_FirstIndex, _NumScissors, scissorData);
}

void AImmediateContextGLImpl::SetScissorIndexed(uint32_t _Index, SRect2D const& _Scissor)
{
    VerifyContext();

    HK_ASSERT(CurrentRenderPass != nullptr);

    const bool bInvertY = true;

    int scissorData[4] = {_Scissor.X, bInvertY ? INVERT_VIEWPORT_Y(&_Scissor) : _Scissor.Y, _Scissor.Width, _Scissor.Height};
    glScissorIndexedv(_Index, scissorData);
}

void AImmediateContextGLImpl::UpdateVertexBuffers()
{
    SVertexLayoutDescGL const& desc = CurrentVertexLayout->GetDesc();
    for (SVertexBindingInfo const* binding = desc.VertexBindings; binding < &desc.VertexBindings[desc.NumVertexBindings]; binding++)
    {
        int slot = binding->InputSlot;

        if (CurrentVAO->VertexBufferUIDs[slot] != VertexBufferUIDs[slot] || CurrentVAO->VertexBufferOffsets[slot] != VertexBufferOffsets[slot])
        {

            glVertexArrayVertexBuffer(CurrentVAO->HandleGL, slot, VertexBufferHandles[slot], VertexBufferOffsets[slot], CurrentVertexLayout->GetVertexBindingsStrides()[slot]);

            CurrentVAO->VertexBufferUIDs[slot]    = VertexBufferUIDs[slot];
            CurrentVAO->VertexBufferOffsets[slot] = VertexBufferOffsets[slot];
        }
        else
        {
            //LOG( "Caching BindVertexBuffer {}\n", vertexBufferId );
        }
    }
}

void AImmediateContextGLImpl::UpdateVertexAndIndexBuffers()
{
    UpdateVertexBuffers();

    if (CurrentVAO->IndexBufferUID != IndexBufferUID)
    {
        glVertexArrayElementBuffer(CurrentVAO->HandleGL, IndexBufferHandle);
        CurrentVAO->IndexBufferUID = IndexBufferUID;

        //LOG( "BindIndexBuffer {}\n", indexBufferId );
    }
    else
    {
        //LOG( "Caching BindIndexBuffer {}\n", indexBufferId );
    }
}

void AImmediateContextGLImpl::UpdateShaderBindings()
{
    // TODO: memcmp CurrentPipeline->TextureBindings, CurrentResourceTable->TextureBindings2

    glBindTextures(0, CurrentPipeline->NumSamplerObjects, CurrentResourceTable->GetTextureBindings()); // 4.4

#if 1
    for (int i = 0; i < CurrentPipeline->NumImages; i++)
    {
        // TODO: cache image bindings (memcmp?)
        glBindImageTexture(i,
                           CurrentResourceTable->GetImageBindings()[i],
                           CurrentResourceTable->GetImageMipLevel()[i],
                           CurrentResourceTable->GetImageLayered()[i],
                           CurrentResourceTable->GetImageLayerIndex()[i],
                           CurrentPipeline->Images[i].AccessMode,
                           CurrentPipeline->Images[i].InternalFormat); // 4.2
    }
#else
    glBindImageTextures(0, CurrentPipeline->NumImages, CurrentResourceTable->ImageBindings); // 4.4
#endif

    for (int i = 0; i < CurrentPipeline->NumBuffers; i++)
    {
        if (BufferBindingUIDs[i] != CurrentResourceTable->GetBufferBindingUIDs()[i] || BufferBindingOffsets[i] != CurrentResourceTable->GetBufferBindingOffsets()[i] || BufferBindingSizes[i] != CurrentResourceTable->GetBufferBindingSizes()[i])
        {

            BufferBindingUIDs[i]    = CurrentResourceTable->GetBufferBindingUIDs()[i];
            BufferBindingOffsets[i] = CurrentResourceTable->GetBufferBindingOffsets()[i];
            BufferBindingSizes[i]   = CurrentResourceTable->GetBufferBindingSizes()[i];

            if (BufferBindingUIDs[i] && BufferBindingSizes[i] > 0)
            {
                glBindBufferRange(CurrentPipeline->Buffers[i].BufferType, i, CurrentResourceTable->GetBufferBindings()[i], BufferBindingOffsets[i], BufferBindingSizes[i]); // 3.0 or GL_ARB_uniform_buffer_object
            }
            else
            {
                glBindBufferBase(CurrentPipeline->Buffers[i].BufferType, i, CurrentResourceTable->GetBufferBindings()[i]); // 3.0 or GL_ARB_uniform_buffer_object
            }
        }
    }
}

void AImmediateContextGLImpl::Draw(SDrawCmd const* _Cmd)
{
    VerifyContext();

    HK_ASSERT(CurrentPipeline != nullptr);

    if (_Cmd->InstanceCount == 0 || _Cmd->VertexCountPerInstance == 0)
    {
        return;
    }

    UpdateVertexBuffers();
    UpdateShaderBindings();

    if (_Cmd->InstanceCount == 1 && _Cmd->StartInstanceLocation == 0)
    {
        glDrawArrays(CurrentPipeline->PrimitiveTopology, _Cmd->StartVertexLocation, _Cmd->VertexCountPerInstance); // Since 2.0
    }
    else
    {
        if (_Cmd->StartInstanceLocation == 0)
        {
            glDrawArraysInstanced(CurrentPipeline->PrimitiveTopology, _Cmd->StartVertexLocation, _Cmd->VertexCountPerInstance, _Cmd->InstanceCount); // Since 3.1
        }
        else
        {
            glDrawArraysInstancedBaseInstance(CurrentPipeline->PrimitiveTopology,
                                              _Cmd->StartVertexLocation,
                                              _Cmd->VertexCountPerInstance,
                                              _Cmd->InstanceCount,
                                              _Cmd->StartInstanceLocation); // Since 4.2 or GL_ARB_base_instance
        }
    }
}

void AImmediateContextGLImpl::Draw(SDrawIndexedCmd const* _Cmd)
{
    VerifyContext();

    HK_ASSERT(CurrentPipeline != nullptr);

    if (_Cmd->InstanceCount == 0 || _Cmd->IndexCountPerInstance == 0)
    {
        return;
    }

    UpdateVertexAndIndexBuffers();
    UpdateShaderBindings();

    const GLubyte* offset = reinterpret_cast<const GLubyte*>(0) + _Cmd->StartIndexLocation * IndexBufferTypeSizeOf + IndexBufferOffset;

    if (_Cmd->InstanceCount == 1 && _Cmd->StartInstanceLocation == 0)
    {
        if (_Cmd->BaseVertexLocation == 0)
        {
            glDrawElements(CurrentPipeline->PrimitiveTopology,
                           _Cmd->IndexCountPerInstance,
                           IndexBufferType,
                           offset); // 2.0
        }
        else
        {
            glDrawElementsBaseVertex(CurrentPipeline->PrimitiveTopology,
                                     _Cmd->IndexCountPerInstance,
                                     IndexBufferType,
                                     offset,
                                     _Cmd->BaseVertexLocation); // 3.2 or GL_ARB_draw_elements_base_vertex
        }
    }
    else
    {
        if (_Cmd->StartInstanceLocation == 0)
        {
            if (_Cmd->BaseVertexLocation == 0)
            {
                glDrawElementsInstanced(CurrentPipeline->PrimitiveTopology,
                                        _Cmd->IndexCountPerInstance,
                                        IndexBufferType,
                                        offset,
                                        _Cmd->InstanceCount); // 3.1
            }
            else
            {
                glDrawElementsInstancedBaseVertex(CurrentPipeline->PrimitiveTopology,
                                                  _Cmd->IndexCountPerInstance,
                                                  IndexBufferType,
                                                  offset,
                                                  _Cmd->InstanceCount,
                                                  _Cmd->BaseVertexLocation); // 3.2 or GL_ARB_draw_elements_base_vertex
            }
        }
        else
        {
            if (_Cmd->BaseVertexLocation == 0)
            {
                glDrawElementsInstancedBaseInstance(CurrentPipeline->PrimitiveTopology,
                                                    _Cmd->IndexCountPerInstance,
                                                    IndexBufferType,
                                                    offset,
                                                    _Cmd->InstanceCount,
                                                    _Cmd->StartInstanceLocation); // 4.2 or GL_ARB_base_instance
            }
            else
            {
                glDrawElementsInstancedBaseVertexBaseInstance(CurrentPipeline->PrimitiveTopology,
                                                              _Cmd->IndexCountPerInstance,
                                                              IndexBufferType,
                                                              offset,
                                                              _Cmd->InstanceCount,
                                                              _Cmd->BaseVertexLocation,
                                                              _Cmd->StartInstanceLocation); // 4.2 or GL_ARB_base_instance
            }
        }
    }
}

void AImmediateContextGLImpl::Draw(ITransformFeedback* _TransformFeedback, unsigned int _InstanceCount, unsigned int _StreamIndex)
{
    VerifyContext();

    HK_ASSERT(CurrentPipeline != nullptr);

    if (_InstanceCount == 0)
    {
        return;
    }

    UpdateShaderBindings();

    if (_InstanceCount > 1)
    {
        if (_StreamIndex == 0)
        {
            glDrawTransformFeedbackInstanced(CurrentPipeline->PrimitiveTopology, _TransformFeedback->GetHandleNativeGL(), _InstanceCount); // 4.2
        }
        else
        {
            glDrawTransformFeedbackStreamInstanced(CurrentPipeline->PrimitiveTopology, _TransformFeedback->GetHandleNativeGL(), _StreamIndex, _InstanceCount); // 4.2
        }
    }
    else
    {
        if (_StreamIndex == 0)
        {
            glDrawTransformFeedback(CurrentPipeline->PrimitiveTopology, _TransformFeedback->GetHandleNativeGL()); // 4.0
        }
        else
        {
            glDrawTransformFeedbackStream(CurrentPipeline->PrimitiveTopology, _TransformFeedback->GetHandleNativeGL(), _StreamIndex); // 4.0
        }
    }
}

#if 0
void AImmediateContextGLImpl::DrawIndirect( SDrawIndirectCmd const * _Cmd )
{
    VerifyContext();

    HK_ASSERT( CurrentPipeline != nullptr );

    UpdateVertexBuffers();
    UpdateShaderBindings();

    if ( Binding.DrawInderectBuffer != 0 ) {
        glBindBuffer( GL_DRAW_INDIRECT_BUFFER, 0 );
        Binding.DrawInderectBuffer = 0;
    }

    // This is similar glDrawArraysInstancedBaseInstance
    glDrawArraysIndirect( CurrentPipeline->PrimitiveTopology, _Cmd ); // Since 4.0 or GL_ARB_draw_indirect
}

void AImmediateContextGLImpl::DrawIndirect( SDrawIndexedIndirectCmd const * _Cmd )
{
    VerifyContext();

    HK_ASSERT( CurrentPipeline != nullptr );

    UpdateVertexAndIndexBuffers();
    UpdateShaderBindings();

    if ( Binding.DrawInderectBuffer != 0 ) {
        glBindBuffer( GL_DRAW_INDIRECT_BUFFER, 0 );
        Binding.DrawInderectBuffer = 0;
    }

    // This is similar glDrawElementsInstancedBaseVertexBaseInstance
    glDrawElementsIndirect( CurrentPipeline->PrimitiveTopology, IndexBufferType, _Cmd ); // Since 4.0 or GL_ARB_draw_indirect
}
#endif

void AImmediateContextGLImpl::DrawIndirect(IBuffer* _DrawIndirectBuffer, unsigned int _AlignedByteOffset)
{
    VerifyContext();

    HK_ASSERT(CurrentPipeline != nullptr);

    GLuint handle = _DrawIndirectBuffer->GetHandleNativeGL();
    if (Binding.DrawInderectBuffer != handle)
    {
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, handle);
        Binding.DrawInderectBuffer = handle;
    }

    UpdateShaderBindings();
    UpdateVertexBuffers();

    // This is similar glDrawArraysInstancedBaseInstance, but with binded INDIRECT buffer
    glDrawArraysIndirect(CurrentPipeline->PrimitiveTopology,
                         reinterpret_cast<const GLubyte*>(0) + _AlignedByteOffset); // Since 4.0 or GL_ARB_draw_indirect
}

void AImmediateContextGLImpl::DrawIndexedIndirect(IBuffer* _DrawIndirectBuffer, unsigned int _AlignedByteOffset)
{
    VerifyContext();

    HK_ASSERT(CurrentPipeline != nullptr);

    GLuint handle = _DrawIndirectBuffer->GetHandleNativeGL();
    if (Binding.DrawInderectBuffer != handle)
    {
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, handle);
        Binding.DrawInderectBuffer = handle;
    }

    UpdateShaderBindings();
    UpdateVertexAndIndexBuffers();

    // This is similar glDrawElementsInstancedBaseVertexBaseInstance, but with binded INDIRECT buffer
    glDrawElementsIndirect(CurrentPipeline->PrimitiveTopology,
                           IndexBufferType,
                           reinterpret_cast<const GLubyte*>(0) + _AlignedByteOffset); // Since 4.0 or GL_ARB_draw_indirect
}

void AImmediateContextGLImpl::MultiDraw(unsigned int _DrawCount, const unsigned int* _VertexCount, const unsigned int* _StartVertexLocations)
{
    VerifyContext();

    HK_ASSERT(CurrentPipeline != nullptr);

    static_assert(sizeof(_VertexCount[0]) == sizeof(GLsizei), "!");
    static_assert(sizeof(_StartVertexLocations[0]) == sizeof(GLint), "!");

    UpdateVertexBuffers();
    UpdateShaderBindings();

    glMultiDrawArrays(CurrentPipeline->PrimitiveTopology, (const GLint*)_StartVertexLocations, (const GLsizei*)_VertexCount, _DrawCount); // Since 2.0

    // Эквивалентный код:
    //for ( unsigned int i = 0 ; i < _DrawCount ; i++ ) {
    //    glDrawArrays( CurrentPipeline->PrimitiveTopology, _StartVertexLocation[i], _VertexCount[i] );
    //}
}

void AImmediateContextGLImpl::MultiDraw(unsigned int _DrawCount, const unsigned int* _IndexCount, const void* const* _IndexByteOffsets, const int* _BaseVertexLocations)
{
    VerifyContext();

    HK_ASSERT(CurrentPipeline != nullptr);

    static_assert(sizeof(unsigned int) == sizeof(GLsizei), "!");

    // FIXME: how to apply IndexBufferOffset?

    UpdateVertexAndIndexBuffers();
    UpdateShaderBindings();

    if (_BaseVertexLocations)
    {
        glMultiDrawElementsBaseVertex(CurrentPipeline->PrimitiveTopology,
                                      (const GLsizei*)_IndexCount,
                                      IndexBufferType,
                                      _IndexByteOffsets,
                                      _DrawCount,
                                      _BaseVertexLocations); // 3.2
        // Эквивалентный код:
        //    for ( int i = 0 ; i < _DrawCount ; i++ )
        //        if ( _IndexCount[i] > 0 )
        //            glDrawElementsBaseVertex( CurrentPipeline->PrimitiveTopology,
        //                                      _IndexCount[i],
        //                                      IndexBufferType,
        //                                      _IndexByteOffsets[i],
        //                                      _BaseVertexLocations[i]);
    }
    else
    {
        glMultiDrawElements(CurrentPipeline->PrimitiveTopology,
                            (const GLsizei*)_IndexCount,
                            IndexBufferType,
                            _IndexByteOffsets,
                            _DrawCount); // 2.0
    }
}

#if 0
void AImmediateContextGLImpl::MultiDrawIndirect( unsigned int _DrawCount, SDrawIndirectCmd const * _Cmds, unsigned int _Stride )
{
    VerifyContext();

    HK_ASSERT( CurrentPipeline != nullptr );

    UpdateVertexBuffers();
    UpdateShaderBindings();

#    if 0
    if ( Binding.DrawInderectBuffer != 0 ) {
        glBindBuffer( GL_DRAW_INDIRECT_BUFFER, 0 );
        Binding.DrawInderectBuffer = 0;
    }

    // This is similar glDrawArraysInstancedBaseInstance
    glMultiDrawArraysIndirect( CurrentPipeline->PrimitiveTopology,
                               _Cmds,
                               _DrawCount,
                               _Stride ); // 4.3 or GL_ARB_multi_draw_indirect
#    else
    for ( unsigned int n = 0 ; n < _DrawCount ; n++ ) {
        SDrawIndirectCmd const *cmd = ( _Stride != 0 ) ?
                    (SDrawIndirectCmd const *)((uintptr_t)_Cmds + n * _Stride) : (_Cmds + n);
        glDrawArraysInstancedBaseInstance( CurrentPipeline->PrimitiveTopology,
                                           cmd->StartVertexLocation,
                                           cmd->VertexCountPerInstance,
                                           cmd->InstanceCount,
                                           cmd->StartInstanceLocation );
    }
#    endif
}

void AImmediateContextGLImpl::MultiDrawIndirect( unsigned int _DrawCount, SDrawIndexedIndirectCmd const * _Cmds, unsigned int _Stride )
{
    VerifyContext();

    HK_ASSERT( CurrentPipeline != nullptr );

    UpdateVertexAndIndexBuffers();
    UpdateShaderBindings();

#    if 0
    if ( Binding.DrawInderectBuffer != 0 ) {
        glBindBuffer( GL_DRAW_INDIRECT_BUFFER, 0 );
        Binding.DrawInderectBuffer = 0;
    }

    glMultiDrawElementsIndirect( CurrentPipeline->PrimitiveTopology,
                                 IndexBufferType,
                                 _Cmds,
                                 _DrawCount,
                                 _Stride ); // 4.3
#    else
    for ( unsigned int n = 0 ; n < _DrawCount ; n++ ) {
        SDrawIndexedIndirectCmd const *cmd = ( _Stride != 0 ) ?
            (SDrawIndexedIndirectCmd const *)((uintptr_t)_Cmds + n * _Stride) : ( (SDrawIndexedIndirectCmd const *)_Cmds + n );
        glDrawElementsInstancedBaseVertexBaseInstance(CurrentPipeline->PrimitiveTopology,
                                                      cmd->IndexCountPerInstance,
                                                      IndexBufferType,
                                                      reinterpret_cast<const GLubyte *>( 0 ) + cmd->StartIndexLocation * IndexBufferTypeSizeOf + IndexBufferOffset,
                                                      cmd->InstanceCount,
                                                      cmd->BaseVertexLocation,
                                                      cmd->StartInstanceLocation);
    }
#    endif
}
#endif

void AImmediateContextGLImpl::MultiDrawIndirect(unsigned int _DrawCount, IBuffer* _DrawIndirectBuffer, unsigned int _AlignedByteOffset, unsigned int _Stride)
{
    VerifyContext();

    HK_ASSERT(CurrentPipeline != nullptr);

    GLuint handle = _DrawIndirectBuffer->GetHandleNativeGL();
    if (Binding.DrawInderectBuffer != handle)
    {
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, handle);
        Binding.DrawInderectBuffer = handle;
    }

    UpdateShaderBindings();
    UpdateVertexBuffers();

    glMultiDrawArraysIndirect(CurrentPipeline->PrimitiveTopology,
                              reinterpret_cast<const GLubyte*>(0) + _AlignedByteOffset,
                              _DrawCount,
                              _Stride); // 4.3 or GL_ARB_multi_draw_indirect
}

void AImmediateContextGLImpl::MultiDrawIndexedIndirect(unsigned int _DrawCount, IBuffer* _DrawIndirectBuffer, unsigned int _AlignedByteOffset, unsigned int _Stride)
{
    VerifyContext();

    HK_ASSERT(CurrentPipeline != nullptr);

    GLuint handle = _DrawIndirectBuffer->GetHandleNativeGL();
    if (Binding.DrawInderectBuffer != handle)
    {
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, handle);
        Binding.DrawInderectBuffer = handle;
    }

    UpdateShaderBindings();
    UpdateVertexAndIndexBuffers();

    glMultiDrawElementsIndirect(CurrentPipeline->PrimitiveTopology,
                                IndexBufferType,
                                reinterpret_cast<const GLubyte*>(0) + _AlignedByteOffset,
                                _DrawCount,
                                _Stride); // 4.3
}

void AImmediateContextGLImpl::DispatchCompute(unsigned int _ThreadGroupCountX,
                                              unsigned int _ThreadGroupCountY,
                                              unsigned int _ThreadGroupCountZ)
{

    VerifyContext();

    // Must be: ThreadGroupCount <= GL_MAX_COMPUTE_WORK_GROUP_COUNT

    glDispatchCompute(_ThreadGroupCountX, _ThreadGroupCountY, _ThreadGroupCountZ); // 4.3 or GL_ARB_compute_shader
}

void AImmediateContextGLImpl::DispatchCompute(SDispatchIndirectCmd const* _Cmd)
{
    VerifyContext();

    if (Binding.DispatchIndirectBuffer != 0)
    {
        glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0);
        Binding.DispatchIndirectBuffer = 0;
    }

    glDispatchComputeIndirect((GLintptr)_Cmd); // 4.3 or GL_ARB_compute_shader

    // or
    //glDispatchCompute( _Cmd->ThreadGroupCountX, _Cmd->ThreadGroupCountY, _Cmd->ThreadGroupCountZ ); // 4.3 or GL_ARB_compute_shader
}

void AImmediateContextGLImpl::DispatchComputeIndirect(IBuffer*     _DispatchIndirectBuffer,
                                                      unsigned int _AlignedByteOffset)
{
    VerifyContext();

    GLuint handle = _DispatchIndirectBuffer->GetHandleNativeGL();
    if (Binding.DispatchIndirectBuffer != handle)
    {
        glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, handle);
        Binding.DispatchIndirectBuffer = handle;
    }

    GLubyte* offset = reinterpret_cast<GLubyte*>(0) + _AlignedByteOffset;
    glDispatchComputeIndirect((GLintptr)offset); // 4.3 or GL_ARB_compute_shader
}

void AImmediateContextGLImpl::BeginQuery(IQueryPool* _QueryPool, uint32_t _QueryID, uint32_t _StreamIndex)
{
    VerifyContext();

    AQueryPoolGLImpl* queryPool = static_cast<AQueryPoolGLImpl*>(_QueryPool);

    HK_ASSERT(_QueryID < queryPool->PoolSize);
    HK_ASSERT(queryPool->QueryType != QUERY_TYPE_TIMESTAMP);

    if (CurrentQueryUID[queryPool->QueryType] != 0)
    {
        LOG("AImmediateContextGLImpl::BeginQuery: missing EndQuery() for the target\n");
        return;
    }

    CurrentQueryUID[queryPool->QueryType] = queryPool->GetUID();

    if (_StreamIndex == 0)
    {
        glBeginQuery(TableQueryTarget[queryPool->QueryType], queryPool->IdPool[_QueryID]); // 2.0
    }
    else
    {
        glBeginQueryIndexed(TableQueryTarget[queryPool->QueryType], queryPool->IdPool[_QueryID], _StreamIndex); // 4.0
    }
}

void AImmediateContextGLImpl::EndQuery(IQueryPool* _QueryPool, uint32_t _StreamIndex)
{
    VerifyContext();

    AQueryPoolGLImpl* queryPool = static_cast<AQueryPoolGLImpl*>(_QueryPool);

    HK_ASSERT(queryPool->QueryType != QUERY_TYPE_TIMESTAMP);

    if (CurrentQueryUID[queryPool->QueryType] != _QueryPool->GetUID())
    {
        LOG("AImmediateContextGLImpl::EndQuery: missing BeginQuery() for the target\n");
        return;
    }

    CurrentQueryUID[queryPool->QueryType] = 0;

    if (_StreamIndex == 0)
    {
        glEndQuery(TableQueryTarget[queryPool->QueryType]); // 2.0
    }
    else
    {
        glEndQueryIndexed(TableQueryTarget[queryPool->QueryType], _StreamIndex); // 4.0
    }
}

void AImmediateContextGLImpl::RecordTimeStamp(IQueryPool* _QueryPool, uint32_t _QueryID)
{
    VerifyContext();

    AQueryPoolGLImpl* queryPool = static_cast<AQueryPoolGLImpl*>(_QueryPool);

    HK_ASSERT(_QueryID < queryPool->PoolSize);

    if (queryPool->QueryType != QUERY_TYPE_TIMESTAMP)
    {
        LOG("AImmediateContextGLImpl::RecordTimeStamp: query pool target must be QUERY_TYPE_TIMESTAMP\n");
        return;
    }

    glQueryCounter(queryPool->IdPool[_QueryID], GL_TIMESTAMP);
}

void AImmediateContextGLImpl::BeginConditionalRender(IQueryPool* _QueryPool, uint32_t _QueryID, CONDITIONAL_RENDER_MODE _Mode)
{
    VerifyContext();

    AQueryPoolGLImpl* queryPool = static_cast<AQueryPoolGLImpl*>(_QueryPool);

    HK_ASSERT(_QueryID < queryPool->PoolSize);
    glBeginConditionalRender(queryPool->IdPool[_QueryID], TableConditionalRenderMode[_Mode]); // 4.4 (with some flags 3.0)
}

void AImmediateContextGLImpl::EndConditionalRender()
{
    VerifyContext();

    glEndConditionalRender(); // 3.0
}

void AImmediateContextGLImpl::CopyQueryPoolResultsAvailable(IQueryPool* _QueryPool,
                                                            uint32_t    _FirstQuery,
                                                            uint32_t    _QueryCount,
                                                            IBuffer*    _DstBuffer,
                                                            size_t      _DstOffst,
                                                            size_t      _DstStride,
                                                            bool        _QueryResult64Bit)
{
    VerifyContext();

    AQueryPoolGLImpl* queryPool = static_cast<AQueryPoolGLImpl*>(_QueryPool);

    HK_ASSERT(_FirstQuery + _QueryCount <= queryPool->PoolSize);

    const GLuint bufferId   = _DstBuffer->GetHandleNativeGL();
    const size_t bufferSize = _DstBuffer->GetDesc().SizeInBytes;

    if (_QueryResult64Bit)
    {

        HK_ASSERT((_DstStride & ~(size_t)7) == _DstStride); // check stride must be multiples of 8

        for (uint32_t index = 0; index < _QueryCount; index++)
        {

            if (_DstOffst + sizeof(uint64_t) > bufferSize)
            {
                LOG("AImmediateContextGLImpl::CopyQueryPoolResultsAvailable: out of buffer size\n");
                break;
            }

            glGetQueryBufferObjectui64v(queryPool->IdPool[_FirstQuery + index], bufferId, GL_QUERY_RESULT_AVAILABLE, _DstOffst); // 4.5

            _DstOffst += _DstStride;
        }
    }
    else
    {
        HK_ASSERT((_DstStride & ~(size_t)3) == _DstStride); // check stride must be multiples of 4

        for (uint32_t index = 0; index < _QueryCount; index++)
        {

            if (_DstOffst + sizeof(uint32_t) > bufferSize)
            {
                LOG("AImmediateContextGLImpl::CopyQueryPoolResultsAvailable: out of buffer size\n");
                break;
            }

            glGetQueryBufferObjectuiv(queryPool->IdPool[_FirstQuery + index], bufferId, GL_QUERY_RESULT_AVAILABLE, _DstOffst); // 4.5

            _DstOffst += _DstStride;
        }
    }
}

void AImmediateContextGLImpl::CopyQueryPoolResults(IQueryPool*        _QueryPool,
                                                   uint32_t           _FirstQuery,
                                                   uint32_t           _QueryCount,
                                                   IBuffer*           _DstBuffer,
                                                   size_t             _DstOffst,
                                                   size_t             _DstStride,
                                                   QUERY_RESULT_FLAGS _Flags)
{
    VerifyContext();

    AQueryPoolGLImpl* queryPool = static_cast<AQueryPoolGLImpl*>(_QueryPool);

    HK_ASSERT(_FirstQuery + _QueryCount <= queryPool->PoolSize);

    const GLuint bufferId   = _DstBuffer->GetHandleNativeGL();
    const size_t bufferSize = _DstBuffer->GetDesc().SizeInBytes;

    GLenum pname = (_Flags & QUERY_RESULT_WAIT_BIT) ? GL_QUERY_RESULT : GL_QUERY_RESULT_NO_WAIT;

    if (_Flags & QUERY_RESULT_WITH_AVAILABILITY_BIT)
    {
        LOG("AImmediateContextGLImpl::CopyQueryPoolResults: ignoring flag QUERY_RESULT_WITH_AVAILABILITY_BIT. Use CopyQueryPoolResultsAvailable to get available status.\n");
    }

    if (_Flags & QUERY_RESULT_64_BIT)
    {

        HK_ASSERT((_DstStride & ~(size_t)7) == _DstStride); // check stride must be multiples of 8

        for (uint32_t index = 0; index < _QueryCount; index++)
        {

            if (_DstOffst + sizeof(uint64_t) > bufferSize)
            {
                LOG("AImmediateContextGLImpl::CopyQueryPoolResults: out of buffer size\n");
                break;
            }

            glGetQueryBufferObjectui64v(queryPool->IdPool[_FirstQuery + index], bufferId, pname, _DstOffst); // 4.5

            _DstOffst += _DstStride;
        }
    }
    else
    {
        HK_ASSERT((_DstStride & ~(size_t)3) == _DstStride); // check stride must be multiples of 4

        for (uint32_t index = 0; index < _QueryCount; index++)
        {

            if (_DstOffst + sizeof(uint32_t) > bufferSize)
            {
                LOG("AImmediateContextGLImpl::CopyQueryPoolResults: out of buffer size\n");
                break;
            }

            glGetQueryBufferObjectuiv(queryPool->IdPool[_FirstQuery + index], bufferId, pname, _DstOffst); // 4.5

            _DstOffst += _DstStride;
        }
    }
}

void AImmediateContextGLImpl::BeginRenderPass(SRenderPassBeginGL const& _RenderPassBegin)
{
    VerifyContext();

    HK_ASSERT(CurrentRenderPass == nullptr);

    CurrentFramebuffer          = _RenderPassBegin.pFramebuffer;
    CurrentRenderPass           = _RenderPassBegin.pRenderPass;
    CurrentSubpass              = 0;
    CurrentRenderPassRenderArea = _RenderPassBegin.RenderArea;
    CurrentPipeline             = nullptr;

    for (int i = 0; i < CurrentRenderPass->GetColorAttachments().Size(); i++)
    {
        ColorAttachmentClearValues[i] = CurrentRenderPass->GetColorAttachments()[i].ClearValue.Color;
    }

    if (CurrentRenderPass->HasDepthStencilAttachment())
    {
        DepthStencilAttachmentClearValue = CurrentRenderPass->GetDepthStencilAttachment().ClearValue.DepthStencil;
    }

    for (int i = 0; i < CurrentRenderPass->GetColorAttachments().Size(); i++)
    {
        ColorAttachmentSubpassUse[i].FirstSubpass = -1;
        ColorAttachmentSubpassUse[i].LastSubpass  = -1;
    }
    DepthStencilAttachmentSubpassUse.FirstSubpass = -1;
    DepthStencilAttachmentSubpassUse.LastSubpass  = -1;

    for (int subpassNum = 0; subpassNum < CurrentRenderPass->GetSubpasses().Size(); subpassNum++)
    {
        ASubpassInfo const& subpass = CurrentRenderPass->GetSubpasses()[subpassNum];

        if (!subpass.Refs.IsEmpty())
        {
            for (uint32_t i = 0; i < subpass.Refs.Size(); i++)
            {
                uint32_t attachmentNum = subpass.Refs[i].Attachment;

                if (ColorAttachmentSubpassUse[attachmentNum].FirstSubpass == -1)
                {
                    ColorAttachmentSubpassUse[attachmentNum].FirstSubpass = subpassNum;
                }
                ColorAttachmentSubpassUse[attachmentNum].LastSubpass = subpassNum;
            }
        }
    }

    // FIXME: Is it correct for depthstencil attachment?
    if (CurrentRenderPass->HasDepthStencilAttachment())
    {
        DepthStencilAttachmentSubpassUse.FirstSubpass = 0;
        DepthStencilAttachmentSubpassUse.LastSubpass  = CurrentRenderPass->GetSubpasses().Size() - 1;
    }

    BeginSubpass();
}

void AImmediateContextGLImpl::UpdateDrawBuffers()
{
    VerifyContext();

    HK_ASSERT(CurrentRenderPass != nullptr);

    const GLuint framebufferId = Binding.DrawFramebuffer;

    if (framebufferId == 0)
    {
        //glDrawBuffer(GL_BACK);
        //glNamedFramebufferDrawBuffer( 0, GL_BACK );
        return;
    }

    ASubpassInfo const& subpass = CurrentRenderPass->GetSubpasses()[CurrentSubpass];

    if (subpass.Refs.Size() > 0)
    {
        for (uint32_t i = 0; i < subpass.Refs.Size(); i++)
        {
            Attachments[i] = GL_COLOR_ATTACHMENT0 + subpass.Refs[i].Attachment;
        }

        glNamedFramebufferDrawBuffers(framebufferId, subpass.Refs.Size(), Attachments);
    }
    else
    {
        glNamedFramebufferDrawBuffer(framebufferId, GL_NONE);
    }
}

void AImmediateContextGLImpl::BeginSubpass()
{
    GLuint framebufferId = CurrentFramebuffer->GetHandleNativeGL();

    if (Binding.DrawFramebuffer != framebufferId)
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferId);

        Binding.DrawFramebuffer       = framebufferId;
        Binding.DrawFramebufferWidth  = CurrentFramebuffer->GetWidth();
        Binding.DrawFramebufferHeight = CurrentFramebuffer->GetHeight();
    }

    UpdateDrawBuffers();

    bool bScissorEnabled    = RasterizerState.bScissorEnable;
    bool bRasterizerDiscard = RasterizerState.bRasterizerDiscard;

    TWeakRef<ITextureView> const* framebufferColorAttachments = CurrentFramebuffer->GetColorAttachments();

    ASubpassInfo const& subpass = CurrentRenderPass->GetSubpasses()[CurrentSubpass];

    for (uint32_t i = 0; i < subpass.Refs.Size(); i++)
    {
        uint32_t attachmentNum = subpass.Refs[i].Attachment;

        STextureAttachment const& attachment = CurrentRenderPass->GetColorAttachments()[attachmentNum];
        ITextureView*             rtv        = framebufferColorAttachments[attachmentNum];

        if (ColorAttachmentSubpassUse[attachmentNum].FirstSubpass == CurrentSubpass && attachment.LoadOp == ATTACHMENT_LOAD_OP_CLEAR)
        {
            if (!bScissorEnabled)
            {
                glEnable(GL_SCISSOR_TEST);
                bScissorEnabled = true;
            }

            SetScissor(CurrentRenderPassRenderArea);

            if (bRasterizerDiscard)
            {
                glDisable(GL_RASTERIZER_DISCARD);
                bRasterizerDiscard = false;
            }

            SRenderTargetBlendingInfo const& currentState = BlendState.RenderTargetSlots[attachmentNum];
            if (currentState.ColorWriteMask != COLOR_WRITE_RGBA)
            {
                glColorMaski(attachmentNum, 1, 1, 1, 1);
            }

            if (framebufferId == 0) // default framebuffer
            {
                glClearNamedFramebufferfv(framebufferId,
                                          GL_COLOR,
                                          0,
                                          ColorAttachmentClearValues[attachmentNum].Float32);
            }
            else
            {
                // Clear attachment
                switch (InternalFormatLUT[rtv->GetDesc().Format].ClearType)
                {
                    case CLEAR_TYPE_FLOAT32:
                        glClearNamedFramebufferfv(framebufferId,
                                                  GL_COLOR,
                                                  attachmentNum,
                                                  ColorAttachmentClearValues[attachmentNum].Float32);
                        break;
                    case CLEAR_TYPE_INT32:
                        glClearNamedFramebufferiv(framebufferId,
                                                  GL_COLOR,
                                                  attachmentNum,
                                                  ColorAttachmentClearValues[attachmentNum].Int32);
                        break;
                    case CLEAR_TYPE_UINT32:
                        glClearNamedFramebufferuiv(framebufferId,
                                                   GL_COLOR,
                                                   attachmentNum,
                                                   ColorAttachmentClearValues[attachmentNum].UInt32);
                        break;
                    default:
                        HK_ASSERT(0);
                }
            }

            // Restore color mask
            if (currentState.ColorWriteMask != COLOR_WRITE_RGBA)
            {
                if (currentState.ColorWriteMask == COLOR_WRITE_DISABLED)
                {
                    glColorMaski(attachmentNum, 0, 0, 0, 0);
                }
                else
                {
                    glColorMaski(attachmentNum,
                                 !!(currentState.ColorWriteMask & COLOR_WRITE_R_BIT),
                                 !!(currentState.ColorWriteMask & COLOR_WRITE_G_BIT),
                                 !!(currentState.ColorWriteMask & COLOR_WRITE_B_BIT),
                                 !!(currentState.ColorWriteMask & COLOR_WRITE_A_BIT));
                }
            }
        }
    }

    if (CurrentRenderPass->HasDepthStencilAttachment())
    {
        STextureAttachment const& attachment = CurrentRenderPass->GetDepthStencilAttachment();
        ITextureView const*       dsv        = CurrentFramebuffer->GetDepthStencilAttachment();

        if (DepthStencilAttachmentSubpassUse.FirstSubpass == CurrentSubpass && attachment.LoadOp == ATTACHMENT_LOAD_OP_CLEAR)
        {
            if (!bScissorEnabled)
            {
                glEnable(GL_SCISSOR_TEST);
                bScissorEnabled = true;
            }

            SetScissor(CurrentRenderPassRenderArea);

            if (bRasterizerDiscard)
            {
                glDisable(GL_RASTERIZER_DISCARD);
                bRasterizerDiscard = false;
            }

            if (DepthStencilState.bDepthWrite == false)
            {
                glDepthMask(1);
            }

            // TODO: table
            switch (InternalFormatLUT[dsv->GetDesc().Format].ClearType)
            {
                case CLEAR_TYPE_STENCIL_ONLY:
                    glClearNamedFramebufferuiv(framebufferId,
                                               GL_STENCIL,
                                               0,
                                               &DepthStencilAttachmentClearValue.Stencil);
                    break;
                case CLEAR_TYPE_DEPTH_ONLY:
                    glClearNamedFramebufferfv(framebufferId,
                                              GL_DEPTH,
                                              0,
                                              &DepthStencilAttachmentClearValue.Depth);
                    break;
                case CLEAR_TYPE_DEPTH_STENCIL:
                    glClearNamedFramebufferfi(framebufferId,
                                              GL_DEPTH_STENCIL,
                                              0,
                                              DepthStencilAttachmentClearValue.Depth,
                                              DepthStencilAttachmentClearValue.Stencil);
                    break;
                default:
                    HK_ASSERT(0);
            }

            if (DepthStencilState.bDepthWrite == false)
            {
                glDepthMask(0);
            }
        }
    }

    // Restore scissor test
    if (bScissorEnabled != RasterizerState.bScissorEnable)
    {
        if (RasterizerState.bScissorEnable)
        {
            glEnable(GL_SCISSOR_TEST);
        }
        else
        {
            glDisable(GL_SCISSOR_TEST);
        }
    }

    // Restore rasterizer discard
    if (bRasterizerDiscard != RasterizerState.bRasterizerDiscard)
    {
        if (RasterizerState.bRasterizerDiscard)
        {
            glEnable(GL_RASTERIZER_DISCARD);
        }
        else
        {
            glDisable(GL_RASTERIZER_DISCARD);
        }
    }
}

void AImmediateContextGLImpl::EndSubpass()
{
    VerifyContext();

    HK_ASSERT(CurrentRenderPass != nullptr);

    TArray<GLenum, MAX_COLOR_ATTACHMENTS + 1> attachments;
    int                                       numAttachments = 0;

    ASubpassInfo const& subpass = CurrentRenderPass->GetSubpasses()[CurrentSubpass];

    for (int i = 0; i < subpass.Refs.Size(); i++)
    {
        int attachmentNum = subpass.Refs[i].Attachment;

        if (ColorAttachmentSubpassUse[attachmentNum].LastSubpass == CurrentSubpass &&
            CurrentRenderPass->GetColorAttachments()[attachmentNum].StoreOp == ATTACHMENT_STORE_OP_DONT_CARE)
        {
            if (CurrentFramebuffer->GetHandleNativeGL() == 0)
            {
                HK_ASSERT(subpass.Refs.Size() == 1);
                attachments[numAttachments++] = GL_COLOR;
            }
            else
            {
                attachments[numAttachments++] = GL_COLOR_ATTACHMENT0 + attachmentNum;
            }
        }
    }

    if (CurrentRenderPass->HasDepthStencilAttachment())
    {
        if (DepthStencilAttachmentSubpassUse.LastSubpass == CurrentSubpass &&
            CurrentRenderPass->GetDepthStencilAttachment().StoreOp == ATTACHMENT_STORE_OP_DONT_CARE)
        {
            HK_ASSERT(CurrentFramebuffer->GetDepthStencilAttachment());

            switch (CurrentFramebuffer->GetDepthStencilAttachment()->GetDesc().Format)
            {
                case TEXTURE_FORMAT_STENCIL1:
                case TEXTURE_FORMAT_STENCIL4:
                case TEXTURE_FORMAT_STENCIL8:
                case TEXTURE_FORMAT_STENCIL16:
                    if (CurrentFramebuffer->GetHandleNativeGL() == 0)
                    {
                        attachments[numAttachments++] = GL_STENCIL;
                    }
                    else
                    {
                        attachments[numAttachments++] = GL_STENCIL_ATTACHMENT;
                    }
                    break;
                case TEXTURE_FORMAT_DEPTH16:
                case TEXTURE_FORMAT_DEPTH24:
                case TEXTURE_FORMAT_DEPTH32:
                    if (CurrentFramebuffer->GetHandleNativeGL() == 0)
                    {
                        attachments[numAttachments++] = GL_DEPTH;
                    }
                    else
                    {
                        attachments[numAttachments++] = GL_DEPTH_ATTACHMENT;
                    }
                    break;
                case TEXTURE_FORMAT_DEPTH24_STENCIL8:
                case TEXTURE_FORMAT_DEPTH32F_STENCIL8:
                    if (CurrentFramebuffer->GetHandleNativeGL() == 0)
                    {
                        attachments[numAttachments++] = GL_DEPTH;
                        attachments[numAttachments++] = GL_STENCIL;
                    }
                    else
                    {
                        attachments[numAttachments++] = GL_DEPTH_STENCIL_ATTACHMENT;
                    }
                    break;
                default:
                    HK_ASSERT(0);
            }
        }
    }

    if (numAttachments > 0)
    {
        if (CurrentRenderPassRenderArea.X == 0 &&
            CurrentRenderPassRenderArea.Y == 0 &&
            CurrentRenderPassRenderArea.Width == CurrentFramebuffer->GetWidth() &&
            CurrentRenderPassRenderArea.Height == CurrentFramebuffer->GetHeight())
        {
            glInvalidateNamedFramebufferData(CurrentFramebuffer->GetHandleNativeGL(), numAttachments, attachments.ToPtr());
        }
        else
        {
            glInvalidateNamedFramebufferSubData(CurrentFramebuffer->GetHandleNativeGL(), numAttachments, attachments.ToPtr(),
                                                CurrentRenderPassRenderArea.X, CurrentRenderPassRenderArea.Y,
                                                CurrentRenderPassRenderArea.Width, CurrentRenderPassRenderArea.Height);
        }
    }
}

void AImmediateContextGLImpl::NextSubpass()
{
    HK_ASSERT(CurrentRenderPass);
    HK_ASSERT(CurrentSubpass + 1 < CurrentRenderPass->GetSubpasses().Size());

    if (CurrentSubpass + 1 < CurrentRenderPass->GetSubpasses().Size())
    {
        EndSubpass();

        CurrentSubpass++;
        BeginSubpass();
    }
}

void AImmediateContextGLImpl::EndRenderPass()
{
    EndSubpass();

    CurrentRenderPass  = nullptr;
    CurrentFramebuffer = nullptr;
}

void AImmediateContextGLImpl::BindTransformFeedback(ITransformFeedback* _TransformFeedback)
{
    VerifyContext();

    // FIXME: Move transform feedback to Pipeline? Call glBindTransformFeedback in BindPipeline()?
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, _TransformFeedback->GetHandleNativeGL());
}

void AImmediateContextGLImpl::BeginTransformFeedback(PRIMITIVE_TOPOLOGY _OutputPrimitive)
{
    VerifyContext();

    GLenum topology = GL_POINTS;

    if (_OutputPrimitive <= PRIMITIVE_TRIANGLE_STRIP_ADJ)
    {
        topology = PrimitiveTopologyLUT[_OutputPrimitive];
    }

    glBeginTransformFeedback(topology); // 3.0
}

void AImmediateContextGLImpl::ResumeTransformFeedback()
{
    VerifyContext();

    glResumeTransformFeedback();
}

void AImmediateContextGLImpl::PauseTransformFeedback()
{
    VerifyContext();

    glPauseTransformFeedback();
}

void AImmediateContextGLImpl::EndTransformFeedback()
{
    VerifyContext();

    glEndTransformFeedback(); // 3.0
}

SyncObject AImmediateContextGLImpl::FenceSync()
{
    VerifyContext();

    SyncObject sync = reinterpret_cast<SyncObject>(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));
    return sync;
}

void AImmediateContextGLImpl::RemoveSync(SyncObject _Sync)
{
    VerifyContext();

    if (_Sync)
    {
        glDeleteSync(reinterpret_cast<GLsync>(_Sync));
    }
}

CLIENT_WAIT_STATUS AImmediateContextGLImpl::ClientWait(SyncObject _Sync, uint64_t _TimeOutNanoseconds)
{
    VerifyContext();

    static_assert(0xFFFFFFFFFFFFFFFF == GL_TIMEOUT_IGNORED, "Constant check");
    return static_cast<CLIENT_WAIT_STATUS>(
        glClientWaitSync(reinterpret_cast<GLsync>(_Sync), GL_SYNC_FLUSH_COMMANDS_BIT, _TimeOutNanoseconds) - GL_ALREADY_SIGNALED);
}

void AImmediateContextGLImpl::ServerWait(SyncObject _Sync)
{
    VerifyContext();

    glWaitSync(reinterpret_cast<GLsync>(_Sync), 0, GL_TIMEOUT_IGNORED);
}

bool AImmediateContextGLImpl::IsSignaled(SyncObject _Sync)
{
    VerifyContext();

    GLint value;
    glGetSynciv(reinterpret_cast<GLsync>(_Sync),
                GL_SYNC_STATUS,
                sizeof(GLint),
                nullptr,
                &value);
    return value == GL_SIGNALED;
}

void AImmediateContextGLImpl::Flush()
{
    VerifyContext();

    glFlush();
}

void AImmediateContextGLImpl::Barrier(int _BarrierBits)
{
    VerifyContext();

    glMemoryBarrier(_BarrierBits); // 4.2
}

void AImmediateContextGLImpl::BarrierByRegion(int _BarrierBits)
{
    VerifyContext();

    glMemoryBarrierByRegion(_BarrierBits); // 4.5
}

void AImmediateContextGLImpl::TextureBarrier()
{
    VerifyContext();

    glTextureBarrier(); // 4.5
}

void AImmediateContextGLImpl::DynamicState_BlendingColor(const float _ConstantColor[4])
{
    VerifyContext();

    constexpr const float defaultColor[4] = {0, 0, 0, 0};

    // Validate blend color
    _ConstantColor = _ConstantColor ? _ConstantColor : defaultColor;

    // Apply blend color
    bool isColorChanged = !BlendCompareColor(BlendColor, _ConstantColor);
    if (isColorChanged)
    {
        glBlendColor(_ConstantColor[0], _ConstantColor[1], _ConstantColor[2], _ConstantColor[3]);
        Platform::Memcpy(BlendColor, _ConstantColor, sizeof(BlendColor));
    }
}

void AImmediateContextGLImpl::DynamicState_SampleMask(const uint32_t _SampleMask[4])
{
    VerifyContext();

    // Apply sample mask
    if (_SampleMask)
    {
        static_assert(sizeof(GLbitfield) == sizeof(_SampleMask[0]), "Type Sizeof check");
        if (_SampleMask[0] != SampleMask[0])
        {
            glSampleMaski(0, _SampleMask[0]);
            SampleMask[0] = _SampleMask[0];
        }
        if (_SampleMask[1] != SampleMask[1])
        {
            glSampleMaski(1, _SampleMask[1]);
            SampleMask[1] = _SampleMask[1];
        }
        if (_SampleMask[2] != SampleMask[2])
        {
            glSampleMaski(2, _SampleMask[2]);
            SampleMask[2] = _SampleMask[2];
        }
        if (_SampleMask[3] != SampleMask[3])
        {
            glSampleMaski(3, _SampleMask[3]);
            SampleMask[3] = _SampleMask[3];
        }

        if (!bSampleMaskEnabled)
        {
            glEnable(GL_SAMPLE_MASK);
            bSampleMaskEnabled = true;
        }
    }
    else
    {
        if (bSampleMaskEnabled)
        {
            glDisable(GL_SAMPLE_MASK);
            bSampleMaskEnabled = false;
        }
    }
}

void AImmediateContextGLImpl::DynamicState_StencilRef(uint32_t _StencilRef)
{
    VerifyContext();

    HK_ASSERT(CurrentPipeline != nullptr);

    if (Binding.DepthStencilState == CurrentPipeline->DepthStencilState)
    {

        if (StencilRef != _StencilRef)
        {
            // Update stencil ref

            SDepthStencilStateInfo const& desc = *CurrentPipeline->DepthStencilState;

#if 0
            if ( desc.FrontFace.StencilFunc == desc.BackFace.StencilFunc ) {
                glStencilFunc( ComparisonFuncLUT[ desc.FrontFace.StencilFunc ],
                               _StencilRef,
                               desc.StencilReadMask );
            }
            else
#endif
            {
                if (desc.FrontFace.StencilFunc == desc.BackFace.StencilFunc)
                {
                    glStencilFuncSeparate(GL_FRONT_AND_BACK,
                                          ComparisonFuncLUT[desc.FrontFace.StencilFunc],
                                          _StencilRef,
                                          desc.StencilReadMask);
                }
                else
                {
                    glStencilFuncSeparate(GL_FRONT,
                                          ComparisonFuncLUT[desc.FrontFace.StencilFunc],
                                          _StencilRef,
                                          desc.StencilReadMask);

                    glStencilFuncSeparate(GL_BACK,
                                          ComparisonFuncLUT[desc.BackFace.StencilFunc],
                                          _StencilRef,
                                          desc.StencilReadMask);
                }
            }

            StencilRef = _StencilRef;
        }
    }
}

void AImmediateContextGLImpl::CopyBuffer(IBuffer* _SrcBuffer, IBuffer* _DstBuffer)
{
    VerifyContext();

    size_t size = _SrcBuffer->GetDesc().SizeInBytes;
    HK_ASSERT(size == _DstBuffer->GetDesc().SizeInBytes);

    glCopyNamedBufferSubData(_SrcBuffer->GetHandleNativeGL(),
                             _DstBuffer->GetHandleNativeGL(),
                             0,
                             0,
                             size); // 4.5 or GL_ARB_direct_state_access
}

void AImmediateContextGLImpl::CopyBufferRange(IBuffer* _SrcBuffer, IBuffer* _DstBuffer, uint32_t _NumRanges, SBufferCopy const* _Ranges)
{
    VerifyContext();

    for (SBufferCopy const* range = _Ranges; range < &_Ranges[_NumRanges]; range++)
    {
        glCopyNamedBufferSubData(_SrcBuffer->GetHandleNativeGL(),
                                 _DstBuffer->GetHandleNativeGL(),
                                 range->SrcOffset,
                                 range->DstOffset,
                                 range->SizeInBytes); // 4.5 or GL_ARB_direct_state_access
    }


    /*
    // Other path:

    glBindBuffer( GL_COPY_READ_BUFFER, srcId );
    glBindBuffer( GL_COPY_WRITE_BUFFER, dstId );

    glCopyBufferSubData( GL_COPY_READ_BUFFER,
                         GL_COPY_WRITE_BUFFER,
                         _SourceOffset,
                         _DstOffset,
                         _SizeInBytes );  // 3.1
    */
}

// Only for TEXTURE_1D
bool AImmediateContextGLImpl::CopyBufferToTexture1D(IBuffer const* _SrcBuffer,
                                                    ITexture*      _DstTexture,
                                                    uint16_t       _MipLevel,
                                                    uint16_t       _OffsetX,
                                                    uint16_t       _DimensionX,
                                                    size_t         _CompressedDataSizeInBytes, // Only for compressed images
                                                    DATA_FORMAT    _Format,
                                                    size_t         _SourceByteOffset,
                                                    unsigned int   _Alignment)
{
    VerifyContext();

    if (_DstTexture->GetDesc().Type != TEXTURE_1D)
    {
        return false;
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _SrcBuffer->GetHandleNativeGL());

    // TODO: check this

    GLuint textureId = _DstTexture->GetHandleNativeGL();

    UnpackAlignment(_Alignment);

    if (_DstTexture->IsCompressed())
    {
        glCompressedTextureSubImage1D(textureId,
                                      _MipLevel,
                                      _OffsetX,
                                      _DimensionX,
                                      InternalFormatLUT[_DstTexture->GetDesc().Format].InternalFormat,
                                      (GLsizei)_CompressedDataSizeInBytes,
                                      ((uint8_t*)0) + _SourceByteOffset);
    }
    else
    {
        glTextureSubImage1D(textureId,
                            _MipLevel,
                            _OffsetX,
                            _DimensionX,
                            TypeLUT[_Format].FormatRGB,
                            TypeLUT[_Format].Type,
                            ((uint8_t*)0) + _SourceByteOffset);
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    return true;
}

// Only for TEXTURE_2D, TEXTURE_1D_ARRAY, TEXTURE_CUBE_MAP
bool AImmediateContextGLImpl::CopyBufferToTexture2D(IBuffer const* _SrcBuffer,
                                                    ITexture*      _DstTexture,
                                                    uint16_t       _MipLevel,
                                                    uint16_t       _OffsetX,
                                                    uint16_t       _OffsetY,
                                                    uint16_t       _DimensionX,
                                                    uint16_t       _DimensionY,
                                                    uint16_t       _CubeFaceIndex,             // only for TEXTURE_CUBE_MAP
                                                    uint16_t       _NumCubeFaces,              // only for TEXTURE_CUBE_MAP
                                                    size_t         _CompressedDataSizeInBytes, // Only for compressed images
                                                    DATA_FORMAT    _Format,
                                                    size_t         _SourceByteOffset,
                                                    unsigned int   _Alignment)
{
    VerifyContext();

    if (_DstTexture->GetDesc().Type != TEXTURE_2D && _DstTexture->GetDesc().Type != TEXTURE_1D_ARRAY && _DstTexture->GetDesc().Type != TEXTURE_CUBE_MAP)
    {
        return false;
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _SrcBuffer->GetHandleNativeGL());

    // TODO: check this

    GLuint textureId = _DstTexture->GetHandleNativeGL();

    UnpackAlignment(_Alignment);

    if (_DstTexture->GetDesc().Type == TEXTURE_CUBE_MAP)
    {

        GLint  i;
        GLuint currentBinding;

        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &i);

        currentBinding = i;

        if (currentBinding != textureId)
        {
            glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
        }

        // TODO: Учитывать _NumCubeFaces!!!

        if (_DstTexture->IsCompressed())
        {
            glCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + _CubeFaceIndex,
                                      _MipLevel,
                                      _OffsetX,
                                      _OffsetY,
                                      _DimensionX,
                                      _DimensionY,
                                      InternalFormatLUT[_DstTexture->GetDesc().Format].InternalFormat,
                                      (GLsizei)_CompressedDataSizeInBytes,
                                      ((uint8_t*)0) + _SourceByteOffset);
        }
        else
        {
            glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + _CubeFaceIndex,
                            _MipLevel,
                            _OffsetX,
                            _OffsetY,
                            _DimensionX,
                            _DimensionY,
                            TypeLUT[_Format].FormatRGB,
                            TypeLUT[_Format].Type,
                            ((uint8_t*)0) + _SourceByteOffset);
        }

        if (currentBinding != textureId)
        {
            glBindTexture(GL_TEXTURE_CUBE_MAP, currentBinding);
        }
    }
    else
    {
        if (_DstTexture->IsCompressed())
        {
            glCompressedTextureSubImage2D(textureId,
                                          _MipLevel,
                                          _OffsetX,
                                          _OffsetY,
                                          _DimensionX,
                                          _DimensionY,
                                          InternalFormatLUT[_DstTexture->GetDesc().Format].InternalFormat,
                                          (GLsizei)_CompressedDataSizeInBytes,
                                          ((uint8_t*)0) + _SourceByteOffset);
        }
        else
        {
            glTextureSubImage2D(textureId,
                                _MipLevel,
                                _OffsetX,
                                _OffsetY,
                                _DimensionX,
                                _DimensionY,
                                TypeLUT[_Format].FormatRGB,
                                TypeLUT[_Format].Type,
                                ((uint8_t*)0) + _SourceByteOffset);
        }
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    return true;
}

// Only for TEXTURE_3D, TEXTURE_2D_ARRAY
bool AImmediateContextGLImpl::CopyBufferToTexture3D(IBuffer const* _SrcBuffer,
                                                    ITexture*      _DstTexture,
                                                    uint16_t       _MipLevel,
                                                    uint16_t       _OffsetX,
                                                    uint16_t       _OffsetY,
                                                    uint16_t       _OffsetZ,
                                                    uint16_t       _DimensionX,
                                                    uint16_t       _DimensionY,
                                                    uint16_t       _DimensionZ,
                                                    size_t         _CompressedDataSizeInBytes, // Only for compressed images
                                                    DATA_FORMAT    _Format,
                                                    size_t         _SourceByteOffset,
                                                    unsigned int   _Alignment)
{
    VerifyContext();

    if (_DstTexture->GetDesc().Type != TEXTURE_3D && _DstTexture->GetDesc().Type != TEXTURE_2D_ARRAY)
    {
        return false;
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _SrcBuffer->GetHandleNativeGL());

    // TODO: check this

    GLuint textureId = _DstTexture->GetHandleNativeGL();

    UnpackAlignment(_Alignment);

    if (_DstTexture->IsCompressed())
    {
        glCompressedTextureSubImage3D(textureId,
                                      _MipLevel,
                                      _OffsetX,
                                      _OffsetY,
                                      _OffsetZ,
                                      _DimensionX,
                                      _DimensionY,
                                      _DimensionZ,
                                      InternalFormatLUT[_DstTexture->GetDesc().Format].InternalFormat,
                                      (GLsizei)_CompressedDataSizeInBytes,
                                      ((uint8_t*)0) + _SourceByteOffset);
    }
    else
    {
        glTextureSubImage3D(textureId,
                            _MipLevel,
                            _OffsetX,
                            _OffsetY,
                            _OffsetZ,
                            _DimensionX,
                            _DimensionY,
                            _DimensionZ,
                            TypeLUT[_Format].FormatRGB,
                            TypeLUT[_Format].Type,
                            ((uint8_t*)0) + _SourceByteOffset);
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    return true;
}

// Types supported: TEXTURE_1D, TEXTURE_1D_ARRAY, TEXTURE_2D, TEXTURE_2D_ARRAY, TEXTURE_3D, TEXTURE_CUBE_MAP
bool AImmediateContextGLImpl::CopyBufferToTexture(IBuffer const*      _SrcBuffer,
                                                  ITexture*           _DstTexture,
                                                  STextureRect const& _Rectangle,
                                                  DATA_FORMAT         _Format,
                                                  size_t              _CompressedDataSizeInBytes, // for compressed images
                                                  size_t              _SourceByteOffset,
                                                  unsigned int        _Alignment)
{
    VerifyContext();

    // FIXME: what about multisample textures?

    switch (_DstTexture->GetDesc().Type)
    {
        case TEXTURE_1D:
            return CopyBufferToTexture1D(_SrcBuffer,
                                         _DstTexture,
                                         _Rectangle.Offset.MipLevel,
                                         _Rectangle.Offset.X,
                                         _Rectangle.Dimension.X,
                                         _CompressedDataSizeInBytes,
                                         _Format,
                                         _SourceByteOffset,
                                         _Alignment);
        case TEXTURE_1D_ARRAY:
        case TEXTURE_2D:
            return CopyBufferToTexture2D(_SrcBuffer,
                                         _DstTexture,
                                         _Rectangle.Offset.MipLevel,
                                         _Rectangle.Offset.X,
                                         _Rectangle.Offset.Y,
                                         _Rectangle.Dimension.X,
                                         _Rectangle.Dimension.Y,
                                         0,
                                         0,
                                         _CompressedDataSizeInBytes,
                                         _Format,
                                         _SourceByteOffset,
                                         _Alignment);
        case TEXTURE_2D_ARRAY:
        case TEXTURE_3D:
            return CopyBufferToTexture3D(_SrcBuffer,
                                         _DstTexture,
                                         _Rectangle.Offset.MipLevel,
                                         _Rectangle.Offset.X,
                                         _Rectangle.Offset.Y,
                                         _Rectangle.Offset.Z,
                                         _Rectangle.Dimension.X,
                                         _Rectangle.Dimension.Y,
                                         _Rectangle.Dimension.Z,
                                         _CompressedDataSizeInBytes,
                                         _Format,
                                         _SourceByteOffset,
                                         _Alignment);
        case TEXTURE_CUBE_MAP:
            return CopyBufferToTexture2D(_SrcBuffer,
                                         _DstTexture,
                                         _Rectangle.Offset.MipLevel,
                                         _Rectangle.Offset.X,
                                         _Rectangle.Offset.Y,
                                         _Rectangle.Dimension.X,
                                         _Rectangle.Dimension.Y,
                                         _Rectangle.Offset.Z,
                                         _Rectangle.Dimension.Z,
                                         _CompressedDataSizeInBytes,
                                         _Format,
                                         _SourceByteOffset,
                                         _Alignment);
        case TEXTURE_CUBE_MAP_ARRAY:
            // FIXME: ???
            //return CopyBufferToTexture3D( static_cast< Buffer const * >( _SrcBuffer ),
            //                              static_cast< Texture * >( _DstTexture ),
            //                              _Rectangle.Offset.MipLevel,
            //                              _Rectangle.Offset.X,
            //                              _Rectangle.Offset.Y,
            //                              _Rectangle.Offset.Z,
            //                              _Rectangle.Dimension.X,
            //                              _Rectangle.Dimension.Y,
            //                              _Rectangle.Dimension.Z,
            //                              _CompressedDataSizeInBytes,
            //                              _Format,
            //                              _SourceByteOffset );
            return false;
        case TEXTURE_RECT_GL:
            // FIXME: ???
            return false;
        default:
            break;
    }

    return false;
}

void AImmediateContextGLImpl::CopyTextureToBuffer(ITexture const*     _SrcTexture,
                                                  IBuffer*            _DstBuffer,
                                                  STextureRect const& _Rectangle,
                                                  DATA_FORMAT         _Format,
                                                  size_t              _SizeInBytes,
                                                  size_t              _DstByteOffset,
                                                  unsigned int        _Alignment)
{
    VerifyContext();

    glBindBuffer(GL_PIXEL_PACK_BUFFER, _DstBuffer->GetHandleNativeGL());

    // TODO: check this

    GLuint textureId = _SrcTexture->GetHandleNativeGL();

    PackAlignment(_Alignment);

    if (_SrcTexture->IsCompressed())
    {
        glGetCompressedTextureSubImage(textureId,
                                       _Rectangle.Offset.MipLevel,
                                       _Rectangle.Offset.X,
                                       _Rectangle.Offset.Y,
                                       _Rectangle.Offset.Z,
                                       _Rectangle.Dimension.X,
                                       _Rectangle.Dimension.Y,
                                       _Rectangle.Dimension.Z,
                                       (GLsizei)_SizeInBytes,
                                       ((uint8_t*)0) + _DstByteOffset);
    }
    else
    {
        glGetTextureSubImage(textureId,
                             _Rectangle.Offset.MipLevel,
                             _Rectangle.Offset.X,
                             _Rectangle.Offset.Y,
                             _Rectangle.Offset.Z,
                             _Rectangle.Dimension.X,
                             _Rectangle.Dimension.Y,
                             _Rectangle.Dimension.Z,
                             TypeLUT[_Format].FormatRGB,
                             TypeLUT[_Format].Type,
                             (GLsizei)_SizeInBytes,
                             ((uint8_t*)0) + _DstByteOffset);
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void AImmediateContextGLImpl::CopyTextureRect(ITexture const*    _SrcTexture,
                                              ITexture*          _DstTexture,
                                              uint32_t           _NumCopies,
                                              TextureCopy const* Copies)
{
    VerifyContext();

    // TODO: check this

    GLenum srcTarget = TextureTargetLUT[_SrcTexture->GetDesc().Type].Target;
    GLenum dstTarget = TextureTargetLUT[_DstTexture->GetDesc().Type].Target;
    GLuint srcId     = _SrcTexture->GetHandleNativeGL();
    GLuint dstId     = _DstTexture->GetHandleNativeGL();

    if (_SrcTexture->IsMultisample())
    {
        switch (srcTarget)
        {
            case GL_TEXTURE_2D: srcTarget = GL_TEXTURE_2D_MULTISAMPLE; break;
            case GL_TEXTURE_2D_ARRAY: srcTarget = GL_TEXTURE_2D_MULTISAMPLE_ARRAY; break;
        }
    }
    if (_DstTexture->IsMultisample())
    {
        switch (dstTarget)
        {
            case GL_TEXTURE_2D: dstTarget = GL_TEXTURE_2D_MULTISAMPLE; break;
            case GL_TEXTURE_2D_ARRAY: dstTarget = GL_TEXTURE_2D_MULTISAMPLE_ARRAY; break;
        }
    }

    for (TextureCopy const* copy = Copies; copy < &Copies[_NumCopies]; copy++)
    {
        glCopyImageSubData(srcId,
                           srcTarget,
                           copy->SrcRect.Offset.MipLevel,
                           copy->SrcRect.Offset.X,
                           copy->SrcRect.Offset.Y,
                           copy->SrcRect.Offset.Z,
                           dstId,
                           dstTarget,
                           copy->DstOffset.MipLevel,
                           copy->DstOffset.X,
                           copy->DstOffset.Y,
                           copy->DstOffset.Z,
                           copy->SrcRect.Dimension.X,
                           copy->SrcRect.Dimension.Y,
                           copy->SrcRect.Dimension.Z);
    }
}

bool AImmediateContextGLImpl::CopyFramebufferToTexture(ARenderPassContext&   RenderPassContext,
                                                       ITexture*             _DstTexture,
                                                       int                   _ColorAttachment,
                                                       STextureOffset const& _Offset,
                                                       SRect2D const&        _SrcRect,
                                                       unsigned int          _Alignment)
{
    VerifyContext();

    if (!ChooseReadBuffer(CurrentFramebuffer, _ColorAttachment))
    {
        LOG("AImmediateContextGLImpl::CopyFramebufferToTexture: invalid framebuffer attachment\n");
        return false;
    }

    PackAlignment(_Alignment);

    BindReadFramebuffer(CurrentFramebuffer);

    // TODO: check this function

    if (_DstTexture->IsMultisample())
    {
        switch (_DstTexture->GetDesc().Type)
        {
            case TEXTURE_2D:
            case TEXTURE_2D_ARRAY:
                // FIXME: в спецификации про multisample-типы ничего не сказано
                return false;
            default:;
        }
    }

    switch (_DstTexture->GetDesc().Type)
    {
        case TEXTURE_1D: {
            glCopyTextureSubImage1D(_DstTexture->GetHandleNativeGL(),
                                    _Offset.MipLevel,
                                    _Offset.X,
                                    _SrcRect.X,
                                    _SrcRect.Y,
                                    _SrcRect.Width);
            break;
        }
        case TEXTURE_1D_ARRAY:
        case TEXTURE_2D: {
            glCopyTextureSubImage2D(_DstTexture->GetHandleNativeGL(),
                                    _Offset.MipLevel,
                                    _Offset.X,
                                    _Offset.Y,
                                    _SrcRect.X,
                                    _SrcRect.Y,
                                    _SrcRect.Width,
                                    _SrcRect.Height);
            break;
        }
        case TEXTURE_2D_ARRAY:
        case TEXTURE_3D: {
            glCopyTextureSubImage3D(_DstTexture->GetHandleNativeGL(),
                                    _Offset.MipLevel,
                                    _Offset.X,
                                    _Offset.Y,
                                    _Offset.Z,
                                    _SrcRect.X,
                                    _SrcRect.Y,
                                    _SrcRect.Width,
                                    _SrcRect.Height);
            break;
        }
        case TEXTURE_CUBE_MAP: {
            // FIXME: в спецификации не сказано, как с помощью glCopyTextureSubImage2D
            // скопировать в грань кубической текстуры, поэтому используем обходной путь
            // через glCopyTexSubImage2D

            GLint currentBinding;
            GLint id = _DstTexture->GetHandleNativeGL();

            glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &currentBinding);
            if (currentBinding != id)
            {
                glBindTexture(GL_TEXTURE_CUBE_MAP, id);
            }

            int face = _Offset.Z < 6 ? _Offset.Z : 5; // cubemap face
            glCopyTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                                _Offset.MipLevel,
                                _Offset.X,
                                _Offset.Y,
                                _SrcRect.X,
                                _SrcRect.Y,
                                _SrcRect.Width,
                                _SrcRect.Height);

            if (currentBinding != id)
            {
                glBindTexture(GL_TEXTURE_CUBE_MAP, currentBinding);
            }
            break;
        }
        case TEXTURE_RECT_GL: {
            glCopyTextureSubImage2D(_DstTexture->GetHandleNativeGL(),
                                    0,
                                    _Offset.X,
                                    _Offset.Y,
                                    _SrcRect.X,
                                    _SrcRect.Y,
                                    _SrcRect.Width,
                                    _SrcRect.Height);
            break;
        }
        case TEXTURE_CUBE_MAP_ARRAY:
            // FIXME: в спецификации про этот тип ничего не сказано
            return false;
    }

    return true;
}

void AImmediateContextGLImpl::CopyColorAttachmentToBuffer(ARenderPassContext& RenderPassContext,
                                                          IBuffer*            pDstBuffer,
                                                          int                 SubpassAttachmentRef,
                                                          SRect2D const&      SrcRect,
                                                          FRAMEBUFFER_CHANNEL FramebufferChannel,
                                                          FRAMEBUFFER_OUTPUT  FramebufferOutput,
                                                          COLOR_CLAMP         InColorClamp,
                                                          size_t              SizeInBytes,
                                                          size_t              DstByteOffset,
                                                          unsigned int        Alignment)
{
    VerifyContext();

    auto&    subpasses     = CurrentRenderPass->GetSubpasses();
    uint32_t attachmentNum = subpasses[RenderPassContext.GetSubpassIndex()].Refs[SubpassAttachmentRef].Attachment;

    // TODO: check this

    if (!ChooseReadBuffer(CurrentFramebuffer, attachmentNum))
    {
        LOG("AImmediateContextGLImpl::CopyFramebufferToBuffer: invalid framebuffer attachment\n");
        return;
    }

    BindReadFramebuffer(CurrentFramebuffer);

    PackAlignment(Alignment);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, pDstBuffer->GetHandleNativeGL());

    ClampReadColor(InColorClamp);

    glReadnPixels(SrcRect.X,
                  SrcRect.Y,
                  SrcRect.Width,
                  SrcRect.Height,
                  FramebufferChannelLUT[FramebufferChannel],
                  FramebufferOutputLUT[FramebufferOutput],
                  (GLsizei)SizeInBytes,
                  ((uint8_t*)0) + DstByteOffset);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

static void ChooseDepthStencilAttachmentFormatAndType(TEXTURE_FORMAT TextureFormat, GLenum& Format, GLenum& Type, GLsizei& SizeInBytes)
{
    Format      = GL_DEPTH_STENCIL;
    Type        = GL_FLOAT;
    SizeInBytes = 4; // FIXME
    switch (TextureFormat)
    {
        case TEXTURE_FORMAT_STENCIL1:
        case TEXTURE_FORMAT_STENCIL4:
        case TEXTURE_FORMAT_STENCIL8:
        case TEXTURE_FORMAT_STENCIL16:
            Format = GL_STENCIL_INDEX;
            Type   = GL_UNSIGNED_INT; // FIXME
            break;
        case TEXTURE_FORMAT_DEPTH16:
        case TEXTURE_FORMAT_DEPTH24:
        case TEXTURE_FORMAT_DEPTH32:
            Format = GL_DEPTH_COMPONENT;
            Type   = GL_FLOAT; // FIXME
            break;
        case TEXTURE_FORMAT_DEPTH24_STENCIL8:
        case TEXTURE_FORMAT_DEPTH32F_STENCIL8:
            Format = GL_DEPTH_STENCIL;
            Type   = GL_FLOAT; // FIXME
            break;
        default:
            HK_ASSERT(0);
    }
}

void AImmediateContextGLImpl::CopyDepthAttachmentToBuffer(ARenderPassContext& RenderPassContext,
                                                          IBuffer*            pDstBuffer,
                                                          SRect2D const&      SrcRect,
                                                          size_t              SizeInBytes,
                                                          size_t              DstByteOffset,
                                                          unsigned int        Alignment)
{
    // TODO: check this

    VerifyContext();

    if (!CurrentFramebuffer->HasDepthStencilAttachment())
    {
        LOG("AImmediateContextGLImpl::CopyFramebufferDepthToBuffer: framebuffer has no depth-stencil attachment\n");
        return;
    }

    BindReadFramebuffer(CurrentFramebuffer);

    PackAlignment(Alignment);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, pDstBuffer->GetHandleNativeGL());

    ClampReadColor(COLOR_CLAMP_OFF);

    GLenum  format, type;
    GLsizei size;
    ChooseDepthStencilAttachmentFormatAndType(CurrentFramebuffer->GetDepthStencilAttachment()->GetDesc().Format, format, type, size);

    size *= SrcRect.Width * SrcRect.Height;

    HK_ASSERT(size == SizeInBytes);
    if (size > SizeInBytes)
        size = SizeInBytes;

    glReadnPixels(SrcRect.X,
                  SrcRect.Y,
                  SrcRect.Width,
                  SrcRect.Height,
                  format,
                  type,
                  size,
                  ((uint8_t*)0) + DstByteOffset);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

bool AImmediateContextGLImpl::BlitFramebuffer(ARenderPassContext&   RenderPassContext,
                                              int                   _ColorAttachment,
                                              uint32_t              _NumRectangles,
                                              SBlitRectangle const* _Rectangles,
                                              FRAMEBUFFER_BLIT_MASK _Mask,
                                              bool                  _LinearFilter)
{
    VerifyContext();

    GLbitfield mask = 0;

    if (_Mask & FB_MASK_COLOR)
    {
        mask |= GL_COLOR_BUFFER_BIT;

        if (!ChooseReadBuffer(CurrentFramebuffer, _ColorAttachment))
        {
            LOG("AImmediateContextGLImpl::BlitFramebuffer: invalid framebuffer attachment\n");
            return false;
        }
    }

    if (_Mask & FB_MASK_DEPTH)
    {
        mask |= GL_DEPTH_BUFFER_BIT;
    }

    if (_Mask & FB_MASK_STENCIL)
    {
        mask |= GL_STENCIL_BUFFER_BIT;
    }

    BindReadFramebuffer(CurrentFramebuffer);

    GLenum filter = _LinearFilter ? GL_LINEAR : GL_NEAREST;

    for (SBlitRectangle const* rect = _Rectangles; rect < &_Rectangles[_NumRectangles]; rect++)
    {
        glBlitFramebuffer(rect->SrcX,
                          rect->SrcY,
                          rect->SrcX + rect->SrcWidth,
                          rect->SrcY + rect->SrcHeight,
                          rect->DstX,
                          rect->DstY,
                          rect->DstX + rect->DstWidth,
                          rect->DstY + rect->DstHeight,
                          mask,
                          filter);
    }

    return true;
}

void AImmediateContextGLImpl::ClearBuffer(IBuffer* _Buffer, BUFFER_VIEW_PIXEL_FORMAT _InternalFormat, DATA_FORMAT _Format, SClearValue const* _ClearValue)
{
    VerifyContext();

    // If GL_RASTERIZER_DISCARD enabled glClear## ignored FIX
    if (RasterizerState.bRasterizerDiscard)
    {
        glDisable(GL_RASTERIZER_DISCARD);
    }

    TableInternalPixelFormat const* format = &InternalFormatLUT[_InternalFormat];

    glClearNamedBufferData(_Buffer->GetHandleNativeGL(),
                           format->InternalFormat,
                           TypeLUT[_Format].FormatRGB,
                           TypeLUT[_Format].Type,
                           _ClearValue); // 4.5 or GL_ARB_direct_state_access

    if (RasterizerState.bRasterizerDiscard)
    {
        glEnable(GL_RASTERIZER_DISCARD);
    }

    // It can be also replaced by glClearBufferData
}

void AImmediateContextGLImpl::ClearBufferRange(IBuffer* _Buffer, BUFFER_VIEW_PIXEL_FORMAT _InternalFormat, uint32_t _NumRanges, SBufferClear const* _Ranges, DATA_FORMAT _Format, const SClearValue* _ClearValue)
{
    VerifyContext();

    // If GL_RASTERIZER_DISCARD enabled glClear## ignored FIX
    if (RasterizerState.bRasterizerDiscard)
    {
        glDisable(GL_RASTERIZER_DISCARD);
    }

    TableInternalPixelFormat const* format = &InternalFormatLUT[_InternalFormat];

    for (SBufferClear const* range = _Ranges; range < &_Ranges[_NumRanges]; range++)
    {
        glClearNamedBufferSubData(_Buffer->GetHandleNativeGL(),
                                  format->InternalFormat,
                                  range->Offset,
                                  range->SizeInBytes,
                                  TypeLUT[_Format].FormatRGB,
                                  TypeLUT[_Format].Type,
                                  _ClearValue); // 4.5 or GL_ARB_direct_state_access
    }

    if (RasterizerState.bRasterizerDiscard)
    {
        glEnable(GL_RASTERIZER_DISCARD);
    }

    // It can be also replaced by glClearBufferSubData
}

void AImmediateContextGLImpl::ClearTexture(ITexture* _Texture, uint16_t _MipLevel, DATA_FORMAT _Format, SClearValue const* _ClearValue)
{
    VerifyContext();

    // If GL_RASTERIZER_DISCARD enabled glClear## ignored FIX
    if (RasterizerState.bRasterizerDiscard)
    {
        glDisable(GL_RASTERIZER_DISCARD);
    }

    GLenum format;

    switch (_Texture->GetDesc().Format)
    {
        case TEXTURE_FORMAT_STENCIL1:
        case TEXTURE_FORMAT_STENCIL4:
        case TEXTURE_FORMAT_STENCIL8:
        case TEXTURE_FORMAT_STENCIL16:
            format = GL_STENCIL_INDEX;
            break;
        case TEXTURE_FORMAT_DEPTH16:
        case TEXTURE_FORMAT_DEPTH24:
        case TEXTURE_FORMAT_DEPTH32:
            format = GL_DEPTH_COMPONENT;
            break;
        case TEXTURE_FORMAT_DEPTH24_STENCIL8:
        case TEXTURE_FORMAT_DEPTH32F_STENCIL8:
            format = GL_DEPTH_STENCIL;
            break;
        default:
            format = TypeLUT[_Format].FormatRGB;
            break;
    };

    glClearTexImage(_Texture->GetHandleNativeGL(),
                    _MipLevel,
                    format,
                    TypeLUT[_Format].Type,
                    _ClearValue);

    if (RasterizerState.bRasterizerDiscard)
    {
        glEnable(GL_RASTERIZER_DISCARD);
    }
}

void AImmediateContextGLImpl::ClearTextureRect(ITexture*           _Texture,
                                               uint32_t            _NumRectangles,
                                               STextureRect const* _Rectangles,
                                               DATA_FORMAT         _Format,
                                               SClearValue const*  _ClearValue)
{
    VerifyContext();

    // If GL_RASTERIZER_DISCARD enabled glClear## ignored FIX
    if (RasterizerState.bRasterizerDiscard)
    {
        glDisable(GL_RASTERIZER_DISCARD);
    }

    GLenum format;

    switch (_Texture->GetDesc().Format)
    {
        case TEXTURE_FORMAT_STENCIL1:
        case TEXTURE_FORMAT_STENCIL4:
        case TEXTURE_FORMAT_STENCIL8:
        case TEXTURE_FORMAT_STENCIL16:
            format = GL_STENCIL_INDEX;
            break;
        case TEXTURE_FORMAT_DEPTH16:
        case TEXTURE_FORMAT_DEPTH24:
        case TEXTURE_FORMAT_DEPTH32:
            format = GL_DEPTH_COMPONENT;
            break;
        case TEXTURE_FORMAT_DEPTH24_STENCIL8:
        case TEXTURE_FORMAT_DEPTH32F_STENCIL8:
            format = GL_DEPTH_STENCIL;
            break;
        default:
            format = TypeLUT[_Format].FormatRGB;
            break;
    };

    for (STextureRect const* rect = _Rectangles; rect < &_Rectangles[_NumRectangles]; rect++)
    {
        glClearTexSubImage(_Texture->GetHandleNativeGL(),
                           rect->Offset.MipLevel,
                           rect->Offset.X,
                           rect->Offset.Y,
                           rect->Offset.Z,
                           rect->Dimension.X,
                           rect->Dimension.Y,
                           rect->Dimension.Z,
                           format,
                           TypeLUT[_Format].Type,
                           _ClearValue);
    }

    if (RasterizerState.bRasterizerDiscard)
    {
        glEnable(GL_RASTERIZER_DISCARD);
    }
}

void AImmediateContextGLImpl::ClearAttachments(ARenderPassContext&            RenderPassContext,
                                               unsigned int*                  _ColorAttachments,
                                               unsigned int                   _NumColorAttachments,
                                               SClearColorValue const*        _ColorClearValues,
                                               SClearDepthStencilValue const* _DepthStencilClearValue,
                                               SRect2D const*                 _Rect)
{
    VerifyContext();

    bool bUpdateDrawBuffers = false;

    HK_ASSERT(_NumColorAttachments <= CurrentFramebuffer->GetNumColorAttachments());

    GLuint framebufferId = CurrentFramebuffer->GetHandleNativeGL();

    if (framebufferId == 0)
    {
        // TODO: Clear attachments for default framebuffer
        HK_ASSERT(framebufferId);
    }

    bool    bScissorEnabled    = RasterizerState.bScissorEnable;
    bool    bRasterizerDiscard = RasterizerState.bRasterizerDiscard;
    SRect2D scissorRect;

    // If clear rect was not specified, use renderpass render area
    if (!_Rect && CurrentRenderPass)
    {
        _Rect = &CurrentRenderPassRenderArea;
    }

    if (_Rect)
    {
        if (!bScissorEnabled)
        {
            glEnable(GL_SCISSOR_TEST);
            bScissorEnabled = true;
        }

        // Save current scissor rectangle
        scissorRect = CurrentScissor;

        // Set new scissor rectangle
        SetScissor(*_Rect);
    }
    else
    {
        if (bScissorEnabled)
        {
            glDisable(GL_SCISSOR_TEST);
            bScissorEnabled = false;
        }
    }

    if (bRasterizerDiscard)
    {
        glDisable(GL_RASTERIZER_DISCARD);
        bRasterizerDiscard = false;
    }

    if (_ColorAttachments)
    {
        // We must set draw buffers to clear attachment :(
        for (unsigned int i = 0; i < _NumColorAttachments; i++)
        {
            unsigned int attachmentIndex = _ColorAttachments[i];

            Attachments[i] = GL_COLOR_ATTACHMENT0 + attachmentIndex;
        }
        glNamedFramebufferDrawBuffers(framebufferId, _NumColorAttachments, Attachments);

        // Mark subpass to reset draw buffers
        bUpdateDrawBuffers = true;

        for (unsigned int i = 0; i < _NumColorAttachments; i++)
        {

            unsigned int attachmentIndex = _ColorAttachments[i];

            HK_ASSERT(attachmentIndex < CurrentFramebuffer->GetNumColorAttachments());
            HK_ASSERT(_ColorClearValues);

            ITextureView const* rtv = CurrentFramebuffer->GetColorAttachments()[attachmentIndex];

            SClearColorValue const* clearValue = &_ColorClearValues[i];

            SRenderTargetBlendingInfo const& currentState = BlendState.RenderTargetSlots[attachmentIndex];
            if (currentState.ColorWriteMask != COLOR_WRITE_RGBA)
            {
                glColorMaski(i, 1, 1, 1, 1);
            }

            // Clear attchment
            switch (InternalFormatLUT[rtv->GetDesc().Format].ClearType)
            {
                case CLEAR_TYPE_FLOAT32:
                    glClearNamedFramebufferfv(framebufferId,
                                              GL_COLOR,
                                              i,
                                              clearValue->Float32);
                    break;
                case CLEAR_TYPE_INT32:
                    glClearNamedFramebufferiv(framebufferId,
                                              GL_COLOR,
                                              i,
                                              clearValue->Int32);
                    break;
                case CLEAR_TYPE_UINT32:
                    glClearNamedFramebufferuiv(framebufferId,
                                               GL_COLOR,
                                               i,
                                               clearValue->UInt32);
                    break;
                default:
                    HK_ASSERT(0);
            }

            // Restore color mask
            if (currentState.ColorWriteMask != COLOR_WRITE_RGBA)
            {
                if (currentState.ColorWriteMask == COLOR_WRITE_DISABLED)
                {
                    glColorMaski(i, 0, 0, 0, 0);
                }
                else
                {
                    glColorMaski(i,
                                 !!(currentState.ColorWriteMask & COLOR_WRITE_R_BIT),
                                 !!(currentState.ColorWriteMask & COLOR_WRITE_G_BIT),
                                 !!(currentState.ColorWriteMask & COLOR_WRITE_B_BIT),
                                 !!(currentState.ColorWriteMask & COLOR_WRITE_A_BIT));
                }
            }
        }
    }

    if (_DepthStencilClearValue)
    {
        HK_ASSERT(CurrentFramebuffer->HasDepthStencilAttachment());

        ITextureView const* dsv = CurrentFramebuffer->GetDepthStencilAttachment();

        // TODO: table
        switch (InternalFormatLUT[dsv->GetDesc().Format].ClearType)
        {
            case CLEAR_TYPE_STENCIL_ONLY:
                glClearNamedFramebufferuiv(framebufferId,
                                           GL_STENCIL,
                                           0,
                                           &_DepthStencilClearValue->Stencil);
                break;
            case CLEAR_TYPE_DEPTH_ONLY:
                glClearNamedFramebufferfv(framebufferId,
                                          GL_DEPTH,
                                          0,
                                          &_DepthStencilClearValue->Depth);
                break;
            case CLEAR_TYPE_DEPTH_STENCIL:
                glClearNamedFramebufferfi(framebufferId,
                                          GL_DEPTH_STENCIL,
                                          0,
                                          _DepthStencilClearValue->Depth,
                                          _DepthStencilClearValue->Stencil);
                break;
            default:
                HK_ASSERT(0);
        }
    }

    // Restore scissor test
    if (bScissorEnabled != RasterizerState.bScissorEnable)
    {
        if (RasterizerState.bScissorEnable)
        {
            glEnable(GL_SCISSOR_TEST);
        }
        else
        {
            glDisable(GL_SCISSOR_TEST);
        }
    }

    // Restore scissor rect
    if (_Rect)
    {
        SetScissor(scissorRect);
    }

    // Restore rasterizer discard
    if (bRasterizerDiscard != RasterizerState.bRasterizerDiscard)
    {
        if (RasterizerState.bRasterizerDiscard)
        {
            glEnable(GL_RASTERIZER_DISCARD);
        }
        else
        {
            glDisable(GL_RASTERIZER_DISCARD);
        }
    }

    if (bUpdateDrawBuffers)
    {
        UpdateDrawBuffers();
    }
}

void AImmediateContextGLImpl::BindReadFramebuffer(AFramebufferGL const* Framebuffer)
{
    GLuint framebufferId = Framebuffer->GetHandleNativeGL();

    if (Binding.ReadFramebuffer != framebufferId)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferId);
        Binding.ReadFramebuffer = framebufferId;
    }
}

bool AImmediateContextGLImpl::ChooseReadBuffer(AFramebufferGL const* pFramebuffer, int _ColorAttachment) const
{
    if (pFramebuffer->GetHandleNativeGL() == 0)
    {
        HK_ASSERT(_ColorAttachment == 0);

        if (_ColorAttachment != 0)
            return false;

        glNamedFramebufferReadBuffer(pFramebuffer->GetHandleNativeGL(), GL_BACK); // FIXME: check this
    }
    else
    {
        HK_ASSERT(_ColorAttachment < MAX_COLOR_ATTACHMENTS);

        glNamedFramebufferReadBuffer(pFramebuffer->GetHandleNativeGL(), GL_COLOR_ATTACHMENT0 + _ColorAttachment);
    }

    return true;
}

bool AImmediateContextGLImpl::ReadFramebufferAttachment(ARenderPassContext& RenderPassContext,
                                                        int                 _ColorAttachment,
                                                        SRect2D const&      _SrcRect,
                                                        FRAMEBUFFER_CHANNEL _FramebufferChannel,
                                                        FRAMEBUFFER_OUTPUT  _FramebufferOutput,
                                                        COLOR_CLAMP         _ColorClamp,
                                                        size_t              _SizeInBytes,
                                                        unsigned int        _Alignment, // Specifies alignment of destination data
                                                        void*               _SysMem)
{
    if (!ChooseReadBuffer(CurrentFramebuffer, _ColorAttachment))
    {
        LOG("Framebuffer::Read: invalid framebuffer attachment\n");
        return false;
    }

    PackAlignment(_Alignment);

    BindReadFramebuffer(CurrentFramebuffer);

    ClampReadColor(_ColorClamp);

    glReadnPixels(_SrcRect.X,
                  _SrcRect.Y,
                  _SrcRect.Width,
                  _SrcRect.Height,
                  FramebufferChannelLUT[_FramebufferChannel],
                  FramebufferOutputLUT[_FramebufferOutput],
                  (GLsizei)_SizeInBytes,
                  _SysMem);
    return true;
}

bool AImmediateContextGLImpl::ReadFramebufferDepthStencilAttachment(ARenderPassContext& RenderPassContext,
                                                                    SRect2D const&      SrcRect,
                                                                    size_t              SizeInBytes,
                                                                    unsigned int        Alignment,
                                                                    void*               SysMem)
{
    if (!CurrentFramebuffer->HasDepthStencilAttachment())
    {
        LOG("AImmediateContextGLImpl::ReadFramebufferDepthStencilAttachment: framebuffer has no depth-stencil attachment\n");
        return false;
    }

    PackAlignment(Alignment);

    BindReadFramebuffer(CurrentFramebuffer);

    ClampReadColor(COLOR_CLAMP_OFF);

    GLenum  format, type;
    GLsizei size;
    ChooseDepthStencilAttachmentFormatAndType(CurrentFramebuffer->GetDepthStencilAttachment()->GetDesc().Format, format, type, size);

    size *= SrcRect.Width * SrcRect.Height;

    HK_ASSERT(size == SizeInBytes);
    if (size > SizeInBytes)
        size = SizeInBytes;

    glReadnPixels(SrcRect.X,
                  SrcRect.Y,
                  SrcRect.Width,
                  SrcRect.Height,
                  format,
                  type,
                  size,
                  SysMem);
    return true;
}

void AImmediateContextGLImpl::ReadTexture(ITexture*    pTexture,
                                          uint16_t     MipLevel,
                                          DATA_FORMAT  Format,
                                          size_t       SizeInBytes,
                                          unsigned int Alignment,
                                          void*        pSysMem)
{
    HK_ASSERT(MipLevel < pTexture->GetDesc().NumMipLevels);

    STextureRect rect;
    rect.Offset.MipLevel = MipLevel;
    rect.Dimension.X     = Math::Max(1u, pTexture->GetWidth() >> MipLevel);
    rect.Dimension.Y     = Math::Max(1u, pTexture->GetHeight() >> MipLevel);
    rect.Dimension.Z     = pTexture->GetSliceCount(MipLevel);

    ReadTextureRect(pTexture, rect, Format, SizeInBytes, Alignment, pSysMem);
}

static bool ChooseBackbufferReadFormat(ITexture* pTexture, DATA_FORMAT Format, GLenum& FormatGL, GLenum& TypeGL)
{
    switch (pTexture->GetDesc().Format)
    {
        case TEXTURE_FORMAT_STENCIL1:
        case TEXTURE_FORMAT_STENCIL4:
        case TEXTURE_FORMAT_STENCIL8:
        case TEXTURE_FORMAT_STENCIL16:
            FormatGL = GL_STENCIL_INDEX;
            TypeGL   = GL_UNSIGNED_INT; // FIXME
            return Format == FORMAT_UINT1;
        case TEXTURE_FORMAT_DEPTH16:
        case TEXTURE_FORMAT_DEPTH24:
        case TEXTURE_FORMAT_DEPTH32:
            FormatGL = GL_DEPTH_COMPONENT;
            TypeGL   = GL_FLOAT; // FIXME
            return Format == FORMAT_FLOAT1;
        case TEXTURE_FORMAT_DEPTH24_STENCIL8:
        case TEXTURE_FORMAT_DEPTH32F_STENCIL8:
            FormatGL = GL_DEPTH_STENCIL;
            TypeGL   = GL_FLOAT; // FIXME
            return Format == FORMAT_FLOAT1;
        case TEXTURE_FORMAT_RGBA8:
        case TEXTURE_FORMAT_SRGB8_ALPHA8:
            FormatGL = GL_BGRA;
            TypeGL   = GL_UNSIGNED_BYTE;
            return Format == FORMAT_UBYTE4;
        default:
            FormatGL = TypeLUT[Format].FormatBGR;
            TypeGL   = TypeLUT[Format].Type;
            break;
    }
    return false;
}

void AImmediateContextGLImpl::ReadTextureRect(ITexture*           pTexture,
                                              STextureRect const& Rectangle,
                                              DATA_FORMAT         Format,
                                              size_t              SizeInBytes,
                                              unsigned int        Alignment,
                                              void*               pSysMem)
{
    GLuint id = pTexture->GetHandleNativeGL();

    PackAlignment(Alignment);

    GLsizei size = TypeLUT[Format].SizeInBytes * Rectangle.Dimension.X * Rectangle.Dimension.Y * Rectangle.Dimension.Z;

    HK_ASSERT(size == SizeInBytes);
    if (size > SizeInBytes)
        size = SizeInBytes;

    HK_ASSERT(Rectangle.Offset.MipLevel < pTexture->GetDesc().NumMipLevels);

    uint32_t maxDimensionZ = pTexture->GetSliceCount(Rectangle.Offset.MipLevel);

    if (Rectangle.Offset.X == 0 &&
        Rectangle.Offset.Y == 0 &&
        Rectangle.Offset.Z == 0 &&
        Rectangle.Dimension.X == pTexture->GetWidth() &&
        Rectangle.Dimension.Y == pTexture->GetHeight() &&
        Rectangle.Dimension.Z == maxDimensionZ)
    {
        // Dummy texture is a default color or depth buffer
        if (static_cast<ATextureGLImpl*>(pTexture)->IsDummyTexture())
        {
            HK_ASSERT(static_cast<ATextureGLImpl*>(pTexture)->pContext == this);
            HK_ASSERT(Rectangle.Offset.MipLevel == 0);
            HK_ASSERT(Rectangle.Dimension.Z == 1);

            GLenum format, type;
            if (!ChooseBackbufferReadFormat(pTexture, Format, format, type))
            {
                HK_ASSERT_(0, "AImmediateContextGLImpl::ReadTextureRect: Uncompatible data format");
                return;
            }

            if (Binding.ReadFramebuffer != 0)
            {
                glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
                Binding.ReadFramebuffer = 0;
            }

            glNamedFramebufferReadBuffer(0, GL_BACK);

            ClampReadColor(COLOR_CLAMP_OFF);

            glReadnPixels(0,
                          0,
                          Rectangle.Dimension.X,
                          Rectangle.Dimension.Y,
                          format,
                          type,
                          size,
                          pSysMem);
        }
        else
        {
            if (pTexture->IsCompressed())
            {
                glGetCompressedTextureImage(id, Rectangle.Offset.MipLevel, size, pSysMem);
            }
            else
            {
                glGetTextureImage(id,
                                  Rectangle.Offset.MipLevel,
                                  TypeLUT[Format].FormatBGR,
                                  TypeLUT[Format].Type,
                                  size,
                                  pSysMem);
            }
        }
    }
    else
    {
        HK_ASSERT(Rectangle.Offset.X + Rectangle.Dimension.X <= pTexture->GetWidth());
        HK_ASSERT(Rectangle.Offset.Y + Rectangle.Dimension.Y <= pTexture->GetHeight());
        HK_ASSERT(Rectangle.Offset.Z + Rectangle.Dimension.Z <= maxDimensionZ);

        // Dummy texture is a default color or depth buffer
        if (static_cast<ATextureGLImpl*>(pTexture)->IsDummyTexture())
        {
            HK_ASSERT(static_cast<ATextureGLImpl*>(pTexture)->pContext == this);
            HK_ASSERT(Rectangle.Offset.MipLevel == 0);
            HK_ASSERT(Rectangle.Offset.Z == 0);
            HK_ASSERT(Rectangle.Dimension.Z == 1);

            GLenum format, type;
            if (!ChooseBackbufferReadFormat(pTexture, Format, format, type))
            {
                HK_ASSERT_(0, "AImmediateContextGLImpl::ReadTextureRect: Uncompatible data format");
                return;
            }

            if (Binding.ReadFramebuffer != 0)
            {
                glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
                Binding.ReadFramebuffer = 0;
            }

            glNamedFramebufferReadBuffer(0, GL_BACK);

            ClampReadColor(COLOR_CLAMP_OFF);

            glReadnPixels(Rectangle.Offset.X,
                          Rectangle.Offset.Y,
                          Rectangle.Dimension.X,
                          Rectangle.Dimension.Y,
                          format,
                          type,
                          size,
                          pSysMem);
        }
        else
        {
            if (pTexture->IsCompressed())
            {
                glGetCompressedTextureSubImage(id,
                                               Rectangle.Offset.MipLevel,
                                               Rectangle.Offset.X,
                                               Rectangle.Offset.Y,
                                               Rectangle.Offset.Z,
                                               Rectangle.Dimension.X,
                                               Rectangle.Dimension.Y,
                                               Rectangle.Dimension.Z,
                                               size,
                                               pSysMem);
            }
            else
            {
                glGetTextureSubImage(id,
                                     Rectangle.Offset.MipLevel,
                                     Rectangle.Offset.X,
                                     Rectangle.Offset.Y,
                                     Rectangle.Offset.Z,
                                     Rectangle.Dimension.X,
                                     Rectangle.Dimension.Y,
                                     Rectangle.Dimension.Z,
                                     TypeLUT[Format].FormatBGR,
                                     TypeLUT[Format].Type,
                                     size,
                                     pSysMem);
            }
        }
    }
}

bool AImmediateContextGLImpl::WriteTexture(ITexture*    pTexture,
                                           uint16_t     MipLevel,
                                           DATA_FORMAT  Type, // Specifies a pixel format for the input data
                                           size_t       SizeInBytes,
                                           unsigned int Alignment, // Specifies alignment of source data
                                           const void*  pSysMem)
{
    HK_ASSERT(MipLevel < pTexture->GetDesc().NumMipLevels);

    STextureRect rect;
    rect.Offset.MipLevel = MipLevel;
    rect.Dimension.X     = Math::Max(1u, pTexture->GetWidth() >> MipLevel);
    rect.Dimension.Y     = Math::Max(1u, pTexture->GetHeight() >> MipLevel);
    rect.Dimension.Z     = pTexture->GetSliceCount(MipLevel);

    return WriteTextureRect(pTexture,
                            rect,
                            Type,
                            SizeInBytes,
                            Alignment,
                            pSysMem);
}

bool AImmediateContextGLImpl::WriteTextureRect(ITexture*           pTexture,
                                               STextureRect const& Rectangle,
                                               DATA_FORMAT         Format, // Specifies a pixel format for the input data
                                               size_t              SizeInBytes,
                                               unsigned int        Alignment, // Specifies alignment of source data
                                               const void*         pSysMem)
{
    GLuint id               = pTexture->GetHandleNativeGL();
    GLenum compressedFormat = InternalFormatLUT[pTexture->GetDesc().Format].InternalFormat;
    GLenum format           = TypeLUT[Format].FormatBGR;
    GLenum type             = TypeLUT[Format].Type;

    HK_ASSERT_(!static_cast<ATextureGLImpl*>(pTexture)->IsDummyTexture(),
               "Attempting to write raw data to OpenGL back buffer");
    // NOTE: For default back buffer we can write data to temp texture and then blit it.

    uint32_t maxDimensionZ = pTexture->GetSliceCount(Rectangle.Offset.MipLevel);

    HK_ASSERT(Rectangle.Offset.X + Rectangle.Dimension.X <= pTexture->GetWidth());
    HK_ASSERT(Rectangle.Offset.Y + Rectangle.Dimension.Y <= pTexture->GetHeight());
    HK_ASSERT(Rectangle.Offset.Z + Rectangle.Dimension.Z <= maxDimensionZ);
    HK_UNUSED(maxDimensionZ);

    if (!id)
    {
        return false;
    }

    UnpackAlignment(Alignment);

    switch (pTexture->GetDesc().Type)
    {
        case TEXTURE_1D:
            if (pTexture->IsCompressed())
            {
                glCompressedTextureSubImage1D(id,
                                              Rectangle.Offset.MipLevel,
                                              Rectangle.Offset.X,
                                              Rectangle.Dimension.X,
                                              compressedFormat,
                                              (GLsizei)SizeInBytes,
                                              pSysMem);
            }
            else
            {
                glTextureSubImage1D(id,
                                    Rectangle.Offset.MipLevel,
                                    Rectangle.Offset.X,
                                    Rectangle.Dimension.X,
                                    format,
                                    type,
                                    pSysMem);
            }
            break;
        case TEXTURE_1D_ARRAY:
            if (pTexture->IsCompressed())
            {
                glCompressedTextureSubImage2D(id,
                                              Rectangle.Offset.MipLevel,
                                              Rectangle.Offset.X,
                                              Rectangle.Offset.Z,
                                              Rectangle.Dimension.X,
                                              Rectangle.Dimension.Z,
                                              compressedFormat,
                                              (GLsizei)SizeInBytes,
                                              pSysMem);
            }
            else
            {
                glTextureSubImage2D(id,
                                    Rectangle.Offset.MipLevel,
                                    Rectangle.Offset.X,
                                    Rectangle.Offset.Z,
                                    Rectangle.Dimension.X,
                                    Rectangle.Dimension.Z,
                                    format,
                                    type,
                                    pSysMem);
            }
            break;
        case TEXTURE_2D:
            if (pTexture->IsMultisample())
            {
                return false;
            }
            if (pTexture->IsCompressed())
            {
                glCompressedTextureSubImage2D(id,
                                              Rectangle.Offset.MipLevel,
                                              Rectangle.Offset.X,
                                              Rectangle.Offset.Y,
                                              Rectangle.Dimension.X,
                                              Rectangle.Dimension.Y,
                                              compressedFormat,
                                              (GLsizei)SizeInBytes,
                                              pSysMem);
            }
            else
            {
                glTextureSubImage2D(id,
                                    Rectangle.Offset.MipLevel,
                                    Rectangle.Offset.X,
                                    Rectangle.Offset.Y,
                                    Rectangle.Dimension.X,
                                    Rectangle.Dimension.Y,
                                    format,
                                    type,
                                    pSysMem);
            }
            break;
        case TEXTURE_2D_ARRAY:
            if (pTexture->IsMultisample())
            {
                return false;
            }
            if (pTexture->IsCompressed())
            {
                glCompressedTextureSubImage3D(id,
                                              Rectangle.Offset.MipLevel,
                                              Rectangle.Offset.X,
                                              Rectangle.Offset.Y,
                                              Rectangle.Offset.Z,
                                              Rectangle.Dimension.X,
                                              Rectangle.Dimension.Y,
                                              Rectangle.Dimension.Z,
                                              compressedFormat,
                                              (GLsizei)SizeInBytes,
                                              pSysMem);
            }
            else
            {
                glTextureSubImage3D(id,
                                    Rectangle.Offset.MipLevel,
                                    Rectangle.Offset.X,
                                    Rectangle.Offset.Y,
                                    Rectangle.Offset.Z,
                                    Rectangle.Dimension.X,
                                    Rectangle.Dimension.Y,
                                    Rectangle.Dimension.Z,
                                    format,
                                    type,
                                    pSysMem);
            }
            break;
        case TEXTURE_3D:
            if (pTexture->IsCompressed())
            {
                glCompressedTextureSubImage3D(id,
                                              Rectangle.Offset.MipLevel,
                                              Rectangle.Offset.X,
                                              Rectangle.Offset.Y,
                                              Rectangle.Offset.Z,
                                              Rectangle.Dimension.X,
                                              Rectangle.Dimension.Y,
                                              Rectangle.Dimension.Z,
                                              compressedFormat,
                                              (GLsizei)SizeInBytes,
                                              pSysMem);
            }
            else
            {
                glTextureSubImage3D(id,
                                    Rectangle.Offset.MipLevel,
                                    Rectangle.Offset.X,
                                    Rectangle.Offset.Y,
                                    Rectangle.Offset.Z,
                                    Rectangle.Dimension.X,
                                    Rectangle.Dimension.Y,
                                    Rectangle.Dimension.Z,
                                    format,
                                    type,
                                    pSysMem);
            }
            break;
        case TEXTURE_CUBE_MAP:
            if (pTexture->IsCompressed())
            {
                //glCompressedTextureSubImage2D( id, Rectangle.Offset.MipLevel, Rectangle.Offset.X, Rectangle.Offset.Y, Rectangle.Dimension.X, Rectangle.Dimension.Y, format, SizeInBytes, pSysMem );

                // Tested on NVidia
                glCompressedTextureSubImage3D(id,
                                              Rectangle.Offset.MipLevel,
                                              Rectangle.Offset.X,
                                              Rectangle.Offset.Y,
                                              Rectangle.Offset.Z,
                                              Rectangle.Dimension.X,
                                              Rectangle.Dimension.Y,
                                              Rectangle.Dimension.Z,
                                              compressedFormat,
                                              (GLsizei)SizeInBytes,
                                              pSysMem);
            }
            else
            {
                //glTextureSubImage2D( id, Rectangle.Offset.MipLevel, Rectangle.Offset.X, Rectangle.Offset.Y, Rectangle.Dimension.X, Rectangle.Dimension.Y, format, type, pSysMem );

                // Tested on NVidia
                glTextureSubImage3D(id,
                                    Rectangle.Offset.MipLevel,
                                    Rectangle.Offset.X,
                                    Rectangle.Offset.Y,
                                    Rectangle.Offset.Z,
                                    Rectangle.Dimension.X,
                                    Rectangle.Dimension.Y,
                                    Rectangle.Dimension.Z,
                                    format,
                                    type,
                                    pSysMem);
            }
            break;
        case TEXTURE_CUBE_MAP_ARRAY:
            // FIXME: В спецификации ничего не сказано о возможности записи в данный тип текстурного таргета
            if (pTexture->IsCompressed())
            {
                //glCompressedTextureSubImage2D( id, Rectangle.Offset.MipLevel, Rectangle.Offset.X, Rectangle.Offset.Y, Rectangle.Dimension.X, Rectangle.Dimension.Y, format, SizeInBytes, pSysMem );

                glCompressedTextureSubImage3D(id,
                                              Rectangle.Offset.MipLevel,
                                              Rectangle.Offset.X,
                                              Rectangle.Offset.Y,
                                              Rectangle.Offset.Z,
                                              Rectangle.Dimension.X,
                                              Rectangle.Dimension.Y,
                                              Rectangle.Dimension.Z,
                                              compressedFormat,
                                              (GLsizei)SizeInBytes,
                                              pSysMem);
            }
            else
            {
                //glTextureSubImage2D( id, Rectangle.Offset.MipLevel, Rectangle.Offset.X, Rectangle.Offset.Y, Rectangle.Dimension.X, Rectangle.Dimension.Y, format, type, pSysMem );

                glTextureSubImage3D(id,
                                    Rectangle.Offset.MipLevel,
                                    Rectangle.Offset.X,
                                    Rectangle.Offset.Y,
                                    Rectangle.Offset.Z,
                                    Rectangle.Dimension.X,
                                    Rectangle.Dimension.Y,
                                    Rectangle.Dimension.Z,
                                    format,
                                    type,
                                    pSysMem);
            }
            break;
        case TEXTURE_RECT_GL:
            // FIXME: В спецификации ничего не сказано о возможности записи в данный тип текстурного таргета
            if (pTexture->IsCompressed())
            {
                glCompressedTextureSubImage2D(id,
                                              Rectangle.Offset.MipLevel,
                                              Rectangle.Offset.X,
                                              Rectangle.Offset.Y,
                                              Rectangle.Dimension.X,
                                              Rectangle.Dimension.Y,
                                              compressedFormat,
                                              (GLsizei)SizeInBytes,
                                              pSysMem);
            }
            else
            {
                glTextureSubImage2D(id,
                                    Rectangle.Offset.MipLevel,
                                    Rectangle.Offset.X,
                                    Rectangle.Offset.Y,
                                    Rectangle.Dimension.X,
                                    Rectangle.Dimension.Y,
                                    format,
                                    type,
                                    pSysMem);
            }
            break;
    }

    return true;
}

void AImmediateContextGLImpl::GenerateTextureMipLevels(ITexture* pTexture)
{
    HK_ASSERT_(!static_cast<ATextureGLImpl*>(pTexture)->IsDummyTexture(),
               "Attempting to generate mipmap levels for OpenGL back buffer");

    GLuint id = pTexture->GetHandleNativeGL();
    if (!id)
    {
        return;
    }

    glGenerateTextureMipmap(id);
}

void AImmediateContextGLImpl::SparseTextureCommitPage(ISparseTexture* _Texture,
                                                      int             InMipLevel,
                                                      int             InPageX,
                                                      int             InPageY,
                                                      int             InPageZ,
                                                      DATA_FORMAT     _Format, // Specifies a pixel format for the input data
                                                      size_t          _SizeInBytes,
                                                      unsigned int    _Alignment, // Specifies alignment of source data
                                                      const void*     _SysMem)
{
    STextureRect rect;

    rect.Offset.MipLevel = InMipLevel;
    rect.Offset.X        = InPageX * _Texture->GetPageSizeX();
    rect.Offset.Y        = InPageY * _Texture->GetPageSizeY();
    rect.Offset.Z        = InPageZ * _Texture->GetPageSizeZ();
    rect.Dimension.X     = _Texture->GetPageSizeX();
    rect.Dimension.Y     = _Texture->GetPageSizeY();
    rect.Dimension.Z     = _Texture->GetPageSizeZ();

    SparseTextureCommitRect(_Texture, rect, _Format, _SizeInBytes, _Alignment, _SysMem);
}

void AImmediateContextGLImpl::SparseTextureCommitRect(ISparseTexture*     _Texture,
                                                      STextureRect const& _Rectangle,
                                                      DATA_FORMAT         _Format, // Specifies a pixel format for the input data
                                                      size_t              _SizeInBytes,
                                                      unsigned int        _Alignment, // Specifies alignment of source data
                                                      const void*         _SysMem)
{
    GLuint id = _Texture->GetHandleNativeGL();

    if (!id)
    {
        LOG("AImmediateContextGLImpl::SparseTextureCommitRect: null handle\n");
        return;
    }

    GLenum compressedFormat = InternalFormatLUT[_Texture->GetDesc().Format].InternalFormat;
    GLenum format           = TypeLUT[_Format].FormatBGR;
    GLenum type             = TypeLUT[_Format].Type;

    glTexturePageCommitmentEXT(id,
                               _Rectangle.Offset.MipLevel,
                               _Rectangle.Offset.X,
                               _Rectangle.Offset.Y,
                               _Rectangle.Offset.Z,
                               _Rectangle.Dimension.X,
                               _Rectangle.Dimension.Y,
                               _Rectangle.Dimension.Z,
                               GL_TRUE);

    UnpackAlignment(_Alignment);

    switch (_Texture->GetDesc().Type)
    {
        case SPARSE_TEXTURE_2D:
            if (_Texture->IsCompressed())
            {
                glCompressedTextureSubImage2D(id,
                                              _Rectangle.Offset.MipLevel,
                                              _Rectangle.Offset.X,
                                              _Rectangle.Offset.Y,
                                              _Rectangle.Dimension.X,
                                              _Rectangle.Dimension.Y,
                                              compressedFormat,
                                              (GLsizei)_SizeInBytes,
                                              _SysMem);
            }
            else
            {
                glTextureSubImage2D(id,
                                    _Rectangle.Offset.MipLevel,
                                    _Rectangle.Offset.X,
                                    _Rectangle.Offset.Y,
                                    _Rectangle.Dimension.X,
                                    _Rectangle.Dimension.Y,
                                    format,
                                    type,
                                    _SysMem);
            }
            break;
        case SPARSE_TEXTURE_2D_ARRAY:
            if (_Texture->IsCompressed())
            {
                glCompressedTextureSubImage3D(id,
                                              _Rectangle.Offset.MipLevel,
                                              _Rectangle.Offset.X,
                                              _Rectangle.Offset.Y,
                                              _Rectangle.Offset.Z,
                                              _Rectangle.Dimension.X,
                                              _Rectangle.Dimension.Y,
                                              _Rectangle.Dimension.Z,
                                              compressedFormat,
                                              (GLsizei)_SizeInBytes,
                                              _SysMem);
            }
            else
            {
                glTextureSubImage3D(id,
                                    _Rectangle.Offset.MipLevel,
                                    _Rectangle.Offset.X,
                                    _Rectangle.Offset.Y,
                                    _Rectangle.Offset.Z,
                                    _Rectangle.Dimension.X,
                                    _Rectangle.Dimension.Y,
                                    _Rectangle.Dimension.Z,
                                    format,
                                    type,
                                    _SysMem);
            }
            break;
        case SPARSE_TEXTURE_3D:
            if (_Texture->IsCompressed())
            {
                glCompressedTextureSubImage3D(id,
                                              _Rectangle.Offset.MipLevel,
                                              _Rectangle.Offset.X,
                                              _Rectangle.Offset.Y,
                                              _Rectangle.Offset.Z,
                                              _Rectangle.Dimension.X,
                                              _Rectangle.Dimension.Y,
                                              _Rectangle.Dimension.Z,
                                              compressedFormat,
                                              (GLsizei)_SizeInBytes,
                                              _SysMem);
            }
            else
            {
                glTextureSubImage3D(id,
                                    _Rectangle.Offset.MipLevel,
                                    _Rectangle.Offset.X,
                                    _Rectangle.Offset.Y,
                                    _Rectangle.Offset.Z,
                                    _Rectangle.Dimension.X,
                                    _Rectangle.Dimension.Y,
                                    _Rectangle.Dimension.Z,
                                    format,
                                    type,
                                    _SysMem);
            }
            break;
        case SPARSE_TEXTURE_CUBE_MAP:
            if (_Texture->IsCompressed())
            {
                //glCompressedTextureSubImage2D( id, _Rectangle.Offset.MipLevel, _Rectangle.Offset.X, _Rectangle.Offset.Y, _Rectangle.Dimension.X, _Rectangle.Dimension.Y, format, _SizeInBytes, _SysMem );

                // Tested on NVidia
                glCompressedTextureSubImage3D(id,
                                              _Rectangle.Offset.MipLevel,
                                              _Rectangle.Offset.X,
                                              _Rectangle.Offset.Y,
                                              _Rectangle.Offset.Z,
                                              _Rectangle.Dimension.X,
                                              _Rectangle.Dimension.Y,
                                              _Rectangle.Dimension.Z,
                                              compressedFormat,
                                              (GLsizei)_SizeInBytes,
                                              _SysMem);
            }
            else
            {
                //glTextureSubImage2D( id, _Rectangle.Offset.MipLevel, _Rectangle.Offset.X, _Rectangle.Offset.Y, _Rectangle.Dimension.X, _Rectangle.Dimension.Y, format, type, _SysMem );

                // Tested on NVidia
                glTextureSubImage3D(id,
                                    _Rectangle.Offset.MipLevel,
                                    _Rectangle.Offset.X,
                                    _Rectangle.Offset.Y,
                                    _Rectangle.Offset.Z,
                                    _Rectangle.Dimension.X,
                                    _Rectangle.Dimension.Y,
                                    _Rectangle.Dimension.Z,
                                    format,
                                    type,
                                    _SysMem);
            }
            break;
        case SPARSE_TEXTURE_CUBE_MAP_ARRAY:
            // FIXME: specs
            if (_Texture->IsCompressed())
            {
                //glCompressedTextureSubImage2D( id, _Rectangle.Offset.MipLevel, _Rectangle.Offset.X, _Rectangle.Offset.Y, _Rectangle.Dimension.X, _Rectangle.Dimension.Y, format, _SizeInBytes, _SysMem );

                glCompressedTextureSubImage3D(id,
                                              _Rectangle.Offset.MipLevel,
                                              _Rectangle.Offset.X,
                                              _Rectangle.Offset.Y,
                                              _Rectangle.Offset.Z,
                                              _Rectangle.Dimension.X,
                                              _Rectangle.Dimension.Y,
                                              _Rectangle.Dimension.Z,
                                              compressedFormat,
                                              (GLsizei)_SizeInBytes,
                                              _SysMem);
            }
            else
            {
                //glTextureSubImage2D( id, _Rectangle.Offset.MipLevel, _Rectangle.Offset.X, _Rectangle.Offset.Y, _Rectangle.Dimension.X, _Rectangle.Dimension.Y, format, type, _SysMem );

                glTextureSubImage3D(id,
                                    _Rectangle.Offset.MipLevel,
                                    _Rectangle.Offset.X,
                                    _Rectangle.Offset.Y,
                                    _Rectangle.Offset.Z,
                                    _Rectangle.Dimension.X,
                                    _Rectangle.Dimension.Y,
                                    _Rectangle.Dimension.Z,
                                    format,
                                    type,
                                    _SysMem);
            }
            break;
        case SPARSE_TEXTURE_RECT_GL:
            // FIXME: specs
            if (_Texture->IsCompressed())
            {
                glCompressedTextureSubImage2D(id,
                                              _Rectangle.Offset.MipLevel,
                                              _Rectangle.Offset.X,
                                              _Rectangle.Offset.Y,
                                              _Rectangle.Dimension.X,
                                              _Rectangle.Dimension.Y,
                                              compressedFormat,
                                              (GLsizei)_SizeInBytes,
                                              _SysMem);
            }
            else
            {
                glTextureSubImage2D(id,
                                    _Rectangle.Offset.MipLevel,
                                    _Rectangle.Offset.X,
                                    _Rectangle.Offset.Y,
                                    _Rectangle.Dimension.X,
                                    _Rectangle.Dimension.Y,
                                    format,
                                    type,
                                    _SysMem);
            }
            break;
    }
}

void AImmediateContextGLImpl::SparseTextureUncommitPage(ISparseTexture* _Texture, int InMipLevel, int InPageX, int InPageY, int InPageZ)
{
    STextureRect rect;

    rect.Offset.MipLevel = InMipLevel;
    rect.Offset.X        = InPageX * _Texture->GetPageSizeX();
    rect.Offset.Y        = InPageY * _Texture->GetPageSizeY();
    rect.Offset.Z        = InPageZ * _Texture->GetPageSizeZ();
    rect.Dimension.X     = _Texture->GetPageSizeX();
    rect.Dimension.Y     = _Texture->GetPageSizeY();
    rect.Dimension.Z     = _Texture->GetPageSizeZ();

    SparseTextureUncommitRect(_Texture, rect);
}

void AImmediateContextGLImpl::SparseTextureUncommitRect(ISparseTexture* _Texture, STextureRect const& _Rectangle)
{
    GLuint id = _Texture->GetHandleNativeGL();

    if (!id)
    {
        LOG("AImmediateContextGLImpl::SparseTextureUncommitRect: null handle\n");
        return;
    }

    glTexturePageCommitmentEXT(id,
                               _Rectangle.Offset.MipLevel,
                               _Rectangle.Offset.X,
                               _Rectangle.Offset.Y,
                               _Rectangle.Offset.Z,
                               _Rectangle.Dimension.X,
                               _Rectangle.Dimension.Y,
                               _Rectangle.Dimension.Z,
                               GL_FALSE);
}

void AImmediateContextGLImpl::GetQueryPoolResults(IQueryPool*        _QueryPool,
                                                  uint32_t           _FirstQuery,
                                                  uint32_t           _QueryCount,
                                                  size_t             _DataSize,
                                                  void*              _SysMem,
                                                  size_t             _DstStride,
                                                  QUERY_RESULT_FLAGS _Flags)
{
    HK_ASSERT(_FirstQuery + _QueryCount <= _QueryPool->GetPoolSize());

    unsigned int* idPool = static_cast<AQueryPoolGLImpl*>(_QueryPool)->IdPool;

    glBindBuffer(GL_QUERY_BUFFER, 0);

    uint8_t* ptr = (uint8_t*)_SysMem;
    uint8_t* end = ptr + _DataSize;

    if (_Flags & QUERY_RESULT_64_BIT)
    {

        HK_ASSERT((_DstStride & ~(size_t)7) == _DstStride); // check stride must be multiples of 8

        for (uint32_t index = 0; index < _QueryCount; index++)
        {

            if (ptr + sizeof(uint64_t) > end)
            {
                LOG("QueryPool::GetResults: out of data size\n");
                break;
            }

            if (_Flags & QUERY_RESULT_WAIT_BIT)
            {
                glGetQueryObjectui64v(idPool[_FirstQuery + index], GL_QUERY_RESULT, (GLuint64*)ptr); // 3.2

                if (_Flags & QUERY_RESULT_WITH_AVAILABILITY_BIT)
                {
                    *(uint64_t*)ptr |= 0x8000000000000000;
                }
            }
            else
            {
                if (_Flags & QUERY_RESULT_WITH_AVAILABILITY_BIT)
                {
                    uint64_t available;
                    glGetQueryObjectui64v(idPool[_FirstQuery + index], GL_QUERY_RESULT_AVAILABLE, &available); // 3.2

                    if (available)
                    {
                        glGetQueryObjectui64v(idPool[_FirstQuery + index], GL_QUERY_RESULT, (GLuint64*)ptr); // 3.2
                        *(uint64_t*)ptr |= 0x8000000000000000;
                    }
                    else
                    {
                        *(uint64_t*)ptr = 0;
                    }
                }
                else
                {
                    *(uint64_t*)ptr = 0;
                    glGetQueryObjectui64v(idPool[_FirstQuery + index], GL_QUERY_RESULT_NO_WAIT, (GLuint64*)ptr); // 3.2
                }
            }

            ptr += _DstStride;
        }
    }
    else
    {

        HK_ASSERT((_DstStride & ~(size_t)3) == _DstStride); // check stride must be multiples of 4

        for (uint32_t index = 0; index < _QueryCount; index++)
        {

            if (ptr + sizeof(uint32_t) > end)
            {
                LOG("QueryPool::GetResults: out of data size\n");
                break;
            }

            if (_Flags & QUERY_RESULT_WAIT_BIT)
            {
                glGetQueryObjectuiv(idPool[_FirstQuery + index], GL_QUERY_RESULT, (GLuint*)ptr); // 2.0

                if (_Flags & QUERY_RESULT_WITH_AVAILABILITY_BIT)
                {
                    *(uint32_t*)ptr |= 0x80000000;
                }
            }
            else
            {
                if (_Flags & QUERY_RESULT_WITH_AVAILABILITY_BIT)
                {
                    uint32_t available;
                    glGetQueryObjectuiv(idPool[_FirstQuery + index], GL_QUERY_RESULT_AVAILABLE, &available); // 2.0

                    if (available)
                    {
                        glGetQueryObjectuiv(idPool[_FirstQuery + index], GL_QUERY_RESULT, (GLuint*)ptr); // 2.0
                        *(uint32_t*)ptr |= 0x80000000;
                    }
                    else
                    {
                        *(uint32_t*)ptr = 0;
                    }
                }
                else
                {
                    *(uint32_t*)ptr = 0;
                    glGetQueryObjectuiv(idPool[_FirstQuery + index], GL_QUERY_RESULT_NO_WAIT, (GLuint*)ptr); // 2.0
                }
            }

            ptr += _DstStride;
        }
    }
}

void AImmediateContextGLImpl::ReadBufferRange(IBuffer* _Buffer, size_t _ByteOffset, size_t _SizeInBytes, void* _SysMem)
{
    HK_ASSERT(_ByteOffset + _SizeInBytes <= _Buffer->GetDesc().SizeInBytes);
    glGetNamedBufferSubData(_Buffer->GetHandleNativeGL(), _ByteOffset, _SizeInBytes, _SysMem); // 4.5 or GL_ARB_direct_state_access

    /*
    // Other path:
    GLint id = GetHandleNativeGL();
    GLenum target = BufferTargetLUT[ Type ].Target;
    GLint currentBinding;
    glGetIntegerv( BufferTargetLUT[ Type ].Binding, &currentBinding );
    if ( currentBinding == id ) {
        glBufferSubData( target, _ByteOffset, _SizeInBytes, _SysMem );
    } else {
        glBindBuffer( GL_COPY_READ_BUFFER, id );
        glGetBufferSubData( GL_COPY_READ_BUFFER, _ByteOffset, _SizeInBytes, _SysMem );
    }
    */
}

void AImmediateContextGLImpl::WriteBufferRange(IBuffer* _Buffer, size_t _ByteOffset, size_t _SizeInBytes, const void* _SysMem)
{
    HK_ASSERT(_ByteOffset + _SizeInBytes <= _Buffer->GetDesc().SizeInBytes);
    glNamedBufferSubData(_Buffer->GetHandleNativeGL(), _ByteOffset, _SizeInBytes, _SysMem); // 4.5 or GL_ARB_direct_state_access

    /*
    // Other path:
    GLint id = GetHandleNativeGL();
    GLint currentBinding;
    GLenum target = BufferTargetLUT[ Type ].Target;
    glGetIntegerv( BufferTargetLUT[ Type ].Binding, &currentBinding );
    if ( currentBinding == id ) {
        glBufferSubData( target, _ByteOffset, _SizeInBytes, _SysMem );
    } else {
        glBindBuffer( target, id );
        glBufferSubData( target, _ByteOffset, _SizeInBytes, _SysMem );
        glBindBuffer( target, currentBinding );
    }
    */
    /*
    // Other path:
    GLint id = GetHandleNativeGL();
    GLenum target = BufferTargetLUT[ Type ].Target;
    GLint currentBinding;
    glGetIntegerv( BufferTargetLUT[ Type ].Binding, &currentBinding );
    if ( currentBinding == id ) {
        glBufferSubData( target, _ByteOffset, _SizeInBytes, _SysMem );
    } else {
        glBindBuffer( GL_COPY_WRITE_BUFFER, id );
        glBufferSubData( GL_COPY_WRITE_BUFFER, _ByteOffset, _SizeInBytes, _SysMem );
    }
    */
}

void* AImmediateContextGLImpl::MapBuffer(IBuffer*        _Buffer,
                                         MAP_TRANSFER    _ClientServerTransfer,
                                         MAP_INVALIDATE  _Invalidate,
                                         MAP_PERSISTENCE _Persistence,
                                         bool            _FlushExplicit,
                                         bool            _Unsynchronized)
{
    return MapBufferRange(_Buffer,
                          0, _Buffer->GetDesc().SizeInBytes,
                          _ClientServerTransfer,
                          _Invalidate,
                          _Persistence,
                          _FlushExplicit,
                          _Unsynchronized);
}

void* AImmediateContextGLImpl::MapBufferRange(IBuffer*        _Buffer,
                                              size_t          _RangeOffset,
                                              size_t          _RangeSize,
                                              MAP_TRANSFER    _ClientServerTransfer,
                                              MAP_INVALIDATE  _Invalidate,
                                              MAP_PERSISTENCE _Persistence,
                                              bool            _FlushExplicit,
                                              bool            _Unsynchronized)
{
    int flags = 0;

    switch (_ClientServerTransfer)
    {
        case MAP_TRANSFER_READ:
            flags |= GL_MAP_READ_BIT;
            break;
        case MAP_TRANSFER_WRITE:
            flags |= GL_MAP_WRITE_BIT;
            break;
        case MAP_TRANSFER_RW:
            flags |= GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
            break;
    }

    if (!flags)
    {
        // At least on of the bits GL_MAP_READ_BIT or GL_MAP_WRITE_BIT
        // must be set
        LOG("AImmediateContextGLImpl::MapBufferRange: invalid map transfer function\n");
        return nullptr;
    }

    if (_Invalidate != MAP_NO_INVALIDATE)
    {
        if (flags & GL_MAP_READ_BIT)
        {
            // This flag may not be used in combination with GL_MAP_READ_BIT.
            LOG("AImmediateContextGLImpl::MapBufferRange: MAP_NO_INVALIDATE may not be used in combination with MAP_TRANSFER_READ/MAP_TRANSFER_RW\n");
            return nullptr;
        }

        if (_Invalidate == MAP_INVALIDATE_ENTIRE_BUFFER)
        {
            flags |= GL_MAP_INVALIDATE_BUFFER_BIT;
        }
        else if (_Invalidate == MAP_INVALIDATE_RANGE)
        {
            flags |= GL_MAP_INVALIDATE_RANGE_BIT;
        }
    }

    switch (_Persistence)
    {
        case MAP_NON_PERSISTENT:
            break;
        case MAP_PERSISTENT_COHERENT:
            flags |= GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
            break;
        case MAP_PERSISTENT_NO_COHERENT:
            flags |= GL_MAP_PERSISTENT_BIT;
            break;
    }

    if (_FlushExplicit)
    {
        flags |= GL_MAP_FLUSH_EXPLICIT_BIT;
    }

    if (_Unsynchronized)
    {
        flags |= GL_MAP_UNSYNCHRONIZED_BIT;
    }

    return glMapNamedBufferRange(_Buffer->GetHandleNativeGL(), _RangeOffset, _RangeSize, flags);
}

void AImmediateContextGLImpl::UnmapBuffer(IBuffer* _Buffer)
{
    glUnmapNamedBuffer(_Buffer->GetHandleNativeGL());
}


void AImmediateContextGLImpl::ExecuteFrameGraph(AFrameGraph* pFrameGraph)
{
    pFramebufferCache->CleanupOutdatedFramebuffers();

    auto& acquiredResources = pFrameGraph->GetAcquiredResources();
    auto& releasedResources = pFrameGraph->GetReleasedResources();

    FGRenderTargetCache* pRenderTargetCache = pFrameGraph->GetRenderTargetCache();

    for (AFrameGraph::STimelineStep const& step : pFrameGraph->GetTimeline())
    {
        // Acquire resources for the render pass
        for (int i = 0; i < step.NumAcquiredResources; i++)
        {
            FGResourceProxyBase* resourceProxy = acquiredResources[step.FirstAcquiredResource + i];
            if (resourceProxy->IsTransient())
            {
                switch (resourceProxy->GetProxyType())
                {
                    case DEVICE_OBJECT_TYPE_TEXTURE:
                        resourceProxy->SetDeviceObject(pRenderTargetCache->Acquire(static_cast<FGTextureProxy*>(resourceProxy)->GetResourceDesc()));
                        break;
                    default:
                        HK_ASSERT(0);
                }
            }
        }

        switch (step.RenderTask->GetProxyType())
        {
            case FG_RENDER_TASK_PROXY_TYPE_RENDER_PASS:
                ExecuteRenderPass(static_cast<ARenderPass*>(step.RenderTask));
                break;
            case FG_RENDER_TASK_PROXY_TYPE_CUSTOM:
                ExecuteCustomTask(static_cast<ACustomTask*>(step.RenderTask));
                break;
            default:
                HK_ASSERT(0);
                break;
        }

        // Release resources that are not needed after the current render pass
        for (int i = 0; i < step.NumReleasedResources; i++)
        {
            FGResourceProxyBase* resourceProxy = releasedResources[step.FirstReleasedResource + i];
            if (resourceProxy->IsTransient() && resourceProxy->GetDeviceObject())
            {
                switch (resourceProxy->GetProxyType())
                {
                    case DEVICE_OBJECT_TYPE_TEXTURE:
                        pRenderTargetCache->Release(static_cast<ITexture*>(resourceProxy->GetDeviceObject()));
                        break;
                    default:
                        HK_ASSERT(0);
                }
            }
        }
    }

    // Unbind current framebuffer
    Binding.DrawFramebuffer = ~0u;
    Binding.ReadFramebuffer = ~0u;

    // This is really not necessary:
    //glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    //glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    BindResourceTable(nullptr);
}

void AImmediateContextGLImpl::ExecuteRenderPass(ARenderPass* pRenderPass)
{
    auto& colorAttachments           = pRenderPass->GetColorAttachments();
    auto& depthStencilAttachment     = pRenderPass->GetDepthStencilAttachment();
    bool  bHasDepthStencilAttachment = pRenderPass->HasDepthStencilAttachment();

    AFramebufferGL* pFramebuffer = pFramebufferCache->GetFramebuffer(pRenderPass->GetName(),
                                                                         const_cast<TStdVector<STextureAttachment>&>(colorAttachments),
                                                                         bHasDepthStencilAttachment ? const_cast<STextureAttachment*>(&depthStencilAttachment) : nullptr);

    if (pFramebuffer->IsDefault())
    {
        AImmediateContextGLImpl* curContext = static_cast<ATextureGLImpl*>(const_cast<TWeakRef<ITextureView>&>(pFramebuffer->GetColorAttachments()[0])->GetTexture())->pContext;
        if (curContext != this)
        {
            MakeCurrent(curContext);

            curContext->ExecuteRenderPass(pRenderPass);

            MakeCurrent(this);
            return;
        }
    }

    SRenderPassBeginGL renderPassBegin = {};
    renderPassBegin.pRenderPass      = pRenderPass;
    renderPassBegin.pFramebuffer     = pFramebuffer;
    if (pRenderPass->IsRenderAreaSpecified())
    {
        auto& renderArea                  = pRenderPass->GetRenderArea();
        renderPassBegin.RenderArea.X      = renderArea.X;
        renderPassBegin.RenderArea.Y      = renderArea.Y;
        renderPassBegin.RenderArea.Width  = renderArea.Width;
        renderPassBegin.RenderArea.Height = renderArea.Height;
    }
    else
    {
        renderPassBegin.RenderArea.Width  = pFramebuffer->GetWidth();
        renderPassBegin.RenderArea.Height = pFramebuffer->GetHeight();
    }

    BeginRenderPass(renderPassBegin);

    SViewport vp;
    vp.X        = renderPassBegin.RenderArea.X;
    vp.Y        = renderPassBegin.RenderArea.Y;
    vp.Width    = renderPassBegin.RenderArea.Width;
    vp.Height   = renderPassBegin.RenderArea.Height;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    SetViewport(vp);

    ACommandBuffer     commandBuffer;
    ARenderPassContext renderPassContext;

    renderPassContext.pRenderPass  = pRenderPass;
    renderPassContext.SubpassIndex = 0;
    renderPassContext.RenderArea   = renderPassBegin.RenderArea;
    renderPassContext.pImmediateContext = this;
    for (ASubpassInfo const& Subpass : pRenderPass->GetSubpasses())
    {
        Subpass.Function(renderPassContext, commandBuffer);
        renderPassContext.SubpassIndex++;
        if (renderPassContext.SubpassIndex < pRenderPass->GetSubpasses().Size())
        {
            NextSubpass();
        }
    }

    EndRenderPass();
}

void AImmediateContextGLImpl::ExecuteCustomTask(ACustomTask* pCustomTask)
{
    ACustomTaskContext taskContext;
    taskContext.pImmediateContext = this;
    pCustomTask->Function(taskContext);
}

GLuint AImmediateContextGLImpl::CreateProgramPipeline(APipelineGLImpl* pPipeline)
{
    GLuint pipelineId;

    glCreateProgramPipelines(1, &pipelineId);

    if (pPipeline->pVS)
    {
        glUseProgramStages(pipelineId, GL_VERTEX_SHADER_BIT, pPipeline->pVS->GetHandleNativeGL());
    }
    if (pPipeline->pTCS)
    {
        glUseProgramStages(pipelineId, GL_TESS_CONTROL_SHADER_BIT, pPipeline->pTCS->GetHandleNativeGL());
    }
    if (pPipeline->pTES)
    {
        glUseProgramStages(pipelineId, GL_TESS_EVALUATION_SHADER_BIT, pPipeline->pTES->GetHandleNativeGL());
    }
    if (pPipeline->pGS)
    {
        glUseProgramStages(pipelineId, GL_GEOMETRY_SHADER_BIT, pPipeline->pGS->GetHandleNativeGL());
    }
    if (pPipeline->pFS)
    {
        glUseProgramStages(pipelineId, GL_FRAGMENT_SHADER_BIT, pPipeline->pFS->GetHandleNativeGL());
    }
    if (pPipeline->pCS)
    {
        glUseProgramStages(pipelineId, GL_COMPUTE_SHADER_BIT, pPipeline->pCS->GetHandleNativeGL());
    }

    glValidateProgramPipeline(pipelineId); // 4.1

    return pipelineId;
}

#if 0
void AImmediateContextGLImpl::DestroyProgramPipeline(APipelineGLImpl* pPipeline)
{
    auto it = ProgramPipelines.find(pPipeline->GetUID());
    if (it == ProgramPipelines.end())
        return;

    GLuint pipelineId = it->second;
    ProgramPipelines.erase(it);

    glDeleteProgramPipelines(1, &pipelineId);
}
#endif

GLuint AImmediateContextGLImpl::GetProgramPipeline(APipelineGLImpl* pPipeline)
{
    // Fast path for apps with single context
    if (IsMainContext())
    {
        GLuint pipelineId = pPipeline->GetHandleNativeGL();
        if (!pipelineId)
        {
            pipelineId = CreateProgramPipeline(pPipeline);

            ProgramPipelines[pPipeline->GetUID()] = pipelineId;
            pPipeline->SetHandleNativeGL(pipelineId);
        }
        return pipelineId;
    }

    auto it = ProgramPipelines.find(pPipeline->GetUID());
    if (it != ProgramPipelines.end())
    {
        return it->second;
    }

    auto pipelineId = CreateProgramPipeline(pPipeline);

    ProgramPipelines[pPipeline->GetUID()] = pipelineId;
    return pipelineId;
}

} // namespace RenderCore
