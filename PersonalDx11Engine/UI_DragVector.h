#pragma once
#include "Math.h"
#include "UIManager.h"
#include <functional>
#include <memory>
#include <atomic>
#include <type_traits>
#include <cstring>
#include <cstdio>

// 드래그 파라미터 구조체 (변경 없음)
struct FDragParameters
{
    float Speed = 0.01f;
    float MinValue = 0.0f;
    float MaxValue = 0.0f;
    const char* Format = "%.3f";
    bool bShowSlider = false;

    FDragParameters() = default;
    FDragParameters(float InSpeed, float InMin, float InMax, const char* InFormat, bool InShowSlider = false)
        : Speed(InSpeed), MinValue(InMin), MaxValue(InMax), Format(InFormat), bShowSlider(InShowSlider)
    {
    }

    static FDragParameters Position(float Speed = 0.1f, const char* Format = "%.2f")
    {
        return FDragParameters(Speed, -1000.0f, 1000.0f, Format, false);
    }
    static FDragParameters Scale(float Speed = 0.1f, const char* Format = "%.1f")
    {
        return FDragParameters(Speed, -1000.0f, 1000.0f, Format, false);
    }

    static FDragParameters Color(float Speed = 0.01f, const char* Format = "%.3f")
    {
        return FDragParameters(Speed, 0.0f, 1.0f, Format, true);
    }

    static FDragParameters Normalized(float Speed = 0.01f, const char* Format = "%.3f")
    {
        return FDragParameters(Speed, -1.0f, 1.0f, Format);
    }

    static FDragParameters Rotation(float Speed = 1.0f, const char* Format = "%.1f")
    {
        return FDragParameters(Speed, -360.0f, 360.0f, Format);
    }
};

// C 스타일 문자열 최적화된 UI 벡터 드래그 생성 클래스
class FUIVectorDrag
{
private:
    // 컴파일 타임 C 스타일 문자열 리터럴
    static constexpr const char* VECTOR2_LABEL = "Vector2";
    static constexpr const char* VECTOR3_LABEL = "Vector3";
    static constexpr const char* VECTOR4_LABEL = "Vector4";
    static constexpr const char* COLOR_LABEL = "Color";
    static constexpr const char* EULER_LABEL = "Euler Angles";

    // 문자열 버퍼 크기 상수
    static constexpr size_t MAX_LABEL_SIZE = 64;

public:
    // Vector2 드래그 내용만 렌더링 (C 스타일 문자열 사용)
    static void DrawVector2Drag(
        Vector2& InVector,
        const FDragParameters& DragParams = FDragParameters(),
        const char* Label = VECTOR2_LABEL,
        std::function<void(const Vector2&)> OnValueChanged = nullptr,
        std::function<bool(const Vector2&)> OnValueChanging = nullptr)
    {
        DrawVectorDragContent(InVector, DragParams, Label, OnValueChanged, OnValueChanging);
    }

    // Vector3 드래그 내용만 렌더링
    static void DrawVector3Drag(
        Vector3& InVector,
        const FDragParameters& DragParams = FDragParameters(),
        const char* Label = VECTOR3_LABEL,
        std::function<void(const Vector3&)> OnValueChanged = nullptr,
        std::function<bool(const Vector3&)> OnValueChanging = nullptr)
    {
        DrawVectorDragContent(InVector, DragParams, Label, OnValueChanged, OnValueChanging);
    }

    // Vector4 드래그 내용만 렌더링
    static void DrawVector4Drag(
        Vector4& InVector,
        const FDragParameters& DragParams = FDragParameters(),
        const char* Label = VECTOR4_LABEL,
        std::function<void(const Vector4&)> OnValueChanged = nullptr,
        std::function<bool(const Vector4&)> OnValueChanging = nullptr)
    {
        DrawVectorDragContent(InVector, DragParams, Label, OnValueChanged, OnValueChanging);
    }

