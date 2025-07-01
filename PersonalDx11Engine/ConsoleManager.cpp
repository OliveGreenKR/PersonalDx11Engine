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
    Shutdown();
}

// ConsoleManager.cpp에 추가할 구현

bool UConsoleManager::Initialize(const FConsoleSettings& Settings)
{
    if (bConsoleCreated)
    {
        return true; // 이미 생성됨
    }

    ConsoleSettings = Settings;

    if (!CreateConsoleWindow())
    {
        return false;
    }

    // 기존 초기화 로직
    CategoryEnabledMask.store(CATEGORY_MASK_ALL);
    LogBuffer.fill('\0');
    WritePosition.store(0);
    PendingBytes.store(0);

    // 콘솔 색상 초기화
    InitializeColors();

    bConsoleCreated = true;
    return true;
}

bool UConsoleManager::Initialize(int Width, int Height, int PosX, int PosY, int BufferLines)
{
    FConsoleSettings Settings;
    Settings.Width = Width;
    Settings.Height = Height;
    Settings.PosX = PosX;
    Settings.PosY = PosY;
    Settings.BufferLines = BufferLines;
    Settings.bAutoPosition = false;

    return Initialize(Settings);
}

bool UConsoleManager::CreateConsoleWindow()
{
#ifdef _WIN32
    // 콘솔 할당
    if (!AllocConsole())
    {
        return false;
    }

    // 표준 입출력 스트림을 콘솔로 리다이렉션
    FILE* pConsole = nullptr;
    if (freopen_s(&pConsole, "CONOUT$", "w", stdout) != 0)
    {
        FreeConsole();
        return false;
    }

    if (freopen_s(&pConsole, "CONIN$", "r", stdin) != 0)
    {
        FreeConsole();
        return false;
    }

    // 콘솔 창 핸들 가져오기
    ConsoleWindow = GetConsoleWindow();
    if (!ConsoleWindow)
    {
        FreeConsole();
        return false;
    }

    // 위치 및 크기 설정
    SetupConsolePosition();

    // 버퍼 설정
    ConfigureConsoleBuffer();

    // 콘솔 핸들 가져오기 (색상 출력용)
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    return true;
#else
    // 다른 플랫폼에서는 콘솔이 기본적으로 존재
    hConsole = nullptr;
    bConsoleCreated = true;
    return true;
#endif
}

void UConsoleManager::SetupConsolePosition()
{
#ifdef _WIN32
    if (!ConsoleWindow) return;

    int PosX = ConsoleSettings.PosX;
    int PosY = ConsoleSettings.PosY;

    // 자동 위치 설정
    if (ConsoleSettings.bAutoPosition)
    {
        // 주 모니터 해상도 가져오기
        int ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
        int ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

        // 화면 우측에 배치
        PosX = ScreenWidth - ConsoleSettings.Width - 50;
        PosY = 50;
    }

    // 창 크기와 위치 설정
    MoveWindow(ConsoleWindow, PosX, PosY,
               ConsoleSettings.Width, ConsoleSettings.Height, TRUE);
#endif
}

void UConsoleManager::ConfigureConsoleBuffer()
{
#ifdef _WIN32
    HANDLE ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (ConsoleHandle == INVALID_HANDLE_VALUE) return;

    // 현재 콘솔 정보 가져오기
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(ConsoleHandle, &csbi);

    // 버퍼 크기 설정
    COORD bufferSize;
    bufferSize.X = static_cast<SHORT>(std::min(ConsoleSettings.Width / 8, static_cast<int>(SHRT_MAX)));
    bufferSize.Y = static_cast<SHORT>(std::min(ConsoleSettings.BufferLines, static_cast<int>(SHRT_MAX)));

    SetConsoleScreenBufferSize(ConsoleHandle, bufferSize);
#endif
}

void UConsoleManager::Shutdown()
{
#ifdef _WIN32
    if (bConsoleCreated && ConsoleWindow)
    {
        // 버퍼 플러시
        FlushBuffer();

        // 콘솔 색상 복원
        ResetConsoleColor();

        // 콘솔 해제
        FreeConsole();

        ConsoleWindow = nullptr;
        hConsole = INVALID_HANDLE_VALUE;
        bConsoleCreated = false;
    }
#endif
}

void UConsoleManager::ShowConsole(bool bShow)
{
#ifdef _WIN32
    if (ConsoleWindow)
    {
        ShowWindow(ConsoleWindow, bShow ? SW_SHOW : SW_HIDE);
    }
#endif
}

void UConsoleManager::SetConsolePosition(int PosX, int PosY)
{
#ifdef _WIN32
    if (ConsoleWindow)
    {
        SetWindowPos(ConsoleWindow, nullptr, PosX, PosY, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER);
        ConsoleSettings.PosX = PosX;
        ConsoleSettings.PosY = PosY;
        ConsoleSettings.bAutoPosition = false;
    }
#endif
}

void UConsoleManager::SetConsoleSize(int Width, int Height)
{
#ifdef _WIN32
    if (ConsoleWindow)
    {
        SetWindowPos(ConsoleWindow, nullptr, 0, 0, Width, Height,
                     SWP_NOMOVE | SWP_NOZORDER);
        ConsoleSettings.Width = Width;
        ConsoleSettings.Height = Height;

        // 버퍼 크기도 재조정
        ConfigureConsoleBuffer();
    }
#endif
}

void UConsoleManager::SetConsoleTitleF(const char* Title)
{
#ifdef _WIN32
    if (bConsoleCreated)
    {
        SetConsoleTitleA(Title);
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
    //const char* CategoryPrefix = GetCategoryName(Category);
    snprintf(FormattedMessage, sizeof(FormattedMessage),
             "%s:%d - %s\n",
             FileName, LineNumber, TempBuffer);

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