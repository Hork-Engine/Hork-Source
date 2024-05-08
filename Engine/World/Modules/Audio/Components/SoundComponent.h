#pragma once

#include "../SoundSource.h"

HK_NAMESPACE_BEGIN

struct SoundComponent
{
    TUniqueRef<SoundSource> Source;
};

HK_NAMESPACE_END
