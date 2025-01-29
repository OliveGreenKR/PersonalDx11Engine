#pragma once
#pragma once
#include "Delegate.h"
#include <windows.h>
#include <unordered_map>
#include <memory>
#include <string>


// Ű �̺�Ʈ Ÿ�� ����
enum class EKeyEvent
{
    Pressed,
    Released,
    Repeat,
};

// �Է� �̺�Ʈ�� ���޵� ������ ����ü
struct FKeyEventData
{
    WPARAM KeyCode;
    EKeyEvent EventType;
    bool bAlt;
    bool bControl;
    bool bShift;
};

// Ű �̺�Ʈ ��������Ʈ Ÿ�� ����
using FOnKeyEvent = FDelegate<const FKeyEventData&>;

class UInputManager
{
private:
    // �� �̺�Ʈ Ÿ�Ժ� ��������Ʈ
    std::unordered_map<EKeyEvent, FOnKeyEvent> KeyEventDelegates;

    // ���� Ű ���� ����
    std::unordered_map<WPARAM, bool> KeyStates;

    UInputManager() = default;

public:
    static UInputManager* Get()
    {
        static std::unique_ptr<UInputManager> Instance = std::unique_ptr<UInputManager>(new UInputManager());
        return Instance.get();
    }

    // ������ ���ν������� ȣ��� �޽��� ó�� �Լ�
    bool ProcessWindowsMessage(UINT Message, WPARAM WParam, LPARAM LParam);


    // Ư�� Ű �̺�Ʈ�� ���� �Լ� ���ε�
    template<typename T>
    void BindKeyEvent(
        EKeyEvent EventType,
        const std::shared_ptr<T>& InObject,
        const std::function<void(const FKeyEventData&)>& InFunction,
        const std::string& InFunctionName)
    {
        KeyEventDelegates[EventType].Bind(InObject, InFunction, InFunctionName);
    }

    // Ư�� Ű �̺�Ʈ���� �Լ� ����ε�
    template<typename T>
    void UnbindKeyEvent(
        EKeyEvent EventType,
        const std::shared_ptr<T>& InObject,
        const std::string& InFunctionName)
    {
        KeyEventDelegates[EventType].Unbind(InObject, InFunctionName);
    }

    // ��ü�� ��� Ű �̺�Ʈ ���ε� ����
    template<typename T>
    void UnbindAllKeyEvents(const std::shared_ptr<T>& InObject)
    {
        for (auto& Delegate : KeyEventDelegates)
        {
            Delegate.second.UnbindAll(InObject);
        }
    }

    // Ư�� Ű�� ���� �����ִ��� Ȯ��
    bool IsKeyDown(WPARAM KeyCode) const
    {
        auto It = KeyStates.find(KeyCode);
        return It != KeyStates.end() && It->second;
    }

    // ���� Alt, Ctrl, Shift�� �Բ� ���ȴ��� Ȯ���� �ʿ��� ���
    bool IsKeyDownWithModifiers(WPARAM KeyCode, bool bCheckAlt, bool bCheckCtrl, bool bCheckShift) const
    {
        if (!IsKeyDown(KeyCode)) return false;

        if (bCheckAlt && !(GetAsyncKeyState(VK_MENU) & 0x8000)) return false;
        if (bCheckCtrl && !(GetAsyncKeyState(VK_CONTROL) & 0x8000)) return false;
        if (bCheckShift && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) return false;

        return true;
    }
};
