#pragma once
#include "Math.h"
#include "UIManager.h"
#include <string>
#include <functional>
#include <memory>
#include <atomic>
#include <type_traits>
#include <limits>

// 드래그 파라미터 설정 구조체
struct FDragParameters
{
    float Speed = 1.0f;                                     // 드래그 속도
    float MinValue = -std::numeric_limits<float>::max();    // 최소값
    float MaxValue = std::numeric_limits<float>::max();     // 최대값
    const char* Format = "%.3f";                            // 표시 형식
    bool bShowSlider = false;                               // 슬라이더 표시 여부

    FDragParameters() = default;

    FDragParameters(float InSpeed, float InMin = -std::numeric_limits<float>::max(),
                    float InMax = std::numeric_limits<float>::max(),
                    const char* InFormat = "%.3f", bool InShowSlider = false)
        : Speed(InSpeed), MinValue(InMin), MaxValue(InMax), Format(InFormat), bShowSlider(InShowSlider)
    {
    }

    // 일반적인 사용 케이스별 프리셋
    static FDragParameters Position(float Speed = 0.1f, const char* Format = "%.3f")
    {
        return FDragParameters(Speed, -10000.0f, 10000.0f, Format);
    }

    static FDragParameters Rotation(float Speed = 1.0f, const char* Format = "%.1f")
    {
        return FDragParameters(Speed, -180.0f, 180.0f, Format);
    }

    static FDragParameters Scale(float Speed = 0.01f, const char* Format = "%.3f")
    {
        return FDragParameters(Speed, 0.001f, 100.0f, Format);
    }

    static FDragParameters Color(float Speed = 0.01f, const char* Format = "%.3f")
    {
        return FDragParameters(Speed, 0.0f, 1.0f, Format, true);
    }

    static FDragParameters Normalized(float Speed = 0.01f, const char* Format = "%.3f")
    {
        return FDragParameters(Speed, -1.0f, 1.0f, Format);
    }
};

// UI 벡터 드래그 생성 클래스
class FUIVectorDrag
{
private:
    // 스레드 안전한 고유 ID 생성
    static std::atomic<uint64_t> NextUIId;

    // 벡터 참조 래퍼 - 생명주기 관리
    template<typename VectorType>
    struct FVectorDragReference
    {
        VectorType* VectorPtr;
        std::string WindowName;
        FDragParameters DragParams;
        std::function<void(const VectorType&)> OnValueChanged;
        std::function<bool(const VectorType&)> OnValueChanging;  // 드래그 중 실시간 콜백

        FVectorDragReference(VectorType* InPtr, const std::string& InName,
                             const FDragParameters& InParams,
                             std::function<void(const VectorType&)> InCallback,
                             std::function<bool(const VectorType&)> InChangingCallback)
            : VectorPtr(InPtr), WindowName(InName), DragParams(InParams),
            OnValueChanged(InCallback), OnValueChanging(InChangingCallback)
        {
        }

        bool IsValid() const { return VectorPtr != nullptr; }
    };

public:
    // Vector2 드래그 UI 생성
    static FUIElement CreateVector2Drag(
        Vector2& InVector,
        const FDragParameters& DragParams = FDragParameters(),
        const std::string& WindowName = "",
        std::function<void(const Vector2&)> OnValueChanged = nullptr,
        std::function<bool(const Vector2&)> OnValueChanging = nullptr)
    {
        return CreateVectorDragInternal(InVector, DragParams, WindowName,
                                        OnValueChanged, OnValueChanging, "Vector2Drag");
    }

    // Vector2 드래그 내용만 렌더링 (윈도우 없이)
    static void DrawVector2Drag(
        Vector2& InVector,
        const FDragParameters& DragParams = FDragParameters(),
        const std::string& Label = "Vector2",
        std::function<void(const Vector2&)> OnValueChanged = nullptr,
        std::function<bool(const Vector2&)> OnValueChanging = nullptr)
    {
        DrawVectorDragContent(InVector, DragParams, Label, OnValueChanged, OnValueChanging);
    }

