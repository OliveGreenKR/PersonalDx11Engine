#pragma once
class UScene
{
public:
    virtual void Initialize() = 0; // 씬 초기화
    virtual void Load() = 0;       // 씬 자원 로드
    virtual void Unload() = 0;     // 씬 자원 해제
    virtual void Update(float DeltaTime) = 0; // 씬 업데이트
    virtual void Render(class URenderer* Renderer) = 0; // 씬 렌더링
    virtual void HandleInput(const class FKeyEventData& EventData) = 0; // 입력 처리
};