#pragma once

#include <Engine/Core/BaseTypes.h>

HK_NAMESPACE_BEGIN

struct WorldTick
{
    int         PrevStateIndex{};
    int         StateIndex{};
    uint64_t    FrameNum{};
    uint64_t    FixedFrameNum{};
    double      FixedTime{};
    double      FrameTime{};
    double      RunningTime{};
    float       FixedTimeStep{};
    float       FrameTimeStep{};
    float       Interpolate{};
    bool        IsPaused{};
};

HK_NAMESPACE_END
