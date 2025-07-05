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
            UE_LOG(LogTemp, Verbose, TEXT("FStockfishReader: Read chunk from pipe: \"%s\""), *Output.Replace(TEXT("\n"), TEXT("\\n")).Replace(TEXT("\r"), TEXT("\\r")));
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
    PipeToStockfish_Write = nullptr;
    PipeFromStockfish_Read = nullptr;
    PipeToStockfish_Read = nullptr;
    PipeFromStockfish_Write = nullptr;
    ReaderThread = nullptr;
    ReaderTask = nullptr;
    UciState = EUciState::NotConnected;
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

    // 1. Define the path to the executable and its directory
    // Note: The user mentioned Content/Binaries, but Binaries/Win64 is the standard location for packaged executables.
    const FString StockfishPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Binaries/Win64/stockfish.exe")));
    const FString StockfishDir = FPaths::GetPath(StockfishPath);
    
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Using absolute executable path: %s"), *StockfishPath);
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Setting working directory to: %s"), *StockfishDir);
    if (!FPaths::FileExists(StockfishPath))
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::LaunchStockfish: stockfish.exe not found at path: %s. Please ensure it is present."), *StockfishPath);
        return;
    }

    // 2. Create two independent pipes for communication
    // Pipe for UE -> Stockfish (Stockfish's stdin)
    if (!FPlatformProcess::CreatePipe(PipeToStockfish_Read, PipeToStockfish_Write))
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::LaunchStockfish: Failed to create stdin pipe for Stockfish process."));
        return;
    }

    // Pipe for Stockfish -> UE (Stockfish's stdout)
    if (!FPlatformProcess::CreatePipe(PipeFromStockfish_Read, PipeFromStockfish_Write))
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::LaunchStockfish: Failed to create stdout pipe for Stockfish process."));
        FPlatformProcess::ClosePipe(PipeToStockfish_Read, PipeToStockfish_Write); // Clean up the first pipe
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Pipes created successfully."));
    
    // 3. Launch the process via cmd.exe for more robust I/O redirection.
    // This mimics how Explorer launches a process and can solve complex handle inheritance issues.
    uint32 ProcessId = 0;
    const FString CmdExePath = TEXT("C:\\Windows\\System32\\cmd.exe");
    const FString CmdArgs = FString::Printf(TEXT("/c \"%s\""), *StockfishPath);
    
    // FPlatformProcess::CreateProc expects: (..., PipeWrite (stdout), PipeRead (stdin), ...)
    ProcessHandle = FPlatformProcess::CreateProc(
        *CmdExePath,
        *CmdArgs,
        false,   // bLaunchDetached
        true,    // bLaunchHidden
        true,    // bLaunchReallyHidden
        &ProcessId, // OutProcessID
        0,       // PriorityModifier
        *StockfishDir, // OptionalWorkingDirectory
        PipeFromStockfish_Write, // Child process's STDOUT is the WRITE end of our "from stockfish" pipe
        PipeToStockfish_Read     // Child process's STDIN is the READ end of our "to stockfish" pipe
    );

    if (!ProcessHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::LaunchStockfish: Failed to launch process container (cmd.exe)."));
        // Clean up all pipe handles if process creation fails
        FPlatformProcess::ClosePipe(PipeToStockfish_Read, PipeToStockfish_Write);
        FPlatformProcess::ClosePipe(PipeFromStockfish_Read, PipeFromStockfish_Write);
        PipeToStockfish_Read = PipeToStockfish_Write = PipeFromStockfish_Read = PipeFromStockfish_Write = nullptr;
        return;
    }
    
    // The parent process should now close the pipe ends that are used by the child process.
    // This is crucial for ensuring proper communication and preventing resource leaks.
    FPlatformProcess::ClosePipe(PipeToStockfish_Read, nullptr);
    FPlatformProcess::ClosePipe(nullptr, PipeFromStockfish_Write);
    // Null out the pointers to prevent them from being used or closed again in Shutdown().
    PipeToStockfish_Read = nullptr;
    PipeFromStockfish_Write = nullptr;

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::LaunchStockfish: Stockfish process container (cmd.exe) launched successfully. PID: %u"), ProcessId);

    // 4. Create and start the reader thread to read from Stockfish's stdout
    ReaderTask = new FStockfishReader(PipeFromStockfish_Read, this);
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
    UciState = EUciState::UciSent;
    WriteCommandToPipe(TEXT("uci"));
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
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Sending 'quit' command."));
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

    // 4. Close all pipe handles to avoid resource leaks
    if (PipeFromStockfish_Read || PipeFromStockfish_Write)
    {
        FPlatformProcess::ClosePipe(PipeFromStockfish_Read, PipeFromStockfish_Write);
        PipeFromStockfish_Read = nullptr;
        PipeFromStockfish_Write = nullptr;
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Closed stdout pipe."));
    }
    if (PipeToStockfish_Read || PipeToStockfish_Write)
    {
        FPlatformProcess::ClosePipe(PipeToStockfish_Read, PipeToStockfish_Write);
        PipeToStockfish_Read = nullptr;
        PipeToStockfish_Write = nullptr;
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: Closed stdin pipe."));
    }
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::Shutdown: All pipes closed. Shutdown complete."));
}