    // 컬러 드래그 내용만 렌더링
    static void DrawColorDrag(
        Vector4& InColor,
        const char* Label = COLOR_LABEL,
        std::function<void(const Vector4&)> OnValueChanged = nullptr,
        std::function<bool(const Vector4&)> OnValueChanging = nullptr)
    {
        DrawColorDragContent(InColor, FDragParameters::Color(), Label, OnValueChanged, OnValueChanging);
    }

    // 오일러 각도 드래그 내용만 렌더링
    static void DrawEulerAngleDrag(
        Vector3& InEulerAngles,
        const char* Label = EULER_LABEL,
        std::function<void(const Vector3&)> OnValueChanged = nullptr,
        std::function<bool(const Vector3&)> OnValueChanging = nullptr)
    {
        DrawVectorDragContent(InEulerAngles, FDragParameters::Rotation(), Label, OnValueChanged, OnValueChanging);
    }

private:
    // C 스타일 최적화된 벡터 드래그 내용 렌더링
    template<typename VectorType>
    static void DrawVectorDragContent(
        VectorType& InVector,
        const FDragParameters& DragParams,
        const char* Label,  // C 스타일 문자열 직접 사용
        std::function<void(const VectorType&)> OnValueChanged,
        std::function<bool(const VectorType&)> OnValueChanging)
    {
        static_assert(std::is_same_v<VectorType, Vector2> ||
                      std::is_same_v<VectorType, Vector3> ||
                      std::is_same_v<VectorType, Vector4>,
                      "Only Vector2, Vector3, Vector4 are supported");

        VectorType previousValue = InVector;
        bool bValueChanged = false;
        bool bIsChanging = false;

        // C 스타일 문자열 검증 (Release 빌드에서는 최적화로 제거됨)
        if (!Label)
        {
            Label = "Unknown";
        }

        // 드래그 컨트롤 (ImGui는 C 스타일 문자열 직접 사용)
        if constexpr (std::is_same_v<VectorType, Vector2>)
        {
            bValueChanged = ImGui::DragFloat2(Label, &InVector.x,
                                              DragParams.Speed,
                                              DragParams.MinValue,
                                              DragParams.MaxValue,
                                              DragParams.Format);
            bIsChanging = ImGui::IsItemActive();
        }
        else if constexpr (std::is_same_v<VectorType, Vector3>)
        {
            bValueChanged = ImGui::DragFloat3(Label, &InVector.x,
                                              DragParams.Speed,
                                              DragParams.MinValue,
                                              DragParams.MaxValue,
                                              DragParams.Format);
            bIsChanging = ImGui::IsItemActive();
        }
        else if constexpr (std::is_same_v<VectorType, Vector4>)
        {
            bValueChanged = ImGui::DragFloat4(Label, &InVector.x,
                                              DragParams.Speed,
                                              DragParams.MinValue,
                                              DragParams.MaxValue,
                                              DragParams.Format);
            bIsChanging = ImGui::IsItemActive();
        }

        // 슬라이더 옵션 (완전히 문자열 처리 없음)
        if (DragParams.bShowSlider)
        {
            // 객체 메모리 주소를 해시로 사용하여 고유 ID 생성
            ImGui::PushID(reinterpret_cast<const void*>(&InVector));

            if constexpr (std::is_same_v<VectorType, Vector2>)
            {
                // "##" 접두사는 라벨을 숨기고 ID만 사용 (ImGui 내장 최적화)
                if (ImGui::SliderFloat2("##Slider", &InVector.x,
                                        DragParams.MinValue, DragParams.MaxValue, DragParams.Format))
                {
                    bValueChanged = true;
                    bIsChanging = ImGui::IsItemActive();
                }
            }
            else if constexpr (std::is_same_v<VectorType, Vector3>)
            {
                if (ImGui::SliderFloat3("##Slider", &InVector.x,
                                        DragParams.MinValue, DragParams.MaxValue, DragParams.Format))
                {
                    bValueChanged = true;
                    bIsChanging = ImGui::IsItemActive();
                }
            }
            else if constexpr (std::is_same_v<VectorType, Vector4>)
            {
                if (ImGui::SliderFloat4("##Slider", &InVector.x,
                                        DragParams.MinValue, DragParams.MaxValue, DragParams.Format))
                {
                    bValueChanged = true;
                    bIsChanging = ImGui::IsItemActive();
                }
            }

            ImGui::PopID();
        }

        // 값 변경 처리
        ProcessValueChange(InVector, previousValue, bValueChanged, bIsChanging,
                           OnValueChanged, OnValueChanging);

        // 벡터 정보 표시
        ShowVectorInfo(InVector);
    }

