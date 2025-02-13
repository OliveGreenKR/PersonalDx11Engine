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
        bool bSystem = false;

        bool operator==(const FBoundFunction& Other) const
        {
            // �� �� �ý��� �Լ��� ��� 
            if (bSystem && Other.bSystem)
            {
                return FunctionName == Other.FunctionName;
            }

            // �ϳ��� �ý��� �Լ��� ���
            if (bSystem || Other.bSystem)
            {
                return false;  // �ý��� �Լ��� ��ü �Լ��� ���� ���� �� ����
            }

            // �� �� ��ü�� ���ε��� �Լ��� ���
            bool bSameObject = !BoundObject.owner_before(Other.BoundObject) &&
                !Other.BoundObject.owner_before(BoundObject);

            return bSameObject && (FunctionName == Other.FunctionName);
        }
    };

    std::vector<FBoundFunction> BoundFunctions;

public:
    // �ý��� �Լ� ���ε� (��ü ���� �Լ��� ���ε�)
    void BindSystem(const std::function<void(Args...)>& InFunction,
                    const std::string& InFunctionName)
    {
        // �ý��� �Լ�
        FBoundFunction NewBinding{ InFunction, std::weak_ptr<void>(), InFunctionName ,true };

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

    // �ý��� �Լ� ����ε�
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

    // ��� �Լ� �����͸� ���� ���ο� �����ε� �߰�
    template<typename U, typename T>
    void Bind(const std::shared_ptr<U>& InObject,
              void (T::* MemberFunction)(Args...),
              const std::string& InFunctionName)
    {
        static_assert(std::is_base_of_v<U, T> || std::is_same_v<T, U>,
                      "U must be base of T or same type");

        FunctionType BoundFunction = [InObject, MemberFunction](Args... args) {
            (static_cast<T*>(InObject.get())->*MemberFunction)(std::forward<Args>(args)...);
            };

        Bind(InObject, BoundFunction, InFunctionName);
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
    void Broadcast(Args... InArgs) 
    {
        bool isDirty = false;
        for (const auto& Binding : BoundFunctions)
        {
            // ��ü�� ���ε��� �Լ��� ��� ��ü ���� Ȯ��
            if (!Binding.bSystem)
            {
                if (Binding.BoundObject.expired())
                {
                    isDirty = true;
                    continue;
                }
                if (auto BoundObject = Binding.BoundObject.lock())
                {
                    if (Binding.Function)
                    {
                        Binding.Function(InArgs...);
                    }
                }
            }
            // �ý��� �Լ��� ��ü Ȯ�� ���� ���� ȣ��
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

    // ���� �Լ� - ������ ��ü�� ���ε��� ����
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

    // ���� �Լ� - null�� �����ϰ� �ִ� ��� ���ε� ����(�ý�������)
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

    // ���ε��� �Լ��� �� ��ȯ
    size_t GetNumBound() const
    {
        return BoundFunctions.size();
    }
};
