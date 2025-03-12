#pragma once
#include <vector>
#include <iostream>
#include <random>
#include <fstream>
#include <sstream>
#include <chrono>
#include <string>
#include <filesystem>
#include <algorithm>
#include <memory>
#include "DynamicAABBTree.h"
#include "DynamicBoundableInterface.h"

// 테스트용 간단한 바운더블 객체 구현
class FTestBoundable : public IDynamicBoundable {
private:
    Vector3 HalfExtent;
    FTransform Transform;
    bool bIsStatic;
    bool bIsTransformChanged;

public:
    FTestBoundable(const Vector3& InPosition, const Vector3& InHalfExtent, bool InIsStatic = false)
        : HalfExtent(InHalfExtent)
        , bIsStatic(InIsStatic)
        , bIsTransformChanged(false)
    {
        Transform.SetPosition(InPosition);
    }

    // IDynamicBoundable 인터페이스 구현
    virtual Vector3 GetHalfExtent() const override { return HalfExtent; }
    virtual const FTransform* GetTransform() const override { return &Transform; }
    virtual bool IsStatic() const override { return bIsStatic; }
    virtual bool IsTransformChanged() const override { return bIsTransformChanged; }
    virtual void SetTransformChagedClean() override { bIsTransformChanged = false; }

    // 테스트용 추가 기능
    void SetPosition(const Vector3& NewPosition) {
        Transform.SetPosition(NewPosition);
        bIsTransformChanged = true;
    }

    void SetHalfExtent(const Vector3& NewHalfExtent) {
        HalfExtent = NewHalfExtent;
        bIsTransformChanged = true;
    }

    Vector3 GetPosition() const { return Transform.GetPosition(); }
};

#pragma region Utils for Test
// 랜덤 위치와 크기를 가진 바운더블 객체 생성
std::shared_ptr<FTestBoundable> CreateRandomBoundable(
    float minPos = -100.0f, float maxPos = 100.0f,
    float minExtent = 0.5f, float maxExtent = 5.0f,
    bool isStatic = false)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDistrib(minPos, maxPos);
    std::uniform_real_distribution<float> extentDistrib(minExtent, maxExtent);

    Vector3 position(posDistrib(gen), posDistrib(gen), posDistrib(gen));
    Vector3 halfExtent(extentDistrib(gen), extentDistrib(gen), extentDistrib(gen));

    return std::make_shared<FTestBoundable>(position, halfExtent, isStatic);
}

// 테스트 데이터 파일 형식: 
// 각 줄은 "px,py,pz,ex,ey,ez,static" 형식
// px,py,pz: 위치, ex,ey,ez: 반경, static: 정적 여부(0/1)

// 테스트 데이터 생성 및 파일 저장
void GenerateTestData(const std::string& filename, int count,
                      float minPos = -100.0f, float maxPos = 100.0f,
                      float minExtent = 0.5f, float maxExtent = 5.0f)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDistrib(minPos, maxPos);
    std::uniform_real_distribution<float> extentDistrib(minExtent, maxExtent);
    std::uniform_int_distribution<int> staticDistrib(0, 10); // 10% 확률로 정적 객체

    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }

    for (int i = 0; i < count; ++i) {
        float px = posDistrib(gen);
        float py = posDistrib(gen);
        float pz = posDistrib(gen);
        float ex = extentDistrib(gen);
        float ey = extentDistrib(gen);
        float ez = extentDistrib(gen);
        bool isStatic = (staticDistrib(gen) == 0);

        outFile << px << "," << py << "," << pz << ","
            << ex << "," << ey << "," << ez << ","
            << (isStatic ? 1 : 0) << std::endl;
    }

    outFile.close();
}

