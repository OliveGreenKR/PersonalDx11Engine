#pragma once
#include <memory>
#include <unordered_map>
#include <string>

class UWorldManager
{
public:
    static UWorldManager* Get()
    {
        static UWorldManager* instance = []() {
            UWorldManager* manager = new UWorldManager();
            return manager;
            }();

        return instance;
    }

    // 현재 활성 카메라 설정/획득
    void SetActiveCamera(class std::shared_ptr<class UCamera> Camera)
    {
        if (Camera.get())
        {
            ActiveCamera = Camera;
        }
    }
    class UCamera* GetActiveCamera() const { return ActiveCamera.lock().get(); }

    // 특정 씬의 카메라 등록/해제
    void RegisterSceneCamera(const std::string& SceneName, std::shared_ptr<class UCamera>& Camera)
    {
        if (!Camera.get())
        {
            return;
        }
        SceneCameras[SceneName] = Camera;
    }
    void UnregisterSceneCamera(const std::string& SceneName)
    {
        auto it = SceneCameras.find(SceneName);
        if (it != SceneCameras.end())
        {
            SceneCameras.erase(it);
        }
    }

private:
    class std::weak_ptr<class UCamera> ActiveCamera;
    std::unordered_map<std::string, std::weak_ptr<class UCamera>> SceneCameras;
};