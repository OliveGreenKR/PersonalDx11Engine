#pragma once
#include "SceneInterface.h"
#include <string>
#include <unordered_map>
#include <memory>
#include "Delegate.h"

class USceneManager
{
private:
    std::vector<std::shared_ptr<ISceneInterface>> Scenes;
    std::unordered_map<std::string, std::uint32_t> SceneNameMap;

    std::shared_ptr<ISceneInterface> ActiveScene;
    std::shared_ptr<ISceneInterface> PendingScene;
    bool bIsTransitioning = false;

    float LastTickTime = 0.0f;

    USceneManager() = default;

public:

    TDelegate<> OnSceneChanged = TDelegate<>();

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
            SceneNameMap[Scene->GetName()] = Scenes.size();
            Scenes.push_back(Scene);
        }
    }

    // 씬 전환
    bool ChangeScene(const std::string& SceneName)
    {
        auto it = SceneNameMap.find(SceneName);
        if (it == SceneNameMap.end())
        {
            return false;
        }
        return ChangeScene(it->second);
    }
    bool ChangeScene(const uint32_t SceneIndex)
    {
        if (SceneIndex >= Scenes.size())
            return false;

        auto ScenePtr = Scenes[SceneIndex];
        if (ScenePtr)
        {
            PendingScene = ScenePtr;
            bIsTransitioning = true;
        }   

        return bIsTransitioning;
    }

    // 씬 업데이트
    void Tick(float DeltaTime)
    {
        LastTickTime = DeltaTime;

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
            OnSceneChanged.Broadcast();
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

    float GetLastTickTime() const { return LastTickTime; }

    std::weak_ptr<ISceneInterface> GetScene(std::uint32_t Index)
    {
        if (Index >= Scenes.size())
        {
            return std::weak_ptr<ISceneInterface>();
        }

        return Scenes[Index];
    }
    std::uint32_t GetRegisteredScenesNum() const { return Scenes.size(); }
};