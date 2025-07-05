#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HAL/Runnable.h"
#include "Async/Async.h"
#include "StockfishManager.generated.h"

// Forward declarations
class FRunnableThread;
class UStockfishManager;

// Delegate for broadcasting the best move
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBestMoveReceived, const FString&, BestMove);

/**
 * FStockfishReader
 * Reads output from the Stockfish process in a separate thread.
 */
class FStockfishReader : public FRunnable
{
public:
    FStockfishReader(void* InReadPipe, UStockfishManager* InManager);
    virtual ~FStockfishReader();

    // FRunnable interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;

private:
    // Pipe to read from Stockfish process
    void* ReadPipe;
    
    // Owning manager to call back to
    UStockfishManager* Manager;

    // Flag to signal the thread to stop
    FThreadSafeBool bStopTask;
};

/**
 * UStockfishManager
 * Manages the Stockfish engine process and communication.
 */
UCLASS(BlueprintType)
class RTX_CHESS_API UStockfishManager : public UObject
{
    GENERATED_BODY()

public:
    UStockfishManager();
    virtual void BeginDestroy() override;

    // --- Blueprint Callable Functions ---

    UFUNCTION(BlueprintCallable, Category = "Stockfish|Control")
    void LaunchStockfish();

    UFUNCTION(BlueprintCallable, Category = "Stockfish|Control")
    void Shutdown();
    
    UFUNCTION(BlueprintCallable, Category = "Stockfish|Communication")
    void SendCommand(const FString& Command);

    UFUNCTION(BlueprintCallable, Category = "Stockfish|Communication", meta = (DisplayName = "Request Best Move (FEN)"))
    void RequestBestMove(const FString& FEN, int32 SkillLevel = 20, int32 SearchTimeMsec = 1000);
    
    // --- Blueprint Assignable Delegate ---

    UPROPERTY(BlueprintAssignable, Category = "Stockfish|Events")
    FOnBestMoveReceived OnBestMoveReceived;

private:
    // --- Internal Methods ---
    
    void HandleStockfishOutput(const FString& Output);

    // --- Process and Thread Management ---
    
    // Handle to the Stockfish process
    FProcHandle ProcessHandle;

    // Pipes for communication with the process
    void* ReadPipe = nullptr;
    void* WritePipe = nullptr;

    // Thread for reading Stockfish output
    FRunnableThread* ReaderThread = nullptr;
    
    // Task for the reader thread
    FStockfishReader* ReaderTask = nullptr;

    friend class FStockfishReader;
};
