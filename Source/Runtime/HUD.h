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

HK_NAMESPACE_BEGIN

class Actor_PlayerController;
class Canvas;

class Actor_HUD : public Actor
{
    HK_ACTOR(Actor_HUD, Actor)

    friend class Actor_PlayerController;

public:
    void Draw(Canvas& canvas, int x, int y, int width, int height);

    int GetViewportX() const { return m_ViewportX; }
    int GetViewportY() const { return m_ViewportY; }
    int GetViewportW() const { return m_ViewportW; }
    int GetViewportH() const { return m_ViewportH; }

    Actor* GetOwnerPawn() const { return m_OwnerPawn; }

protected:
    Actor_HUD();

    virtual void DrawHUD(Canvas& canvas) {}

private:
    int m_ViewportX = 0;
    int m_ViewportY = 0;
    int m_ViewportW = 0;
    int m_ViewportH = 0;

    Actor_PlayerController* m_OwnerPlayer = nullptr;
    Actor*            m_OwnerPawn   = nullptr;
};

HK_NAMESPACE_END