    // Vector3 드래그 UI 생성
    static FUIElement CreateVector3Drag(
        Vector3& InVector,
        const FDragParameters& DragParams = FDragParameters(),
        const std::string& WindowName = "",
        std::function<void(const Vector3&)> OnValueChanged = nullptr,
        std::function<bool(const Vector3&)> OnValueChanging = nullptr)
    {
        return CreateVectorDragInternal(InVector, DragParams, WindowName,
                                        OnValueChanged, OnValueChanging, "Vector3Drag");
    }

    // Vector3 드래그 내용만 렌더링 (윈도우 없이)
    static void DrawVector3Drag(
        Vector3& InVector,
        const FDragParameters& DragParams = FDragParameters(),
        const std::string& Label = "Vector3",
        std::function<void(const Vector3&)> OnValueChanged = nullptr,
        std::function<bool(const Vector3&)> OnValueChanging = nullptr)
    {
        DrawVectorDragContent(InVector, DragParams, Label, OnValueChanged, OnValueChanging);
    }

    // Vector4 드래그 UI 생성
    static FUIElement CreateVector4Drag(
        Vector4& InVector,
        const FDragParameters& DragParams = FDragParameters(),
        const std::string& WindowName = "",
        std::function<void(const Vector4&)> OnValueChanged = nullptr,
        std::function<bool(const Vector4&)> OnValueChanging = nullptr)
    {
        return CreateVectorDragInternal(InVector, DragParams, WindowName,
                                        OnValueChanged, OnValueChanging, "Vector4Drag");
    }

    // Vector4 드래그 내용만 렌더링 (윈도우 없이)
    static void DrawVector4Drag(
        Vector4& InVector,
        const FDragParameters& DragParams = FDragParameters(),
        const std::string& Label = "Vector4",
        std::function<void(const Vector4&)> OnValueChanged = nullptr,
        std::function<bool(const Vector4&)> OnValueChanging = nullptr)
    {
        DrawVectorDragContent(InVector, DragParams, Label, OnValueChanged, OnValueChanging);
    }

    // 컬러 드래그 (Vector4, 0-1 범위 제한)
    static FUIElement CreateColorDrag(
        Vector4& InColor,
        const std::string& WindowName = "",
        std::function<void(const Vector4&)> OnValueChanged = nullptr,
        std::function<bool(const Vector4&)> OnValueChanging = nullptr)
    {
        std::string finalWindowName = WindowName.empty() ?
            GenerateWindowName("ColorDrag") : WindowName;

        auto reference = std::make_shared<FVectorDragReference<Vector4>>(
            &InColor, finalWindowName, FDragParameters::Color(),
            OnValueChanged, OnValueChanging);

        FUIElement element;
        element.WindowName = finalWindowName;
        element.DrawFunction = [reference]()
            {
                if (!reference->IsValid()) return;

                if (ImGui::Begin(reference->WindowName.c_str()))
                {
                    DrawColorDragContent(*reference->VectorPtr, reference->DragParams,
                                         "Color", reference->OnValueChanged, reference->OnValueChanging);
                }
                ImGui::End();
            };

        return element;
    }

    // 컬러 드래그 내용만 렌더링 (윈도우 없이)
    static void DrawColorDrag(
        Vector4& InColor,
        const std::string& Label = "Color",
        std::function<void(const Vector4&)> OnValueChanged = nullptr,
        std::function<bool(const Vector4&)> OnValueChanging = nullptr)
    {
        DrawColorDragContent(InColor, FDragParameters::Color(), Label, OnValueChanged, OnValueChanging);
    }

    // 오일러 각도 드래그 (Vector3, 각도 표시)
    static FUIElement CreateEulerAngleDrag(
        Vector3& InEulerAngles,
        const std::string& WindowName = "",
        std::function<void(const Vector3&)> OnValueChanged = nullptr,
        std::function<bool(const Vector3&)> OnValueChanging = nullptr)
    {
        return CreateVectorDragInternal(InEulerAngles, FDragParameters::Rotation(),
                                        WindowName.empty() ? GenerateWindowName("EulerDrag") : WindowName,
                                        OnValueChanged, OnValueChanging, "EulerDrag");
    }

