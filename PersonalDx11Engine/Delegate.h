#pragma once

#include <functional>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include "BindableInterface.h"

// 객체가 IBindable을 구현했는지 확인하는 헬퍼 함수
template<typename T>
constexpr bool IsBindable = std::is_base_of_v<IBindable, T>;

template<typename... Args>
class FDelegate
{
public:
    using FunctionType = std::function<void(Args...)>;

private:
    // 델리게이트에 바인딩된 함수의 정보를 저장하는 구조체
    struct FBoundFunction
    {
        FunctionType Function;
        IBindable* BoundObject;
        std::string FunctionName;
        bool bSystem;

        bool operator==(const FBoundFunction& Other) const
        {
            // 둘 다 시스템 함수인 경우 
            if (bSystem && Other.bSystem)
            {
                return FunctionName == Other.FunctionName;
            }

            // 하나만 시스템 함수인 경우
            if (bSystem || Other.bSystem)
            {
                return false;
            }

            // 둘 다 객체에 바인딩된 함수인 경우
            return BoundObject == Other.BoundObject && FunctionName == Other.FunctionName;
        }
    };

    std::vector<FBoundFunction> BoundFunctions;

    // 자신을 키로 사용하는 고유 포인터
    void* GetKeyForThis() const { return const_cast<FDelegate*>(this); }

public:
    ~FDelegate()
    {
        UnbindAll();
    }

    // 시스템 함수 바인딩 (객체 없이 함수만 바인딩)
    void BindSystem(const FunctionType& InFunction, const std::string& InFunctionName)
    {
        // 이미 동일한 함수가 바인딩되어 있는지 확인
        FBoundFunction NewBinding{ InFunction, nullptr, InFunctionName, true };

        // 중복 바인딩 방지
        auto ExistingBinding = std::find_if(
            BoundFunctions.begin(),
            BoundFunctions.end(),
            [&](const FBoundFunction& Existing)
            {
                return Existing == NewBinding;
            }
        );

        if (ExistingBinding == BoundFunctions.end())
        {
            BoundFunctions.push_back(std::move(NewBinding));
        }
    }

    // 시스템 함수 언바인딩
    void UnbindSystem(const std::string& InFunctionName)
    {
        BoundFunctions.erase(
            std::remove_if(
                BoundFunctions.begin(),
                BoundFunctions.end(),
                [&](const FBoundFunction& Binding)
                {
                    return Binding.bSystem && Binding.FunctionName == InFunctionName;
                }
            ),
            BoundFunctions.end()
        );
    }

    // 함수를 델리게이트에 바인딩 - IBindable 구현 객체용
    template<typename T, typename = std::enable_if_t<IsBindable<T>>>
    void Bind(T* InObject, const FunctionType& InFunction, const std::string& InFunctionName)
    {
        if (!InObject)
            return;

        // 이미 존재하는지 확인
        FBoundFunction NewBinding{ InFunction, InObject, InFunctionName, false };

        // 중복 바인딩 방지
        auto ExistingBinding = std::find_if(
            BoundFunctions.begin(),
            BoundFunctions.end(),
            [&](const FBoundFunction& Existing)
            {
                return Existing == NewBinding;
            }
        );

        if (ExistingBinding == BoundFunctions.end())
        {
            // 객체에 삭제 콜백 등록
            InObject->RegisterDeletionCallback(GetKeyForThis(), [this, InObject, InFunctionName]() {
                this->Unbind(InObject, InFunctionName);
                                               });

            BoundFunctions.push_back(std::move(NewBinding));
        }
    }

    // 멤버 함수 포인터를 위한 오버로드 - IBindable 구현 객체용
    template<typename U, typename T, typename = std::enable_if_t<IsBindable<U>>>
    void Bind(U* InObject, void (T::* MemberFunction)(Args...), const std::string& InFunctionName)
    {
        static_assert(std::is_base_of_v<T, U> || std::is_same_v<T, U>,
                      "U must be base of T or same type");

        if (!InObject)
            return;

        FunctionType BoundFunction = [InObject, MemberFunction](Args... args) {
            (static_cast<T*>(InObject)->*MemberFunction)(std::forward<Args>(args)...);
            };

        Bind(InObject, BoundFunction, InFunctionName);
    }

    // 특정 객체의 특정 함수를 언바인딩
    template<typename T>
    void Unbind(T* InObject, const std::string& InFunctionName)
    {
        if (!InObject)
            return;

        // IBindable 인터페이스 구현 확인
        if constexpr (IsBindable<T>)
        {
            InObject->UnregisterDeletionCallback(GetKeyForThis());
        }

        BoundFunctions.erase(
            std::remove_if(
                BoundFunctions.begin(),
                BoundFunctions.end(),
                [&](const FBoundFunction& Binding)
                {
                    return !Binding.bSystem &&
                        Binding.BoundObject == InObject &&
                        Binding.FunctionName == InFunctionName;
                }
            ),
            BoundFunctions.end()
        );
    }

    // 특정 객체의 모든 바인딩 제거
    template<typename T>
    void UnbindAll(T* InObject)
    {
        if (!InObject)
            return;

        // IBindable 인터페이스 구현 확인
        if constexpr (IsBindable<T>)
        {
            InObject->UnregisterDeletionCallback(GetKeyForThis());
        }

        BoundFunctions.erase(
            std::remove_if(
                BoundFunctions.begin(),
                BoundFunctions.end(),
                [&](const FBoundFunction& Binding)
                {
                    return !Binding.bSystem && Binding.BoundObject == InObject;
                }
            ),
            BoundFunctions.end()
        );
    }

    // 모든 바인딩 제거
    void UnbindAll()
    {
        // 모든 객체에서 콜백 등록 해제
        for (const auto& Binding : BoundFunctions)
        {
            if (!Binding.bSystem && Binding.BoundObject)
            {
                Binding.BoundObject->UnregisterDeletionCallback(GetKeyForThis());
            }
        }

        BoundFunctions.clear();
    }

    // 모든 시스템 바인딩 제거
    void UnbindAllSystem()
    {
        BoundFunctions.erase(
            std::remove_if(
                BoundFunctions.begin(),
                BoundFunctions.end(),
                [](const FBoundFunction& Binding)
                {
                    return Binding.bSystem;
                }
            ),
            BoundFunctions.end()
        );
    }

    // 델리게이트 실행
    void Broadcast(Args... InArgs) const
    {
        for (const auto& Binding : BoundFunctions)
        {
            if (Binding.bSystem)
            {
                if (Binding.Function)
                {
                    Binding.Function(std::forward<Args>(InArgs)...);
                }
            }
            else if (Binding.BoundObject)
            {
                if (Binding.Function)
                {
                    Binding.Function(std::forward<Args>(InArgs)...);
                }
            }
        }
    }

    // 바인딩된 함수의 수 반환
    size_t GetNumBound() const
    {
        return BoundFunctions.size();
    }
};