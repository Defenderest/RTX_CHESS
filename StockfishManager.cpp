#include "StockfishManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/RunnableThread.h"
#include "Misc/Paths.h"
#include "Async/Async.h"
#include "Containers/StringConv.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformProcess.h"

FStockfishReader::FStockfishReader(void* InReadPipe, UStockfishManager* InManager)
    : ReadPipe(InReadPipe)
    , Manager(InManager)
    , bStopTask(false)
{
    UE_LOG(LogTemp, Log, TEXT("FStockfishReader: Created with ReadPipe=%p, Manager=%p"), ReadPipe, Manager);
}

FStockfishReader::~FStockfishReader()
{
    UE_LOG(LogTemp, Log, TEXT("FStockfishReader: Destructor called"));
}

bool FStockfishReader::Init()
{
    UE_LOG(LogTemp, Log, TEXT("FStockfishReader: Thread initialized successfully"));
    return true;
}

uint32 FStockfishReader::Run()
{
    UE_LOG(LogTemp, Log, TEXT("FStockfishReader: Thread started, entering read loop"));

    int32 ReadAttempts = 0;
    int32 EmptyReads = 0;

    while (!bStopTask)
    {
        ReadAttempts++;
        FString Output = FPlatformProcess::ReadPipe(ReadPipe);

        if (!Output.IsEmpty())
        {
            EmptyReads = 0;
            UE_LOG(LogTemp, Log, TEXT("FStockfishReader: [Attempt %d] Read data from pipe: \"%s\""),
                ReadAttempts, *Output.Replace(TEXT("\n"), TEXT("\\n")).Replace(TEXT("\r"), TEXT("\\r")));

            AsyncTask(ENamedThreads::GameThread, [this, Output]()
                {
                    if (Manager && IsValid(Manager))
                    {
                        Manager->HandleStockfishOutput(Output);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("FStockfishReader: Manager is invalid when trying to handle output"));
                    }
                });
        }
        else
        {
            EmptyReads++;
            if (EmptyReads % 1000 == 0)
            {
                UE_LOG(LogTemp, Verbose, TEXT("FStockfishReader: [Attempt %d] Empty read count: %d"), ReadAttempts, EmptyReads);
            }
        }

        FPlatformProcess::Sleep(0.01f);
    }

    UE_LOG(LogTemp, Log, TEXT("FStockfishReader: Thread finished after %d read attempts"), ReadAttempts);
    return 0;
}

void FStockfishReader::Stop()
{
    UE_LOG(LogTemp, Log, TEXT("FStockfishReader: Stop requested, setting bStopTask=true"));
    bStopTask = true;
}

UStockfishManager::UStockfishManager()
    : PipeToStockfish_Write(nullptr)
    , PipeFromStockfish_Read(nullptr)
    , ReaderThread(nullptr)
    , ReaderTask(nullptr)
    , UciState(EUciState::NotConnected)
    , bIsInitialized(false)
{
    ProcessHandle = FProcHandle();
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager: Constructor called - Instance created at %p"), this);
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager: Initial state - UciState=%d, bIsInitialized=%s"),
        (int32)UciState, bIsInitialized ? TEXT("true") : TEXT("false"));
}

void UStockfishManager::BeginDestroy()
{
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager: BeginDestroy called - initiating shutdown"));
    Shutdown();
    Super::BeginDestroy();
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager: BeginDestroy completed"));
}

