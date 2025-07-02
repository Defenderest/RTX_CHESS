#include "StockfishManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "Containers/StringConv.h"

UStockfishManager::UStockfishManager()
{
    ReadPipe = nullptr;
    WritePipe = nullptr;
    bIsEngineRunningPrivate = false;
    LastBestMovePrivate = TEXT("N/A");
    SearchTimeMsecPrivate = 1000; // Default search time
}

UStockfishManager::~UStockfishManager()
{
    StopEngine();
}

void UStockfishManager::StartEngine()
{
    if (bIsEngineRunningPrivate)
    {
        UE_LOG(LogTemp, Warning, TEXT("StockfishManager::StartEngine: Engine is already running."));
        return;
    }

    FString StockfishPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Binaries/Win64/stockfish.exe"));

    if (!FPaths::FileExists(StockfishPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Stockfish executable not found at %s"), *StockfishPath);
        return;
    }

    // Создаем два канала: один для STDIN (ввод), другой для STDOUT (вывод)
    void* ChildStdoutRead = nullptr;
    void* ChildStdoutWrite = nullptr;
    FPlatformProcess::CreatePipe(ChildStdoutRead, ChildStdoutWrite);

    void* ChildStdinRead = nullptr;
    void* ChildStdinWrite = nullptr;
    FPlatformProcess::CreatePipe(ChildStdinRead, ChildStdinWrite);

    // Родительский процесс будет использовать противоположные концы каналов
    ReadPipe = ChildStdoutRead;  // Родитель читает из stdout дочернего процесса
    WritePipe = ChildStdinWrite; // Родитель пишет в stdin дочернего процесса

    // Концы каналов дочернего процесса передаются в CreateProc
    // STDOUT дочернего процесса - это пишущий конец канала stdout (ChildStdoutWrite)
    // STDIN дочернего процесса - это читающий конец канала stdin (ChildStdinRead)
    ProcessHandle = FPlatformProcess::CreateProc(*StockfishPath, nullptr, false, true, true, nullptr, 0, nullptr, ChildStdoutWrite, ChildStdinRead);

    if (!ProcessHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to start Stockfish process."));
        bIsEngineRunningPrivate = false;
        // Закрываем все дескрипторы каналов
        FPlatformProcess::ClosePipe(ChildStdoutRead, ChildStdoutWrite);
        FPlatformProcess::ClosePipe(ChildStdinRead, ChildStdinWrite);
        ReadPipe = nullptr;
        WritePipe = nullptr;
    }
    else
    {
        bIsEngineRunningPrivate = true;
        UE_LOG(LogTemp, Log, TEXT("Stockfish process started successfully."));
        // В родительском процессе мы можем закрыть концы каналов дочернего процесса,
        // так как они больше не используются родителем.
        FPlatformProcess::ClosePipe(0, ChildStdoutWrite);
        FPlatformProcess::ClosePipe(ChildStdinRead, 0);
    }
}

void UStockfishManager::StopEngine()
{
    if (ProcessHandle.IsValid())
    {
        FPlatformProcess::TerminateProc(ProcessHandle);
        FPlatformProcess::CloseProc(ProcessHandle);
        UE_LOG(LogTemp, Log, TEXT("Stockfish process terminated."));
    }
    bIsEngineRunningPrivate = false;

    // Закрываем дескрипторы каналов родительского процесса
    if (WritePipe)
    {
        FPlatformProcess::ClosePipe(nullptr, WritePipe);
        WritePipe = nullptr;
    }
    if (ReadPipe)
    {
        FPlatformProcess::ClosePipe(ReadPipe, nullptr);
        ReadPipe = nullptr;
    }
}

FString UStockfishManager::GetBestMove(const FString& FEN)
{
    if (!bIsEngineRunningPrivate || !ProcessHandle.IsValid() || !WritePipe || !ReadPipe)
    {
        UE_LOG(LogTemp, Error, TEXT("StockfishManager: Cannot get best move, process or pipes are not valid."));
        return TEXT("");
    }

    // Отправляем команду для установки позиции и начала поиска
    FString Command = FString::Printf(TEXT("position fen %s\ngo movetime %d\n"), *FEN, SearchTimeMsecPrivate);
    UE_LOG(LogTemp, Log, TEXT("StockfishManager: Sending command to Stockfish: %s"), *Command.Replace(TEXT("\n"), TEXT(" ")));

    FTCHARToUTF8 Converter(*Command);
    if (!FPlatformProcess::WritePipe(WritePipe, (uint8*)Converter.Get(), Converter.Length()))
    {
        UE_LOG(LogTemp, Error, TEXT("StockfishManager: Failed to write to pipe."));
        return TEXT("");
    }

    // Читаем ответ с тайм-аутом
    FString FullOutput;
    FDateTime StartTime = FDateTime::UtcNow();
    // Добавляем небольшой буфер к тайм-ауту для учета накладных расходов
    const FTimespan Timeout = FTimespan::FromMilliseconds(SearchTimeMsecPrivate + 500);

    while (FDateTime::UtcNow() - StartTime < Timeout)
    {
        FString CurrentOutput = FPlatformProcess::ReadPipe(ReadPipe);
        if (!CurrentOutput.IsEmpty())
        {
            FullOutput += CurrentOutput;
            // 'bestmove' - это последнее, что печатает Stockfish для команды 'go'.
            // Поэтому мы можем прекратить чтение, как только увидим это.
            if (FullOutput.Contains(TEXT("bestmove")))
            {
                break;
            }
        }
        // Небольшая задержка, чтобы не загружать ЦП на 100%
        FPlatformProcess::Sleep(0.01f);
    }

    UE_LOG(LogTemp, Log, TEXT("StockfishManager: Received output:\n%s"), *FullOutput);

    TArray<FString> Lines;
    FullOutput.ParseIntoArray(Lines, TEXT("\n"), true);

    // Ищем с конца, так как 'bestmove' обычно последняя строка
    for (int32 i = Lines.Num() - 1; i >= 0; --i)
    {
        const FString& Line = Lines[i];
        if (Line.StartsWith(TEXT("bestmove")))
        {
            TArray<FString> Parts;
            Line.ParseIntoArray(Parts, TEXT(" "), true);
            if (Parts.Num() > 1)
            {
                LastBestMovePrivate = Parts[1];
                UE_LOG(LogTemp, Log, TEXT("StockfishManager: Parsed best move: %s"), *LastBestMovePrivate);
                return LastBestMovePrivate;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("StockfishManager: Could not find 'bestmove' in Stockfish output. Full output was:\n%s"), *FullOutput);
    LastBestMovePrivate = TEXT("Not Found");
    return TEXT("");
}

bool UStockfishManager::IsEngineRunning() const
{
    return bIsEngineRunningPrivate;
}

FString UStockfishManager::GetLastBestMove() const
{
    return LastBestMovePrivate;
}

int32 UStockfishManager::GetSearchTimeMsec() const
{
    return SearchTimeMsecPrivate;
}