void UStockfishManager::WriteCommandToPipe(const FString& Command)
{
    if (!ProcessHandle.IsValid() || !PipeToStockfish_Write)
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::WriteCommandToPipe: Cannot send command, Stockfish is not running or write pipe is invalid. Command: %s"), *Command);
        return;
    }

    // Add a newline character, as Stockfish expects commands to be terminated by it
    FString FullCommand = Command + TEXT("\n");
    
    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::WriteCommandToPipe: Writing to pipe: '%s'"), *Command);
    
    // Convert FString to ANSI for the pipe
    FTCHARToUTF8 Converter(*FullCommand);
    const ANSICHAR* AnsiCommand = (const ANSICHAR*)Converter.Get();
    const int32 CommandLength = Converter.Length();

    // Write to the pipe
    if (!FPlatformProcess::WritePipe(PipeToStockfish_Write, (uint8*)AnsiCommand, CommandLength))
    {
        UE_LOG(LogTemp, Error, TEXT("UStockfishManager::WriteCommandToPipe: Failed to write to pipe. Command: %s"), *Command);
    }
}

void UStockfishManager::SendCommand(const FString& Command)
{
    if (UciState == EUciState::Ready)
    {
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager: UCI ready, sending command immediately: '%s'"), *Command);
        WriteCommandToPipe(Command);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("UStockfishManager: UCI not ready, queueing command: '%s'"), *Command);
        CommandQueue.Add(Command);
    }
}

void UStockfishManager::ProcessCommandQueue()
{
    if (CommandQueue.IsEmpty())
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager: Processing %d queued commands."), CommandQueue.Num());
    for (const FString& Command : CommandQueue)
    {
        WriteCommandToPipe(Command);
    }
    CommandQueue.Empty();
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

    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::RequestBestMove: Using clamped Skill: %d, Time: %dms"), ClampedSkill, ClampedTime);

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

void UStockfishManager::HandleStockfishOutput(const FString& OutputChunk)
{
    UE_LOG(LogTemp, Verbose, TEXT("UStockfishManager::HandleStockfishOutput: Received chunk: \"%s\""), *OutputChunk.Replace(TEXT("\n"), TEXT("\\n")).Replace(TEXT("\r"), TEXT("\\r")));
    // Append new data to the buffer
    OutputBuffer.Append(OutputChunk);

    // Process all complete lines (ending with \n) in the buffer
    int32 NewLineIndex;
    while (OutputBuffer.FindChar(TCHAR('\n'), NewLineIndex))
    {
        // Extract the line (without the newline character)
        FString Line = OutputBuffer.Left(NewLineIndex);
        // Remove the processed line and the newline character from the buffer
        OutputBuffer.RemoveAt(0, NewLineIndex + 1, false);

        // Trim any whitespace (like \r that might be present) from the line itself
        Line.TrimStartAndEndInline();

        if (Line.IsEmpty())
        {
            continue;
        }

        UE_LOG(LogTemp, Log, TEXT("Stockfish Parsed Line: %s"), *Line);

        // State machine for UCI protocol
        if (UciState == EUciState::UciSent)
        {
            if (Line.Equals(TEXT("uciok")))
            {
                UE_LOG(LogTemp, Log, TEXT("UStockfishManager: 'uciok' received. Engine is ready."));
                UciState = EUciState::Ready;
                ProcessCommandQueue();
            }
            // Ignore other info lines while waiting for uciok
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
                    UE_LOG(LogTemp, Log, TEXT("UStockfishManager::HandleStockfishOutput: Best move found: %s"), *BestMove);
                    
                    // Broadcast the result on the game thread
                    OnBestMoveReceived.Broadcast(BestMove);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("UStockfishManager::HandleStockfishOutput: 'bestmove' line received but could not parse move: %s"), *Line);
                }
            }
            // Here you could also parse "info" lines if needed for analysis
        }
    }
}