void UStockfishManager::LaunchStockfish()
{
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: === LAUNCH PROCEDURE STARTED ==="));

    if (ProcessHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("UStockfishManager::LaunchStockfish: Stockfish is already running (ProcessHandle.IsValid()=true)"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Attempting to launch Stockfish..."));

    const FString StockfishPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Binaries/Win64/stockfish.exe")));
    const FString StockfishDir = FPaths::GetPath(StockfishPath);

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Project directory: %s"), *FPaths::ProjectDir());
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Stockfish executable path: %s"), *StockfishPath);
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Working directory: %s"), *StockfishDir);

    if (!FPaths::FileExists(StockfishPath))
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::LaunchStockfish: ERROR - stockfish.exe not found at path: %s"), *StockfishPath);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Stockfish executable found, proceeding with pipe creation"));

    void* StockfishStdinRead = nullptr;
    void* StockfishStdoutWrite = nullptr;

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Creating stdin pipe..."));
    if (!FPlatformProcess::CreatePipe(StockfishStdinRead, PipeToStockfish_Write))
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::LaunchStockfish: FAILED to create stdin pipe"));
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Stdin pipe created successfully - Read=%p, Write=%p"), StockfishStdinRead, PipeToStockfish_Write);

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Creating stdout pipe..."));
    if (!FPlatformProcess::CreatePipe(PipeFromStockfish_Read, StockfishStdoutWrite))
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::LaunchStockfish: FAILED to create stdout pipe"));
        FPlatformProcess::ClosePipe(StockfishStdinRead, PipeToStockfish_Write);
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Stdout pipe created successfully - Read=%p, Write=%p"), PipeFromStockfish_Read, StockfishStdoutWrite);

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: All pipes created successfully, launching process..."));

    uint32 ProcessId = 0;
    ProcessHandle = FPlatformProcess::CreateProc(
        *StockfishPath,
        TEXT(""),
        false,
        true,
        true,
        &ProcessId,
        0,
        *StockfishDir,
        StockfishStdoutWrite,
        StockfishStdinRead,
        nullptr
    );

    if (!ProcessHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::LaunchStockfish: FAILED to launch Stockfish process"));
        FPlatformProcess::ClosePipe(StockfishStdinRead, PipeToStockfish_Write);
        FPlatformProcess::ClosePipe(PipeFromStockfish_Read, StockfishStdoutWrite);
        PipeToStockfish_Write = nullptr;
        PipeFromStockfish_Read = nullptr;
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Process launched successfully - PID: %u, Handle valid: %s"),
        ProcessId, ProcessHandle.IsValid() ? TEXT("true") : TEXT("false"));

    FPlatformProcess::ClosePipe(StockfishStdinRead, nullptr);
    FPlatformProcess::ClosePipe(nullptr, StockfishStdoutWrite);
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Unused pipe ends closed"));

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Final pipe state - ToStockfish_Write=%p, FromStockfish_Read=%p"),
        PipeToStockfish_Write, PipeFromStockfish_Read);

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Creating reader task..."));
    ReaderTask = new FStockfishReader(PipeFromStockfish_Read, this);

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Creating reader thread..."));
    ReaderThread = FRunnableThread::Create(ReaderTask, TEXT("StockfishReaderThread"), 0, TPri_BelowNormal);

    if (ReaderThread)
    {
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Reader thread created successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::LaunchStockfish: FAILED to create reader thread"));
        delete ReaderTask;
        ReaderTask = nullptr;
        Shutdown();
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Waiting for process to initialize..."));
    FPlatformProcess::Sleep(0.2f);

    FProcHandle ProcessHandleCopy = ProcessHandle;
    bool bStillRunning = FPlatformProcess::IsProcRunning(ProcessHandleCopy);
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Process running check: %s"), bStillRunning ? TEXT("true") : TEXT("false"));

    if (!bStillRunning)
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::LaunchStockfish: Process terminated immediately after launch"));
        Shutdown();
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Setting state to UciSent and sending UCI command..."));
    UciState = EUciState::UciSent;

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: About to send 'uci' command"));
    WriteCommandToPipe(TEXT("uci"));

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: === LAUNCH PROCEDURE COMPLETED ==="));
}

