#pragma once
#include <windows.h>
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"
#include <functional>

// UI 데이터 구조체
struct FUIElement {
    std::string WindowName;
    std::function<void()> DrawFunction;
};

// ImGuiManager.h
class UUIManager
{
private:
    std::vector<FUIElement> UIElements;
    bool bIsFrameActive = false;

    UUIManager() = default;


public:
    static UUIManager* Get() {
        static UUIManager* manager = []() {
            UUIManager* instance = new UUIManager;
            return instance;
            }();
        return manager;
    }
    // UI 요소 등록
    void RegisterUIElement(const std::string& WindowName, const std::function<void()>& DrawFunction) {
        UIElements.push_back({ WindowName, DrawFunction });
    }

    void Initialize(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* context) 
    {
    // ImGui 초기화 코드
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        // 필요한 설정
        ImGui::SetNextWindowPos(ImVec2(2.0f, 2.0f), ImGuiCond_FirstUseEver);//영구위치설정
        ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver); //자동조절

        ImGui_ImplWin32_Init(hWnd);
        ImGui_ImplDX11_Init(device, context);
    }
    void Shutdown()
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
// 모든 UI 렌더링 (메인 루프에서 호출)
    void RenderUI() {

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        bIsFrameActive = true;

        // 등록된 모든 UI 요소 그리기
        for (const auto& element : UIElements) {
            element.DrawFunction();
        }

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        bIsFrameActive = false;


        UIElements.clear();
    }

};