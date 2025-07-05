#include "StockfishManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/RunnableThread.h"
#include "Misc/Paths.h"
#include "Async/Async.h"
#include "Containers/StringConv.h"

// --- FStockfishReader Implementation ---

FStockfishReader::FStockfishReader(void* InReadPipe, UStockfishManager* InManager)
    : ReadPipe(InReadPipe), Manager(InManager), bStopTask(false)
{
    UE_LOG(LogTemp, Log, TEXT("FStockfishReader: Created."));
}

FStockfishReader::~FStockfishReader()
{
    UE_LOG(LogTemp, Log, TEXT("FStockfishReader: Destroyed."));
}

bool FStockfishReader::Init()
{
    UE_LOG(LogTemp, Log, TEXT("FStockfishReader: Thread Initialized."));
    return true;
}

uint32 FStockfishReader::Run()
{
    UE_LOG(LogTemp, Log, TEXT("FStockfishReader: Thread Started. Reading from pipe."));
    // Continuously read from the pipe until we're asked to stop
    while (!bStopTask)
    {
        // Read from the pipe
        FString Output = FPlatformProcess::ReadPipe(ReadPipe);
        if (!Output.IsEmpty())
        {
            // Pass the data to the game thread for processing
            AsyncTask(ENamedThreads::GameThread, [this, Output]()
            {
                if (Manager)
                {
                    Manager->HandleStockfishOutput(Output);
                }
            });
        }
        // Sleep for a short duration to avoid pegging the CPU
        FPlatformProcess::Sleep(0.01f);
    }
    UE_LOG(LogTemp, Log, TEXT("FStockfishReader: Thread Finished."));
    return 0;
}

void FStockfishReader::Stop()
{
    UE_LOG(LogTemp, Log, TEXT("FStockfishReader: Stop requested."));
    bStopTask = true;
}

// --- UStockfishManager Implementation ---

UStockfishManager::UStockfishManager()
{
    // Initialize all handles and pointers to null
    ProcessHandle.Reset();
    ReadPipe = nullptr;
    WritePipe = nullptr;
    ReaderThread = nullptr;
    ReaderTask = nullptr;
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager: Instance created."));
}

void UStockfishManager::BeginDestroy()
{
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager: BeginDestroy called. Shutting down..."));
    Shutdown();
    Super::BeginDestroy();
}

void UStockfishManager::LaunchStockfish()
{
    // Prevent launching if already running
    if (ProcessHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("UStockfishManager::LaunchStockfish: Stockfish is already running."));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Attempting to launch Stockfish..."));

    // 1. Define the path to the executable
    // Note: The user mentioned Content/Binaries, but Binaries/Win64 is the standard location for packaged executables.
    const FString StockfishPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Binaries/Win64/stockfish.exe"));
    
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Checking for executable at: %s"), *StockfishPath);
    if (!FPaths::FileExists(StockfishPath))
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::LaunchStockfish: stockfish.exe not found at path: %s. Please ensure it is present."), *StockfishPath);
        return;
    }

    // 2. Create pipes for communication
    if (!FPlatformProcess::CreatePipe(ReadPipe, WritePipe))
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::LaunchStockfish: Failed to create pipes for Stockfish process."));
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Pipes created successfully."));

    // 3. Launch the process
    // We pass an empty string for parameters. Use CREATE_NO_WINDOW to hide the console.
    uint32 ProcessId = 0;
    ProcessHandle = FPlatformProcess::CreateProc(
        *StockfishPath,
        nullptr, // No parameters
        false,   // bLaunchDetached
        false,   // bLaunchHidden
        true,    // bLaunchReallyHidden -> this should hide the window
        &ProcessId, // OutProcessID
        0,       // PriorityModifier
        nullptr, // OptionalWorkingDirectory
        WritePipe, // PipeWrite
        ReadPipe  // PipeRead
    );

    if (!ProcessHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::LaunchStockfish: Failed to launch Stockfish process."));
        // Clean up the pipes if process creation fails
        FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
        ReadPipe = nullptr;
        WritePipe = nullptr;
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Stockfish process launched successfully. PID: %u"), ProcessId);

    // 4. Create and start the reader thread
    ReaderTask = new FStockfishReader(ReadPipe, this);
    ReaderThread = FRunnableThread::Create(ReaderTask, TEXT("StockfishReaderThread"), 0, TPri_BelowNormal);
    
    if (ReaderThread)
    {
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Reader thread created successfully."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::LaunchStockfish: Failed to create reader thread."));
        Shutdown(); // Attempt to clean up
        return;
    }

    // 5. Initialize UCI protocol
    SendCommand(TEXT("uci"));
}

