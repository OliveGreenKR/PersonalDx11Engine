#pragma once
#include <memory>
#include <vector>
#include <algorithm>
#include "CollisionDefines.h"
#include "CollisionComponent.h"

class UCollisionComponent;

//�浹 �Ŵ����� ������Ʈ�����Ҷ� ����� ����ü
struct FCollisionComponentState
{
	std::shared_ptr<UCollisionComponent> Component;
	std::vector<std::weak_ptr<UCollisionComponent>> CurrentCollisions;  // ���� �������� �浹 ����
	std::vector<std::weak_ptr<UCollisionComponent>> PreviousCollisions; // ���� �������� �浹 ����

	// CCD�� ���� �߰� ����
	FTransform PreviousTransform;
	FTransform CurrentTransform;
};

//todo ���� �ý��� ����
class FCollisionDectection;
class FCollisionResponse;
class FCollisionEvent;


class UCollisionManager
{
	//��� collision component���� ���� �ֱ⸦ �����ϰ�

	/*
	* ��� Collision Component���� ���� �� �Ҹ��� ����.
	* ��� �ݸ��� ������ �����ְ��� �浹 �˻� �� �浹 ������ �Ѱ�.
	* 
	* 3���� ���� �ý������� ����� ����
	* 
	* - Collision Detection System : �浹 Ž��
	* - Collision Response System: 
			�浹 ������ ����Ͽ� �ش� ������Ʈ�� ���� ������ �ٵ𿡰� ���� ���·� ����
	* - Collision Event System : �浹 �̺�Ʈ �߼�
	* 
	* **Broad Phase�� ���� ����.
	*/

private :
	UCollisionManager() = default;
public:
	~UCollisionManager() { Release(); }
	
	// ���� �� �̵� ����
	UCollisionManager(const UCollisionManager&) = delete;
	UCollisionManager& operator=(const UCollisionManager&) = delete;
	UCollisionManager(UCollisionManager&&) = delete;
	UCollisionManager& operator=(UCollisionManager&&) = delete;

	static UCollisionManager* Get()
	{
		static std::unique_ptr<UCollisionManager> Instance
			= std::unique_ptr<UCollisionManager>(new UCollisionManager());

		return Instance.get();

	}
	void Tick(const float DeltaTime);

	static UCollisionManager* CreateCollisionComponent(const std::shared_ptr<URigidBodyComponent>& InRigidBody);
	UCollisionManager* GetCollisionByIndex(const unsigned int InIndex) const;

	void Release();

private:
	void DeleteCollisionByIndex(const unsigned itn);


private:
	std::vector<std::shared_ptr<UCollisionManager>> Collisions;

	//std::unique_ptr<class FCollisionDetection> Detector;
	//std::unique_ptr<class FCollisionResponse> ResponseCalculator;
	//std::unique_ptr<class FCollisionEventPublisher> EventPublisher;

};

