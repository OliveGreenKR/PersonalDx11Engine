#pragma once
#include <memory>
#include <vector>
#include "GameObject.h"
#include "Camera.h"
#include "SceneInterface.h"
#include "ElasticBody.h"
#include "D3DShader.h"
#include "Model.h"
#include "InputContext.h"
#include "ResourceHandle.h"
#include "FixedObjectPool.h"

class UGameplayScene01 : public ISceneInterface
{
public:
    // Inherited via ISceneInterface
    void Initialize() override;
    void Load() override;
    void Unload() override;
    void Tick(float DeltaTime) override;
    void SubmitRender(URenderer* Renderer) override;
    void SubmitRenderUI() override;
    void HandleInput(const FKeyEventData& EventData) override;
    class UCamera* GetMainCamera() const override { return Camera.get(); }
     
    std::string& GetName() override { return SceneName; }

public:
    UGameplayScene01();
    ~UGameplayScene01() = default;

private:
    void SetupInput();
    void SetupBorderTriggers(std::weak_ptr<UElasticBody>& InBody);
    void SpawnElasticBody();
    void DeSpawnElasticBody(std::weak_ptr<UElasticBody>& InBody);

private:
    // 객체 및 카메라
    std::shared_ptr<UCamera> Camera;
    std::vector<std::shared_ptr<UElasticBody>> ElasticBodies;

    // 씬 이름
    std::string SceneName = "GameplayScene01";

    // 스폰 관련 변수
    float AccumTime = 0.0f;
    const float SPAWN_FREQUENCY = 0.75f;
    bool bSpawnBody = true;
    bool bGravity = false;

    // 경계 값
    const float XBorder = 3.0f;
    const float YBorder = 2.0f;
    const float ZBorder = 5.0f;

    //리소스
    FResourceHandle PoleMaterialHandle;

    // 입력 컨텍스트
    std::shared_ptr<UInputContext> InputContext;

    //탄성체 풀
    TFixedObjectPool<UElasticBody> ElasticBodyPool;


};