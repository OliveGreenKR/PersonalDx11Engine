#pragma once

#include <functional>
#include <vector>
#include <string>
#include <algorithm>
#include <cassert>
#include <cstring>
#include "BindableInterface.h"
#include "StringHash.h"

// 객체가 IBindable을 구현했는지 확인하는 헬퍼 함수
template<typename T>
constexpr bool IsBindable = std::is_base_of_v<IBindable, T>;

template<typename... Args>
class TDelegate
{
private:
    static constexpr size_t MAX_FUNCTION_NAME_LENGTH = 127;  // null terminator 고려

public:
    using FunctionType = std::function<void(Args...)>;

private:
    // 델리게이트에 바인딩된 함수의 정보를 저장하는 구조체
    struct FBoundFunction
    {
        FunctionType Function;
        IBindable* BoundObject;
        FStringHash FunctionNameHash; 
        bool bSystem;

        FBoundFunction(const FunctionType& InFunction, IBindable* InBoundObject,
                       const FStringHash& InNameHash, bool InIsSystem)
            : Function(InFunction), BoundObject(InBoundObject),
            FunctionNameHash(InNameHash), bSystem(InIsSystem) {
        }

        bool operator==(const FBoundFunction& Other) const
        {
            // 둘 다 시스템 함수인 경우 
            if (bSystem && Other.bSystem)
            {
                return FunctionNameHash == Other.FunctionNameHash;
            }

            // 하나만 시스템 함수인 경우
            if (bSystem || Other.bSystem)
            {
                return false;
            }

            // 둘 다 객체에 바인딩된 함수인 경우
            return BoundObject == Other.BoundObject && FunctionNameHash == Other.FunctionNameHash;
        }
    };

    std::vector<FBoundFunction> BoundFunctions;

    // 자신을 키로 사용하는 고유 포인터
    void* GetKeyForThis() const { return const_cast<TDelegate*>(this); }

    // 안전성 검사: 유효한 함수 이름인지 확인
    bool IsValidFunctionName(const char* InFunctionName) const
    {
        if (!InFunctionName || *InFunctionName == '\0')
        {
            return false;
        }

        //길이 초과 검사
        size_t length = strnlen(InFunctionName, MAX_FUNCTION_NAME_LENGTH+1);
        return length > 0 && length <= MAX_FUNCTION_NAME_LENGTH;
    }

public:
    ~TDelegate()
    {
        UnbindAll();
    }

    // 시스템 함수 바인딩 (객체 없이 함수만 바인딩)
    void BindSystem(const FunctionType& InFunction, const char* InFunctionName)
    {
        if (!IsValidFunctionName(InFunctionName))
        {
            assert(false && "Invalid function name provided to BindSystem");
            return;
        }

        if (!InFunction)
        {
            assert(false && "Invalid function provided to BindSystem");
            return;
        }

        FStringHash nameHash(InFunctionName);
        FBoundFunction NewBinding(InFunction, nullptr, nameHash, true);

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

    // std::string 편의성 오버로드
    void BindSystem(const FunctionType& InFunction, const std::string& InFunctionName)
    {
        BindSystem(InFunction, InFunctionName.c_str());
    }

    // 시스템 함수 언바인딩
    void UnbindSystem(const char* InFunctionName)
    {
        if (!IsValidFunctionName(InFunctionName))
        {
            return;
        }

        FStringHash nameHash(InFunctionName);

        BoundFunctions.erase(
            std::remove_if(
                BoundFunctions.begin(),
                BoundFunctions.end(),
                [&](const FBoundFunction& Binding)
                {
                    return Binding.bSystem && Binding.FunctionNameHash == nameHash;
                }
            ),
            BoundFunctions.end()
        );
    }

    // std::string 편의성 오버로드
    void UnbindSystem(const std::string& InFunctionName)
    {
        UnbindSystem(InFunctionName.c_str());
    }

    // 함수를 델리게이트에 바인딩 - IBindable 구현 객체용
    template<typename T, typename = std::enable_if_t<IsBindable<T>>>
    void Bind(T* InObject, const FunctionType& InFunction, const char* InFunctionName)
    {
        if (!InObject)
        {
            assert(false && "Cannot bind to null object");
            return;
        }

        if (!IsValidFunctionName(InFunctionName))
        {
            assert(false && "Invalid function name provided to Bind");
            return;
        }

        if (!InFunction)
        {
            assert(false && "Invalid function provided to Bind");
            return;
        }

        FStringHash nameHash(InFunctionName);
        FBoundFunction NewBinding(InFunction, InObject, nameHash, false);

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
            InObject->RegisterDeletionCallback(GetKeyForThis(), [this, InObject, nameHash]() {
                this->UnbindByObjectAndHash(InObject, nameHash);
                                               });

            BoundFunctions.push_back(std::move(NewBinding));
        }
    }

