#pragma once
#include <string>
class ISceneInterface
{
public:
    virtual void Initialize() = 0; // 씬 초기화
    virtual void Load(class ID3D11Device* Device , class ID3D11DeviceContext* DeviceContext) = 0;       // 씬 자원 로드
    virtual void Unload() = 0;     // 씬 자원 해제
    virtual void Update(float DeltaTime) = 0; // 씬 업데이트
    virtual void SubmitRender(class URenderer* Renderer) = 0; // 씬 렌더링
    virtual void SubmitRenderUI() = 0;  //UI 렌더링 요청
    virtual void HandleInput(const class FKeyEventData& EventData) = 0; // 입력 처리
    virtual class UCamera* GetMainCamera() const = 0; //메인 카메라 접근자

    virtual std::string& GetName() = 0; //Scene 이름
};