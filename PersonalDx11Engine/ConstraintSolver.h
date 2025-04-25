#pragma once
#include "ConstraintInterface.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>

//한 제약조건에 대응하는 데이터
struct FConstraintData
{
    EConstraintType Type = EConstraintType::None;
    XMVECTOR Direction = XMVectorZero();
};

namespace
{
    struct FPhysicsPair
    {
        FPhysicsPair(std::weak_ptr<IPhysicsState>& A, std::weak_ptr<IPhysicsState>& B)
            : BodyA(A.lock() && B.lock() ? (A.lock().get() < B.lock().get() ? A : B) : (std::weak_ptr<IPhysicsState>())),
              BodyB(A.lock() && B.lock() ? (A.lock().get() < B.lock().get() ? B : A) : (std::weak_ptr<IPhysicsState>()))
        {
        }

        std::weak_ptr<IPhysicsState> BodyA;
        std::weak_ptr<IPhysicsState> BodyB;
    };

    //한 물리현상에 대응하는 쌍과 여러 제약조건을 묶은 구조체
    struct FConstraintGroup
    {
        FPhysicsPair Pair;
        std::vector<FConstraintData> ConstraintData;
        std::vector<float> InitialLamda; //warStarting을 위한 이전 정보 저장

    };
}

//Physics Pairs Hash
namespace std
{
    template<>
    struct hash<FPhysicsPair>
    {
        size_t operator()(const FPhysicsPair& Pair) const {
                if (Pair.BodyA.lock() && Pair.BodyB.lock())
                {
                    return std::hash<void*>()(Pair.BodyA.lock().get()) ^ std::hash<void*>()(Pair.BodyB.lock().get()) << 1;
                }
        }
    };
}

/**
 * 제약조건 솔버
 * 여러 제약조건을 순차적 임펄스 방식으로 해결.
 */
class FConstraintSolver
{
    using ConstraintGroupKey = size_t;
private:
    std::unordered_map<ConstraintGroupKey, FConstraintGroup> GroupMap;

public:

    FConstraintSolver() = default;
    ~FConstraintSolver() = default;

    void SubmitProblem(std::weak_ptr<IPhysicsState>& InBodyA, std::weak_ptr<IPhysicsState>& InBodyB, 
                                 const char* CategoryName, const FConstraintData& InConstraintData);

    void UnSubmitProblem(std::weak_ptr<IPhysicsState>& InBodyA, std::weak_ptr<IPhysicsState>& InBodyB,
                         const char* CategoryName);

    void SolveAll();
    void SolveCategory(const char* CategoryName);

    

private:
    ConstraintGroupKey GetGroupKey(IPhysicsState* InBodyA, IPhysicsState* InBodyB, const char* CategoryName) const;


};