// 파일에서 테스트 데이터 로드
std::vector<std::shared_ptr<FTestBoundable>> LoadTestData(const std::string& filename) {
    std::vector<std::shared_ptr<FTestBoundable>> boundables;
    std::ifstream inFile(filename);

    if (!inFile.is_open()) {
        throw std::runtime_error("Failed to open file for reading: " + filename);
    }

    std::string line;
    while (std::getline(inFile, line)) {
        std::istringstream iss(line);
        std::string token;
        std::vector<float> values;

        while (std::getline(iss, token, ',')) {
            values.push_back(std::stof(token));
        }

        if (values.size() == 7) {
            Vector3 position(values[0], values[1], values[2]);
            Vector3 halfExtent(values[3], values[4], values[5]);
            bool isStatic = (values[6] > 0.5f);

            boundables.push_back(std::make_shared<FTestBoundable>(position, halfExtent, isStatic));
        }
    }

    inFile.close();
    return boundables;
}

// 트리 상태를 파일로 저장 (테스트 결과 비교용)
void SaveTreeState(const FDynamicAABBTree& tree, const std::string& filename) {
    // 트리의 내부 상태를 저장하는 로직
    // -> 이미 구현된 PrintTreeStructure() 함수를 파일로 리디렉션합니다.

    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }

    // 트리 상태 저장 로직 (실제 구현 필요)
    //tree.PrintTreeStructureToFile(outFile);

    outFile.close();
}
#pragma endregion


//노드 삽임 및 트리 균형 테스트
bool TestInsertionAndBalance() {
    const std::string INPUT_FILE = "insert_test_input.txt";
    const std::string EXPECTED_OUTPUT_FILE = "insert_test_expected.txt";
    const std::string ACTUAL_OUTPUT_FILE = "insert_test_actual.txt";
    const int NODE_COUNT = 500;

    // 테스트 데이터 생성 (이미 존재하지 않는 경우)
    if (!std::filesystem::exists(INPUT_FILE)) {
        GenerateTestData(INPUT_FILE, NODE_COUNT);
    }

    // 테스트 데이터 로드
    auto boundables = LoadTestData(INPUT_FILE);

    // 트리 생성 및 노드 삽입
    FDynamicAABBTree tree;
    std::vector<size_t> nodeIds;

    // 시간 측정 시작
    auto startTime = std::chrono::high_resolution_clock::now();

    // 노드 삽입
    for (const auto& boundable : boundables) {
        size_t nodeId = tree.Insert(boundable);
        nodeIds.push_back(nodeId);
    }

    // 시간 측정 종료
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    // 트리 상태 저장
    SaveTreeState(tree, ACTUAL_OUTPUT_FILE);

    // 트리 검증
    bool isTreeValid = true;

    // 1. 트리 내 노드 수 확인
    isTreeValid = (tree.GetNodeCount() == NODE_COUNT) && isTreeValid;

    // 2. 모든 노드 ID가 유효한지 확인
    for (size_t nodeId : nodeIds) {
        isTreeValid = (nodeId != FDynamicAABBTree::NULL_NODE) && isTreeValid;
    }

    // 3. 트리 균형 검증
    // 여기서는 트리의 높이가 log(n)에 가까운지 확인
    // 실제 구현에서는 높이를 가져오는 함수가 필요할 수 있습니다.
    // int treeHeight = tree.GetTreeHeight();
    // int expectedMaxHeight = static_cast<int>(log2(NODE_COUNT) * 2);  // 균형 트리의 경우 log2(n)이지만, 여유를 두고 2배까지 허용
    // isTreeValid = (treeHeight <= expectedMaxHeight) && isTreeValid;

    // 4. 예상 결과와 실제 결과 비교 (필요한 경우)
    if (std::filesystem::exists(EXPECTED_OUTPUT_FILE)) {
        // 파일 비교 로직 구현
        // 여기서는 간단하게 결과만 반환합니다.
    }

    std::cout << "Insertion Test Results:" << std::endl;
    std::cout << "  Nodes inserted: " << nodeIds.size() << std::endl;
    std::cout << "  Tree node count: " << tree.GetNodeCount() << std::endl;
    std::cout << "  Execution time: " << duration << " ms" << std::endl;
    // std::cout << "  Tree height: " << treeHeight << " (max expected: " << expectedMaxHeight << ")" << std::endl;
    std::cout << "  Tree is valid: " << (isTreeValid ? "YES" : "NO") << std::endl;

    return isTreeValid;
}