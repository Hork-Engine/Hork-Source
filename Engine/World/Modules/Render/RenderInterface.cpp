/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include "RenderInterfaceImpl.h"

#include <Engine/Core/ConsoleVar.h>

HK_NAMESPACE_BEGIN

RenderInterfaceImpl::RenderInterfaceImpl()
{
}

RenderInterface::RenderInterface() :
    m_pImpl(new RenderInterfaceImpl)
{
}

void RenderInterface::Initialize()
{
    RegisterDebugDrawFunction({this, &RenderInterface::DrawDebug});
}

void RenderInterface::Deinitialize()
{
}

void RenderInterface::DrawDebug(DebugRenderer& renderer)
{

}


#if 0
class EnvironmentMapCache
{
public:
    uint32_t AddMap(EnvironmentMapData const& data)
    {

    }

    void RemoveMap(uint32_t id)
    {

    }

    void Touch(uint32_t id)
    {
        if (m_NumPending < MAX_GPU_RESIDENT)
            m_Pending[m_NumPending++] = id;
        else
            LOG("MAX_GPU_RESIDENT hit\n");
    }

    void Update()
    {
        if (m_NumPending > 0)
        {
            int j = 0;
            for (int i = 0 ; i < m_NumPending ; i++)
            {
                auto& envMap = m_EnvMaps[m_Pending[i]];

                if (envMap.Index != -1)
                    m_Resident[envMap.Index].TouchTime = curtime;
                else
                    m_Pending[j] = m_Pending[i];
            }

            // Sort m_Resident by TouchTime

            for (int i = 0 ; i < MAX_GPU_RESIDENT && j > 0 ; i++)
            {
                if (m_Resident[i].TouchTime < curtime)
                {
                    m_Resident[i].TouchTime = curtime;
                    m_Resident[i].EnvMapID = m_Pending[--j];

                    auto& envMap = m_EnvMaps[m_Resident[i].EnvMapID];
                    envMap.Index = i;

                    UploadEnvironmentMap(i);
                }
            }

            HK_ASSERT(j == 0);

            m_NumPending = 0;
        }
    }

    void UploadEnvironmentMap(int index)
    {
        uint32_t id = m_Resident[index].EnvMapID;

        // TODO: Here upload from m_EnvMaps[id] to m_ReflectionMapArray[index], m_IrradianceMapArray[index]
    }

    // CPU cache
    Vector<TUniquePtr<EnvironmentMapData>> m_EnvMaps;
    Vector<uint32_t> m_FreeList;

    static constexpr uint32_t MAX_GPU_RESIDENT = 256;

    // GPU cache
    Ref<RenderCore::ITexture> m_IrradianceMapArray;
    Ref<RenderCore::ITexture> m_ReflectionMapArray;

    struct Resident
    {
        uint32_t EnvMapID;
        uint32_t TouchTime;
    };
    uint32_t m_Resident[MAX_GPU_RESIDENT];

    uint32_t m_Pending[MAX_GPU_RESIDENT];
    uint32_t m_NumPending{};


};
#endif

HK_NAMESPACE_END