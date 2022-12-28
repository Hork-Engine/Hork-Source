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

#include "GameSession.h"
#include "ResourceManager.h"

HK_NAMESPACE_BEGIN

GameSession::GameSession()
{

}

GameSession::~GameSession()
{

}

void GameSession::Start()
{
    m_PrecacheResources.Clear();

    Rand.Seed(0xc0dec0de);
}

void GameSession::Stop()
{
    m_PrecacheResources.Clear();
    m_Resources.Clear();
}

void GameSession::PrecacheResource(ClassMeta const& classMeta, StringView path)
{
    m_PrecacheResources[path] = &classMeta;
}

void GameSession::LoadResources()
{
    ResourceManager* resourceManager = GEngine->GetResourceManager();

    for (auto& it : m_PrecacheResources)
    {
        TRef<Resource> resource;
        resource = resourceManager->GetOrCreateResource(*it.second, it.first);
        m_Resources.Add(resource);
    }
}

void GameSession::UnloadResources()
{
    m_Resources.Clear();
}

HK_NAMESPACE_END
