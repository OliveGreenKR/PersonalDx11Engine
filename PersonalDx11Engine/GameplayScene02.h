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
    class UCamera* GetMainCamera() const override { return Camera.get(); }

    std::string& GetName() override { return SceneName; }

    void SetMaxSpeeds(const float InMaxSpeed);
    void SetPowerMagnitude(const float InMagnitude);

    float GetMaxSpeeds() const { return MaxSpeed; }
    float GetPowerMagnitude() const { return PowerMagnitude; }

public:
    UGameplayScene02();
    ~UGameplayScene02() = default;

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
    float CharacterMass = 5.0f;
    float Character2Mass = 15.0f;

    // 텍스처 
    std::shared_ptr<class UTexture2D> TextureTile;
    std::shared_ptr<class UTexture2D> TexturePole;
    std::shared_ptr<class UShader> Shader;

    // 경계 값
    const float XBorder = 3.0f;
    const float YBorder = 2.0f;
    const float ZBorder = 5.0f;

    // 입력 컨텍스트
    std::shared_ptr<UInputContext> InputContext;
};
