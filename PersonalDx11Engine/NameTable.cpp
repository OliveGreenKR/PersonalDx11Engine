#include "NameTable.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cctype>

FNameTable::FNameTable()
{
    HashTable.resize(HASH_TABLE_SIZE, INDEX_NONE);

    // 인덱스 0은 무효한 값으로 예약
    StringArray.resize(1);
    StringArray[0] = nullptr;
}

FNameTable::~FNameTable()
{
    // unique_ptr이 자동으로 정리
}

FNameTable& FNameTable::Get()
{
    static FNameTable Instance;
    return Instance;
}

uint32_t FNameTable::GetOrAddString(const char* InString)
{
    if (!InString || *InString == '\0')
    {
        return INDEX_NONE;
    }

    // 기존 문자열 검색
    uint32_t existingIndex = FindString(InString);
    if (existingIndex != INDEX_NONE)
    {
        AddReference(existingIndex);
        return existingIndex;
    }

    // 새 인덱스 할당
    uint32_t newIndex = static_cast<uint32_t>(StringArray.size());

    // 해시값 계산
    uint32_t hashValue = CalculateHash(InString);
    uint32_t hashSlot = hashValue % HASH_TABLE_SIZE;

    // 기존 체인의 첫 번째 엔트리 인덱스
    uint32_t existingChainHead = HashTable[hashSlot];

    // 새 엔트리 생성 및 배열에 추가
    StringArray.emplace_back(std::make_unique<FNameEntry>(InString, newIndex, existingChainHead));

    // 해시 테이블 업데이트
    HashTable[hashSlot] = newIndex;

    // 빠른 검색용 맵 업데이트
    StringToIndexMap[InString] = newIndex;

    // 참조 카운트 증가
    AddReference(newIndex);

    return newIndex;
}

uint32_t FNameTable::FindString(const char* InString) const
{
    if (!InString || *InString == '\0')
    {
        return INDEX_NONE;
    }

    // 빠른 검색용 맵에서 먼저 확인
    auto it = StringToIndexMap.find(InString);
    if (it != StringToIndexMap.end())
    {
        return it->second;
    }

    // 해시 테이블에서 검색
    uint32_t hashValue = CalculateHash(InString);
    uint32_t hashSlot = hashValue % HASH_TABLE_SIZE;

    uint32_t currentIndex = HashTable[hashSlot];

    while (currentIndex != INDEX_NONE && currentIndex < StringArray.size())
    {
        const auto& entry = StringArray[currentIndex];
        if (entry && StringsEqualIgnoreCase(entry->StringData.c_str(), InString))
        {
            return currentIndex;
        }
        currentIndex = entry->HashNext;
    }

    return INDEX_NONE;
}

const char* FNameTable::GetString(uint32_t Index) const
{
    if (Index == INDEX_NONE || Index == 0 || Index >= StringArray.size())
    {
        return nullptr;
    }

    const auto& entry = StringArray[Index];
    return entry ? entry->StringData.c_str() : nullptr;
}

void FNameTable::AddReference(uint32_t Index)
{
    if (Index == INDEX_NONE || Index == 0 || Index >= StringArray.size())
    {
        return;
    }

    auto& entry = StringArray[Index];
    if (entry)
    {
        entry->ReferenceCount++;
    }
}

void FNameTable::RemoveReference(uint32_t Index)
{
    if (Index == INDEX_NONE || Index == 0 || Index >= StringArray.size())
    {
        return;
    }

    auto& entry = StringArray[Index];
    if (!entry)
    {
        return;
    }

    if (entry->ReferenceCount > 0)
    {
        entry->ReferenceCount--;
    }

    // 참조 카운트가 0이 되면 메모리 해제
    if (entry->ReferenceCount == 0)
    {
        // 해시 테이블에서 제거
        uint32_t hashValue = CalculateHash(entry->StringData.c_str());
        uint32_t hashSlot = hashValue % HASH_TABLE_SIZE;

        // 체인에서 제거
        if (HashTable[hashSlot] == Index)
        {
            // 첫 번째 엔트리인 경우
            HashTable[hashSlot] = entry->HashNext;
        }
        else
        {
            // 체인 중간에 있는 경우
            uint32_t currentIndex = HashTable[hashSlot];
            while (currentIndex != INDEX_NONE && currentIndex < StringArray.size())
            {
                auto& currentEntry = StringArray[currentIndex];
                if (currentEntry && currentEntry->HashNext == Index)
                {
                    currentEntry->HashNext = entry->HashNext;
                    break;
                }
                currentIndex = currentEntry->HashNext;
            }
        }

        // 빠른 검색용 맵에서 제거
        StringToIndexMap.erase(entry->StringData);

        // 엔트리 제거
        entry.reset();
    }
}

uint32_t FNameTable::GetReferenceCount(uint32_t Index) const
{
    if (Index == INDEX_NONE || Index == 0 || Index >= StringArray.size())
    {
        return 0;
    }

    const auto& entry = StringArray[Index];
    return entry ? entry->ReferenceCount : 0;
}

size_t FNameTable::GetStringCount() const
{
    size_t count = 0;
    for (size_t i = 1; i < StringArray.size(); ++i)  // INDEX_NONE(0) 제외
    {
        if (StringArray[i])
        {
            count++;
        }
    }
    return count;
}

void FNameTable::PrintAllStrings() const
{
    std::cout << "=== FName String Table ===" << std::endl;
    std::cout << "Active Strings: " << GetStringCount() << std::endl;
    std::cout << std::endl;

    for (size_t i = 1; i < StringArray.size(); ++i)  // INDEX_NONE(0) 제외
    {
        const auto& entry = StringArray[i];
        if (entry)
        {
            std::cout << std::setw(4) << i
                << ": \"" << entry->StringData << "\""
                << " (refs: " << entry->ReferenceCount << ")" << std::endl;
        }
    }
    std::cout << std::endl;
}

uint32_t FNameTable::CalculateHash(const char* InString) const
{
    if (!InString)
    {
        return 0;
    }

    // FNV-1a 해시 알고리즘 (대소문자 무시)
    constexpr uint32_t FNV_PRIME = 16777619u;
    constexpr uint32_t FNV_OFFSET_BASIS = 2166136261u;

    uint32_t hash = FNV_OFFSET_BASIS;

    while (*InString)
    {
        char ch = static_cast<char>(std::tolower(static_cast<unsigned char>(*InString)));
        hash ^= static_cast<uint32_t>(ch);
        hash *= FNV_PRIME;
        ++InString;
    }

    return hash;
}

bool FNameTable::StringsEqualIgnoreCase(const char* Str1, const char* Str2) const
{
    if (!Str1 || !Str2)
    {
        return Str1 == Str2;
    }

    while (*Str1 && *Str2)
    {
        if (std::tolower(static_cast<unsigned char>(*Str1)) !=
            std::tolower(static_cast<unsigned char>(*Str2)))
        {
            return false;
        }
        ++Str1;
        ++Str2;
    }

    return *Str1 == *Str2;
}