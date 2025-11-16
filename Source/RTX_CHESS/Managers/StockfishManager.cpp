#include "StockfishManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "Dom/JsonObject.h"
#include "GenericPlatform/GenericPlatformHttp.h"

UStockfishManager::UStockfishManager()
{
	// Initialize a simple opening book to provide move variety in the early game.
	// Key: FEN board state (only the piece placement part)
	// Value: Array of possible moves in UCI format

	// Initial position (start of game)
	OpeningBook.Add(TEXT("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"), 
		{ TEXT("e2e4"), TEXT("d2d4"), TEXT("c2c4"), TEXT("g1f3") });

	// After 1. e4
	OpeningBook.Add(TEXT("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR"),
		{ TEXT("c7c5"), TEXT("e7e5"), TEXT("e7e6"), TEXT("c7c6"), TEXT("g8f6") });

	// After 1. d4
	OpeningBook.Add(TEXT("rnbqkbnr/pppppppp/8/8/3P4/8/PPPPPPPP/RNBQKBNR"),
		{ TEXT("g8f6"), TEXT("d7d5") });
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

			// Use a timer to broadcast the result on the next frame to avoid potential re-entrancy issues.
			FTimerHandle DummyTimerHandle;
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(DummyTimerHandle, [this, ChosenMove]()
				{
					UE_LOG(LogTemp, Log, TEXT("StockfishManager: Opening move chosen: %s"), *ChosenMove);
					OnBestMoveReceived.Broadcast(ChosenMove);
				}, 0.1f, false);
				return; // Skip API call for this move
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("StockfishManager: GetWorld() returned null. Cannot use timer for opening move. Falling back to API call."));
			}
		}
	}
	// --- End Opening Move Variability ---

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
