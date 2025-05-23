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
    ~UGameplayScene01();

private:
    void SetupInput();
    void SetupBorderTriggers(UElasticBody* InBody);
    void SpawnElasticBody();
    void DeSpawnElasticBody();
    void LoadConfigFromIni();

private:
    //카메라 움직임
    float LongitudeThreshold = 180 - KINDA_SMALL;
    float LatitudeThreshold = 89.0f - KINDA_SMALL;
    float Latitude = 0.0f;
    float Longitude = 0.0f;
    Vector3 CalculateSphericPosition(float Latidue, float Longitude, float radius);

private:
    // 객체 및 카메라
    std::shared_ptr<UCamera> Camera;

    // 씬 이름
    std::string SceneName = "GameplayScene01";

    // 스폰 관련 변수
    float AccumTime = 0.0f;
    const float SPAWN_FREQUENCY = 0.75f;
    bool bSpawnBody = true;
    bool bGravity = false;

    int BodyNum = 0;

    // 경계 값
    float XBorder = 300.0f;
    float YBorder = 300.0f;
    float ZBorder = 300.0f;

    

    //리소스
    FResourceHandle PoleMaterialHandle;

    // 입력 컨텍스트
    std::shared_ptr<UInputContext> InputContext;

    //탄성체 풀
    std::unique_ptr<TFixedObjectPool<UElasticBody,512>> ElasticBodyPool;


};