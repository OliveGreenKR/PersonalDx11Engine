#pragma once
#pragma once

#include <functional>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <unordered_map>

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
        std::weak_ptr<void> BoundObject;  // void 타입으로 모든 객체 타입 지원
        std::string FunctionName;
        bool bSystem = false;

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
                return false;  // 시스템 함수와 객체 함수는 절대 같을 수 없음
            }

            // 둘 다 객체에 바인딩된 함수인 경우
            bool bSameObject = !BoundObject.owner_before(Other.BoundObject) &&
                !Other.BoundObject.owner_before(BoundObject);

            return bSameObject && (FunctionName == Other.FunctionName);
        }
    };

    std::vector<FBoundFunction> BoundFunctions;

public:
    // 시스템 함수 바인딩 (객체 없이 함수만 바인딩)
    void BindSystem(const std::function<void(Args...)>& InFunction,
                    const std::string& InFunctionName)
    {
        // 시스템 함수
        FBoundFunction NewBinding{ InFunction, std::weak_ptr<void>(), InFunctionName ,true };

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
                    return Binding.bSystem &&
                        Binding.FunctionName == InFunctionName;
                }
            ),
            BoundFunctions.end()
        );
    }


    // 멤버 함수를 델리게이트에 바인딩
    template<typename T>
    void Bind(const std::weak_ptr<T>& InObject,
              const FunctionType& InFunction,
              const std::string& InFunctionName)
    {
        // 이미 존재하는지 확인
        FBoundFunction NewBinding{ InFunction, InObject, InFunctionName };

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

    // 멤버 함수 포인터를 위한 새로운 오버로드 추가
    template<typename U, typename T>
    void Bind(const std::weak_ptr<U>& InObject,
              void (T::* MemberFunction)(Args...),
              const std::string& InFunctionName)
    {
        static_assert(std::is_base_of_v<U, T> || std::is_same_v<T, U>,
                      "U must be base of T or same type");

        FunctionType BoundFunction = [InObject, MemberFunction](Args... args) {
            (static_cast<T*>(InObject.lock().get())->*MemberFunction)(std::forward<Args>(args)...);
            };

        Bind(InObject, BoundFunction, InFunctionName);
    }

    // 특정 객체의 특정 함수를 언바인딩
    template<typename T>
    void Unbind(const std::weak_ptr<T>& InObject, const std::string& InFunctionName)
    {
        BoundFunctions.erase(
            std::remove_if(
                BoundFunctions.begin(),
                BoundFunctions.end(),
                [&](const FBoundFunction& Binding)
                {
                    if (Binding.FunctionName != InFunctionName)
                    {
                        return false;
                    }

                    auto BoundObjectLocked = Binding.BoundObject.lock();
                    if (!BoundObjectLocked)
                    {
                        // 객체가 이미 삭제됨
                        return true;
                    }

                    return BoundObjectLocked == InObject;
                }
            ),
            BoundFunctions.end()
        );
    }

    // 특정 객체의 모든 바인딩 제거
    template<typename T>
    void UnbindAll(const std::weak_ptr<T>& InObject)
    {
        BoundFunctions.erase(
            std::remove_if(
                BoundFunctions.begin(),
                BoundFunctions.end(),
                [&](const FBoundFunction& Binding)
                {
                    auto BoundObjectLocked = Binding.BoundObject.lock();
                    if (!BoundObjectLocked)
                    {
                        // 객체가 이미 삭제됨
                        return true;
                    }

                    return BoundObjectLocked == InObject;
                }
            ),
            BoundFunctions.end()
        );
    }

    // 모든 바인딩 제거
    void UnbindAll()
    {
        BoundFunctions.clear();
    }

    // 델리게이트 실행 (살아있는 객체의 함수만 호출)
    void Broadcast(Args... InArgs) 
    {
        bool isDirty = false;
        for (const auto& Binding : BoundFunctions)
        {
            // 객체에 바인딩된 함수의 경우 객체 생존 확인
            if (!Binding.bSystem)
            {
                if (auto BoundObject = Binding.BoundObject.lock())
                {
                    if (!BoundObject)
                    {
                        isDirty = true;
                        continue;
                    }

                    if (Binding.Function)
                    {
                        Binding.Function(InArgs...);
                    }
                }
            }
            // 시스템 함수는 객체 확인 없이 직접 호출
            else if (Binding.Function)
            {
                Binding.Function(InArgs...);
            }
        }

        if (isDirty)
        {
            RemoveExpiredBindings();
        }
    }

    // 정리 함수 - 삭제된 객체의 바인딩을 제거
    void RemoveExpiredBindings()
    {
        BoundFunctions.erase(
            std::remove_if(
                BoundFunctions.begin(),
                BoundFunctions.end(),
                [](const FBoundFunction& Binding)
                {
                    return !Binding.bSystem && Binding.BoundObject.expired();
                }
            ),
            BoundFunctions.end()
        );
    }

    // 정리 함수 - null을 참조하고 있는 모든 바인딩 제거(시스템포함)
    void RemoveSystemBindings()
    {
        BoundFunctions.erase(
            std::remove_if(
                BoundFunctions.begin(),
                BoundFunctions.end(),
                [](const FBoundFunction& Binding)
                {
                    return Binding.bSystem || Binding.BoundObject.expired();
                }
            ),
            BoundFunctions.end()
        );
    }

    // 바인딩된 함수의 수 반환
    size_t GetNumBound() const
    {
        return BoundFunctions.size();
    }
};
