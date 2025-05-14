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

namespace TestDynamicAABBTree
{

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
            Transform.Position = InPosition;
        }

        // IDynamicBoundable 인터페이스 구현
        virtual Vector3 GetHalfExtent() const override { return HalfExtent;  }
        virtual Vector3 GetScaledHalfExtent() const override { return HalfExtent; }
        virtual const FTransform& GetWorldTransform() const override { return Transform; }
        virtual bool IsStatic() const override { return bIsStatic; }

        // 테스트용 추가 기능
        void SetPosition(const Vector3& NewPosition) {
            Transform.Position = NewPosition;
            bIsTransformChanged = true;
        }

        void SetHalfExtent(const Vector3& NewHalfExtent) {
            HalfExtent = NewHalfExtent;
            bIsTransformChanged = true;
        }

        Vector3 GetPosition() const { return Transform.Position; }
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
        if (!outFile.is_open())
        {
            throw std::runtime_error("Failed to open file for writing: " + filename);
        }

        for (int i = 0; i < count; ++i)
        {
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

        if (!inFile.is_open())
        {
            throw std::runtime_error("Failed to open file for reading: " + filename);
        }

        std::string line;
        while (std::getline(inFile, line))
        {
            std::istringstream iss(line);
            std::string token;
            std::vector<float> values;

            while (std::getline(iss, token, ','))
            {
                values.push_back(std::stof(token));
            }

            if (values.size() == 7)
            {
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
        std::ofstream outFile(filename);
        if (!outFile.is_open())
        {
            throw std::runtime_error("Failed to open file for writing: " + filename);
        }
        // 트리 상태 저장
        tree.PrintTreeStructure(outFile);
        outFile.close();
    }

    // 쿼리 테스트를 위한 AABB 영역 데이터 생성
    bool GenerateQueryTestData(const std::string& filename, int count,
                               float minPos = -120.0f, float maxPos = 120.0f,
                               float minSize = 5.0f, float maxSize = 30.0f)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> posDistrib(minPos, maxPos);
        std::uniform_real_distribution<float> sizeDistrib(minSize, maxSize);

        std::ofstream outFile(filename);
        if (!outFile.is_open())
        {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return false;
        }

        // 파일 헤더 작성
        outFile << "# QueryOverlap 테스트 데이터\n";
        outFile << "# 형식: minx,miny,minz,maxx,maxy,maxz\n";
        outFile << "# 총 " << count << "개의 쿼리 영역 데이터\n\n";

        for (int i = 0; i < count; ++i)
        {
            Vector3 center(posDistrib(gen), posDistrib(gen), posDistrib(gen));
            Vector3 halfExtent(sizeDistrib(gen), sizeDistrib(gen), sizeDistrib(gen));

            Vector3 min = center - halfExtent;
            Vector3 max = center + halfExtent;

            outFile << min.x << "," << min.y << "," << min.z << ","
                << max.x << "," << max.y << "," << max.z << std::endl;
        }

        outFile.close();
        std::cout << "생성된 쿼리 테스트 데이터: " << filename << " (" << count << "개 쿼리)" << std::endl;
        return true;
    }
#pragma endregion

    bool TestInsertionAndBalance(
        int nodeCount,
        const std::string& inputFile,
        const std::string& outputFile,
        int& insertedNodes,
        int& treeNodeCount,
        float& executionTimeMs)
    {
        // 테스트 데이터 생성 (이미 존재하지 않는 경우)
        if (!std::filesystem::exists(inputFile))
        {
            GenerateTestData(inputFile, nodeCount);
        }

        // 테스트 데이터 로드
        auto boundables = LoadTestData(inputFile);

        // 트리 생성 및 노드 삽입
        FDynamicAABBTree tree;
        std::vector<size_t> insertLeafIds;

        // 시간 측정 시작
        auto startTime = std::chrono::high_resolution_clock::now();

        // 노드 삽입
        for (const auto& boundable : boundables)
        {
            size_t nodeId = tree.Insert(boundable);
            insertLeafIds.push_back(nodeId);
        }

        // 시간 측정 종료
        auto endTime = std::chrono::high_resolution_clock::now();
        executionTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        // 트리 상태 저장
        SaveTreeState(tree, outputFile);

        // 트리 검증
        bool isTreeValid = true;

        auto outLeafIds = tree.GetAllLeafNodeIds();
        std::sort(outLeafIds.begin(), outLeafIds.end());
        std::sort(insertLeafIds.begin(), insertLeafIds.end());

        // 1. 트리 내 리프 노드 수 확인
        isTreeValid = (outLeafIds.size() == nodeCount) && isTreeValid;

        // 2. 모든 노드 ID가 리프노드인지 확인
        for (int i = 0; i < nodeCount; ++i)
        {
            isTreeValid = outLeafIds[i] == insertLeafIds[i] && isTreeValid;
        }

        // 출력 매개변수 설정
        insertedNodes = insertLeafIds.size();
        treeNodeCount = outLeafIds.size();

        return isTreeValid;
    }

    bool TestDeletionAndIntegrity(
        int nodeCount,
        int deleteCount,
        const std::string& inputFile,
        const std::string& beforeOutputFile,
        const std::string& afterOutputFile,
        int& remainingNodes,
        float& executionTimeMs)
    {
        // 테스트 데이터 생성 (이미 존재하지 않는 경우)
        if (!std::filesystem::exists(inputFile))
        {
            GenerateTestData(inputFile, nodeCount);
        }

        // 테스트 데이터 로드
        auto boundables = LoadTestData(inputFile);

        // 트리 생성 및 노드 삽입
        FDynamicAABBTree tree;
        std::vector<size_t> insertLeafIds;

        // 모든 노드 삽입
        for (const auto& boundable : boundables)
        {
            size_t nodeId = tree.Insert(boundable);
            insertLeafIds.push_back(nodeId);
        }

        // 삭제할 노드 선택 (랜덤)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(insertLeafIds.begin(), insertLeafIds.end(), gen);
        std::vector<size_t> nodesToDelete(insertLeafIds.begin(), insertLeafIds.begin() + deleteCount);

        // 트리 상태 저장 (삭제 전)
        SaveTreeState(tree, beforeOutputFile);

        // 시간 측정 시작
        auto startTime = std::chrono::high_resolution_clock::now();

        // 노드 삭제
        for (size_t nodeId : nodesToDelete)
        {
            tree.Remove(nodeId);
        }

        // 시간 측정 종료
        auto endTime = std::chrono::high_resolution_clock::now();
        executionTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        // 트리 상태 저장 (삭제 후)
        SaveTreeState(tree, afterOutputFile);

        // 트리 검증
        bool isTreeValid = true;

        auto outLeafIds = tree.GetAllLeafNodeIds();
        std::sort(outLeafIds.begin(), outLeafIds.end());
        std::sort(insertLeafIds.begin(), insertLeafIds.end());

        // 1. 트리 내 리프 노드 수 확인
        isTreeValid = (outLeafIds.size() == (nodeCount - deleteCount)) && isTreeValid;

        // 출력 매개변수 설정
        remainingNodes = tree.GetNodeCount();

        return isTreeValid;
    }

    bool TestQueryOverlap(
        int nodeCount,
        int queryCount,
        float acceptableQueryTimeMs,
        const std::string& inputFile,
        const std::string& outputFile,
        float& avgQueryTimeMs,
        float& maxQueryTimeMs,
        int& incorrectQueries)
    {
        // 테스트 데이터 생성 (이미 존재하지 않는 경우)
        if (!std::filesystem::exists(inputFile))
        {
            GenerateTestData(inputFile, nodeCount);
        }

        // 테스트 데이터 로드
        auto boundables = LoadTestData(inputFile);

        // 트리 생성 및 노드 삽입
        FDynamicAABBTree tree;

        // 모든 노드 삽입
        std::vector<size_t> insertedLeafIds;
        insertedLeafIds.reserve(nodeCount);
        for (const auto& boundable : boundables)
        {
            size_t nodeId = tree.Insert(boundable);
            insertedLeafIds.push_back(nodeId);
        }

        // 쿼리 영역 생성 (랜덤)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> posDistrib(-110.0f, 110.0f);  // 데이터 범위보다 약간 넓게
        std::uniform_real_distribution<float> sizeDistrib(5.0f, 30.0f);

        std::vector<FDynamicAABBTree::AABB> queryRegions;
        for (int i = 0; i < queryCount; ++i)
        {
            FDynamicAABBTree::AABB queryBox;
            Vector3 center(posDistrib(gen), posDistrib(gen), posDistrib(gen));
            Vector3 halfExtent(sizeDistrib(gen), sizeDistrib(gen), sizeDistrib(gen));

            queryBox.Min = center - halfExtent;
            queryBox.Max = center + halfExtent;

            queryRegions.push_back(queryBox);
        }

        // 쿼리 결과 저장
        std::vector<std::vector<size_t>> queryResults;
        std::vector<float> queryTimes;

        // 쿼리 실행 및 시간 측정
        for (const auto& queryBox : queryRegions)
        {
            std::vector<size_t> result;

            auto startTime = std::chrono::high_resolution_clock::now();

            tree.QueryOverlap(queryBox, [&result](size_t nodeId) {
                result.push_back(nodeId);
                              });

            auto endTime = std::chrono::high_resolution_clock::now();
            float duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1000.0f;

            queryResults.push_back(result);
            queryTimes.push_back(duration);
        }

        // 결과 저장
        std::ofstream outFile(outputFile);
        if (outFile.is_open())
        {
            for (size_t i = 0; i < queryResults.size(); ++i)
            {
                outFile << "Query " << i << " (" << queryTimes[i] << " ms): ";
                for (size_t nodeId : queryResults[i])
                {
                    outFile << nodeId << " ";
                }
                outFile << std::endl;
            }
            outFile.close();
        }

        // 쿼리 결과 검증
        bool isValid = true;

        // 1. 쿼리 시간 검증
        float totalQueryTime = 0.0f;
        maxQueryTimeMs = queryTimes.empty() ? 0.0f : queryTimes[0];
        for (float time : queryTimes)
        {
            totalQueryTime += time;
            maxQueryTimeMs = std::max(maxQueryTimeMs, time);
            if (time > acceptableQueryTimeMs)
            {
                isValid = false;
            }
        }
        avgQueryTimeMs = queryTimes.empty() ? 0.0f : totalQueryTime / queryTimes.size();

        // 2. 쿼리 결과 정확성 검증 - 브루트 포스 방식으로 검증
        incorrectQueries = 0;
        for (size_t i = 0; i < queryRegions.size(); ++i)
        {
            const auto& queryBox = queryRegions[i];
            std::vector<size_t> bruteForceResult;

            // 모든 바운더블과 직접 AABB 교차 테스트
            for (size_t j = 0; j < boundables.size(); ++j)
            {
                const auto& boundable = boundables[j];
                Vector3 pos = boundable->GetWorldTransform().Position;
                Vector3 ext = boundable->GetScaledHalfExtent();

                FDynamicAABBTree::AABB objectBox;
                objectBox.Min = pos - ext;
                objectBox.Max = pos + ext;

                if (queryBox.Overlaps(objectBox))
                {
                    size_t nodeId = insertedLeafIds[j];
                    bruteForceResult.push_back(nodeId);
                }
            }

            // 결과 비교 
            std::sort(bruteForceResult.begin(), bruteForceResult.end());
            std::sort(queryResults[i].begin(), queryResults[i].end());
            if (bruteForceResult.size() != queryResults[i].size())
            {
                incorrectQueries++;
            }
            else
            {
                for (size_t k = 0; k < bruteForceResult.size(); ++k)
                {
                    if (bruteForceResult[k] != queryResults[i][k])
                    {
                        incorrectQueries++;
                        break;
                    }
                }
            }
        }

        if (incorrectQueries > 0)
        {
            isValid = false;
        }

        return isValid;
    }

    int RunAllTests(std::ostream& os = std::cout, int nodeCount = 500, int deleteCount = 100, int queryCount = 100) {
        const std::string targetPath = "test\\";
        const std::string insertInputFile = targetPath + "insert_test_input.txt";
        const std::string insertOutputFile = targetPath + "insert_test_actual.txt";
        const std::string deleteInputFile = targetPath + "delete_test_input.txt";
        const std::string deleteBeforeFile = targetPath + "before_deletion.txt";
        const std::string deleteAfterFile = targetPath + "delete_test_actual.txt";
        const std::string queryInputFile = targetPath + "query_test_input.txt";
        const std::string queryOutputFile = targetPath + "query_test_actual.txt";

        float acceptableQueryTimeMs = 5.0f;

        // 테스트 데이터 생성
        GenerateTestData(insertInputFile, nodeCount);
        GenerateTestData(deleteInputFile, nodeCount);
        GenerateTestData(queryInputFile, nodeCount);
        GenerateQueryTestData(targetPath + "query_test_queries.txt", queryCount);

        // 테스트 1: 삽입 및 밸런싱
        int insertedNodes = 0;
        int treeNodeCount = 0;
        float insertionTimeMs = 0.0f;
        bool test1 = TestInsertionAndBalance(nodeCount, insertInputFile, insertOutputFile,
                                             insertedNodes, treeNodeCount, insertionTimeMs);

        os << "Insertion Test Results:" << std::endl;
        os << "  Nodes inserted: " << insertedNodes << std::endl;
        os << "  Tree node count: " << treeNodeCount << std::endl;
        os << "  Execution time: " << insertionTimeMs << " ms" << std::endl;
        os << "  Tree is valid: " << (test1 ? "YES" : "NO") << std::endl;
        os << "\n-----------------------------------------\n" << std::endl;

        // 테스트 2: 삭제 및 무결성
        int remainingNodes = 0;
        float deletionTimeMs = 0.0f;
        bool test2 = TestDeletionAndIntegrity(nodeCount, deleteCount, deleteInputFile,
                                              deleteBeforeFile, deleteAfterFile,
                                              remainingNodes, deletionTimeMs);

        os << "Deletion Test Results:" << std::endl;
        os << "  Nodes deleted: " << deleteCount << std::endl;
        os << "  Tree node count: " << remainingNodes << " (expected: " << (nodeCount - deleteCount) << ")" << std::endl;
        os << "  Execution time: " << deletionTimeMs << " ms" << std::endl;
        os << "  Tree is valid: " << (test2 ? "YES" : "NO") << std::endl;
        os << "\n-----------------------------------------\n" << std::endl;

        // 테스트 3: 쿼리 오버랩
        float avgQueryTimeMs = 0.0f;
        float maxQueryTimeMs = 0.0f;
        int incorrectQueries = 0;
        bool test3 = TestQueryOverlap(nodeCount, queryCount, acceptableQueryTimeMs,
                                      queryInputFile, queryOutputFile,
                                      avgQueryTimeMs, maxQueryTimeMs, incorrectQueries);

        os << "Query Overlap Test Results:" << std::endl;
        os << "  Queries executed: " << queryCount << std::endl;
        os << "  Average query time: " << avgQueryTimeMs << " ms" << std::endl;
        if (queryCount > 0)
        {
            os << "  Max query time: " << maxQueryTimeMs << " ms" << std::endl;
        }
        os << "  Incorrect queries: " << incorrectQueries << std::endl;
        os << "  Tests passed: " << (test3 ? "YES" : "NO") << std::endl;
        os << "\n-----------------------------------------\n" << std::endl;

        // 종합 결과 출력
        os << "Overall Test Results:" << std::endl;
        os << "  Test 1 (Insertion & Balance): " << (test1 ? "PASSED" : "FAILED") << std::endl;
        os << "  Test 2 (Deletion & Integrity): " << (test2 ? "PASSED" : "FAILED") << std::endl;
        os << "  Test 3 (Query Overlap): " << (test3 ? "PASSED" : "FAILED") << std::endl;

        return (test1 && test2 && test3) ? 0 : 1;
    }
    int RunAllTestsIterate(std::ostream& os, int iterations, int nodeCount = 500, int deleteCount = 100, int queryCount = 100) {
        const std::string targetPath = "test\\";
        const std::string insertInputFile = targetPath + "insert_test_input.txt";
        const std::string insertOutputFile = targetPath + "insert_test_actual.txt";
        const std::string deleteInputFile = targetPath + "delete_test_input.txt";
        const std::string deleteBeforeFile = targetPath + "before_deletion.txt";
        const std::string deleteAfterFile = targetPath + "delete_test_actual.txt";
        const std::string queryInputFile = targetPath + "query_test_input.txt";
        const std::string queryOutputFile = targetPath + "query_test_actual.txt";

        float acceptableQueryTimeMs = 5.0f;

        // 테스트 데이터 생성
        GenerateTestData(insertInputFile, nodeCount);
        GenerateTestData(deleteInputFile, nodeCount);
        GenerateTestData(queryInputFile, nodeCount);
        GenerateQueryTestData(targetPath + "query_test_queries.txt", queryCount);

        // 반복 테스트 결과 누적 변수
        int totalInsertedNodes = 0;
        int totalTreeNodeCount = 0;
        float totalInsertionTime = 0.0f;
        int insertPassCount = 0;

        int totalRemainingNodes = 0;
        float totalDeletionTime = 0.0f;
        int deletePassCount = 0;

        float totalAvgQueryTime = 0.0f;
        float overallMaxQueryTime = 0.0f;
        int totalIncorrectQueries = 0;
        int queryPassCount = 0;

        // 반복 테스트 실행
        for (int i = 0; i < iterations; ++i)
        {
            // 테스트 1: 삽입 및 밸런싱
            int insertedNodes = 0;
            int treeNodeCount = 0;
            float insertionTimeMs = 0.0f;
            bool testInsert = TestInsertionAndBalance(nodeCount, insertInputFile, insertOutputFile,
                                                      insertedNodes, treeNodeCount, insertionTimeMs);

            totalInsertedNodes += insertedNodes;
            totalTreeNodeCount += treeNodeCount;
            totalInsertionTime += insertionTimeMs;
            if (testInsert) insertPassCount++;

            // 테스트 2: 삭제 및 무결성
            int remainingNodes = 0;
            float deletionTimeMs = 0.0f;
            bool testDelete = TestDeletionAndIntegrity(nodeCount, deleteCount, deleteInputFile,
                                                       deleteBeforeFile, deleteAfterFile,
                                                       remainingNodes, deletionTimeMs);

            totalRemainingNodes += remainingNodes;
            totalDeletionTime += deletionTimeMs;
            if (testDelete) deletePassCount++;

            // 테스트 3: 쿼리 오버랩
            float avgQueryTimeMs = 0.0f;
            float maxQueryTimeMs = 0.0f;
            int incorrectQueries = 0;
            bool testQuery = TestQueryOverlap(nodeCount, queryCount, acceptableQueryTimeMs,
                                              queryInputFile, queryOutputFile,
                                              avgQueryTimeMs, maxQueryTimeMs, incorrectQueries);

            totalAvgQueryTime += avgQueryTimeMs;
            overallMaxQueryTime = std::max(overallMaxQueryTime, maxQueryTimeMs);
            totalIncorrectQueries += incorrectQueries;
            if (testQuery) queryPassCount++;

            os << (i + 1) << "/" << iterations << "\n";
        }

        // 평균 계산
        float avgInsertedNodes = static_cast<float>(totalInsertedNodes) / iterations;
        float avgTreeNodeCount = static_cast<float>(totalTreeNodeCount) / iterations;
        float avgInsertionTime = totalInsertionTime / iterations;

        float avgRemainingNodes = static_cast<float>(totalRemainingNodes) / iterations;
        float avgDeletionTime = totalDeletionTime / iterations;

        float avgQueryTime = totalAvgQueryTime / iterations;
        float avgIncorrectQueries = static_cast<float>(totalIncorrectQueries) / iterations;

        // 결과 출력
        os << "Insertion Test Results (Average of " << iterations << " iterations):" << std::endl;
        os << "  Nodes inserted: " << avgInsertedNodes << std::endl;
        os << "  Tree node count: " << avgTreeNodeCount << std::endl;
        os << "  Execution time: " << avgInsertionTime << " ms" << std::endl;
        os << "  Pass rate: " << (static_cast<float>(insertPassCount) / iterations * 100) << "%" << std::endl;
        os << "\n-----------------------------------------\n" << std::endl;

        os << "Deletion Test Results (Average of " << iterations << " iterations):" << std::endl;
        os << "  Nodes deleted: " << deleteCount << std::endl;
        os << "  Tree node count: " << avgRemainingNodes << " (expected: " << (nodeCount - deleteCount) << ")" << std::endl;
        os << "  Execution time: " << avgDeletionTime << " ms" << std::endl;
        os << "  Pass rate: " << (static_cast<float>(deletePassCount) / iterations * 100) << "%" << std::endl;
        os << "\n-----------------------------------------\n" << std::endl;

        os << "Query Overlap Test Results (Average of " << iterations << " iterations):" << std::endl;
        os << "  Queries executed: " << queryCount << std::endl;
        os << "  Average query time: " << avgQueryTime << " ms" << std::endl;
        os << "  Max query time: " << overallMaxQueryTime << " ms" << std::endl;
        os << "  Average incorrect queries: " << avgIncorrectQueries << std::endl;
        os << "  Pass rate: " << (static_cast<float>(queryPassCount) / iterations * 100) << "%" << std::endl;
        os << "\n-----------------------------------------\n" << std::endl;

        os << "Overall Test Results:" << std::endl;
        os << "  Test 1 (Insertion & Balance) pass rate: " << (static_cast<float>(insertPassCount) / iterations * 100) << "%" << std::endl;
        os << "  Test 2 (Deletion & Integrity) pass rate: " << (static_cast<float>(deletePassCount) / iterations * 100) << "%" << std::endl;
        os << "  Test 3 (Query Overlap) pass rate: " << (static_cast<float>(queryPassCount) / iterations * 100) << "%" << std::endl;

        // 모든 테스트의 통과 비율이 100%인지 확인
        bool allTestsPassed = (insertPassCount == iterations) &&
            (deletePassCount == iterations) &&
            (queryPassCount == iterations);
        return allTestsPassed ? 0 : 1;
    }
}
//how to use in main
////test dynamic tree
//{
//    std::ofstream outFile("test\\test_all_result.txt");
//    if (!outFile.is_open())
//    {
//        throw std::runtime_error("Failed to open file for writing");
//    }

//    TestDynamicAABBTree::RunAllTests(outFile, 500, 455, 100);
//    CoUninitialize();
//    return 0;
//}