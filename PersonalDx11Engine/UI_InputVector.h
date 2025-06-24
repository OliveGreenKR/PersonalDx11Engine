#pragma once
#include "Math.h"
#include "UIManager.h"
#include <functional>
#include <memory>
#include <atomic>
#include <type_traits>
#include <cstring>
#include <cstdio>

// C 스타일 문자열 최적화된 UI 벡터 입력 생성 클래스
class FUIVectorInput
{
private:
    // 컴파일 타임 C 스타일 문자열 리터럴
    static constexpr const char* VECTOR2_LABEL = "Vector2";
    static constexpr const char* VECTOR3_LABEL = "Vector3";
    static constexpr const char* VECTOR4_LABEL = "Vector4";
    static constexpr const char* COLOR_LABEL = "Color";

    // 컴파일 타임 고정 서브 라벨들
    static constexpr const char* COLOR_PICKER_ID = "##ColorPicker";
    static constexpr const char* COLOR_VALUES_ID = "##ColorValues";

public:
    // Vector2 입력 내용만 렌더링 (C 스타일 문자열 사용)
    static void DrawVector2Input(
        Vector2& InVector,
        const char* Label = VECTOR2_LABEL,
        std::function<void(const Vector2&)> OnValueChanged = nullptr)
    {
        DrawVectorInputContent(InVector, Label, OnValueChanged);
    }

    // Vector3 입력 내용만 렌더링
    static void DrawVector3Input(
        Vector3& InVector,
        const char* Label = VECTOR3_LABEL,
        std::function<void(const Vector3&)> OnValueChanged = nullptr)
    {
        DrawVectorInputContent(InVector, Label, OnValueChanged);
    }

    // Vector4 입력 내용만 렌더링
    static void DrawVector4Input(
        Vector4& InVector,
        const char* Label = VECTOR4_LABEL,
        std::function<void(const Vector4&)> OnValueChanged = nullptr)
    {
        DrawVectorInputContent(InVector, Label, OnValueChanged);
    }

    // 컬러 입력 내용만 렌더링
    static void DrawColorInput(
        Vector4& InColor,
        const char* Label = COLOR_LABEL,
        std::function<void(const Vector4&)> OnValueChanged = nullptr)
    {
        DrawColorInputContent(InColor, Label, OnValueChanged);
    }

private:
    // C 스타일 최적화된 벡터 입력 내용 렌더링
    template<typename VectorType>
    static void DrawVectorInputContent(
        VectorType& InVector,
        const char* Label,  // C 스타일 문자열 직접 사용
        std::function<void(const VectorType&)> OnValueChanged)
    {
        static_assert(std::is_same_v<VectorType, Vector2> ||
                      std::is_same_v<VectorType, Vector3> ||
                      std::is_same_v<VectorType, Vector4>,
                      "Only Vector2, Vector3, Vector4 are supported");

        VectorType previousValue = InVector;
        bool bValueChanged = false;

        // C 스타일 문자열 검증 (Release 빌드에서는 최적화로 제거됨)
        if (!Label)
        {
            Label = "Unknown";
        }

        // 입력 컨트롤 (ImGui는 C 스타일 문자열을 직접 사용하므로 최적)
        if constexpr (std::is_same_v<VectorType, Vector2>)
        {
            bValueChanged = ImGui::InputFloat2(Label, &InVector.x, "%.3f");
        }
        else if constexpr (std::is_same_v<VectorType, Vector3>)
        {
            bValueChanged = ImGui::InputFloat3(Label, &InVector.x, "%.3f");
        }
        else if constexpr (std::is_same_v<VectorType, Vector4>)
        {
            bValueChanged = ImGui::InputFloat4(Label, &InVector.x, "%.3f");
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

    // C 스타일 최적화된 컬러 입력 내용 렌더링
    static void DrawColorInputContent(
        Vector4& InColor,
        const char* Label,
        std::function<void(const Vector4&)> OnValueChanged)
    {
        Vector4 previousValue = InColor;
        bool bValueChanged = false;

        // C 스타일 문자열 검증
        if (!Label)
        {
            Label = COLOR_LABEL;
        }

        // 메모리 주소 기반 고유 ID 생성 (문자열 처리 완전 제거)
        ImGui::PushID(reinterpret_cast<const void*>(&InColor));

        // 컬러 에디터 (컴파일 타임 상수 ID 사용)
        if (ImGui::ColorEdit4(COLOR_PICKER_ID, &InColor.x))
        {
            bValueChanged = true;
        }

        // 수치 입력 (컴파일 타임 상수 ID 사용)
        if (ImGui::InputFloat4(COLOR_VALUES_ID, &InColor.x, "%.3f"))
        {
            ClampColorValues(InColor);
            bValueChanged = true;
        }

        ImGui::PopID();

        // 값 변경 콜백
        if (bValueChanged && OnValueChanged &&
            !Math::IsEqual(previousValue, InColor))
        {
            OnValueChanged(InColor);
        }

        // 컬러 정보 표시
        ShowColorInfo(InColor);
    }

    // 컬러 값 범위 제한 (수학 연산만 사용, 문자열 처리 없음)
    static void ClampColorValues(Vector4& Color)
    {
        Color.x = Math::Clamp(Color.x, 0.0f, 1.0f);
        Color.y = Math::Clamp(Color.y, 0.0f, 1.0f);
        Color.z = Math::Clamp(Color.z, 0.0f, 1.0f);
        Color.w = Math::Clamp(Color.w, 0.0f, 1.0f);
    }

    // 벡터 정보 표시 (C printf 스타일 최적화)
    template<typename VectorType>
    static void ShowVectorInfo(const VectorType& Vector)
    {
        if constexpr (std::is_same_v<VectorType, Vector2> ||
                      std::is_same_v<VectorType, Vector3>)
        {
            float length = Vector.Length();
            // ImGui::Text는 내부적으로 C printf를 사용하므로 최적
            ImGui::Text("Length: %.3f", length);

            constexpr float EPSILON = 1e-6f;
            if (length > EPSILON)
            {
                auto normalized = Vector.GetNormalized();
                if constexpr (std::is_same_v<VectorType, Vector2>)
                {
                    ImGui::Text("Normalized: (%.3f, %.3f)", normalized.x, normalized.y);
                }
                else
                {
                    ImGui::Text("Normalized: (%.3f, %.3f, %.3f)",
                                normalized.x, normalized.y, normalized.z);
                }
            }
        }
        else if constexpr (std::is_same_v<VectorType, Vector4>)
        {
            float length = Vector.Length();
            ImGui::Text("Length: %.3f", length);
        }
    }

    // 컬러 정보 표시 (C printf 스타일 최적화)
    static void ShowColorInfo(const Vector4& Color)
    {
        // ImGui::Text는 C 스타일 포맷팅을 직접 사용하므로 최적
        ImGui::Text("HSV: ");
        ImGui::SameLine();

        float h, s, v;
        ImGui::ColorConvertRGBtoHSV(Color.x, Color.y, Color.z, h, s, v);
        ImGui::Text("H:%.0f° S:%.0f%% V:%.0f%%", h * 360.0f, s * 100.0f, v * 100.0f);

        // 정수 변환을 명시적 캐스트로 최적화
        ImGui::Text("Hex: #%02X%02X%02X%02X",
                    static_cast<int>(Color.x * 255.0f),
                    static_cast<int>(Color.y * 255.0f),
                    static_cast<int>(Color.z * 255.0f),
                    static_cast<int>(Color.w * 255.0f));
    }
};