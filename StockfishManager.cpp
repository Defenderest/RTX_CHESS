#include "StockfishManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "Dom/JsonObject.h"
#include "GenericPlatform/GenericPlatformHttp.h"

UStockfishManager::UStockfishManager()
{
	// Nothing to do in the constructor for the API-based approach
}

void UStockfishManager::RequestBestMove(const FString& FEN, int32 Depth)
{
	if (FEN.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("StockfishManager::RequestBestMove: FEN string is empty."));
		return;
	}

	// The API documentation states depth should be < 16. We'll clamp it to be safe.
	const int32 ClampedDepth = FMath::Clamp(Depth, 1, 15);
	
	FString URL = FString::Printf(TEXT("%s?fen=%s&depth=%d"), *ApiEndpoint, *FGenericPlatformHttp::UrlEncode(FEN), ClampedDepth);
	
	UE_LOG(LogTemp, Log, TEXT("StockfishManager: Sending request to URL: %s"), *URL);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(URL);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->OnProcessRequestComplete().BindUObject(this, &UStockfishManager::OnBestMoveResponseReceived);
	
	if (!Request->ProcessRequest())
	{
		UE_LOG(LogTemp, Error, TEXT("StockfishManager::RequestBestMove: Failed to start HTTP request."));
	}
}

void UStockfishManager::OnBestMoveResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Stockfish API request failed or response was invalid."));
		return;
	}

	if (Response->GetResponseCode() != 200)
	{
		UE_LOG(LogTemp, Error, TEXT("Stockfish API request failed with response code %d: %s"), Response->GetResponseCode(), *Response->GetContentAsString());
		return;
	}
	
	const FString ResponseString = Response->GetContentAsString();
	UE_LOG(LogTemp, Log, TEXT("Stockfish API Response: %s"), *ResponseString);

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		bool bSuccess = false;
		if (JsonObject->TryGetBoolField(TEXT("success"), bSuccess) && bSuccess)
		{
			FString BestMoveString;
			if (JsonObject->TryGetStringField(TEXT("bestmove"), BestMoveString))
			{
				// The API returns "bestmove e2e4 ponder e7e5", we only need "e2e4"
				TArray<FString> Parts;
				BestMoveString.ParseIntoArray(Parts, TEXT(" "), true);

				if (Parts.Num() > 1 && Parts[0].Equals(TEXT("bestmove"), ESearchCase::IgnoreCase))
				{
					const FString Move = Parts[1];
					UE_LOG(LogTemp, Log, TEXT("Stockfish API - Best move parsed: %s"), *Move);
					OnBestMoveReceived.Broadcast(Move);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Could not parse best move from API response field 'bestmove': %s"), *BestMoveString);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Stockfish API response was successful, but 'bestmove' field was not found."));
			}
		}
		else
		{
			FString ErrorMessage;
			if (JsonObject->TryGetStringField(TEXT("error"), ErrorMessage))
			{
				UE_LOG(LogTemp, Error, TEXT("Stockfish API error: %s"), *ErrorMessage);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Stockfish API request was not successful, but no error message was provided."));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON response from Stockfish API. Response: %s"), *ResponseString);
	}
}
