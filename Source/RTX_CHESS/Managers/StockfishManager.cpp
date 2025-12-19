#include "Managers/StockfishManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "Dom/JsonObject.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Async/Async.h"

// Include for Windows Interactive Process
#include "Misc/InteractiveProcess.h"

#if PLATFORM_ANDROID
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#endif

UStockfishManager::UStockfishManager()
{
	OpeningBook.Add(TEXT("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"), 
		{ TEXT("e2e4"), TEXT("d2d4"), TEXT("c2c4"), TEXT("g1f3") });

	OpeningBook.Add(TEXT("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR"),
		{ TEXT("c7c5"), TEXT("e7e5"), TEXT("e7e6"), TEXT("c7c6"), TEXT("g8f6") });

	OpeningBook.Add(TEXT("rnbqkbnr/pppppppp/8/8/3P4/8/PPPPPPPP/RNBQKBNR"),
		{ TEXT("g8f6"), TEXT("d7d5") });
}

void UStockfishManager::BeginDestroy()
{
    StopLocalProcess();
    Super::BeginDestroy();
}

void UStockfishManager::InitializeLocalEngine()
{
    if (!bIsLocalEngineRunning)
    {
        StartLocalProcess();
    }
}

void UStockfishManager::StartLocalProcess()
{
    FString ContentDir = FPaths::ProjectContentDir();
    FString ExePath;

    // Reset pipe handles
    InPipeRead = nullptr;
    InPipeWrite = nullptr;
    OutPipeRead = nullptr;
    OutPipeWrite = nullptr;

#if PLATFORM_ANDROID
    // --- ANDROID IMPLEMENTATION (Custom Fork/Exec) ---
    UE_LOG(LogTemp, Log, TEXT("StockfishManager: Starting Android process logic..."));

    FString BinaryName = TEXT("stockfish_android");
    FString SourcePath = FPaths::Combine(ContentDir, TEXT("Stockfish"), BinaryName);
    FString DestDir = FPaths::ProjectSavedDir();
    ExePath = FPaths::Combine(DestDir, BinaryName);
    
    // Copy Binary
    bool bFileExists = FPaths::FileExists(ExePath);
    if (!bFileExists || IFileManager::Get().Copy(*ExePath, *SourcePath) != COPY_OK)
    {
         if (!FPaths::FileExists(ExePath))
         {
             UE_LOG(LogTemp, Error, TEXT("StockfishManager: Failed to copy android binary to %s"), *ExePath);
             return; 
         }
    }

    // Permissions
    const char* PathAnsi = TCHAR_TO_UTF8(*ExePath);
    chmod(PathAnsi, 0755);

    // Pipes
    int pipe_in[2];  
    int pipe_out[2]; 

    if (pipe(pipe_in) != 0 || pipe(pipe_out) != 0)
    {
        UE_LOG(LogTemp, Error, TEXT("StockfishManager: Failed to create pipes on Android."));
        return;
    }

    // Fork
    pid_t pid = fork();

    if (pid == -1)
    {
         UE_LOG(LogTemp, Error, TEXT("StockfishManager: fork() failed."));
         close(pipe_in[0]); close(pipe_in[1]);
         close(pipe_out[0]); close(pipe_out[1]);
         return;
    }
    else if (pid == 0)
    {
        // Child
        dup2(pipe_in[0], STDIN_FILENO);
        dup2(pipe_out[1], STDOUT_FILENO);
        
        close(pipe_in[0]); close(pipe_in[1]);
        close(pipe_out[0]); close(pipe_out[1]);

        execl(PathAnsi, PathAnsi, (char*)NULL);
        _exit(127);
    }
    else
    {
        // Parent
        AndroidPID = (int)pid;
        close(pipe_in[0]); 
        close(pipe_out[1]); 

        InPipeWrite = (void*)(intptr_t)pipe_in[1];
        OutPipeRead = (void*)(intptr_t)pipe_out[0];

        int flags = fcntl(pipe_out[0], F_GETFL, 0);
        fcntl(pipe_out[0], F_SETFL, flags | O_NONBLOCK);

        bIsLocalEngineRunning = true;
        bIsEngineReady = false;
        UE_LOG(LogTemp, Log, TEXT("StockfishManager: Android process started. PID: %d"), AndroidPID);
    }

#else
    // --- WINDOWS IMPLEMENTATION (FInteractiveProcess) ---
    UE_LOG(LogTemp, Log, TEXT("StockfishManager: Starting Windows/PC process logic (FInteractiveProcess)..."));

    ExePath = FPaths::Combine(ContentDir, TEXT("Stockfish"), TEXT("stockfish.exe"));
    ExePath = FPaths::ConvertRelativePathToFull(ExePath);

    if (!FPaths::FileExists(ExePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("StockfishManager: Local stockfish.exe not found at %s. Will use Online API."), *ExePath);
        bIsLocalEngineRunning = false;
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("StockfishManager: Found Stockfish at %s. Launching process..."), *ExePath);

    // Create FInteractiveProcess
    // Note: We pass TEXT("") as params to ensure no garbage is passed
    FInteractiveProcess* Proc = new FInteractiveProcess(ExePath, TEXT(""), true);

    if (Proc)
    {
        // Bind Output Delegate
        Proc->OnOutput().BindLambda([this](const FString& Output)
        {
            // FInteractiveProcess runs on a background thread.
            // We must dispatch to Game Thread for logging and UObject interaction.
            AsyncTask(ENamedThreads::GameThread, [this, Output]()
            {
                // Process output line by line (it might come in chunks)
                TArray<FString> Lines;
                Output.ParseIntoArray(Lines, TEXT("\n"), true);
                for (const FString& Line : Lines)
                {
                    FString CleanLine = Line.Replace(TEXT("\r"), TEXT(""));
                    CleanLine.TrimStartAndEndInline();
                    if (!CleanLine.IsEmpty())
                    {
                         ProcessEngineOutputLine(CleanLine);
                    }
                }
            });
        });

        // Launch
        if (Proc->Launch())
        {
            ProcessHandler = Proc;
            bIsLocalEngineRunning = true;
            bIsEngineReady = false;
            UE_LOG(LogTemp, Log, TEXT("StockfishManager: Local engine started successfully via FInteractiveProcess."));
        }
        else
        {
             UE_LOG(LogTemp, Error, TEXT("StockfishManager: Failed to launch FInteractiveProcess."));
             delete Proc;
             ProcessHandler = nullptr;
             bIsLocalEngineRunning = false;
             return;
        }
    }
#endif

    // --- Common Setup ---
    if (bIsLocalEngineRunning)
    {
         if (UWorld* World = GetWorld())
        {
            FTimerHandle UciTimer;
            World->GetTimerManager().SetTimer(UciTimer, [this]()
            {
                UE_LOG(LogTemp, Log, TEXT("StockfishManager: Sending 'uci' handshake..."));
                SendCommandToEngine(TEXT("uci"));
            }, 0.5f, false);

#if PLATFORM_ANDROID
            // Only poll manually on Android
            World->GetTimerManager().SetTimer(OutputPollTimer, this, &UStockfishManager::PollEngineOutput, 0.03f, true);
#endif
        }
    }
}

