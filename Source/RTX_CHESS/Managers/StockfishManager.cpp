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

UStockfishManager::UStockfishManager()
{
	// Initialize a simple opening book to provide move variety in the early game.
	OpeningBook.Add(TEXT("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"), 
		{ TEXT("e2e4"), TEXT("d2d4"), TEXT("c2c4"), TEXT("g1f3") });

	OpeningBook.Add(TEXT("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR"),
		{ TEXT("c7c5"), TEXT("e7e5"), TEXT("e7e6"), TEXT("c7c6"), TEXT("g8f6") });

	OpeningBook.Add(TEXT("rnbqkbnr/pppppppp/8/8/3P4/8/PPPPPPPP/RNBQKBNR"),
		{ TEXT("g8f6"), TEXT("d7d5") });

    // Do not start process in constructor. It causes issues with CDO and Editor previews.
    // InitializeLocalEngine will be called explicitly by GameMode.
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

    // Looking for stockfish.exe in Content/Stockfish/
    FString ExePath = FPaths::Combine(ContentDir, TEXT("Stockfish"), TEXT("stockfish.exe"));
    ExePath = FPaths::ConvertRelativePathToFull(ExePath);

    if (!FPaths::FileExists(ExePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("StockfishManager: Local stockfish.exe not found at %s. Will use Online API."), *ExePath);
        bIsLocalEngineRunning = false;
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("StockfishManager: Found Stockfish at %s. Launching process..."), *ExePath);

    // Create pipe for Input (We Write -> Child Reads)
    if (!FPlatformProcess::CreatePipe(InPipeRead, InPipeWrite))
    {
        UE_LOG(LogTemp, Error, TEXT("StockfishManager: Failed to create Input pipe."));
        return;
    }

    // Create pipe for Output (Child Writes -> We Read)
    if (!FPlatformProcess::CreatePipe(OutPipeRead, OutPipeWrite))
    {
        UE_LOG(LogTemp, Error, TEXT("StockfishManager: Failed to create Output pipe."));
        FPlatformProcess::ClosePipe(InPipeRead, InPipeWrite);
        return;
    }

    // Set Working Directory to the folder containing the exe
    FString WorkingDir = FPaths::GetPath(ExePath);

    EngineProcessHandle = FPlatformProcess::CreateProc(
        *ExePath,
        nullptr, // Params
        false,   // Launch detached (FALSE for pipes to work reliably)
        true,    // Launch hidden
        true,    // Launch really hidden
        nullptr, // Priority
        0,       // Priority mod
        *WorkingDir, // Working dir
        OutPipeWrite, // Child Stdout (Write end of Output pipe)
        InPipeRead    // Child Stdin  (Read end of Input pipe)
    );

    if (EngineProcessHandle.IsValid())
    {
        bIsLocalEngineRunning = true;
        bIsEngineReady = false;

        UE_LOG(LogTemp, Log, TEXT("StockfishManager: Local engine started successfully."));

        // DO NOT CLOSE PIPES HERE.
                    // Use a timer to send 'uci' after a short delay.
                    if (UWorld* World = GetWorld())
                    {
                        FTimerHandle UciTimer;
                        // Increased delay to 0.5s to ensure engine process is fully ready to receive input
                        World->GetTimerManager().SetTimer(UciTimer, [this]()
                        {
                            UE_LOG(LogTemp, Log, TEXT("StockfishManager: Sending 'uci' handshake..."));
                            SendCommandToEngine(TEXT("uci"));
                        }, 0.5f, false);
        
                        // Start polling timer
                        World->GetTimerManager().SetTimer(OutputPollTimer, this, &UStockfishManager::PollEngineOutput, 0.03f, true);
                    }    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("StockfishManager: Failed to launch Stockfish process."));
        FPlatformProcess::ClosePipe(InPipeRead, InPipeWrite);
        FPlatformProcess::ClosePipe(OutPipeRead, OutPipeWrite);
        InPipeRead = InPipeWrite = OutPipeRead = OutPipeWrite = nullptr;
    }
}

void UStockfishManager::StopLocalProcess()
{
    if (bIsLocalEngineRunning)
    {
        SendCommandToEngine(TEXT("quit"));

        if (EngineProcessHandle.IsValid())
        {
            // Give it a moment to close gracefully
            FPlatformProcess::Sleep(0.1f);
            FPlatformProcess::TerminateProc(EngineProcessHandle);
            FPlatformProcess::CloseProc(EngineProcessHandle);
        }

        // Close the handles we kept open
        // Note: InPipeRead and OutPipeWrite were closed in StartLocalProcess

        if (InPipeWrite) FPlatformProcess::ClosePipe(nullptr, InPipeWrite);
        if (OutPipeRead) FPlatformProcess::ClosePipe(OutPipeRead, nullptr);

        InPipeRead = InPipeWrite = OutPipeRead = OutPipeWrite = nullptr;
        bIsLocalEngineRunning = false;

        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(OutputPollTimer);
        }
    }
}

