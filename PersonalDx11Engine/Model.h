#pragma once
#include "ModelBufferManager.h"

class UModel
{
public:
    UModel() = default;
    ~UModel() = default;

    // �⺻ ������Ƽ�� �� �ʱ�ȭ �޼���
    bool InitializeAsCube();
    bool InitializeAsSphere();
    bool InitializeAsPlane();

    // Ŀ���� ���� �����ͷ� �ʱ�ȭ�ϴ� �޼���
    bool InitializeFromVertexData(const FVertexDataContainer& InVertexData)
    {
        if (InVertexData.Vertices.empty())
            return false;

        DataHash = UModelBufferManager::Get()->RegisterVertexData(InVertexData);
        bIsInitialized = (DataHash != 0);
        return bIsInitialized;
    }

    // ���� ���ҽ� ������
    FBufferResource* GetBufferResource();

    // �ʱ�ȭ ���� Ȯ��
    bool IsInitialized() const { return bIsInitialized; }

private:
    size_t DataHash = 0;           // ���� �������� �ؽ� (�� �ĺ���)
    bool bIsInitialized = false;   // �ʱ�ȭ ���� �÷���
};