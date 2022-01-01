/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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

#include "Actor.h"
#include <World/Public/Resource/FontAtlas.h>

class APawn;
class APlayerController;
class ACanvas;

class AHUD : public AActor {
    AN_ACTOR( AHUD, AActor )

    friend class APlayerController;
public:

    void Draw( ACanvas * _Canvas, int _X, int _Y, int _W, int _H );

    void DrawText( AFont * _Font, int x, int y, Color4 const & color, const char * _Text );

    int GetViewportX() const { return ViewportX; }
    int GetViewportY() const { return ViewportY; }
    int GetViewportW() const { return ViewportW; }
    int GetViewportH() const { return ViewportH; }

    APawn * GetOwnerPawn() const { return OwnerPawn; }

protected:
    AHUD();

    virtual void DrawHUD();

    // Read only
    ACanvas * Canvas = nullptr;
    int ViewportX = 0;
    int ViewportY = 0;
    int ViewportW = 0;
    int ViewportH = 0;

private:
    APlayerController * OwnerPlayer = nullptr;
    APawn * OwnerPawn = nullptr;
};
