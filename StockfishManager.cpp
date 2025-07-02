#include "StockfishManager.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Containers/Queue.h"
#include "Containers/StringConv.h"
#include "HAL/ThreadSafeBool.h"

// --- FStockfishTask Declaration ---
class FStockfishTask : public FRunnable
{
public:
    FStockfishTask(UStockfishManager* InManager);
    virtual ~FStockfishTask();

    // FRunnable interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;

    // Interface for UStockfishManager
    void EnqueueCommand(const FString& Command);
    bool DequeueResult(FString& OutResult);

private:
    UStockfishManager* Manager;
    FThreadSafeBool bStopTask;

    TQueue<FString, EQueueMode::Mpsc> CommandQueue;
    TQueue<FString, EQueueMode::Mpsc> ResultQueue;

    FProcHandle ProcessHandle;
    void* ReadPipe;
    void* WritePipe;
};

// --- UStockfishManager Implementation ---
UStockfishManager::UStockfishManager()
{
    StockfishThread = nullptr;
    StockfishTask = nullptr;
    bIsEngineRunningPrivate = false;
    LastBestMovePrivate = TEXT("N/A");
    SearchTimeMsecPrivate = 1000;
}

void UStockfishManager::BeginDestroy()
{
    Super::BeginDestroy();
    StopEngine();
}