    // 오일러 각도 드래그 내용만 렌더링 (윈도우 없이)
    static void DrawEulerAngleDrag(
        Vector3& InEulerAngles,
        const std::string& Label = "Euler Angles",
        std::function<void(const Vector3&)> OnValueChanged = nullptr,
        std::function<bool(const Vector3&)> OnValueChanging = nullptr)
    {
        DrawVectorDragContent(InEulerAngles, FDragParameters::Rotation(), Label, OnValueChanged, OnValueChanging);
    }

private:
    // 벡터 드래그 내용 렌더링 (템플릿)
    template<typename VectorType>
    static void DrawVectorDragContent(
        VectorType& InVector,
        const FDragParameters& DragParams,
        const std::string& Label,
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

        // 드래그 컨트롤
        if constexpr (std::is_same_v<VectorType, Vector2>)
        {
            bValueChanged = ImGui::DragFloat2(Label.c_str(), &InVector.x,
                                              DragParams.Speed,
                                              DragParams.MinValue,
                                              DragParams.MaxValue,
                                              DragParams.Format);
        }
        else if constexpr (std::is_same_v<VectorType, Vector3>)
        {
            bValueChanged = ImGui::DragFloat3(Label.c_str(), &InVector.x,
                                              DragParams.Speed,
                                              DragParams.MinValue,
                                              DragParams.MaxValue,
                                              DragParams.Format);
        }
        else if constexpr (std::is_same_v<VectorType, Vector4>)
        {
            bValueChanged = ImGui::DragFloat4(Label.c_str(), &InVector.x,
                                              DragParams.Speed,
                                              DragParams.MinValue,
                                              DragParams.MaxValue,
                                              DragParams.Format);
        }

        bIsChanging = ImGui::IsItemActive();

        // 슬라이더 옵션
        if (DragParams.bShowSlider)
        {
            bool sliderChanged = false;
            std::string sliderLabel = Label + " Slider";

            if constexpr (std::is_same_v<VectorType, Vector2>)
            {
                sliderChanged = ImGui::SliderFloat2(sliderLabel.c_str(), &InVector.x,
                                                    DragParams.MinValue,
                                                    DragParams.MaxValue,
                                                    DragParams.Format);
            }
            else if constexpr (std::is_same_v<VectorType, Vector3>)
            {
                sliderChanged = ImGui::SliderFloat3(sliderLabel.c_str(), &InVector.x,
                                                    DragParams.MinValue,
                                                    DragParams.MaxValue,
                                                    DragParams.Format);
            }
            else if constexpr (std::is_same_v<VectorType, Vector4>)
            {
                sliderChanged = ImGui::SliderFloat4(sliderLabel.c_str(), &InVector.x,
                                                    DragParams.MinValue,
                                                    DragParams.MaxValue,
                                                    DragParams.Format);
            }

            if (sliderChanged)
            {
                bValueChanged = true;
                bIsChanging = ImGui::IsItemActive();
            }
        }

        // 값 변경 처리
        ProcessValueChangeStatic(InVector, previousValue, bValueChanged, bIsChanging,
                                 OnValueChanged, OnValueChanging);

        // 벡터 정보 표시
        ShowVectorInfo(InVector);
    }

    // 컬러 드래그 내용 렌더링
    static void DrawColorDragContent(
        Vector4& InColor,
        const FDragParameters& DragParams,
        const std::string& Label,
        std::function<void(const Vector4&)> OnValueChanged,
        std::function<bool(const Vector4&)> OnValueChanging)
    {
        Vector4 previousValue = InColor;
        bool bValueChanged = false;
        bool bIsChanging = false;

        // 컬러 선택기
        if (ImGui::ColorEdit4((Label + " Picker").c_str(), &InColor.x))
        {
            bValueChanged = true;
        }

        // 드래그 컨트롤
        if (ImGui::DragFloat4((Label + " RGBA").c_str(), &InColor.x,
                              DragParams.Speed,
                              DragParams.MinValue,
                              DragParams.MaxValue,
                              DragParams.Format))
        {
            bValueChanged = true;
            bIsChanging = ImGui::IsItemActive();
        }

        // 슬라이더 옵션
        if (DragParams.bShowSlider)
        {
            if (ImGui::SliderFloat4((Label + " Slider").c_str(), &InColor.x,
                                    DragParams.MinValue,
                                    DragParams.MaxValue,
                                    DragParams.Format))
            {
                bValueChanged = true;
                bIsChanging = ImGui::IsItemActive();
            }
        }

        // 값 변경 처리
        ProcessValueChangeStatic(InColor, previousValue, bValueChanged, bIsChanging,
                                 OnValueChanged, OnValueChanging);

        // 컬러 정보 표시
        ShowColorInfo(InColor);
    }

