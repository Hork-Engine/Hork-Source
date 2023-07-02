#pragma once

HK_NAMESPACE_BEGIN

struct Velocity
{
    float x;
    float y;
    float z;

    Velocity() = default;
    Velocity(float x, float y, float z) :
        x(x), y(y), z(z) {}
};

struct AngularVelocity
{
    float vel;
};

HK_NAMESPACE_END