    // std::string 편의성 오버로드
    template<typename T, typename = std::enable_if_t<IsBindable<T>>>
    void Bind(T* InObject, const FunctionType& InFunction, const std::string& InFunctionName)
    {
        Bind(InObject, InFunction, InFunctionName.c_str());
    }

    // 멤버 함수 포인터를 위한 오버로드 - IBindable 구현 객체용
    template<typename U, typename T, typename = std::enable_if_t<IsBindable<U>>>
    void Bind(U* InObject, void (T::* MemberFunction)(Args...), const char* InFunctionName)
    {
        static_assert(std::is_base_of_v<T, U> || std::is_same_v<T, U>,
                      "U must be base of T or same type");

        if (!InObject || !MemberFunction)
        {
            assert(false && "Invalid object or member function provided to Bind");
            return;
        }

        FunctionType BoundFunction = [InObject, MemberFunction](Args... args) {
            (static_cast<T*>(InObject)->*MemberFunction)(std::forward<Args>(args)...);
            };

        Bind(InObject, BoundFunction, InFunctionName);
    }

    // std::string 편의성 오버로드
    template<typename U, typename T, typename = std::enable_if_t<IsBindable<U>>>
    void Bind(U* InObject, void (T::* MemberFunction)(Args...), const std::string& InFunctionName)
    {
        Bind(InObject, MemberFunction, InFunctionName.c_str());
    }

    // 특정 객체의 특정 함수를 언바인딩
    template<typename T>
    void Unbind(T* InObject, const char* InFunctionName)
    {
        if (!InObject || !IsValidFunctionName(InFunctionName))
        {
            return;
        }

        FStringHash nameHash(InFunctionName);
        UnbindByObjectAndHash(InObject, nameHash);
    }

    // std::string 편의성 오버로드
    template<typename T>
    void Unbind(T* InObject, const std::string& InFunctionName)
    {
        Unbind(InObject, InFunctionName.c_str());
    }

    // 특정 객체의 모든 바인딩 제거
    template<typename T>
    void UnbindAll(T* InObject)
    {
        if (!InObject)
        {
            return;
        }

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
        // 복사본 생성으로 실행 중 수정에 대한 안전성 보장
        auto FunctionsCopy = BoundFunctions;

        for (const auto& Binding : FunctionsCopy)
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

    // 특정 시스템 함수가 바인딩되어 있는지 확인
    bool IsSystemFunctionBound(const char* InFunctionName) const
    {
        if (!IsValidFunctionName(InFunctionName))
        {
            return false;
        }

        FStringHash nameHash(InFunctionName);

        return std::find_if(
            BoundFunctions.begin(),
            BoundFunctions.end(),
            [&](const FBoundFunction& Binding)
            {
                return Binding.bSystem && Binding.FunctionNameHash == nameHash;
            }
        ) != BoundFunctions.end();
    }

    // std::string 편의성 오버로드
    bool IsSystemFunctionBound(const std::string& InFunctionName) const
    {
        return IsSystemFunctionBound(InFunctionName.c_str());
    }

private:
    // 내부 헬퍼: 객체와 해시로 언바인딩
    void UnbindByObjectAndHash(IBindable* InObject, const FStringHash& InNameHash)
    {
        if constexpr (std::is_base_of_v<IBindable, std::remove_pointer_t<decltype(InObject)>>)
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
                        Binding.FunctionNameHash == InNameHash;
                }
            ),
            BoundFunctions.end()
        );
    }
};