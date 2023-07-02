#pragma once

HK_NAMESPACE_BEGIN

struct GameFrame
{
    int PrevStateIndex{};
    int StateIndex{};
    uint64_t FrameNum{};
    uint64_t FixedFrameNum{};
    double FixedTime{};
    double VariableTime{};
    double RunningTime{};
    float FixedTimeStep{};
    float VariableTimeStep{};
    float Interpolate{};
};

HK_NAMESPACE_END