void UStockfishManager::Shutdown()
{
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Starting shutdown procedure."));

    if (!ProcessHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("UStockfishManager::Shutdown: Shutdown called, but process is not valid."));
        return;
    }

    // 1. Send quit command to Stockfish to ensure a graceful exit
    SendCommand(TEXT("quit"));

    // 2. Stop the reader thread
    if (ReaderThread)
    {
        if (ReaderTask)
        {
            UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Stopping reader task..."));
            ReaderTask->Stop();
        }
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Waiting for reader thread to complete..."));
        ReaderThread->WaitForCompletion();
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Reader thread completed."));
        delete ReaderThread;
        ReaderThread = nullptr;
    }

    if (ReaderTask)
    {
        delete ReaderTask;
        ReaderTask = nullptr;
    }

    // 3. Terminate the process if it's still running
    if (FPlatformProcess::IsProcRunning(ProcessHandle))
    {
        UE_LOG(LogTemp, Warning, TEXT("UStockfishManager::Shutdown: Process is still running after quit command. Forcing termination."));
        FPlatformProcess::TerminateProc(ProcessHandle);
    }
    FPlatformProcess::CloseProc(ProcessHandle);
    ProcessHandle.Reset();
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Process terminated and handle closed."));

    // 4. Close pipes (WritePipe is owned by the process, ReadPipe is ours)
    // Closing ReadPipe here might not be necessary as it's closed by the reader, but it's safe to do so.
    if (ReadPipe)
    {
        FPlatformProcess::ClosePipe(ReadPipe, nullptr);
        ReadPipe = nullptr;
    }
    if (WritePipe)
    {
        FPlatformProcess::ClosePipe(nullptr, WritePipe);
        WritePipe = nullptr;
    }
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Pipes closed. Shutdown complete."));
}

void UStockfishManager::SendCommand(const FString& Command)
{
    if (!ProcessHandle.IsValid() || !WritePipe)
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::SendCommand: Cannot send command, Stockfish is not running or write pipe is invalid. Command: %s"), *Command);
        return;
    }

    // Add a newline character, as Stockfish expects commands to be terminated by it
    FString FullCommand = Command + TEXT("\n");
    
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::SendCommand: Sending command: '%s'"), *Command);
    
    // Convert FString to ANSI for the pipe
    FTCHARToUTF8 Converter(*FullCommand);
    const ANSICHAR* AnsiCommand = (const ANSICHAR*)Converter.Get();
    const int32 CommandLength = Converter.Length();

    // Write to the pipe
    if (!FPlatformProcess::WritePipe(WritePipe, (uint8*)AnsiCommand, CommandLength))
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::SendCommand: Failed to write to pipe. Command: %s"), *Command);
    }
}

void UStockfishManager::RequestBestMove(const FString& FEN, int32 SkillLevel, int32 SearchTimeMsec)
{
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::RequestBestMove: Received request. FEN: %s, Skill: %d, Time: %dms"), *FEN, SkillLevel, SearchTimeMsec);
    
    if (!ProcessHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::RequestBestMove: Stockfish is not running. Cannot process request."));
        return;
    }
    
    // Clamp values to be safe
    const int32 ClampedSkill = FMath::Clamp(SkillLevel, 0, 20);
    const int32 ClampedTime = FMath::Max(100, SearchTimeMsec); // Minimum 100ms

    SendCommand(FString::Printf(TEXT("setoption name Skill Level value %d"), ClampedSkill));

    if (FEN.Equals(TEXT("startpos"), ESearchCase::IgnoreCase))
    {
        SendCommand(TEXT("position startpos"));
    }
    else
    {
        SendCommand(FString::Printf(TEXT("position fen %s"), *FEN));
    }
    
    SendCommand(FString::Printf(TEXT("go movetime %d"), ClampedTime));
}

void UStockfishManager::HandleStockfishOutput(const FString& Output)
{
    UE_LOG(LogTemp, Verbose, TEXT("Stockfish Raw Output: %s"), *Output);

    // Stockfish can send multiple lines in one go. We need to split them.
    TArray<FString> Lines;
    Output.ParseIntoArrayLines(Lines, false); // Keep empty lines to see full output

    for (const FString& Line : Lines)
    {
        if (Line.IsEmpty()) continue;

        UE_LOG(LogTemp, Log, TEXT("Stockfish Parsed Line: %s"), *Line);

        if (Line.StartsWith(TEXT("bestmove")))
        {
            TArray<FString> Parts;
            Line.ParseIntoArray(Parts, TEXT(" "), true);
            if (Parts.Num() > 1)
            {
                const FString BestMove = Parts[1];
                UE_LOG(LogTemp, Log, TEXT("UStockfishManager::HandleStockfishOutput: Best move found: %s"), *BestMove);
                
                // Broadcast the result on the game thread
                OnBestMoveReceived.Broadcast(BestMove);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("UStockfishManager::HandleStockfishOutput: 'bestmove' line received but could not parse move: %s"), *Line);
            }
        }
    }
}