void UStockfishManager::StartEngine()
{
    FScopeLock Lock(&DataMutex);
    if (bIsEngineRunningPrivate || StockfishThread)
    {
        UE_LOG(LogTemp, Warning, TEXT("StockfishManager::StartEngine: Engine is already running or starting."));
        return;
    }

    StockfishTask = new FStockfishTask(this);
    StockfishThread = FRunnableThread::Create(StockfishTask, TEXT("StockfishThread"), 0, TPri_BelowNormal);
    if (StockfishThread)
    {
        bIsEngineRunningPrivate = true;
        UE_LOG(LogTemp, Log, TEXT("StockfishManager: Stockfish thread created."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("StockfishManager: Failed to create Stockfish thread."));
        delete StockfishTask;
        StockfishTask = nullptr;
    }
}

void UStockfishManager::StopEngine()
{
    FScopeLock Lock(&DataMutex);
    if (!bIsEngineRunningPrivate && !StockfishThread)
    {
        return;
    }

    if (StockfishTask)
    {
        StockfishTask->Stop();
    }

    if (StockfishThread)
    {
        StockfishThread->WaitForCompletion();
        delete StockfishThread;
        StockfishThread = nullptr;
    }

    if (StockfishTask)
    {
        delete StockfishTask;
        StockfishTask = nullptr;
    }
    
    bIsEngineRunningPrivate = false;
    UE_LOG(LogTemp, Log, TEXT("StockfishManager: Engine stopped."));
}

FString UStockfishManager::GetBestMove(const FString& FEN)
{
    if (!IsEngineRunning() || !StockfishTask)
    {
        UE_LOG(LogTemp, Error, TEXT("StockfishManager::GetBestMove: Engine is not running."));
        return TEXT("");
    }

    FString Command = FString::Printf(TEXT("position fen %s\ngo movetime %d"), *FEN, GetSearchTimeMsec());
    StockfishTask->EnqueueCommand(Command);

    // Blocking wait for the result
    FString Result;
    FDateTime StartTime = FDateTime::UtcNow();
    const FTimespan Timeout = FTimespan::FromMilliseconds(GetSearchTimeMsec() + 2000); // Search time + buffer

    while (FDateTime::UtcNow() - StartTime < Timeout)
    {
        if (StockfishTask->DequeueResult(Result))
        {
            FScopeLock Lock(&DataMutex);
            LastBestMovePrivate = Result;
            return Result;
        }
        FPlatformProcess::Sleep(0.02f);
    }

    UE_LOG(LogTemp, Warning, TEXT("StockfishManager::GetBestMove: Timed out waiting for a response from the engine."));
    return TEXT("");
}

FString UStockfishManager::GetBestMoveForNewGame(const TArray<FString>& Moves)
{
    if (!IsEngineRunning() || !StockfishTask)
    {
        UE_LOG(LogTemp, Error, TEXT("StockfishManager::GetBestMoveForNewGame: Engine is not running."));
        return TEXT("");
    }

    FString MovesString = FString::Join(Moves, TEXT(" "));
    FString Command;
    if (Moves.Num() > 0)
    {
        Command = FString::Printf(TEXT("position startpos moves %s\ngo movetime %d"), *MovesString, GetSearchTimeMsec());
    }
    else
    {
        Command = FString::Printf(TEXT("position startpos\ngo movetime %d"), GetSearchTimeMsec());
    }
    StockfishTask->EnqueueCommand(Command);

    // Blocking wait for the result
    FString Result;
    FDateTime StartTime = FDateTime::UtcNow();
    const FTimespan Timeout = FTimespan::FromMilliseconds(GetSearchTimeMsec() + 2000); // Search time + buffer

    while (FDateTime::UtcNow() - StartTime < Timeout)
    {
        if (StockfishTask->DequeueResult(Result))
        {
            FScopeLock Lock(&DataMutex);
            LastBestMovePrivate = Result;
            return Result;
        }
        FPlatformProcess::Sleep(0.02f);
    }

    UE_LOG(LogTemp, Warning, TEXT("StockfishManager::GetBestMoveForNewGame: Timed out waiting for a response from the engine."));
    return TEXT("");
}

bool UStockfishManager::IsEngineRunning() const
{
    FScopeLock Lock(&DataMutex);
    return bIsEngineRunningPrivate;
}

FString UStockfishManager::GetLastBestMove() const
{
    FScopeLock Lock(&DataMutex);
    return LastBestMovePrivate;
}

int32 UStockfishManager::GetSearchTimeMsec() const
{
    FScopeLock Lock(&DataMutex);
    return SearchTimeMsecPrivate;
}

// --- FStockfishTask Implementation ---
FStockfishTask::FStockfishTask(UStockfishManager* InManager)
    : Manager(InManager), bStopTask(false), ProcessHandle(), ReadPipe(nullptr), WritePipe(nullptr)
{
}

FStockfishTask::~FStockfishTask()
{
}

bool FStockfishTask::Init()
{
    UE_LOG(LogTemp, Log, TEXT("FStockfishTask: Initializing background thread..."));
    return true;
}

uint32 FStockfishTask::Run()
{
    FString StockfishPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Binaries/Win64/stockfish.exe"));
    if (!FPaths::FileExists(StockfishPath))
    {
        UE_LOG(LogTemp, Error, TEXT("FStockfishTask: stockfish.exe not found at %s"), *StockfishPath);
        return 1;
    }

    void* ChildStdoutRead, *ChildStdoutWrite;
    FPlatformProcess::CreatePipe(ChildStdoutRead, ChildStdoutWrite);
    ReadPipe = ChildStdoutRead;

    void* ChildStdinRead, *ChildStdinWrite;
    FPlatformProcess::CreatePipe(ChildStdinRead, ChildStdinWrite);
    WritePipe = ChildStdinWrite;

    FString StockfishDir = FPaths::GetPath(StockfishPath);
    ProcessHandle = FPlatformProcess::CreateProc(*StockfishPath, nullptr, false, true, true, nullptr, 0, *StockfishDir, ChildStdoutWrite, ChildStdinRead, ChildStdoutWrite);

    if (!ProcessHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FStockfishTask: Failed to create Stockfish process."));
        FPlatformProcess::ClosePipe(ReadPipe, ChildStdoutWrite);
        FPlatformProcess::ClosePipe(ChildStdinRead, WritePipe);
        return 1;
    }

    // Check if the process is running immediately after launch
    if (!FPlatformProcess::IsProcRunning(ProcessHandle))
    {
        UE_LOG(LogTemp, Error, TEXT("FStockfishTask: Stockfish process terminated immediately after launch. Check for missing dependencies or incorrect executable path."));
        FPlatformProcess::ClosePipe(ReadPipe, ChildStdoutWrite);
        FPlatformProcess::ClosePipe(ChildStdinRead, WritePipe);
        FPlatformProcess::CloseProc(ProcessHandle);
        return 1;
    }

    // DEBUG: Temporarily disabling pipe handle closing to test a theory that it causes the child process to exit.
    // FPlatformProcess::ClosePipe(nullptr, ChildStdoutWrite);
    // FPlatformProcess::ClosePipe(ChildStdinRead, nullptr);

    auto SendCommandToPipe = [&](const FString& Cmd) {
        if (!WritePipe) return false;
        FString FullCmd = Cmd + TEXT("\n");
        FTCHARToUTF8 Converter(*FullCmd);
        bool bSuccess = FPlatformProcess::WritePipe(WritePipe, (uint8*)Converter.Get(), Converter.Length());
        UE_LOG(LogTemp, Log, TEXT("FStockfishTask: Sent command '%s'. Success: %s"), *Cmd, bSuccess ? TEXT("true") : TEXT("false"));
        return bSuccess;
    };

    auto ReadPipeWithTimeout = [&](const FString& ExpectedResponse, float TimeoutSeconds) -> bool {
        FDateTime StartTime = FDateTime::UtcNow();
        FString Buffer;
        UE_LOG(LogTemp, Log, TEXT("FStockfishTask: Waiting for '%s' with %.1f sec timeout."), *ExpectedResponse, TimeoutSeconds);
        while (FDateTime::UtcNow() - StartTime < FTimespan::FromSeconds(TimeoutSeconds))
        {
            if (bStopTask) return false;

            FString PipeData = FPlatformProcess::ReadPipe(ReadPipe);
            if (!PipeData.IsEmpty())
            {
                UE_LOG(LogTemp, Log, TEXT("FStockfishTask: Read from pipe: %s"), *PipeData);
                Buffer += PipeData;
            }
            
            if (Buffer.Contains(ExpectedResponse))
            {
                UE_LOG(LogTemp, Log, TEXT("FStockfishTask: Received expected response '%s'"), *ExpectedResponse);
                return true;
            }
            FPlatformProcess::Sleep(0.05f);
        }
        
        UE_LOG(LogTemp, Error, TEXT("FStockfishTask: Timed out waiting for '%s'."), *ExpectedResponse);
        if (Buffer.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("FStockfishTask: The read buffer was empty. The process might have crashed or is not producing output."));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("FStockfishTask: Full buffer on timeout:\n%s"), *Buffer);
        }
        return false;
    };

    // UCI Handshake
    if (!SendCommandToPipe(TEXT("uci")) || !ReadPipeWithTimeout(TEXT("uciok"), 5.0f))
    {
        UE_LOG(LogTemp, Error, TEXT("FStockfishTask: UCI handshake step 1 ('uci' -> 'uciok') failed."));
        Stop();
    }

    if (!bStopTask)
    {
        if (!SendCommandToPipe(TEXT("isready")) || !ReadPipeWithTimeout(TEXT("readyok"), 5.0f))
        {
            UE_LOG(LogTemp, Error, TEXT("FStockfishTask: UCI handshake step 2 ('isready' -> 'readyok') failed."));
            Stop();
        }
    }

    if (!bStopTask)
    {
        UE_LOG(LogTemp, Log, TEXT("FStockfishTask: UCI Handshake complete. Entering main loop."));
    }

    // Main loop
    while (!bStopTask)
    {
        if (!FPlatformProcess::IsProcRunning(ProcessHandle))
        {
            UE_LOG(LogTemp, Error, TEXT("FStockfishTask: Stockfish process is no longer running. Exiting thread."));
            break;
        }

        FString CommandToRun;
        if (CommandQueue.Dequeue(CommandToRun))
        {
            SendCommandToPipe(CommandToRun);
        }

        FString Output = FPlatformProcess::ReadPipe(ReadPipe);
        if (!Output.IsEmpty())
        {
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
                        ResultQueue.Enqueue(Parts[1]);
                    }
                }
            }
        }
        FPlatformProcess::Sleep(0.01f);
    }

    // Cleanup before exiting thread
    UE_LOG(LogTemp, Log, TEXT("FStockfishTask: Exiting run loop, cleaning up process and pipes."));
    if (ProcessHandle.IsValid())
    {
        if (FPlatformProcess::IsProcRunning(ProcessHandle))
        {
            SendCommandToPipe(TEXT("quit"));
            FPlatformProcess::Sleep(0.1f);
        }
        FPlatformProcess::TerminateProc(ProcessHandle);
        FPlatformProcess::CloseProc(ProcessHandle);
    }
    FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
    ReadPipe = nullptr;
    WritePipe = nullptr;

    if (Manager)
    {
        FScopeLock Lock(&Manager->DataMutex);
        Manager->bIsEngineRunningPrivate = false;
    }

    return 0;
}

void FStockfishTask::Stop()
{
    bStopTask = true;
}

void FStockfishTask::Exit()
{
    UE_LOG(LogTemp, Log, TEXT("FStockfishTask: Thread is exiting."));
}

void FStockfishTask::EnqueueCommand(const FString& Command)
{
    CommandQueue.Enqueue(Command);
}

bool FStockfishTask::DequeueResult(FString& OutResult)
{
    return ResultQueue.Dequeue(OutResult);
}
