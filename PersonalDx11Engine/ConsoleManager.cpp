#include "ConsoleManager.h"

// ConsoleManager.cpp
#include "ConsoleManager.h"

UConsoleManager::UConsoleManager()
{
    // 기본값: 모든 카테고리 활성화
    CategoryEnabledMask.store(CATEGORY_MASK_ALL);

    // 버퍼 초기화
    LogBuffer.fill('\0');
    WritePosition.store(0);
    PendingBytes.store(0);

    // 콘솔 색상 초기화
    InitializeColors();
}


UConsoleManager::~UConsoleManager()
{
#ifdef _WIN32
    if (hConsole != INVALID_HANDLE_VALUE)
    {
        ResetConsoleColor();
    }
#endif
}

void UConsoleManager::Log(ELogCategory Category, const char* FileName, int LineNumber, const char* Format, ...)
{
    // 카테고리 필터링
    if (!IsCategoryEnabled(Category))
        return;

    // 임시 버퍼에 포맷팅 (스택 사용)
    char TempBuffer[MAX_SINGLE_LOG_SIZE];
    char FormattedMessage[MAX_SINGLE_LOG_SIZE];

    // 가변 인자 처리
    va_list Args;
    va_start(Args, Format);
    vsnprintf(TempBuffer, sizeof(TempBuffer), Format, Args);
    va_end(Args);

    // 최종 메시지 조립 (카테고리 + 소스 정보 + 메시지)
    const char* CategoryPrefix = GetCategoryName(Category);
    snprintf(FormattedMessage, sizeof(FormattedMessage),
             "[%s] %s:%d - %s\n",
             CategoryPrefix, FileName, LineNumber, TempBuffer);

    // 버퍼에 쓰기
    WriteToBuffer(Category, FormattedMessage);
}

void UConsoleManager::WriteToBuffer(ELogCategory Category, const char* FormattedMessage)
{
    std::lock_guard<std::mutex> Lock(BufferMutex);

    size_t MessageLength = strlen(FormattedMessage);
    size_t CurrentPos = WritePosition.load();

    // 버퍼 오버플로우 체크
    if (MessageLength >= BUFFER_SIZE - CurrentPos)
    {
        // 버퍼가 가득 찼으면 강제 플러시
        FlushBuffer();
        CurrentPos = 0;
    }

    // 문자열 복사
    memcpy(&LogBuffer[CurrentPos], FormattedMessage, MessageLength);

    // 위치 업데이트
    WritePosition.store(CurrentPos + MessageLength);
    PendingBytes.fetch_add(MessageLength);

    // 자동 플러시 체크
    if (bAutoFlush.load() && PendingBytes.load() >= FLUSH_THRESHOLD)
    {
        FlushBuffer();
    }
}

void UConsoleManager::FlushBuffer()
{
    std::lock_guard<std::mutex> Lock(BufferMutex);

    if (PendingBytes.load() == 0)
        return;

    // 버퍼 전체를 한 번에 출력
    size_t CurrentWrite = WritePosition.load();
    LogBuffer[CurrentWrite] = '\0'; // null 종료 보장

#ifdef _WIN32
    // Windows 콘솔에 직접 출력
    DWORD Written;
    WriteConsoleA(hConsole, LogBuffer.data(), static_cast<DWORD>(CurrentWrite), &Written, nullptr);
#else
    // POSIX 시스템
    fwrite(LogBuffer.data(), 1, CurrentWrite, stdout);
    fflush(stdout);
#endif

    // 버퍼 초기화
    WritePosition.store(0);
    PendingBytes.store(0);
}

void UConsoleManager::ClearBuffer()
{
    std::lock_guard<std::mutex> Lock(BufferMutex);
    WritePosition.store(0);
    PendingBytes.store(0);
}

bool UConsoleManager::IsCategoryEnabled(ELogCategory Category) const
{
    // enum 값이 유효 범위인지 확인
    uint8_t CategoryIndex = static_cast<uint8_t>(Category);
    if (CategoryIndex >= static_cast<uint8_t>(ELogCategory::COUNT))
        return false;

    // 비트마스킹으로 확인
    uint8_t CategoryMask = GetCategoryMask(Category);
    uint8_t CurrentMask = CategoryEnabledMask.load();

    return (CurrentMask & CategoryMask) != 0;
}

void UConsoleManager::SetCategoryEnabled(ELogCategory Category, bool bEnabled)
{
    // enum 값이 유효 범위인지 확인
    uint8_t CategoryIndex = static_cast<uint8_t>(Category);
    if (CategoryIndex >= static_cast<uint8_t>(ELogCategory::COUNT))
        return;

    uint8_t CategoryMask = GetCategoryMask(Category);

    if (bEnabled)
    {
        // 비트 설정 (OR 연산)
        uint8_t OldMask = CategoryEnabledMask.load();
        uint8_t NewMask = OldMask | CategoryMask;
        CategoryEnabledMask.store(NewMask);
    }
    else
    {
        // 비트 해제 (AND + NOT 연산)
        uint8_t OldMask = CategoryEnabledMask.load();
        uint8_t NewMask = OldMask & (~CategoryMask);
        CategoryEnabledMask.store(NewMask);
    }
}

WORD UConsoleManager::GetCategoryColor(ELogCategory Category) const
{
#ifdef _WIN32
    size_t Index = static_cast<size_t>(Category);
    if (Index < CategoryColors.size())
    {
        return CategoryColors[Index];
    }
    return OriginalAttributes;
#else
    return 0;
#endif
}

void UConsoleManager::SetCategoryColor(ELogCategory Category, WORD Color)
{
#ifdef _WIN32
    size_t Index = static_cast<size_t>(Category);
    if (Index < CategoryColors.size())
    {
        CategoryColors[Index] = Color;
    }
#endif
}

const char* UConsoleManager::GetCategoryName(ELogCategory Category) const
{
    switch (Category)
    {
        case ELogCategory::LogLevel_Normal:  return "NORMAL";
        case ELogCategory::LogLevel_Info:    return "INFO";
        case ELogCategory::LogLevel_Error:   return "ERROR";
        case ELogCategory::LogLevel_Warning: return "WARNING";
        default:                             return "UNKNOWN";
    }
}

void UConsoleManager::InitializeColors()
{
#ifdef _WIN32
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hConsole != INVALID_HANDLE_VALUE)
    {
        CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
        GetConsoleScreenBufferInfo(hConsole, &ConsoleInfo);
        OriginalAttributes = ConsoleInfo.wAttributes;

        // 카테고리별 색상 설정
        CategoryColors[static_cast<size_t>(ELogCategory::LogLevel_Normal)] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // 흰색
        CategoryColors[static_cast<size_t>(ELogCategory::LogLevel_Info)] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // 흰색
        CategoryColors[static_cast<size_t>(ELogCategory::LogLevel_Error)] = FOREGROUND_RED | FOREGROUND_INTENSITY;             // 밝은 빨간색
        CategoryColors[static_cast<size_t>(ELogCategory::LogLevel_Warning)] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; // 노란색
    }
#endif
}

void UConsoleManager::SetConsoleColor(ELogCategory Category)
{
#ifdef _WIN32
    if (hConsole != INVALID_HANDLE_VALUE)
    {
        WORD Color = GetCategoryColor(Category);
        SetConsoleTextAttribute(hConsole, Color);
    }
#endif
}

void UConsoleManager::ResetConsoleColor()
{
#ifdef _WIN32
    if (hConsole != INVALID_HANDLE_VALUE)
    {
        SetConsoleTextAttribute(hConsole, OriginalAttributes);
    }
#endif
}