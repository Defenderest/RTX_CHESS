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
    FString StockfishPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Binaries/Win64/stockfish.exe"));

    if (!FPaths::FileExists(StockfishPath))
    {
        UE_LOG(LogTemp, Error, TEXT("Stockfish executable not found at %s"), *StockfishPath);
        return;
    }

    FPlatformProcess::CreatePipe(ReadPipe, WritePipe);
    ProcessHandle = FPlatformProcess::CreateProc(*StockfishPath, nullptr, false, true, true, nullptr, 0, nullptr, WritePipe, ReadPipe);

    if (!ProcessHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to start Stockfish process."));
        bIsEngineRunningPrivate = false;
    }
    else
    {
        bIsEngineRunningPrivate = true;
    }
}

void UStockfishManager::StopEngine()
{
    if (ProcessHandle.IsValid())
    {
        FPlatformProcess::TerminateProc(ProcessHandle);
        FPlatformProcess::CloseProc(ProcessHandle);
    }
    bIsEngineRunningPrivate = false;

    if (ReadPipe || WritePipe)
    {
        FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
        ReadPipe = nullptr;
        WritePipe = nullptr;
    }
}

FString UStockfishManager::GetBestMove(const FString& FEN)
{
    if (!ProcessHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("StockfishManager: Cannot get best move, process is not valid."));
        return TEXT("");
    }

    SearchTimeMsecPrivate = 1000;
    FString Command = FString::Printf(TEXT("position fen %s\ngo movetime %d\n"), *FEN, SearchTimeMsecPrivate);
    UE_LOG(LogTemp, Log, TEXT("StockfishManager: Sending command to Stockfish: %s"), *Command.Replace(TEXT("\n"), TEXT(" ")));

    FTCHARToUTF8 Converter(*Command);
    FPlatformProcess::WritePipe(WritePipe, (uint8*)Converter.Get(), Converter.Length());

    FPlatformProcess::Sleep(1.1f); // Wait for stockfish to think

    FString Output = FPlatformProcess::ReadPipe(ReadPipe);
    UE_LOG(LogTemp, Log, TEXT("StockfishManager: Received output:\n%s"), *Output);
    
    TArray<FString> Lines;
    Output.ParseIntoArray(Lines, TEXT("\n"), true);

    for (const FString& Line : Lines)
    {
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

    UE_LOG(LogTemp, Warning, TEXT("StockfishManager: Could not find 'bestmove' in Stockfish output."));
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