void UStockfishManager::Shutdown()
{
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: === SHUTDOWN PROCEDURE STARTED ==="));

    if (!ProcessHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("UStockfishManager::Shutdown: Process handle is not valid, cleaning up resources only"));
        CleanupResources();
        return;
    }

    FProcHandle ProcessHandleCopy = ProcessHandle;
    bool bProcessRunning = FPlatformProcess::IsProcRunning(ProcessHandleCopy);
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Process running status: %s"), bProcessRunning ? TEXT("true") : TEXT("false"));

    if (bProcessRunning)
    {
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Sending 'quit' command to Stockfish..."));
        WriteCommandToPipe(TEXT("quit"));
    }

    if (ReaderTask)
    {
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Stopping reader task..."));
        ReaderTask->Stop();
    }

    if (ReaderThread)
    {
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Waiting for reader thread to complete..."));
        ReaderThread->WaitForCompletion();
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Reader thread completed, deleting thread object"));
        delete ReaderThread;
        ReaderThread = nullptr;
    }

    if (ReaderTask)
    {
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Deleting reader task"));
        delete ReaderTask;
        ReaderTask = nullptr;
    }

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Waiting for process to terminate gracefully..."));
    FPlatformProcess::Sleep(0.3f);

    ProcessHandleCopy = ProcessHandle;
    bProcessRunning = FPlatformProcess::IsProcRunning(ProcessHandleCopy);
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Process running after quit: %s"), bProcessRunning ? TEXT("true") : TEXT("false"));

    if (bProcessRunning)
    {
        UE_LOG(LogTemp, Warning, TEXT("UStockfishManager::Shutdown: Process still running, forcing termination..."));
        FPlatformProcess::TerminateProc(ProcessHandle);

        FPlatformProcess::Sleep(0.1f);
        ProcessHandleCopy = ProcessHandle;
        bProcessRunning = FPlatformProcess::IsProcRunning(ProcessHandleCopy);
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Process running after force termination: %s"), bProcessRunning ? TEXT("true") : TEXT("false"));
    }

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Closing process handle..."));
    FPlatformProcess::CloseProc(ProcessHandle);
    ProcessHandle = FProcHandle();
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Process handle closed and reset"));

    CleanupResources();
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: === SHUTDOWN PROCEDURE COMPLETED ==="));
}

void UStockfishManager::CleanupResources()
{
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::CleanupResources: Starting resource cleanup..."));

    if (PipeFromStockfish_Read)
    {
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::CleanupResources: Closing read pipe..."));
        FPlatformProcess::ClosePipe(PipeFromStockfish_Read, nullptr);
        PipeFromStockfish_Read = nullptr;
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::CleanupResources: Read pipe closed"));
    }

    if (PipeToStockfish_Write)
    {
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::CleanupResources: Closing write pipe..."));
        FPlatformProcess::ClosePipe(nullptr, PipeToStockfish_Write);
        PipeToStockfish_Write = nullptr;
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::CleanupResources: Write pipe closed"));
    }

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::CleanupResources: Resetting state variables..."));
    UciState = EUciState::NotConnected;
    bIsInitialized = false;

    int32 QueuedCommands = CommandQueue.Num();
    CommandQueue.Empty();
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::CleanupResources: Cleared %d queued commands"), QueuedCommands);

    OutputBuffer.Empty();
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::CleanupResources: Output buffer cleared"));

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::CleanupResources: Resource cleanup completed"));
}

void UStockfishManager::WriteCommandToPipe(const FString& Command)
{
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::WriteCommandToPipe: === WRITE COMMAND START ==="));
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::WriteCommandToPipe: Command to write: '%s'"), *Command);

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::WriteCommandToPipe: ProcessHandle.IsValid(): %s"), ProcessHandle.IsValid() ? TEXT("true") : TEXT("false"));
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::WriteCommandToPipe: PipeToStockfish_Write: %p"), PipeToStockfish_Write);

    if (!ProcessHandle.IsValid() || !PipeToStockfish_Write)
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::WriteCommandToPipe: Cannot send command - Process invalid or pipe null"));
        return;
    }

    FProcHandle ProcessHandleCopy = ProcessHandle;
    bool bProcessRunning = FPlatformProcess::IsProcRunning(ProcessHandleCopy);
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::WriteCommandToPipe: Process running check: %s"), bProcessRunning ? TEXT("true") : TEXT("false"));

    FString FullCommand = Command + TEXT("\n");
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::WriteCommandToPipe: Full command with newline: '%s'"), *FullCommand.Replace(TEXT("\n"), TEXT("\\n")));

    FTCHARToUTF8 Converter(*FullCommand);
    bool bWriteSuccess = FPlatformProcess::WritePipe(PipeToStockfish_Write, (const uint8*)Converter.Get(), Converter.Length());

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::WriteCommandToPipe: Write result: %s"), bWriteSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));

    if (!bWriteSuccess)
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::WriteCommandToPipe: WRITE FAILED for command: '%s'"), *Command);

        ProcessHandleCopy = ProcessHandle;
        if (!FPlatformProcess::IsProcRunning(ProcessHandleCopy))
        {
            UE_LOG(LogTemp, Error, TEXT("UStockfishManager::WriteCommandToPipe: Process has terminated unexpectedly"));
            UciState = EUciState::NotConnected;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UStockfishManager::WriteCommandToPipe: Process is still running but pipe write failed"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::WriteCommandToPipe: Command written successfully to pipe"));
    }

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::WriteCommandToPipe: === WRITE COMMAND END ==="));
}

