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
    // ��������Ʈ�� ���ε��� �Լ��� ������ �����ϴ� ����ü
    struct FBoundFunction
    {
        FunctionType Function;
        std::weak_ptr<void> BoundObject;  // void Ÿ������ ��� ��ü Ÿ�� ����
        std::string FunctionName;

        bool operator==(const FBoundFunction& Other) const
        {
            // weak_ptr�� �����ڰ� ������ Ȯ��
            bool bSameObject = !BoundObject.owner_before(Other.BoundObject) &&
                !Other.BoundObject.owner_before(BoundObject);

            return bSameObject && (FunctionName == Other.FunctionName);
        }
    };

    std::vector<FBoundFunction> BoundFunctions;

public:
    // ��� �Լ��� ��������Ʈ�� ���ε�
    template<typename T>
    void Bind(const std::shared_ptr<T>& InObject,
              const FunctionType& InFunction,
              const std::string& InFunctionName)
    {
        // �̹� �����ϴ��� Ȯ��
        FBoundFunction NewBinding{ InFunction, InObject, InFunctionName };

        // �ߺ� ���ε� ����
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

    // Ư�� ��ü�� Ư�� �Լ��� ����ε�
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
                        // ��ü�� �̹� ������
                        return true;
                    }

                    return BoundObjectLocked == InObject;
                }
            ),
            BoundFunctions.end()
        );
    }

    // Ư�� ��ü�� ��� ���ε� ����
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
                        // ��ü�� �̹� ������
                        return true;
                    }

                    return BoundObjectLocked == InObject;
                }
            ),
            BoundFunctions.end()
        );
    }

    // ��� ���ε� ����
    void UnbindAll()
    {
        BoundFunctions.clear();
    }

    // ��������Ʈ ���� (����ִ� ��ü�� �Լ��� ȣ��)
    void Broadcast(Args... InArgs) const
    {
        for (const auto& Binding : BoundFunctions)
        {
            if (auto BoundObject = Binding.BoundObject.lock())
            {
                // ��ü�� ���� ����ִ� ��쿡�� �Լ� ȣ��
                if (Binding.Function)
                {
                    Binding.Function(InArgs...);
                }
            }
        }
    }

    // ���� �Լ� - ������ ��ü�� ���ε��� ����
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

    // ���ε��� �Լ��� �� ��ȯ
    size_t GetNumBound() const
    {
        return BoundFunctions.size();
    }
};
