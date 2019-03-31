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

#include "M_Army.h"
#include "M_Demon.h"
#include "M_Dog.h"
#include "M_HellKnight.h"
#include "M_Knight.h"
#include "M_Ogre.h"
#include "M_Shalrath.h"
#include "M_Shambler.h"
#include "M_Wizard.h"
#include "M_Zombie.h"

#if 0


#include <Engine/World/Public/Pawn.h>

#include "QuakeModelFrame.h"

class FQuakeMonster : public FPawn {
    AN_ACTOR( FQuakeMonster, FPawn )

public:
    void SetModel( FQuakeModel * _Model );

protected:

    FQuakeMonster();

    void PreInitializeComponents() override;
    void PostInitializeComponents() override;
    void BeginPlay() override;
    void EndPlay() override;
    void Tick( float _TimeStep ) override;

private:
    FQuakeModelFrame * Frame;
    int FramesCount;
    Float AnimationTime;
};
#endif