void UStockfishManager::SendCommand(const FString& Command)
{
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::SendCommand: Received command: '%s'"), *Command);
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::SendCommand: Current UCI state: %d"), (int32)UciState);

    if (UciState == EUciState::Ready)
    {
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::SendCommand: UCI is ready, sending command immediately"));
        WriteCommandToPipe(Command);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::SendCommand: UCI not ready, adding to queue (current queue size: %d)"), CommandQueue.Num());
        CommandQueue.Add(Command);
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::SendCommand: Command queued, new queue size: %d"), CommandQueue.Num());
    }
}

void UStockfishManager::ProcessCommandQueue()
{
    if (CommandQueue.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::ProcessCommandQueue: Queue is empty, nothing to process"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::ProcessCommandQueue: Processing %d queued commands"), CommandQueue.Num());

    for (int32 i = 0; i < CommandQueue.Num(); i++)
    {
        const FString& Command = CommandQueue[i];
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::ProcessCommandQueue: Processing command %d/%d: '%s'"), i + 1, CommandQueue.Num(), *Command);
        WriteCommandToPipe(Command);

        FPlatformProcess::Sleep(0.05f);
    }

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::ProcessCommandQueue: All commands processed, clearing queue"));
    CommandQueue.Empty();
}

void UStockfishManager::RequestBestMove(const FString& FEN, int32 SkillLevel, int32 SearchTimeMsec)
{
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::RequestBestMove: === REQUEST BEST MOVE START ==="));
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::RequestBestMove: FEN: '%s'"), *FEN);
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::RequestBestMove: Skill Level: %d"), SkillLevel);
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::RequestBestMove: Search Time: %d ms"), SearchTimeMsec);
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::RequestBestMove: Current UCI state: %d"), (int32)UciState);
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::RequestBestMove: Process valid: %s"), ProcessHandle.IsValid() ? TEXT("true") : TEXT("false"));

    if (!ProcessHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::RequestBestMove: ERROR - Stockfish process is not running"));
        return;
    }

    if (UciState != EUciState::Ready)
    {
        UE_LOG(LogTemp, Warning, TEXT("UStockfishManager::RequestBestMove: WARNING - UCI is not ready (state: %d), commands will be queued"), (int32)UciState);
    }

    const int32 ClampedSkill = FMath::Clamp(SkillLevel, 0, 20);
    const int32 ClampedTime = FMath::Max(100, SearchTimeMsec);

    if (ClampedSkill != SkillLevel)
    {
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::RequestBestMove: Skill level clamped from %d to %d"), SkillLevel, ClampedSkill);
    }

    if (ClampedTime != SearchTimeMsec)
    {
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::RequestBestMove: Search time clamped from %d to %d"), SearchTimeMsec, ClampedTime);
    }

    FString SkillCommand = FString::Printf(TEXT("setoption name Skill Level value %d"), ClampedSkill);
    FString PositionCommand;
    FString GoCommand = FString::Printf(TEXT("go movetime %d"), ClampedTime);

    if (FEN.Equals(TEXT("startpos"), ESearchCase::IgnoreCase))
    {
        PositionCommand = TEXT("position startpos");
    }
    else
    {
        PositionCommand = FString::Printf(TEXT("position fen %s"), *FEN);
    }

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::RequestBestMove: Commands to send:"));
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::RequestBestMove:   1. %s"), *SkillCommand);
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::RequestBestMove:   2. %s"), *PositionCommand);
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::RequestBestMove:   3. %s"), *GoCommand);

    SendCommand(SkillCommand);
    SendCommand(PositionCommand);
    SendCommand(GoCommand);

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::RequestBestMove: === REQUEST BEST MOVE END ==="));
}

