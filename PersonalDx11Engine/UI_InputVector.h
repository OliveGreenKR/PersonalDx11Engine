#pragma once
#include "Math.h"
#include "UIManager.h"
#include <string>
#include <functional>
#include <memory>
#include <atomic>
#include <type_traits>

// UI 벡터 입력 생성 클래스
class FUIVectorInput
{
#pragma region Internal
private:
    // 벡터 입력 내용 렌더링 (템플릿)
    template<typename VectorType>
    static void DrawVectorInputContent(
        VectorType& InVector,
        const std::string& Label,
        std::function<void(const VectorType&)> OnValueChanged)
    {
        static_assert(std::is_same_v<VectorType, Vector2> ||
                      std::is_same_v<VectorType, Vector3> ||
                      std::is_same_v<VectorType, Vector4>,
                      "Only Vector2, Vector3, Vector4 are supported");

        VectorType previousValue = InVector;
        bool bValueChanged = false;

        if constexpr (std::is_same_v<VectorType, Vector2>)
        {
            bValueChanged = ImGui::InputFloat2(Label.c_str(), &InVector.x, "%.3f");
        }
        else if constexpr (std::is_same_v<VectorType, Vector3>)
        {
            bValueChanged = ImGui::InputFloat3(Label.c_str(), &InVector.x, "%.3f");
        }
        else if constexpr (std::is_same_v<VectorType, Vector4>)
        {
            bValueChanged = ImGui::InputFloat4(Label.c_str(), &InVector.x, "%.3f");
        }

        // 값이 변경되었고 콜백이 있다면 호출
        if (bValueChanged && OnValueChanged &&
            !Math::IsEqual(previousValue, InVector))
        {
            OnValueChanged(InVector);
        }

        // 추가 정보 표시
        ShowVectorInfo(InVector);
    }

    // 컬러 입력 내용 렌더링
    static void DrawColorInputContent(
        Vector4& InColor,
        const std::string& Label,
        std::function<void(const Vector4&)> OnValueChanged)
    {
        Vector4 previousValue = InColor;
        bool bValueChanged = false;

        // 컬러 선택기
        if (ImGui::ColorEdit4((Label + " Picker").c_str(), &InColor.x))
        {
            bValueChanged = true;
        }

        // 수치 입력
        if (ImGui::InputFloat4((Label + " Values").c_str(), &InColor.x, "%.3f"))
        {
            ClampColorValues(InColor);
            bValueChanged = true;
        }

        // 값 변경 콜백
        if (bValueChanged && OnValueChanged &&
            !Math::IsEqual(previousValue, InColor))
        {
            OnValueChanged(InColor);
        }
    }
    // 스레드 안전한 고유 ID 생성
    static std::atomic<uint64_t> NextUIId;

    // 벡터 참조 래퍼 - 생명주기 관리
    template<typename VectorType>
    struct FVectorReference
    {
        VectorType* VectorPtr;
        std::string WindowName;
        std::function<void(const VectorType&)> OnValueChanged;

        FVectorReference(VectorType* InPtr, const std::string& InName,
                         std::function<void(const VectorType&)> InCallback)
            : VectorPtr(InPtr), WindowName(InName), OnValueChanged(InCallback)
        {
        }

        bool IsValid() const { return VectorPtr != nullptr; }
    };

    // 내부 벡터 입력 구현
    template<typename VectorType>
    static FUIElement CreateVectorInputInternal(
        VectorType& InVector,
        const std::string& WindowName,
        std::function<void(const VectorType&)> OnValueChanged,
        const std::string& DefaultPrefix)
    {
        static_assert(std::is_same_v<VectorType, Vector2> ||
                      std::is_same_v<VectorType, Vector3> ||
                      std::is_same_v<VectorType, Vector4>,
                      "Only Vector2, Vector3, Vector4 are supported");

        std::string finalWindowName = WindowName.empty() ?
            GenerateWindowName(DefaultPrefix) : WindowName;

        auto reference = std::make_shared<FVectorReference<VectorType>>(
            &InVector, finalWindowName, OnValueChanged);

        FUIElement element;
        element.WindowName = finalWindowName;
        element.DrawFunction = [reference]()
            {
                if (!reference->IsValid()) return;

                if (ImGui::Begin(reference->WindowName.c_str()))
                {
                    DrawVectorInputContent(*reference->VectorPtr, "Input", reference->OnValueChanged);
                }
                ImGui::End();
            };

        return element;
    }

#pragma endregion
public:
    // Vector2 입력 UI 생성
    static FUIElement CreateVector2Input(
        Vector2& InVector,
        const std::string& WindowName = "",
        std::function<void(const Vector2&)> OnValueChanged = nullptr)
    {
        return CreateVectorInputInternal(InVector, WindowName, OnValueChanged, "Vector2");
    }

