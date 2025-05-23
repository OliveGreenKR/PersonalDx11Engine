#pragma once
#include <memory>
#include <vector>
#include "GameObject.h"
#include "Camera.h"
#include "SceneInterface.h"
#include "ElasticBody.h"
#include "Model.h"
#include "InputContext.h"
#include "ResourceHandle.h"


class UGameplayScene02 : public ISceneInterface
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
    class UCamera* GetMainCamera() const override;

    std::string& GetName() override { return SceneName; } 

    void SetMaxSpeeds(const float InMaxSpeed);
    void SetPowerMagnitude(const float InMagnitude);

    float GetMaxSpeeds() const { return MaxSpeed; }
    float GetPowerMagnitude() const { return PowerMagnitude; }

public:
    UGameplayScene02();
    ~UGameplayScene02();

private:
    void SetupInput();
    void SetupBorderTriggers();

private:
    float MaxSpeed = 5.0f;
    float PowerMagnitude = 100.0f;

    // 객체 및 카메라
    std::shared_ptr<UCamera> Camera;
    std::shared_ptr<UElasticBody> Character;
    std::shared_ptr<UElasticBody> Character2;

    // 씬 이름
    std::string SceneName = "GameplayScene02";

    // 물리 관련 변수
    float CharacterMass = 15.0f;
    float Character2Mass = 5.0f;
     
    //매터리얼
    FResourceHandle TileMaterialHandle;
    FResourceHandle PoleMaterialHandle;
    FResourceHandle DefaultMaterialHandle;

    // 경계 값
    const float XBorder = 300.0f;
    const float YBorder = 200.0f;
    const float ZBorder = 500.0f;

    // 입력 컨텍스트
    std::shared_ptr<UInputContext> InputContext;
};