void UStockfishManager::SendCommandToEngine(const FString& Command)
{
    if (bIsLocalEngineRunning && InPipeWrite && !Command.IsEmpty())
    {
        // Use \n only, as some engines/platforms might double up \r with \r\n
        FString CommandWithNewline = Command + TEXT("\n");
        
        auto AnsiString = StringCast<ANSICHAR>(*CommandWithNewline);
        const ANSICHAR* Data = AnsiString.Get();
        int32 BytesToWrite = AnsiString.Length();

        UE_LOG(LogTemp, Log, TEXT("StockfishManager: Sending '%s' (%d bytes)"), *Command, BytesToWrite);

        FString HexStr;
        for (int32 i = 0; i < BytesToWrite; i++)
        {
            HexStr += FString::Printf(TEXT("%02X "), (uint8)Data[i]);
        }
        UE_LOG(LogTemp, Log, TEXT("StockfishManager: Hex Bytes: %s"), *HexStr);

        if (BytesToWrite > 0)
        {
            bool bSuccess = FPlatformProcess::WritePipe(InPipeWrite, (const uint8*)Data, BytesToWrite);
            if (!bSuccess)
            {
                UE_LOG(LogTemp, Error, TEXT("StockfishManager: WritePipe failed!"));
            }
        }
    }
}

void UStockfishManager::PollEngineOutput()
{
    if (bIsLocalEngineRunning && OutPipeRead)
    {
        // Read from the Read end of the Output pipe
        FString Output = FPlatformProcess::ReadPipe(OutPipeRead);
        if (!Output.IsEmpty())
        {
            // Log raw output for debugging
            UE_LOG(LogTemp, Log, TEXT("StockfishManager: Raw Output Chunk: [[%s]]"), *Output);

            // Output can contain multiple lines
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
}

void UStockfishManager::ProcessEngineOutputLine(const FString& Line)
{
    UE_LOG(LogTemp, Log, TEXT("Stockfish Output Line: %s"), *Line);

    if (Line.Contains(TEXT("Unknown command")))
    {
         UE_LOG(LogTemp, Warning, TEXT("StockfishManager: Stockfish reported error: [[%s]]. Check command formatting."), *Line);
    }

    // Handshake Logic
    if (Line.Equals(TEXT("uciok"), ESearchCase::IgnoreCase))
    {
        UE_LOG(LogTemp, Log, TEXT("StockfishManager: Received 'uciok'. Sending 'isready'."));
        SendCommandToEngine(TEXT("isready"));
    }
        else if (Line.Equals(TEXT("readyok"), ESearchCase::IgnoreCase))
        {
            UE_LOG(LogTemp, Log, TEXT("StockfishManager: Received 'readyok'. Engine is ready."));
            bIsEngineReady = true;
    
            // Clear the timeout timer since the engine is now responsive
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().ClearTimer(EngineHandshakeTimeoutTimer);
            }
            
            if (bHasPendingRequest)
            {
                UE_LOG(LogTemp, Log, TEXT("StockfishManager: Processing pending request..."));
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
	if (FEN.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("StockfishManager::RequestBestMove: FEN string is empty."));
		return;
	}

	// --- Opening Move Variability ---
	TArray<FString> FenParts;
	FEN.ParseIntoArray(FenParts, TEXT(" "), true);
	if (FenParts.Num() > 0)
	{
		const FString& BoardState = FenParts[0];
		if (OpeningBook.Contains(BoardState))
		{
			UE_LOG(LogTemp, Log, TEXT("StockfishManager: Position found in opening book. Using for variability."));
			const TArray<FString>& PossibleMoves = OpeningBook.FindChecked(BoardState);
			const FString ChosenMove = PossibleMoves[FMath::RandRange(0, PossibleMoves.Num() - 1)];

			FTimerHandle DummyTimerHandle;
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(DummyTimerHandle, [this, ChosenMove]()
				{
					UE_LOG(LogTemp, Log, TEXT("StockfishManager: Opening move chosen: %s"), *ChosenMove);
					OnBestMoveReceived.Broadcast(ChosenMove);
				}, 0.1f, false);
				return; 
			}
		}
	}
	// --- End Opening Move Variability ---

    if (bIsLocalEngineRunning)
    {
        if (!bIsEngineReady)
        {
            UE_LOG(LogTemp, Warning, TEXT("StockfishManager: Engine not ready yet. Queuing request for FEN: %s"), *FEN);
            bHasPendingRequest = true;
            PendingFEN = FEN;
            PendingDepth = Depth;
            PendingMultiPV = MultiPV;

            // Start a timeout timer. If the engine doesn't become ready in 3 seconds, fall back to API.
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().SetTimer(EngineHandshakeTimeoutTimer, [this, FEN, Depth, MultiPV]()
                {
                    if (bHasPendingRequest)
                    {
                        UE_LOG(LogTemp, Error, TEXT("StockfishManager: Engine handshake timed out (3s)! Falling back to Online API."));
                        bHasPendingRequest = false;
                        // Mark local engine as failed/not running so we don't try it again immediately
                        bIsLocalEngineRunning = false; 
                        StopLocalProcess(); // Clean up the stuck process
                        RequestBestMoveFromAPI(FEN, Depth, MultiPV);
                    }
                }, 3.0f, false);
            }
            return;
        }

        UE_LOG(LogTemp, Log, TEXT("StockfishManager: Calculating local move for FEN: %s (Depth: %d)"), *FEN, Depth);
        
        // Set position
        // If it's the start position, use "position startpos"
        if (FEN.Contains(TEXT("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR")))
        {
             SendCommandToEngine(TEXT("position startpos"));
        }
        else
        {
             SendCommandToEngine(FString::Printf(TEXT("position fen %s"), *FEN));
        }
        
        // Start thinking
        int32 MoveTimeMs = 2000; // 2 seconds by default
        SendCommandToEngine(FString::Printf(TEXT("go depth %d movetime %d"), Depth, MoveTimeMs));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("StockfishManager: Local engine not running. Fallback to API."));
        RequestBestMoveFromAPI(FEN, Depth, MultiPV);
    }
}