void UStockfishManager::StopLocalProcess()
{
    if (bIsLocalEngineRunning)
    {
        SendCommandToEngine(TEXT("quit"));
        
        UE_LOG(LogTemp, Log, TEXT("StockfishManager: Stopping local process..."));

#if PLATFORM_ANDROID
        if (AndroidPID != -1)
        {
            FPlatformProcess::Sleep(0.1f);
            kill(AndroidPID, SIGTERM);
            int status;
            waitpid(AndroidPID, &status, WNOHANG); 
            AndroidPID = -1;
        }

        if (InPipeWrite) close((int)(intptr_t)InPipeWrite);
        if (OutPipeRead) close((int)(intptr_t)OutPipeRead);
        InPipeWrite = OutPipeRead = nullptr;
#else
        // Windows Cleanup
        if (ProcessHandler)
        {
            FInteractiveProcess* Proc = (FInteractiveProcess*)ProcessHandler;
            // Destructor calls Cancel(true) which terminates the process
            delete Proc; 
            ProcessHandler = nullptr;
        }
#endif

        bIsLocalEngineRunning = false;

        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(OutputPollTimer);
        }
    }
}

void UStockfishManager::SendCommandToEngine(const FString& Command)
{
    if (!bIsLocalEngineRunning || Command.IsEmpty()) return;

#if PLATFORM_ANDROID
    if (InPipeWrite)
    {
        FString CommandWithNewline = Command + TEXT("\n");
        auto AnsiString = StringCast<ANSICHAR>(*CommandWithNewline);
        const ANSICHAR* Data = AnsiString.Get();
        int32 BytesToWrite = AnsiString.Length();

        UE_LOG(LogTemp, Log, TEXT("StockfishManager (Android): Sending '%s'"), *Command);

        if (BytesToWrite > 0)
        {
            write((int)(intptr_t)InPipeWrite, Data, BytesToWrite);
        }
    }
#else
    if (ProcessHandler)
    {
        FInteractiveProcess* Proc = (FInteractiveProcess*)ProcessHandler;
        UE_LOG(LogTemp, Log, TEXT("StockfishManager (Win): Sending '%s'"), *Command);
        // FInteractiveProcess handles encoding/writing safely
        Proc->SendWhenReady(Command + TEXT("\n"));
    }
#endif
}

