/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include <Hork/Runtime/GameApplication/GameApplication.h>
#include <Hork/Runtime/World/World.h>

HK_NAMESPACE_BEGIN

class UIGrid;
class UIViewport;
class UIImage;

class SampleApplication final : public GameApplication
{
public:
    SampleApplication(ArgumentPack const& args);
    ~SampleApplication();

    void Initialize();
    void Deinitialize();

private:
    void CreateResources();
    void CreateScene();
    GameObject* CreatePlayer(Float3 const& position, Quat const& rotation);
    void Pause();
    void Quit();
    void ToggleWireframe();
    void Screenshot();
    void ShowLoadingScreen(bool show);
    void PlayAnimation();
    void SpawnPaladin();
    void SpawnPaladin(Float3 const& position, Quat const& rotation, int anim);
    void ShowSkeleton();

    void OnStartLoading();
    void OnUpdateLoading(float timeStep);
    void OnStartPlay();

    UIDesktop* m_Desktop;
    UIViewport* m_Viewport;
    UIWidget* m_LoadingScreen;
    ResourceAreaID m_Resources;
    TextureHandle m_LoadingTexture;
    World* m_World{};
    Ref<WorldRenderView> m_WorldRenderView;
    GameObjectHandle m_Player;
    Handle32<class AnimatorComponent> m_Animator;
    bool m_Play{};
};

HK_NAMESPACE_END
