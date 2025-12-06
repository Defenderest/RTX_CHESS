#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/IHttpRequest.h"
#include "StockfishManager.generated.h"

// Delegate for broadcasting the best move
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBestMoveReceived, const FString&, BestMove);

/**
 * UStockfishManager
 * Manages communication with the Stockfish engine (Local .exe or Online API).
 */
UCLASS(BlueprintType)
class RTX_CHESS_API UStockfishManager : public UObject
{
    GENERATED_BODY()

public:
    UStockfishManager();
    virtual void BeginDestroy() override;

    // --- Blueprint Callable Functions ---

    /**
     * Initializes the local engine process.
     */
    UFUNCTION(BlueprintCallable, Category = "Stockfish|Local")
    void InitializeLocalEngine();

    /**
     * Requests the best move(s) from the Stockfish engine (Local or API).
     * @param FEN The FEN string of the board state. Can be "startpos" for the starting position.
     * @param Depth The search depth for the engine.
     * @param MultiPV The number of best lines to analyze.
     */
    UFUNCTION(BlueprintCallable, Category = "Stockfish|Communication", meta = (DisplayName = "Request Best Move (FEN)"))
    void RequestBestMove(const FString& FEN, int32 Depth = 10, int32 MultiPV = 1);

    /**
     * Sends a test request with a known FEN string.
     */
    UFUNCTION(BlueprintCallable, Category = "Stockfish|Testing", meta = (DisplayName = "Test API with Known FEN"))
    void TestRequestWithKnownFEN();
    
    // --- Blueprint Assignable Delegate ---

    UPROPERTY(BlueprintAssignable, Category = "Stockfish|Events")
    FOnBestMoveReceived OnBestMoveReceived;

private:
    // --- Local Engine Logic ---
    FProcHandle EngineProcessHandle;
    
    // Pipe for sending commands TO the engine (We Write, Engine Reads)
    void* InPipeRead = nullptr;  // Engine's Stdin
    void* InPipeWrite = nullptr; // Our write handle

    // Pipe for reading output FROM the engine (Engine Writes, We Read)
    void* OutPipeRead = nullptr; // Our read handle
    void* OutPipeWrite = nullptr;// Engine's Stdout

    bool bIsLocalEngineRunning = false;
    FTimerHandle OutputPollTimer;

    void StartLocalProcess();
    void StopLocalProcess();
    void SendCommandToEngine(const FString& Command);
    void PollEngineOutput();
    void ProcessEngineOutputLine(const FString& Line);

    // --- HTTP/API Logic (Fallback) ---
    void RequestBestMoveFromAPI(const FString& FEN, int32 Depth, int32 MultiPV);
    void OnBestMoveResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString OriginalFEN, int32 OriginalDepth);
    void RequestBestMoveFromFallback(const FString& FEN, int32 Depth);
    void OnFallbackBestMoveResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    // --- Opening Book for Variability ---
    TMap<FString, TArray<FString>> OpeningBook;

    // --- Properties ---
    const FString ApiEndpoint = TEXT("https://lichess.org/api/cloud-eval");
    const FString FallbackApiEndpoint = TEXT("https://stockfish.online/api/s/v2.php");
};