void UStockfishManager::PollEngineOutput()
{
    // Only used on Android
#if PLATFORM_ANDROID
    if (bIsLocalEngineRunning && OutPipeRead)
    {
        char buffer[4096];
        ssize_t bytesRead = read((int)(intptr_t)OutPipeRead, buffer, sizeof(buffer) - 1);
        
        if (bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            FString Output = UTF8_TO_TCHAR(buffer);
            
            UE_LOG(LogTemp, Log, TEXT("StockfishManager: Raw Output Chunk: [[%s]]"), *Output);

            TArray<FString> Lines;
            Output.ParseIntoArray(Lines, TEXT("\n"), true);

            for (const FString& Line : Lines)
            {
                FString CleanLine = Line.Replace(TEXT("\r"), TEXT(""));
                CleanLine.TrimStartAndEndInline();
                if (!CleanLine.IsEmpty())
                {
                     ProcessEngineOutputLine(CleanLine);
                }
            }
        }
    }
#endif
}

void UStockfishManager::ProcessEngineOutputLine(const FString& Line)
{
    UE_LOG(LogTemp, Log, TEXT("Stockfish Output Line: %s"), *Line);

    if (Line.Contains(TEXT("Unknown command")))
    {
         UE_LOG(LogTemp, Warning, TEXT("StockfishManager: Stockfish reported error: [[%s]]."), *Line);
    }

    if (Line.Equals(TEXT("uciok"), ESearchCase::IgnoreCase))
    {
        UE_LOG(LogTemp, Log, TEXT("StockfishManager: Received 'uciok'. Sending 'isready'."));
        SendCommandToEngine(TEXT("isready"));
    }
    else if (Line.Equals(TEXT("readyok"), ESearchCase::IgnoreCase))
    {
        UE_LOG(LogTemp, Log, TEXT("StockfishManager: Received 'readyok'. Engine is ready."));
        bIsEngineReady = true;

        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(EngineHandshakeTimeoutTimer);
        }
        
        if (bHasPendingRequest)
        {
            bHasPendingRequest = false;
            RequestBestMove(PendingFEN, PendingDepth, PendingMultiPV);
        }
    }

    if (Line.StartsWith(TEXT("bestmove")))
    {
        TArray<FString> Tokens;
        Line.ParseIntoArray(Tokens, TEXT(" "), true);

        if (Tokens.Num() >= 2)
        {
            FString BestMove = Tokens[1];
            UE_LOG(LogTemp, Log, TEXT("StockfishManager (Local): Best move received: %s"), *BestMove);
            OnBestMoveReceived.Broadcast(BestMove);
        }
    }
}

void UStockfishManager::RequestBestMove(const FString& FEN, int32 Depth, int32 MultiPV)
{
	if (FEN.IsEmpty()) return;

	// Opening Book Logic
	TArray<FString> FenParts;
	FEN.ParseIntoArray(FenParts, TEXT(" "), true);
	if (FenParts.Num() > 0)
	{
		const FString& BoardState = FenParts[0];
		if (OpeningBook.Contains(BoardState))
		{
			const TArray<FString>& PossibleMoves = OpeningBook.FindChecked(BoardState);
			const FString ChosenMove = PossibleMoves[FMath::RandRange(0, PossibleMoves.Num() - 1)];

			if (UWorld* World = GetWorld())
			{
                FTimerHandle DummyTimer;
				World->GetTimerManager().SetTimer(DummyTimer, [this, ChosenMove]()
				{
					OnBestMoveReceived.Broadcast(ChosenMove);
				}, 0.1f, false);
				return; 
			}
		}
	}

    if (bIsLocalEngineRunning)
    {
        if (!bIsEngineReady)
        {
            bHasPendingRequest = true;
            PendingFEN = FEN;
            PendingDepth = Depth;
            PendingMultiPV = MultiPV;

            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().SetTimer(EngineHandshakeTimeoutTimer, [this, FEN, Depth, MultiPV]()
                {
                    if (bHasPendingRequest)
                    {
                        UE_LOG(LogTemp, Error, TEXT("StockfishManager: Engine handshake timed out! Using API."));
                        bHasPendingRequest = false;
                        bIsLocalEngineRunning = false; 
                        StopLocalProcess(); 
                        RequestBestMoveFromAPI(FEN, Depth, MultiPV);
                    }
                }, 3.0f, false);
            }
            return;
        }

        UE_LOG(LogTemp, Log, TEXT("StockfishManager: Local Calc: %s"), *FEN);

        // Set Difficulty/Skill Level
        // Stockfish supports "Skill Level" option from 0 to 20.
        // We use the passed Depth as a proxy for this skill level.
        int32 SkillLevel = FMath::Clamp(Depth, 0, 20);
        SendCommandToEngine(FString::Printf(TEXT("setoption name Skill Level value %d"), SkillLevel));
        
        if (FEN.Contains(TEXT("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR")))
        {
             SendCommandToEngine(TEXT("position startpos"));
        }
        else
        {
             SendCommandToEngine(FString::Printf(TEXT("position fen %s"), *FEN));
        }
        
        SendCommandToEngine(FString::Printf(TEXT("go depth %d movetime 2000"), Depth));
    }
    else
    {
        RequestBestMoveFromAPI(FEN, Depth, MultiPV);
    }
}

