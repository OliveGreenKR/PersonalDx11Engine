#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

/**
 * 전역 문자열 테이블 관리자 (싱글톤)
 * 문자열의 유일성 보장 및 참조 카운팅을 통한 메모리 관리
 */
class FNameTable
{
private:
    FNameTable();
    ~FNameTable();

    FNameTable(const FNameTable&) = delete;
    FNameTable& operator=(const FNameTable&) = delete;
    FNameTable(FNameTable&&) = delete;
    FNameTable& operator=(FNameTable&&) = delete;

public:
    static FNameTable& Get();

    /**
     * 문자열을 테이블에 추가하거나 기존 인덱스 반환
     * 참조 카운트 증가
     */
    uint32_t GetOrAddString(const char* InString);

    /**
     * 문자열 검색 (추가하지 않음)
     */
    uint32_t FindString(const char* InString) const;

    /**
     * 인덱스로 문자열 반환
     */
    const char* GetString(uint32_t Index) const;

    /**
     * 참조 카운트 증가
     */
    void AddReference(uint32_t Index);

    /**
     * 참조 카운트 감소, 0이 되면 메모리 해제
     */
    void RemoveReference(uint32_t Index);

    /**
     * 참조 카운트 반환
     */
    uint32_t GetReferenceCount(uint32_t Index) const;

    /**
     * 현재 저장된 문자열 개수
     */
    size_t GetStringCount() const;

    /**
     * 디버깅용 출력
     */
    void PrintAllStrings() const;

private:
    struct FNameEntry
    {
        std::string StringData;
        uint32_t Index;
        uint32_t ReferenceCount;
        uint32_t HashNext;

        FNameEntry(const char* InString, uint32_t InIndex, uint32_t InHashNext)
            : StringData(InString), Index(InIndex), ReferenceCount(0), HashNext(InHashNext) {
        }
    };

    uint32_t CalculateHash(const char* InString) const;
    bool StringsEqualIgnoreCase(const char* Str1, const char* Str2) const;

private:
    static constexpr uint32_t HASH_TABLE_SIZE = 4096;
    static constexpr uint32_t INDEX_NONE = 0;

    std::vector<std::unique_ptr<FNameEntry>> StringArray;
    std::vector<uint32_t> HashTable;
    std::unordered_map<std::string, uint32_t> StringToIndexMap;
};