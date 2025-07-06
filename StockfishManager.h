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
     * Requests the best move from the Stockfish API for a given FEN position.
     * @param FEN The FEN string of the board state. Can be "startpos" for the starting position.
     * @param Depth The search depth for the engine (1-15).
     */
    UFUNCTION(BlueprintCallable, Category = "Stockfish|Communication", meta = (DisplayName = "Request Best Move (FEN)"))
    void RequestBestMove(const FString& FEN, int32 Depth = 10);
    
    // --- Blueprint Assignable Delegate ---

    UPROPERTY(BlueprintAssignable, Category = "Stockfish|Events")
    FOnBestMoveReceived OnBestMoveReceived;

private:
    // --- Internal Methods ---
    void OnBestMoveResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    // --- Properties ---
    const FString ApiEndpoint = TEXT("https://chess-api.com/v1");
};