void UStockfishManager::RequestBestMoveFromAPI(const FString& FEN, int32 Depth, int32 MultiPV)
{
	// The Lichess API uses GET requests with URL parameters
	const FString Url = FString::Printf(TEXT("%s?fen=%s&multiPv=%d"), 
		*ApiEndpoint, 
		*FGenericPlatformHttp::UrlEncode(FEN), 
		MultiPV);

	UE_LOG(LogTemp, Log, TEXT("StockfishManager: Sending GET request to URL: %s"), *Url);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));

	// Use a lambda to capture the FEN and Depth for potential fallback
	Request->OnProcessRequestComplete().BindLambda([this, FEN, Depth](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess) 
	{
		this->OnBestMoveResponseReceived(Req, Resp, bSuccess, FEN, Depth);
	});

	if (!Request->ProcessRequest())
	{
		UE_LOG(LogTemp, Error, TEXT("StockfishManager::RequestBestMove: Failed to start HTTP request."));
		// If request fails to even start, immediately try fallback
		RequestBestMoveFromFallback(FEN, Depth);
	}
}

void UStockfishManager::TestRequestWithKnownFEN()
{
    const FString TestFEN = TEXT("8/1P1R4/n1r2B2/3Pp3/1k4P1/6K1/Bppr1P2/2q5 w - - 0 1");
    UE_LOG(LogTemp, Log, TEXT("StockfishManager: Sending test request with known FEN: %s"), *TestFEN);
    RequestBestMove(TestFEN, 10, 1);
}

