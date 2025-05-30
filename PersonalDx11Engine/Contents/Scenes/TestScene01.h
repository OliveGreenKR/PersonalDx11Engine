#pragma once
#include <memory>
#include <vector>
#include "../SceneInterface.h"
#include "../InputContext.h"
#include "../ResourceHandle.h"
#include "../FixedObjectPool.h"
#include "../PrimitiveComponent.h"
#include "../CollisionDetector.h"


class UTestScene01 : public ISceneInterface
{
public:
	UTestScene01();
	~UTestScene01();
	// Inherited via ISceneInterface
	void Initialize() override;
	void Load() override;
	void Unload() override;
	void Tick(float DeltaTime) override;
	void SubmitRender(URenderer* Renderer) override;
	void SubmitRenderUI() override;
	
	void HandleInput(const FKeyEventData& EventData) override;
	UCamera* GetMainCamera() const override;
	std::string& GetName() override;

private:
	std::unique_ptr<TFixedObjectPool<UPrimitiveComponent, 256>> ObjectPool;
	std::shared_ptr<class UCamera> Camera;

	std::shared_ptr<class UInputContext> InputContext;
	
	std::string SceneName = "TestScene01";

	std::shared_ptr<class UBoxComponent> Box;
	std::shared_ptr<class USphereComponent> Sphere;

	bool bDebug01 = true;
	bool bDebug02 = true;
	
	FCollisionDetector::PolytopeSOA Poly;
	FCollisionDetector Detector;
	int CuurentIteration = 1;
	bool bEPAConverged = false;
private:
	void RandSpawn();
private:
	void SetupInput();

	bool CreateInitialPolytope(const FCollisionDetector::FSimplex& InSimplex, FCollisionDetector& InDetector, FCollisionDetector::PolytopeSOA& OutPoly);

	bool EPACollision(const ICollisionShape& ShapeA, const FTransform& TransformA, const ICollisionShape& ShapeB, const FTransform& TransformB, FCollisionDetector::PolytopeSOA& InOutPoly, FCollisionDetector& InDetector);

	void DrawPolytope(const FCollisionDetector::PolytopeSOA& Polytope, const Vector4& Color, float LifeTime, bool bDrawNormals);
};