    // 내부 벡터 드래그 구현
    template<typename VectorType>
    static FUIElement CreateVectorDragInternal(
        VectorType& InVector,
        const FDragParameters& DragParams,
        const std::string& WindowName,
        std::function<void(const VectorType&)> OnValueChanged,
        std::function<bool(const VectorType&)> OnValueChanging,
        const std::string& DefaultPrefix)
    {
        static_assert(std::is_same_v<VectorType, Vector2> ||
                      std::is_same_v<VectorType, Vector3> ||
                      std::is_same_v<VectorType, Vector4>,
                      "Only Vector2, Vector3, Vector4 are supported");

        std::string finalWindowName = WindowName.empty() ?
            GenerateWindowName(DefaultPrefix) : WindowName;

        auto reference = std::make_shared<FVectorDragReference<VectorType>>(
            &InVector, finalWindowName, DragParams, OnValueChanged, OnValueChanging);

        FUIElement element;
        element.WindowName = finalWindowName;
        element.DrawFunction = [reference]()
            {
                if (!reference->IsValid()) return;

                if (ImGui::Begin(reference->WindowName.c_str()))
                {
                    DrawVectorDragContent(*reference->VectorPtr, reference->DragParams,
                                          "Drag", reference->OnValueChanged, reference->OnValueChanging);
                }
                ImGui::End();
            };

        return element;
    }

    // 값 변경 처리 정적 버전
    template<typename VectorType>
    static void ProcessValueChangeStatic(
        const VectorType& CurrentValue,
        const VectorType& PreviousValue,
        bool bValueChanged,
        bool bIsChanging,
        std::function<void(const VectorType&)> OnValueChanged,
        std::function<bool(const VectorType&)> OnValueChanging)
    {
        if (!bValueChanged || Math::IsEqual(PreviousValue, CurrentValue))
            return;

        // 드래그 중 실시간 콜백
        if (bIsChanging && OnValueChanging)
        {
            bool bShouldContinue = OnValueChanging(CurrentValue);
            if (!bShouldContinue)
            {
                // 변경 취소는 호출자가 처리해야 함
                return;
            }
        }

        // 드래그 완료 콜백
        if (!bIsChanging && OnValueChanged)
        {
            OnValueChanged(CurrentValue);
        }
    }

    // 값 변경 처리 참조 버전
    template<typename VectorType>
    static void ProcessValueChange(
        std::shared_ptr<FVectorDragReference<VectorType>> Reference,
        const VectorType& PreviousValue,
        bool bValueChanged,
        bool bIsChanging)
    {
        ProcessValueChangeStatic(*Reference->VectorPtr, PreviousValue, bValueChanged, bIsChanging,
                                 Reference->OnValueChanged, Reference->OnValueChanging);
    }

    // 고유 윈도우 이름 생성
    static std::string GenerateWindowName(const std::string& Prefix)
    {
        return Prefix + "_" + std::to_string(NextUIId.fetch_add(1));
    }

    // 벡터 정보 표시
    template<typename VectorType>
    static void ShowVectorInfo(const VectorType& Vector)
    {
        if constexpr (std::is_same_v<VectorType, Vector2> ||
                      std::is_same_v<VectorType, Vector3>)
        {
            float length = Vector.Length();
            ImGui::Text("Length: %.3f", length);

            if (length > KINDA_SMALL)
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

    // 컬러 정보 표시
    static void ShowColorInfo(const Vector4& Color)
    {
        ImGui::Text("HSV: ");
        ImGui::SameLine();

        float h, s, v;
        ImGui::ColorConvertRGBtoHSV(Color.x, Color.y, Color.z, h, s, v);
        ImGui::Text("H:%.0f° S:%.0f%% V:%.0f%%", h * 360.0f, s * 100.0f, v * 100.0f);

        ImGui::Text("Hex: #%02X%02X%02X%02X",
                    (int)(Color.x * 255), (int)(Color.y * 255),
                    (int)(Color.z * 255), (int)(Color.w * 255));
    }
};

// 정적 멤버 정의
std::atomic<uint64_t> FUIVectorDrag::NextUIId{ 0 };