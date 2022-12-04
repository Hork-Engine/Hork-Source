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

#pragma once

#include "Actor.h"
#include "Font.h"

class APlayerController;
class ACanvas;

class AHUD : public AActor
{
    HK_ACTOR(AHUD, AActor)

    friend class APlayerController;

public:
    void Draw(ACanvas& canvas, int x, int y, int width, int height);

    int GetViewportX() const { return m_ViewportX; }
    int GetViewportY() const { return m_ViewportY; }
    int GetViewportW() const { return m_ViewportW; }
    int GetViewportH() const { return m_ViewportH; }

    AActor* GetOwnerPawn() const { return m_OwnerPawn; }

protected:
    AHUD();

    virtual void DrawHUD(ACanvas& canvas) {}

private:
    int m_ViewportX = 0;
    int m_ViewportY = 0;
    int m_ViewportW = 0;
    int m_ViewportH = 0;

    APlayerController* m_OwnerPlayer = nullptr;
    AActor*            m_OwnerPawn   = nullptr;
};
