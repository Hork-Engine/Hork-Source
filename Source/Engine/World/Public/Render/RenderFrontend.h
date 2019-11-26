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

#include <Runtime/Public/RenderCore.h>
#include <World/Public/Base/DebugRenderer.h>
#include <World/Public/Canvas.h>

struct SRenderFrontendStat {
    int PolyCount;
    int ShadowMapPolyCount;
    int FrontendTime;
};

class ARenderFrontend
{
    AN_SINGLETON( ARenderFrontend )

public:
    void Render( ACanvas * InCanvas );

    //int GetFrameNumber() const { return FrameNumber; }

    SRenderFrontendStat const & GetStat() const { return Stat; }

private:
    void RenderCanvas( ACanvas * InCanvas );
    //void RenderImgui();
    //void RenderImgui( struct ImDrawList const * _DrawList );
    void RenderView( int _Index );

    SRenderFrame * FrameData;
    ADebugRenderer DebugDraw;
    int VisMarker = 0;
    int FrameNumber;

    SRenderFrontendStat Stat;

    SViewport const * Viewports[MAX_RENDER_VIEWS];
    int NumViewports = 0;
    int MaxViewportWidth = 0;
    int MaxViewportHeight = 0;

};

extern ARenderFrontend & GRenderFrontend;