void UStockfishManager::RequestBestMoveFromAPI(const FString& FEN, int32 Depth, int32 MultiPV)
{
	const FString Url = FString::Printf(TEXT("%s?fen=%s&multiPv=%d"), 
		*ApiEndpoint, 
		*FGenericPlatformHttp::UrlEncode(FEN), 
		MultiPV);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));

	Request->OnProcessRequestComplete().BindLambda([this, FEN, Depth](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess) 
	{
		this->OnBestMoveResponseReceived(Req, Resp, bSuccess, FEN, Depth);
	});

	if (!Request->ProcessRequest())
	{
		RequestBestMoveFromFallback(FEN, Depth);
	}
}

void UStockfishManager::TestRequestWithKnownFEN()
{
    const FString TestFEN = TEXT("8/1P1R4/n1r2B2/3Pp3/1k4P1/6K1/Bppr1P2/2q5 w - - 0 1");
    RequestBestMove(TestFEN, 10, 1);
}

void UStockfishManager::OnBestMoveResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString OriginalFEN, int32 OriginalDepth)
{
	if (!bWasSuccessful || !Response.IsValid() || Response->GetResponseCode() != 200)
	{
		RequestBestMoveFromFallback(OriginalFEN, OriginalDepth);
		return;
	}
    	
	const FString ResponseString = Response->GetContentAsString();
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* PvsArray;
		if (JsonObject->TryGetArrayField(TEXT("pvs"), PvsArray))
		{
            TArray<FString> BestMoves;
            for (const TSharedPtr<FJsonValue>& PvValue : *PvsArray)
            {
                const TSharedPtr<FJsonObject> PvObject = PvValue->AsObject();
                if (PvObject.IsValid())
                {
                    FString MovesString;
                    if (PvObject->TryGetStringField(TEXT("moves"), MovesString))
                    {
                        TArray<FString> UciMoves;
                        MovesString.ParseIntoArray(UciMoves, TEXT(" "), true);
                        if (UciMoves.Num() > 0) BestMoves.Add(UciMoves[0]);
                    }
                }
            }
            if (BestMoves.Num() > 0)
            {
                OnBestMoveReceived.Broadcast(BestMoves[FMath::RandRange(0, BestMoves.Num() - 1)]);
                return;
            }
		}
	}
    RequestBestMoveFromFallback(OriginalFEN, OriginalDepth);
}


void UStockfishManager::RequestBestMoveFromFallback(const FString& FEN, int32 Depth)
{
	const FString Url = FString::Printf(TEXT("%s?fen=%s&depth=%d"),
		*FallbackApiEndpoint,
		*FGenericPlatformHttp::UrlEncode(FEN),
		Depth);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->OnProcessRequestComplete().BindUObject(this, &UStockfishManager::OnFallbackBestMoveResponseReceived);

	Request->ProcessRequest();
}

void UStockfishManager::OnFallbackBestMoveResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid() || Response->GetResponseCode() != 200) return;

	const FString ResponseString = Response->GetContentAsString();
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		FString BestMoveString;
		if (JsonObject->TryGetStringField("bestmove", BestMoveString))
		{
			TArray<FString> Tokens;
			BestMoveString.ParseIntoArray(Tokens, TEXT(" "), true);
			if (Tokens.Num() >= 2 && Tokens[0] == TEXT("bestmove"))
			{
				OnBestMoveReceived.Broadcast(Tokens[1]);
			}
		}
	}
}