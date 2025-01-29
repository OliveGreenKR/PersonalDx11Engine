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

        bool operator==(const FBoundFunction& Other) const
        {
            // weak_ptr의 소유자가 같은지 확인
            bool bSameObject = !BoundObject.owner_before(Other.BoundObject) &&
                !Other.BoundObject.owner_before(BoundObject);

            return bSameObject && (FunctionName == Other.FunctionName);
        }
    };

    std::vector<FBoundFunction> BoundFunctions;

public:
    // 멤버 함수를 델리게이트에 바인딩
    template<typename T>
    void Bind(const std::shared_ptr<T>& InObject,
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

    // 특정 객체의 특정 함수를 언바인딩
    template<typename T>
    void Unbind(const std::shared_ptr<T>& InObject, const std::string& InFunctionName)
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
    void UnbindAll(const std::shared_ptr<T>& InObject)
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
    void Broadcast(Args... InArgs) const
    {
        for (const auto& Binding : BoundFunctions)
        {
            if (auto BoundObject = Binding.BoundObject.lock())
            {
                // 객체가 아직 살아있는 경우에만 함수 호출
                if (Binding.Function)
                {
                    Binding.Function(InArgs...);
                }
            }
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
                    return Binding.BoundObject.expired();
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
