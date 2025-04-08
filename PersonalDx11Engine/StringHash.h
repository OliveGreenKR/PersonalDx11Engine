#pragma once
#include <cstdint>
#include <cstddef>

struct FStringHash
{
private:
    uint64_t HashValue = 0;

    explicit FStringHash(uint64_t value)
        : HashValue(value)
    {
    }

public:



    void Invalidate() { HashValue = 0; }

    bool IsValid() const { return HashValue != 0; }
    uint64_t GetHash() const { return HashValue; }

    // 기본 생성자 -  invalid Key
    explicit FStringHash() : HashValue(0) {} 
     
    // 와이드 문자열 생성자
    explicit FStringHash(const wchar_t* Str)
    { 
        if (!Str) return; 

        constexpr std::uint64_t FNV_PRIME = 1099511628211ULL;
        constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;

        HashValue = FNV_OFFSET_BASIS;

        while (*Str)
        {
            HashValue ^= static_cast<std::uint64_t>(*Str++);
            HashValue *= FNV_PRIME;
        }
    }

    // 문자열 생성자
    explicit FStringHash(const char* Str)
    {
        if (!Str) return;

        // FNV-1a 해시 알고리즘 사용
        constexpr std::uint64_t FNV_PRIME = 1099511628211ULL;
        constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;

        HashValue = FNV_OFFSET_BASIS;

        while (*Str)
        {
            HashValue ^= static_cast<std::uint64_t>(*Str++);
            HashValue *= FNV_PRIME;
        }
    }

    // 연산자
    bool operator==(const FStringHash& Other) const { return HashValue == Other.HashValue; }
    bool operator!=(const FStringHash& Other) const { return HashValue != Other.HashValue; }
    bool operator<(const FStringHash& Other) const { return HashValue < Other.HashValue; }

    //for unordered_map
    struct Hasher
    {
        std::size_t operator()(const FStringHash& Key) const
        {
            return static_cast<std::size_t>(Key.HashValue);
        }
    };
};