    // C 스타일 최적화된 컬러 드래그 내용 렌더링
    static void DrawColorDragContent(
        Vector4& InColor,
        const FDragParameters& DragParams,
        const char* Label,
        std::function<void(const Vector4&)> OnValueChanged,
        std::function<bool(const Vector4&)> OnValueChanging)
    {
        Vector4 previousValue = InColor;
        bool bValueChanged = false;
        bool bIsChanging = false;

        // C 스타일 문자열 검증
        if (!Label)
        {
            Label = "Color";
        }

        // 컬러 드래그 (C 스타일 문자열 직접 사용)
        if (ImGui::DragFloat4(Label, &InColor.x,
                              DragParams.Speed,
                              DragParams.MinValue,
                              DragParams.MaxValue,
                              DragParams.Format))
        {
            bValueChanged = true;
            bIsChanging = ImGui::IsItemActive();
        }

        // 슬라이더 옵션 (메모리 주소 기반 ID)
        if (DragParams.bShowSlider)
        {
            ImGui::PushID(reinterpret_cast<const void*>(&InColor));
            if (ImGui::SliderFloat4("##ColorSlider", &InColor.x,
                                    DragParams.MinValue, DragParams.MaxValue, DragParams.Format))
            {
                bValueChanged = true;
                bIsChanging = ImGui::IsItemActive();
            }
            ImGui::PopID();
        }

        // 값 변경 처리
        ProcessValueChange(InColor, previousValue, bValueChanged, bIsChanging,
                           OnValueChanged, OnValueChanging);

        // 컬러 정보 표시
        ShowColorInfo(InColor);
    }

    // 값 변경 처리 (공통 로직)
    template<typename VectorType>
    static void ProcessValueChange(
        const VectorType& CurrentValue,
        const VectorType& PreviousValue,
        bool bValueChanged,
        bool bIsChanging,
        std::function<void(const VectorType&)> OnValueChanged,
        std::function<bool(const VectorType&)> OnValueChanging)
    {
        if (bValueChanged && !Math::IsEqual(PreviousValue, CurrentValue))
        {
            // 실시간 콜백 (드래그 중)
            if (bIsChanging && OnValueChanging)
            {
                OnValueChanging(CurrentValue);
            }

            // 값 변경 완료 콜백
            if (OnValueChanged)
            {
                OnValueChanged(CurrentValue);
            }
        }
    }

    // 벡터 정보 표시 (C printf 스타일 최적화)
    template<typename VectorType>
    static void ShowVectorInfo(const VectorType& Vector)
    {
        if constexpr (std::is_same_v<VectorType, Vector2> ||
                      std::is_same_v<VectorType, Vector3>)
        {
            float length = Vector.Length();
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

    // 컬러 정보 표시 (C printf 스타일)
    static void ShowColorInfo(const Vector4& Color)
    {
        ImGui::Text("HSV: ");
        ImGui::SameLine();

        float h, s, v;
        ImGui::ColorConvertRGBtoHSV(Color.x, Color.y, Color.z, h, s, v);
        ImGui::Text("H:%.0f° S:%.0f%% V:%.0f%%", h * 360.0f, s * 100.0f, v * 100.0f);

        ImGui::Text("Hex: #%02X%02X%02X%02X",
                    static_cast<int>(Color.x * 255), static_cast<int>(Color.y * 255),
                    static_cast<int>(Color.z * 255), static_cast<int>(Color.w * 255));
    }
};