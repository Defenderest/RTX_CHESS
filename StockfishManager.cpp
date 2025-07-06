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

void UStockfishManager::RequestBestMove(const FString& FEN, int32 Depth)
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

	// Create JSON request body
	TSharedPtr<FJsonObject> RequestJson = MakeShareable(new FJsonObject);
	RequestJson->SetStringField(TEXT("fen"), FEN);
	RequestJson->SetNumberField(TEXT("depth"), Depth);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestJson.ToSharedRef(), Writer);

	UE_LOG(LogTemp, Log, TEXT("StockfishManager: Sending POST request to URL: %s with body: %s"), *ApiEndpoint, *RequestBody);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(ApiEndpoint);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(RequestBody);
	Request->OnProcessRequestComplete().BindUObject(this, &UStockfishManager::OnBestMoveResponseReceived);

	if (!Request->ProcessRequest())
	{
		UE_LOG(LogTemp, Error, TEXT("StockfishManager::RequestBestMove: Failed to start HTTP request."));
	}
}

void UStockfishManager::TestRequestWithKnownFEN()
{
    const FString TestFEN = TEXT("8/1P1R4/n1r2B2/3Pp3/1k4P1/6K1/Bppr1P2/2q5 w - - 0 1");
    UE_LOG(LogTemp, Log, TEXT("StockfishManager: Sending test request with known FEN: %s"), *TestFEN);
    RequestBestMove(TestFEN, 10); // Depth is ignored by the current implementation, but we provide a default.
}

void UStockfishManager::OnBestMoveResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Chess-API request failed or response was invalid."));
		return;
	}

	if (Response->GetResponseCode() != 200)
	{
		UE_LOG(LogTemp, Error, TEXT("Chess-API request failed with response code %d: %s"), Response->GetResponseCode(), *Response->GetContentAsString());
		return;
	}
    	
	const FString ResponseString = Response->GetContentAsString();
	UE_LOG(LogTemp, Log, TEXT("Chess-API Response: %s"), *ResponseString);

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		// The new API response contains the move directly in the "move" field.
		FString BestMoveString;
		if (JsonObject->TryGetStringField(TEXT("move"), BestMoveString) && !BestMoveString.IsEmpty())
		{
			UE_LOG(LogTemp, Log, TEXT("Chess-API - Best move parsed: %s"), *BestMoveString);
			OnBestMoveReceived.Broadcast(BestMoveString);
		}
		else
		{
			// Check for an error message if the "move" field is missing.
			FString ErrorMessage;
			if (JsonObject->TryGetStringField(TEXT("text"), ErrorMessage))
			{
				 UE_LOG(LogTemp, Error, TEXT("Chess-API error: %s"), *ErrorMessage);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Chess-API response did not contain a 'move' field or it was empty."));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON response from Chess-API. Response: %s"), *ResponseString);
	}
}
