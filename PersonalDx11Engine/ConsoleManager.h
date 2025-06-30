// ConsoleManager.h
#pragma once

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cassert>
#include <cstdint>
#include <mutex>
#include <atomic>
#include <array>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif

// 로그 카테고리
enum class ELogCategory : uint8_t
{
    LogLevel_Normal = 0,
    LogLevel_Info,
    LogLevel_Error,    
    LogLevel_Warning,
    COUNT
};

// 편의를 위한 별칭 
namespace LogLevel
{
    constexpr ELogCategory Normal = ELogCategory::LogLevel_Normal;
    constexpr ELogCategory Info = ELogCategory::LogLevel_Info;
    constexpr ELogCategory Error = ELogCategory::LogLevel_Error;
    constexpr ELogCategory Warning = ELogCategory::LogLevel_Warning;
}
class UConsoleManager
{
private:
    // 컴파일 타임 상수
    static constexpr size_t BUFFER_SIZE = 1024 * 64;  // 64KB 문자열 버퍼
    static constexpr size_t MAX_SINGLE_LOG_SIZE = 512; // 단일 로그 최대 크기
    static constexpr size_t FLUSH_THRESHOLD = 32 * 1024; // 32KB에서 플러시

    // 비트마스킹용 상수 정의
    static constexpr uint8_t CATEGORY_MASK_NORMAL = 1 << static_cast<uint8_t>(ELogCategory::LogLevel_Normal);
    static constexpr uint8_t CATEGORY_MASK_INFO = 1 << static_cast<uint8_t>(ELogCategory::LogLevel_Info);
    static constexpr uint8_t CATEGORY_MASK_ERROR = 1 << static_cast<uint8_t>(ELogCategory::LogLevel_Error);
    static constexpr uint8_t CATEGORY_MASK_WARNING = 1 << static_cast<uint8_t>(ELogCategory::LogLevel_Warning);
    static constexpr uint8_t CATEGORY_MASK_ALL = 0xFF;

    // 로깅 카테고리
    std::atomic<uint8_t> CategoryEnabledMask{ CATEGORY_MASK_ALL }; // 기본값: 모든 카테고리 활성화

    // 색상 배열은 유지 (자주 변경되지 않으므로)
    std::array<WORD, static_cast<size_t>(ELogCategory::COUNT)> CategoryColors;

    // 단순한 순환 문자열 버퍼
    std::array<char, BUFFER_SIZE> LogBuffer;
    std::atomic<size_t> WritePosition{ 0 };
    std::atomic<size_t> PendingBytes{ 0 };

    // 동기화
    mutable std::mutex BufferMutex;
    std::atomic<bool> bAutoFlush{ true };

#ifdef _WIN32
    HANDLE hConsole;
    WORD OriginalAttributes;
#endif

    // 싱글톤
    UConsoleManager();
    ~UConsoleManager();
    UConsoleManager(const UConsoleManager&) = delete;
    UConsoleManager& operator=(const UConsoleManager&) = delete;

public:
    static UConsoleManager* Get()
    {
        static UConsoleManager* Instance = new UConsoleManager();
        return Instance;
    }

    // 핵심 로깅 함수
    void Log(ELogCategory Category, const char* FileName, int LineNumber,
             const char* Format, ...);

    // 버퍼 관리
    void FlushBuffer();
    void ClearBuffer();

    // 설정 접근자
    bool IsCategoryEnabled(ELogCategory Category) const;
    void SetCategoryEnabled(ELogCategory Category, bool bEnabled);
    void EnableAllCategories() { CategoryEnabledMask.store(CATEGORY_MASK_ALL); }
    void DisableAllCategories() { CategoryEnabledMask.store(0); }

    WORD GetCategoryColor(ELogCategory Category) const;
    void SetCategoryColor(ELogCategory Category, WORD Color);

    bool IsAutoFlushEnabled() const { return bAutoFlush.load(); }
    void SetAutoFlush(bool bEnabled) { bAutoFlush.store(bEnabled); }

    size_t GetPendingBytes() const { return PendingBytes.load(); }
    size_t GetBufferCapacity() const { return BUFFER_SIZE; }

    // 디버그 정보
    const char* GetCategoryName(ELogCategory Category) const;
    static constexpr size_t GetCategoryCount() { return static_cast<size_t>(ELogCategory::COUNT); }

private:
    static constexpr uint8_t GetCategoryMask(ELogCategory Category)
    {
        return 1 << static_cast<uint8_t>(Category);
    }

    void WriteToBuffer(ELogCategory Category, const char* FormattedMessage);
    void SetConsoleColor(ELogCategory Category);
    void ResetConsoleColor();
    void InitializeColors();
};