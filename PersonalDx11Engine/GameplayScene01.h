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

class UGameplayScene01 : public ISceneInterface
{
public:
    // Inherited via ISceneInterface
    void Initialize() override;
    void Load(class ID3D11Device* Device, class ID3D11DeviceContext* DeviceContext) override;
    void Unload() override;
    void Update(float DeltaTime) override;
    void Render(URenderer* Renderer) override;
    void HandleInput(const FKeyEventData& EventData) override;

    std::string& GetName() override { return SceneName; }

public:
    UGameplayScene01();
    ~UGameplayScene01() = default;

private:
    void SetupInput();
    void SetupBorderTriggers();
    void SpawnElasticBody();

private:
    // 객체 및 카메라
    std::shared_ptr<UCamera> Camera;
    std::shared_ptr<UGameObject> Character;
    std::shared_ptr<UElasticBody> Character2;
    std::vector<std::shared_ptr<UElasticBody>> ElasticBodies;

    // 씬 이름
    std::string SceneName = "GameplayScene01";

    // 스폰 관련 변수
    float AccumTime = 0.0f;
    const float SPAWN_FREQUENCY = 0.75f;
    bool bSpawnBody = true;

    // 물리 관련 변수
    float CharacterMass = 5.0f;
    float Character2Mass = 15.0f;

    // 텍스처 
    std::shared_ptr<ID3D11ShaderResourceView*> TextureTile;
    std::shared_ptr<ID3D11ShaderResourceView*> TexturePole;

    // 경계 값
    const float XBorder = 3.0f;
    const float YBorder = 2.0f;
    const float ZBorder = 5.0f;

    // 입력 컨텍스트
    std::shared_ptr<UInputContext> InputContext;
};