void UStockfishManager::OnBestMoveResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString OriginalFEN, int32 OriginalDepth)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Lichess API request failed or response was invalid. Trying fallback."));
		RequestBestMoveFromFallback(OriginalFEN, OriginalDepth);
		return;
	}

	// Check for 404 Not Found, which means Lichess doesn't have a cloud eval for this position.
	if (Response->GetResponseCode() == 404)
	{
		UE_LOG(LogTemp, Warning, TEXT("Lichess API returned 404 (No cloud evaluation available). Switching to fallback API."));
		RequestBestMoveFromFallback(OriginalFEN, OriginalDepth);
		return;
	}

	if (Response->GetResponseCode() != 200)
	{
		UE_LOG(LogTemp, Error, TEXT("Lichess API request failed with response code %d: %s. Trying fallback."), Response->GetResponseCode(), *Response->GetContentAsString());
		RequestBestMoveFromFallback(OriginalFEN, OriginalDepth);
		return;
	}
    	
	const FString ResponseString = Response->GetContentAsString();
	UE_LOG(LogTemp, Log, TEXT("Lichess API Response: %s"), *ResponseString);

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* PvsArray;
		if (!JsonObject->TryGetArrayField(TEXT("pvs"), PvsArray))
		{
			// Lichess might return an error object
			FString ErrorMessage;
			if (JsonObject->TryGetStringField(TEXT("error"), ErrorMessage))
			{
				UE_LOG(LogTemp, Error, TEXT("Lichess API returned an error: %s. Trying fallback."), *ErrorMessage);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Lichess API response did not contain a 'pvs' array or 'error' field. Response: %s. Trying fallback."), *ResponseString);
			}
			RequestBestMoveFromFallback(OriginalFEN, OriginalDepth);
			return;
		}

		TArray<FString> BestMoves;
		for (const TSharedPtr<FJsonValue>& PvValue : *PvsArray)
		{
			const TSharedPtr<FJsonObject> PvObject = PvValue->AsObject();
			if (PvObject.IsValid())
			{
				FString MovesString;
				if (PvObject->TryGetStringField(TEXT("moves"), MovesString) && !MovesString.IsEmpty())
				{
					// The "moves" string is a space-separated list of UCI moves, e.g., "e2e4 e7e5 g1f3"
					TArray<FString> UciMoves;
					MovesString.ParseIntoArray(UciMoves, TEXT(" "), true);
					if (UciMoves.Num() > 0)
					{
						BestMoves.Add(UciMoves[0]);
					}
				}
			}
		}

		if (BestMoves.Num() > 0)
		{
			// Randomly choose one of the best moves returned by the API
			const FString ChosenMove = BestMoves[FMath::RandRange(0, BestMoves.Num() - 1)];
			UE_LOG(LogTemp, Log, TEXT("Lichess API - %d moves received. Randomly selected move: %s"), BestMoves.Num(), *ChosenMove);
			OnBestMoveReceived.Broadcast(ChosenMove);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to parse any valid moves from Lichess API 'pvs' data. Response: %s. Trying fallback."), *ResponseString);
			RequestBestMoveFromFallback(OriginalFEN, OriginalDepth);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON response from Lichess API. Response: %s. Trying fallback."), *ResponseString);
		RequestBestMoveFromFallback(OriginalFEN, OriginalDepth);
	}
}


void UStockfishManager::RequestBestMoveFromFallback(const FString& FEN, int32 Depth)
{
	// Fallback API uses GET requests with fen and depth. MultiPV is not supported/reliable.
	const FString Url = FString::Printf(TEXT("%s?fen=%s&depth=%d"),
		*FallbackApiEndpoint,
		*FGenericPlatformHttp::UrlEncode(FEN),
		Depth);

	UE_LOG(LogTemp, Log, TEXT("StockfishManager: Sending GET request to FALLBACK URL: %s"), *Url);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->OnProcessRequestComplete().BindUObject(this, &UStockfishManager::OnFallbackBestMoveResponseReceived);

	if (!Request->ProcessRequest())
	{
		UE_LOG(LogTemp, Error, TEXT("StockfishManager::RequestBestMoveFromFallback: Failed to start HTTP request."));
	}
}

void UStockfishManager::OnFallbackBestMoveResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Fallback Stockfish API request failed or response was invalid."));
		return;
	}

	if (Response->GetResponseCode() != 200)
	{
		UE_LOG(LogTemp, Error, TEXT("Fallback Stockfish API request failed with response code %d: %s"), Response->GetResponseCode(), *Response->GetContentAsString());
		return;
	}

	const FString ResponseString = Response->GetContentAsString();
	UE_LOG(LogTemp, Log, TEXT("Fallback Stockfish API Response: %s"), *ResponseString);

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		bool bSuccess = false;
		if (!JsonObject->TryGetBoolField("success", bSuccess) || !bSuccess)
		{
			UE_LOG(LogTemp, Error, TEXT("Fallback Stockfish API returned an error. Response: %s"), *ResponseString);
			return;
		}

		FString BestMoveString;
		if (!JsonObject->TryGetStringField("bestmove", BestMoveString) || BestMoveString.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("Fallback Stockfish API response did not contain a 'bestmove' field or it was empty. Response: %s"), *ResponseString);
			return;
		}

		// The 'bestmove' field contains a string like "bestmove e2e4 ponder e7e5" or just "bestmove e2e4"
		TArray<FString> Tokens;
		BestMoveString.ParseIntoArray(Tokens, TEXT(" "), true);

		if (Tokens.Num() >= 2 && Tokens[0] == TEXT("bestmove"))
		{
			const FString Move = Tokens[1];
			UE_LOG(LogTemp, Log, TEXT("Fallback Stockfish API - Best move parsed: %s"), *Move);
			OnBestMoveReceived.Broadcast(Move);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to parse best move from fallback 'bestmove' string: %s"), *BestMoveString);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON response from Fallback Stockfish API. Response: %s"), *ResponseString);
	}
}