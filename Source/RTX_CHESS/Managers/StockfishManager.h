#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/IHttpRequest.h"
#include "StockfishManager.generated.h"

// Delegate for broadcasting the best move
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBestMoveReceived, const FString&, BestMove);

/**
 * UStockfishManager
 * Manages communication with the Stockfish Online API.
 */
UCLASS(BlueprintType)
class RTX_CHESS_API UStockfishManager : public UObject
{
    GENERATED_BODY()

public:
    UStockfishManager();

    // --- Blueprint Callable Functions ---

    /**
     * Requests the best move(s) from the Stockfish API for a given FEN position.
     * @param FEN The FEN string of the board state. Can be "startpos" for the starting position.
     * @param Depth The search depth for the engine.
     * @param MultiPV The number of best lines to analyze. Higher values allow for move variation.
     */
    UFUNCTION(BlueprintCallable, Category = "Stockfish|Communication", meta = (DisplayName = "Request Best Move (FEN)"))
    void RequestBestMove(const FString& FEN, int32 Depth = 10, int32 MultiPV = 1);

    /**
     * Sends a test request with a known FEN string from the API documentation.
     */
    UFUNCTION(BlueprintCallable, Category = "Stockfish|Testing", meta = (DisplayName = "Test API with Known FEN"))
    void TestRequestWithKnownFEN();
    
    // --- Blueprint Assignable Delegate ---

    UPROPERTY(BlueprintAssignable, Category = "Stockfish|Events")
    FOnBestMoveReceived OnBestMoveReceived;

private:
    // --- Internal Methods ---
    void OnBestMoveResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString OriginalFEN, int32 OriginalDepth);
    void RequestBestMoveFromFallback(const FString& FEN, int32 Depth);
    void OnFallbackBestMoveResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    // --- Opening Book for Variability ---
    TMap<FString, TArray<FString>> OpeningBook;

    // --- Properties ---
    const FString ApiEndpoint = TEXT("https://lichess.org/api/cloud-eval");
    const FString FallbackApiEndpoint = TEXT("https://stockfish.online/api/s/v2.php");
};
