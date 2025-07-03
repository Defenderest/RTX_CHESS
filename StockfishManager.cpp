#include "StockfishManager.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Containers/Queue.h"
#include "Containers/StringConv.h"
#include "HAL/ThreadSafeBool.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

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

    // No process or pipe handles are needed here anymore,
    // as the process is spawned per-command in Run().
};

// --- UStockfishManager Implementation ---
UStockfishManager::UStockfishManager()
{
    StockfishThread = nullptr;
    StockfishTask = nullptr;
    bIsEngineRunningPrivate = false;
    LastBestMovePrivate = TEXT("N/A");
    SearchTimeMsecPrivate = 1000;
    SkillLevelPrivate = 20; // Уровень сложности по умолчанию (0-20)
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

    FString Command = FString::Printf(TEXT("fen %s"), *FEN);
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
        Command = FString::Printf(TEXT("startpos moves %s"), *MovesString);
    }
    else
    {
        Command = TEXT("startpos");
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

void UStockfishManager::SetSkillLevel(int32 NewSkillLevel)
{
    FScopeLock Lock(&DataMutex);
    // Ограничиваем значение допустимым диапазоном для Stockfish (0-20)
    SkillLevelPrivate = FMath::Clamp(NewSkillLevel, 0, 20);
    UE_LOG(LogTemp, Log, TEXT("StockfishManager: Skill level set to %d"), SkillLevelPrivate);
}

int32 UStockfishManager::GetSkillLevel() const
{
    FScopeLock Lock(&DataMutex);
    return SkillLevelPrivate;
}

// --- FStockfishTask Implementation ---
FStockfishTask::FStockfishTask(UStockfishManager* InManager)
    : Manager(InManager), bStopTask(false)
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
    // This thread now waits for commands and executes them one by one by spawning a new Stockfish process.
    while (!bStopTask)
    {
        FString PositionCommand;
        if (CommandQueue.Dequeue(PositionCommand))
        {
            FString StockfishPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Binaries/Win64/stockfish.exe"));
            if (!FPaths::FileExists(StockfishPath))
            {
                UE_LOG(LogTemp, Error, TEXT("FStockfishTask: stockfish.exe not found at %s"), *StockfishPath);
                ResultQueue.Enqueue(TEXT("")); // Enqueue empty result on error
                continue;
            }

            // 1. Prepare file paths
            const FString TempDir = FPaths::ProjectIntermediateDir() / TEXT("StockfishTemp");
            IFileManager::Get().MakeDirectory(*TempDir, true);
            const FString InputFilePath = TempDir / TEXT("stockfish_input.txt");
            const FString OutputFilePath = TempDir / TEXT("stockfish_output.txt");
            const FString BatchFilePath = TempDir / TEXT("run_stockfish.bat");

            // 2. Create input file with commands for Stockfish
            const FString CommandList = FString::Printf(
                TEXT("setoption name Skill Level value %d\nposition %s\ngo movetime %d\n"),
                Manager->GetSkillLevel(),
                *PositionCommand,
                Manager->GetSearchTimeMsec()
            );

            if (!FFileHelper::SaveStringToFile(CommandList, *InputFilePath, FFileHelper::EEncodingOptions::ForceAnsi))
            {
                UE_LOG(LogTemp, Error, TEXT("FStockfishTask: Failed to write to input file: %s"), *InputFilePath);
                ResultQueue.Enqueue(TEXT(""));
                continue;
            }

            // 3. Create a batch file to run Stockfish for robust debugging
            const FString StockfishDir = FPaths::GetPath(StockfishPath);
            const FString BatchFileContent = FString::Printf(
                TEXT("@echo off\r\n")
                TEXT("cd /d \"%s\"\r\n")
                TEXT("echo --- Running Stockfish from batch file --- > \"%s\"\r\n")
                TEXT("\"%s\" < \"%s\" > \"%s\" 2>&1\r\n")
                TEXT("echo --- Batch finished with exit code %%errorlevel%% --- >> \"%s\"\r\n"),
                *StockfishDir,
                *OutputFilePath,
                *StockfishPath, *InputFilePath, *OutputFilePath,
                *OutputFilePath
            );

            if (!FFileHelper::SaveStringToFile(BatchFileContent, *BatchFilePath, FFileHelper::EEncodingOptions::ForceAnsi))
            {
                UE_LOG(LogTemp, Error, TEXT("FStockfishTask: Failed to create batch file: %s"), *BatchFilePath);
                ResultQueue.Enqueue(TEXT(""));
                continue;
            }
            
            UE_LOG(LogTemp, Log, TEXT("FStockfishTask: Executing batch file: %s"), *FPaths::ConvertRelativePathToFull(*BatchFilePath));

            // 4. Launch the batch file
            FProcHandle ProcessHandle = FPlatformProcess::CreateProc(*BatchFilePath, nullptr, false, true, true, nullptr, 0, nullptr, nullptr, nullptr);

            if (!ProcessHandle.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("FStockfishTask: Failed to create process for batch file: %s"), *BatchFilePath);
                ResultQueue.Enqueue(TEXT(""));
                continue;
            }

            // 5. Wait for the process to complete
            FPlatformProcess::WaitForProc(ProcessHandle);
            FPlatformProcess::CloseProc(ProcessHandle);

            // 6. Read the output file
            if (!IFileManager::Get().FileExists(*OutputFilePath))
            {
                UE_LOG(LogTemp, Error, TEXT("FStockfishTask: Batch file did not create the output file: %s. Check batch file execution."), *FPaths::ConvertRelativePathToFull(*OutputFilePath));
                ResultQueue.Enqueue(TEXT(""));
                continue;
            }

            const FString FullPath = FPaths::ConvertRelativePathToFull(*OutputFilePath);
            FString OutputData;
            if (!FFileHelper::LoadFileToString(OutputData, *OutputFilePath))
            {
                UE_LOG(LogTemp, Error, TEXT("FStockfishTask: Failed to read from output file, although it exists: %s"), *FullPath);
                ResultQueue.Enqueue(TEXT(""));
                continue;
            }

            // 7. Parse the result to find the best move
            FString BestMoveResult = TEXT("");
            TArray<FString> Lines;
            OutputData.ParseIntoArrayLines(Lines);
            for (const FString& Line : Lines)
            {
                if (Line.StartsWith(TEXT("bestmove")))
                {
                    TArray<FString> Parts;
                    Line.ParseIntoArray(Parts, TEXT(" "), true);
                    if (Parts.Num() > 1)
                    {
                        BestMoveResult = Parts[1];
                        UE_LOG(LogTemp, Log, TEXT("FStockfishTask: Found bestmove: %s"), *BestMoveResult);
                        break; 
                    }
                }
            }
            
            if (BestMoveResult.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("FStockfishTask: Could not find 'bestmove' in Stockfish output. Full output:\n%s"), *OutputData);
            }

            ResultQueue.Enqueue(BestMoveResult);

            // For debugging, it's useful to keep the temp files.
            // To re-enable cleanup, uncomment the following lines:
            // IFileManager::Get().Delete(*InputFilePath);
            // IFileManager::Get().Delete(*OutputFilePath);
            // IFileManager::Get().Delete(*BatchFilePath);
        }
        else
        {
            // Wait for a bit if there are no commands to process
            FPlatformProcess::Sleep(0.02f);
        }
    }

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
