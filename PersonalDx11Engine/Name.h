#pragma once

#include <cstdint>
#include <string>
#include <iostream>

class UNameTableManager;

/**
 * 경량 문자열 식별자 - 전역 테이블에 대한 약한 참조
 * 내부적으로 4바이트 인덱스만 저장
 */
class FName
{
public:
    FName();
    FName(const char* InString);
    FName(const std::string& InString);
    FName(const FName& Other);
    FName(FName&& Other) noexcept;

    ~FName();

    FName& operator=(const FName& Other)
    {
        if (this != &Other)
        {
            RemoveRef();
            Index = Other.Index;
            AddRef();
        }
        return *this;
    }
    FName& operator=(FName&& Other) noexcept
    {
        if (this != &Other)
        {
            RemoveRef();
            Index = Other.Index;
            Other.Index = INDEX_NONE;
        }
        return *this;
    }

    bool IsValid() const;
    void Invalidate();

    std::string ToString() const;
    const char* GetCString() const;
    uint32_t GetIndex() const;

    /**
     * 검색 전용 - 문자열이 존재하지 않으면 무효한 FName 반환
     */
    static FName FindName(const char* InString);
    static FName FindName(const std::string& InString);

    bool operator==(const FName& Other) const
    {
        return Index == Other.Index;
    }
    bool operator!=(const FName& Other) const
    {
        return Index != Other.Index;
    }
    bool operator<(const FName& Other) const
    {
        return Index < Other.Index;
    }

    bool operator==(const char* Other) const;

    bool operator!=(const char* Other) const
    {
        return !(*this == Other);
    }
    bool operator==(const std::string& Other) const
    {
        return *this == Other.c_str();
    }
    bool operator!=(const std::string& Other) const
    {
        return !(*this == Other);
    }

    /**
     * 디버깅용 출력
     */
    static void PrintNameTable();

    static const FName NAME_None;

private:
    explicit FName(uint32_t InIndex)
        : Index(InIndex)
    {
        AddRef();
    }

    void AddRef();
    void RemoveRef();

private:
    static constexpr uint32_t INDEX_NONE = 0;
    uint32_t Index;
};

/**
 * 해시 함수 객체 (컨테이너용)
 */
struct FNameHash
{
    size_t operator()(const FName& Name) const;
};

std::ostream& operator<<(std::ostream& Stream, const FName& Name);
