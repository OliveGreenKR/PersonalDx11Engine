#pragma once
#include "SceneInterface.h"
#include <string>
#include <unordered_map>
#include <memory>

class USceneManager
{
private:
    std::unordered_map<std::string, std::shared_ptr<ISceneInterface>> Scenes;
    std::shared_ptr<ISceneInterface> ActiveScene;
    std::shared_ptr<ISceneInterface> PendingScene;
    bool bIsTransitioning = false;

    USceneManager() = default;

public:

    static USceneManager* Get()
    {
        static USceneManager* instance = []() {
            USceneManager* manager = new USceneManager();
            return manager;
            }();

        return instance;
    }

    // 씬 등록
    void RegisterScene(const std::shared_ptr<ISceneInterface>& Scene)
    {
        if (Scene)
        {
            Scenes[Scene->GetName()] = Scene;
        }
    }

    // 씬 전환
    void ChangeScene(const std::string& SceneName)
    {
        auto it = Scenes.find(SceneName);
        if (it != Scenes.end())
        {
            PendingScene = it->second;
            bIsTransitioning = true;
        }
    }

    // 씬 업데이트
    void Tick(float DeltaTime)
    {
        // 씬 전환 처리
        if (bIsTransitioning && PendingScene)
        {
            if (ActiveScene)
            {
                ActiveScene->Unload();
            }

            ActiveScene = PendingScene;
            PendingScene = nullptr;
            
            ActiveScene->Load();
            ActiveScene->Initialize();
     
            bIsTransitioning = false;
        }

        // 활성 씬 업데이트
        if (ActiveScene)
        {
            ActiveScene->Tick(DeltaTime);
        }
    }

    // 씬 렌더링
    void Render(class URenderer* Renderer)
    {
        if (ActiveScene && Renderer)
        {
            ActiveScene->SubmitRender(Renderer);
        }

    }

    void RenderUI()
    {
        if (ActiveScene)
        {
            ActiveScene->SubmitRenderUI();
        }
    }

    class UCamera* GetActiveCamera()
    {
        return ActiveScene->GetMainCamera();
    }

    // 활성 씬 반환
    std::shared_ptr<ISceneInterface> GetActiveScene() const
    {
        return ActiveScene;
    }
};