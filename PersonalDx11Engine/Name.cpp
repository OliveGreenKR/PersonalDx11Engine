#include "Name.h"
#include "NameTableManager.h"
#include <cctype>

const FName FName::NAME_None;

inline FName::FName() : Index(INDEX_NONE)
{
}

inline FName::FName(const char* InString)
{
    if (!InString || *InString == '\0')
    {
        Index = INDEX_NONE;
        return;
    }

    Index = UNameTableManager::Get().GetOrAddString(InString);
}

inline FName::FName(const std::string& InString)
    : FName(InString.c_str())
{
}

inline FName::FName(const FName& Other)
    : Index(Other.Index)
{
    AddRef();
}

inline FName::FName(FName&& Other) noexcept
    : Index(Other.Index)
{
    Other.Index = INDEX_NONE;
}

inline FName::~FName()
{
    RemoveRef();
}

bool FName::IsValid() const
{
    return Index != INDEX_NONE && Index != 0;
}

void FName::Invalidate()
{
    RemoveRef();
    Index = INDEX_NONE;
}

std::string FName::ToString() const
{
    const char* str = GetCString();
    return str ? std::string(str) : std::string();
}

const char* FName::GetCString() const
{
    if (!IsValid())
    {
        return nullptr;
    }

    return UNameTableManager::Get().GetString(Index);
}

uint32_t FName::GetIndex() const
{
    return Index;
}

FName FName::FindName(const char* InString)
{
    if (!InString || *InString == '\0')
    {
        return FName();
    }

    uint32_t index = UNameTableManager::Get().FindString(InString);
    if (index == INDEX_NONE)
    {
        return FName();
    }

    return FName(index);
}

FName FName::FindName(const std::string& InString)
{
    return FindName(InString.c_str());
}

bool FName::operator==(const char* Other) const
{
    if (!Other)
    {
        return !IsValid();
    }

    if (!IsValid())
    {
        return false;
    }

    const char* myString = GetCString();
    if (!myString)
    {
        return false;
    }

    // 대소문자 무시 비교
    const char* str1 = myString;
    const char* str2 = Other;

    while (*str1 && *str2)
    {
        if (std::tolower(static_cast<unsigned char>(*str1)) !=
            std::tolower(static_cast<unsigned char>(*str2)))
        {
            return false;
        }
        ++str1;
        ++str2;
    }

    return *str1 == *str2;
}

void FName::PrintNameTable()
{
    UNameTableManager::Get().PrintAllStrings();
}

void FName::AddRef()
{
    if (IsValid())
    {
        UNameTableManager::Get().AddReference(Index);
    }
}

void FName::RemoveRef()
{
    if (IsValid())
    {
        UNameTableManager::Get().RemoveReference(Index);
    }
}

size_t FNameHash::operator()(const FName& Name) const
{
    return static_cast<size_t>(Name.GetIndex());
}

std::ostream& operator<<(std::ostream& Stream, const FName& Name)
{
    const char* str = Name.GetCString();
    if (str)
    {
        Stream << str;
    }
    else
    {
        Stream << "(None)";
    }
    return Stream;
}