void UStockfishManager::HandleStockfishOutput(const FString& OutputChunk)
{
    UE_LOG(LogTemp, Verbose, TEXT("UStockfishManager::HandleStockfishOutput: Received chunk: \"%s\""), *OutputChunk.Replace(TEXT("\n"), TEXT("\\n")).Replace(TEXT("\r"), TEXT("\\r")));

    OutputBuffer.Append(OutputChunk);

    int32 NewLineIndex;
    while (OutputBuffer.FindChar(TCHAR('\n'), NewLineIndex))
    {
        FString Line = OutputBuffer.Left(NewLineIndex);
        OutputBuffer.RemoveAt(0, NewLineIndex + 1, false);

        Line = Line.Replace(TEXT("\r"), TEXT(""));
        Line.TrimStartAndEndInline();

        if (Line.IsEmpty())
        {
            continue;
        }

        UE_LOG(LogTemp, Log, TEXT("Stockfish Parsed Line: %s"), *Line);

        HandleParsedLine(Line);
    }
}

void UStockfishManager::HandleParsedLine(const FString& Line)
{
    if (UciState == EUciState::UciSent)
    {
        if (Line.Equals(TEXT("uciok")))
        {
            UE_LOG(LogTemp, Log, TEXT("UStockfishManager: 'uciok' received. Engine is ready."));
            UciState = EUciState::Ready;
            bIsInitialized = true;

            ProcessCommandQueue();
        }
        else if (Line.StartsWith(TEXT("id")) || Line.StartsWith(TEXT("option")) || Line.StartsWith(TEXT("Stockfish")))
        {
            UE_LOG(LogTemp, Log, TEXT("UStockfishManager: UCI info line: %s"), *Line);
        }
        else
        {
            UE_LOG(LogTemp, Verbose, TEXT("UStockfishManager: Unhandled UCI initialization line: %s"), *Line);
        }
    }
    else if (UciState == EUciState::Ready)
    {
        if (Line.StartsWith(TEXT("bestmove")))
        {
            TArray<FString> Parts;
            Line.ParseIntoArray(Parts, TEXT(" "), true);
            if (Parts.Num() > 1)
            {
                const FString BestMove = Parts[1];
                UE_LOG(LogTemp, Log, TEXT("UStockfishManager::HandleParsedLine: Best move found: %s"), *BestMove);

                if (!BestMove.Equals(TEXT("none"), ESearchCase::IgnoreCase) &&
                    !BestMove.Equals(TEXT("(none)"), ESearchCase::IgnoreCase))
                {
                    OnBestMoveReceived.Broadcast(BestMove);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("UStockfishManager::HandleParsedLine: Stockfish returned no valid move: %s"), *BestMove);
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("UStockfishManager::HandleParsedLine: 'bestmove' line received but could not parse move: %s"), *Line);
            }
        }
        else if (Line.StartsWith(TEXT("info")))
        {
            UE_LOG(LogTemp, Verbose, TEXT("UStockfishManager: Search info: %s"), *Line);
        }
        else
        {
            UE_LOG(LogTemp, Verbose, TEXT("UStockfishManager: Unhandled ready state line: %s"), *Line);
        }
    }
    else
    {
        UE_LOG(LogTemp, Verbose, TEXT("UStockfishManager: Received line in unexpected state %d: %s"), (int32)UciState, *Line);
    }
}

bool UStockfishManager::IsReady() const
{
    return UciState == EUciState::Ready && bIsInitialized;
}

bool UStockfishManager::IsRunning() const
{
    FProcHandle ProcessHandleCopy = ProcessHandle;
    return ProcessHandle.IsValid() && FPlatformProcess::IsProcRunning(ProcessHandleCopy);
}