    // Vector2 입력 내용만 렌더링 (윈도우 없이)
    static void DrawVector2Input(
        Vector2& InVector,
        const std::string& Label = "Vector2",
        std::function<void(const Vector2&)> OnValueChanged = nullptr)
    {
        DrawVectorInputContent(InVector, Label, OnValueChanged);
    }

    // Vector3 입력 UI 생성
    static FUIElement CreateVector3Input(
        Vector3& InVector,
        const std::string& WindowName = "",
        std::function<void(const Vector3&)> OnValueChanged = nullptr)
    {
        return CreateVectorInputInternal(InVector, WindowName, OnValueChanged, "Vector3");
    }

    // Vector3 입력 내용만 렌더링 (윈도우 없이)
    static void DrawVector3Input(
        Vector3& InVector,
        const std::string& Label = "Vector3",
        std::function<void(const Vector3&)> OnValueChanged = nullptr)
    {
        DrawVectorInputContent(InVector, Label, OnValueChanged);
    }

    // Vector4 입력 UI 생성
    static FUIElement CreateVector4Input(
        Vector4& InVector,
        const std::string& WindowName = "",
        std::function<void(const Vector4&)> OnValueChanged = nullptr)
    {
        return CreateVectorInputInternal(InVector, WindowName, OnValueChanged, "Vector4");
    }

    // Vector4 입력 내용만 렌더링 (윈도우 없이)
    static void DrawVector4Input(
        Vector4& InVector,
        const std::string& Label = "Vector4",
        std::function<void(const Vector4&)> OnValueChanged = nullptr)
    {
        DrawVectorInputContent(InVector, Label, OnValueChanged);
    }

    // 컬러 선택기가 포함된 Vector4 입력 (RGBA)
    static FUIElement CreateColorInput(
        Vector4& InColor,
        const std::string& WindowName = "",
        std::function<void(const Vector4&)> OnValueChanged = nullptr)
    {
        std::string finalWindowName = WindowName.empty() ?
            GenerateWindowName("ColorPicker") : WindowName;

        auto reference = std::make_shared<FVectorReference<Vector4>>(
            &InColor, finalWindowName, OnValueChanged);

        FUIElement element;
        element.WindowName = finalWindowName;
        element.DrawFunction = [reference]()
            {
                if (!reference->IsValid()) return;

                if (ImGui::Begin(reference->WindowName.c_str()))
                {
                    DrawColorInputContent(*reference->VectorPtr, "Color", reference->OnValueChanged);
                }
                ImGui::End();
            };

        return element;
    }

    // 컬러 입력 내용만 렌더링 (윈도우 없이)
    static void DrawColorInput(
        Vector4& InColor,
        const std::string& Label = "Color",
        std::function<void(const Vector4&)> OnValueChanged = nullptr)
    {
        DrawColorInputContent(InColor, Label, OnValueChanged);
    }

private:

    // 고유 윈도우 이름 생성
    static std::string GenerateWindowName(const std::string& Prefix)
    {
        return Prefix + "_" + std::to_string(NextUIId.fetch_add(1));
    }

    // 컬러 값 범위 제한
    static void ClampColorValues(Vector4& Color)
    {
        Color.x = Math::Clamp(Color.x, 0.0f, 1.0f);
        Color.y = Math::Clamp(Color.y, 0.0f, 1.0f);
        Color.z = Math::Clamp(Color.z, 0.0f, 1.0f);
        Color.w = Math::Clamp(Color.w, 0.0f, 1.0f);
    }

    // 벡터 정보 표시 (길이, 정규화 등)
    template<typename VectorType>
    static void ShowVectorInfo(const VectorType& Vector)
    {
        if constexpr (std::is_same_v<VectorType, Vector2> ||
                      std::is_same_v<VectorType, Vector3>)
        {
            float length = Vector.Length();
            ImGui::Text("Length: %.3f", length);

            if (length > 1e-6f)
            {
                auto normalized = Vector.GetNormalized();
                if constexpr (std::is_same_v<VectorType, Vector2>)
                {
                    ImGui::Text("Normalized: (%.3f, %.3f)", normalized.x, normalized.y);
                }
                else
                {
                    ImGui::Text("Normalized: (%.3f, %.3f, %.3f)", normalized.x, normalized.y, normalized.z);
                }
            }
        }
        else if constexpr (std::is_same_v<VectorType, Vector4>)
        {
            float length = Vector.Length();
            ImGui::Text("Length: %.3f", length);
        }
    }
};

// 정적 멤버 정의
std::atomic<uint64_t> FUIVectorInput::NextUIId{ 0 };