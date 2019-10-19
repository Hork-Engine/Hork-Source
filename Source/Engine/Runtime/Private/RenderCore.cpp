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

#include <Engine/Runtime/Public/RenderCore.h>
#include <Engine/Core/Public/Logger.h>
#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

#include <Engine/Renderer/OpenGL4.5/OpenGL45RenderBackend.h>

IRenderBackend * GRenderBackend = &OpenGL45::GOpenGL45RenderBackend;

FResourceGPU * FResourceGPU::GPUResources;
FResourceGPU * FResourceGPU::GPUResourcesTail;

void IRenderBackend::RegisterGPUResource( FResourceGPU * _Resource ) {
    IntrusiveAddToList( _Resource, pNext, pPrev, FResourceGPU::GPUResources, FResourceGPU::GPUResourcesTail );
}

void IRenderBackend::UnregisterGPUResource( FResourceGPU * _Resource ) {
    IntrusiveRemoveFromList( _Resource, pNext, pPrev, FResourceGPU::GPUResources, FResourceGPU::GPUResourcesTail );
}

void IRenderBackend::UploadGPUResources() {
    for ( FResourceGPU * resource = FResourceGPU::GPUResources ; resource ; resource = resource->pNext ) {
        resource->pOwner->UploadResourceGPU( resource );
    }
}
