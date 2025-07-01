// ConsoleManager.h
#pragma once
#include <cstdint>
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
enum class ELogCategory : std::uint8_t
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

    // 색상 배열 (Windows API 호환용)
    std::array<WORD, static_cast<size_t>(ELogCategory::COUNT)> CategoryColors;

    // 단순한 순환 문자열 버퍼
    std::array<char, BUFFER_SIZE> LogBuffer;
    std::atomic<size_t> WritePosition{ 0 };
    std::atomic<size_t> PendingBytes{ 0 };

    // 동기화
    mutable std::mutex BufferMutex;
    std::atomic<bool> bAutoFlush{ true };

#ifdef _WIN32
private:
    HANDLE hConsole;
    WORD OriginalAttributes;
#endif

private:
    // 콘솔 창 관련 멤버
    bool bConsoleCreated = false;
    HWND ConsoleWindow = nullptr;

    // 콘솔 설정
    struct FConsoleSettings
    {
        int Width = 800;
        int Height = 600;
        int PosX = 100;
        int PosY = 100;
        int BufferLines = 1000;
        bool bAutoPosition = true;
    } ConsoleSettings;

private:
    // Virtual Terminal 지원 여부 (내부적으로만 사용)
    bool bVirtualTerminalEnabled = false;

    // ANSI 색상 코드 상수 (내부적으로만 사용)
    static constexpr const char* ANSI_RESET = "\033[0m";
    static constexpr const char* ANSI_RED = "\033[91m";
    static constexpr const char* ANSI_GREEN = "\033[92m";
    static constexpr const char* ANSI_YELLOW = "\033[93m";
    static constexpr const char* ANSI_WHITE = "\033[97m";

private:
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

    bool IsAutoFlushEnabled() const { return bAutoFlush.load(); }
    void SetAutoFlush(bool bEnabled) { bAutoFlush.store(bEnabled); }

    size_t GetPendingBytes() const { return PendingBytes.load(); }
    size_t GetBufferCapacity() const { return BUFFER_SIZE; }

    // 디버그 정보
    const char* GetCategoryName(ELogCategory Category) const;
    static constexpr size_t GetCategoryCount() { return static_cast<size_t>(ELogCategory::COUNT); }

#pragma region Console
public:
    bool Initialize(const FConsoleSettings& Settings = FConsoleSettings{});
    bool Initialize(int Width, int Height, int PosX, int PosY, int BufferLines = 1000);

    // 콘솔 해제
    void Shutdown();

    // 콘솔 창 제어
    void ShowConsole(bool bShow);
    void SetConsolePosition(int PosX, int PosY);
    void SetConsoleSize(int Width, int Height);
    void SetConsoleTitleF(const char* Title);

    // 설정 접근자
    const FConsoleSettings& GetConsoleSettings() const { return ConsoleSettings; }
    bool IsConsoleCreated() const { return bConsoleCreated; }

private:
    // 내부 콘솔 생성 함수
    bool CreateConsoleWindow();
    void ConfigureConsoleBuffer();
    void SetupConsolePosition();

#pragma endregion

private:
    static constexpr uint8_t GetCategoryMask(ELogCategory Category)
    {
        return 1 << static_cast<uint8_t>(Category);
    }

    void WriteToBuffer(ELogCategory Category, const char* FormattedMessage);

    // 색상 설정 (Virtual Terminal 지원 여부에 따라 내부적으로 방식 선택)
    void SetConsoleColor(ELogCategory Category);
    void ResetConsoleColor();
    void InitializeColors();

    // Virtual Terminal 관련 (내부적으로만 사용)
    bool EnableVirtualTerminal();
    const char* GetAnsiColorCode(ELogCategory Category) const;
    WORD GetCategoryColor(ELogCategory Category) const;
    void SetCategoryColor(ELogCategory Category, WORD Color);
};