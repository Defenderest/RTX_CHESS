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
 * EUciState
 * Represents the state of the UCI protocol connection with the Stockfish engine.
 */
enum class EUciState : uint8
{
    NotConnected,
    UciSent,
    Ready
};

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
    void WriteCommandToPipe(const FString& Command);
    void ProcessCommandQueue();
    
    void HandleStockfishOutput(const FString& OutputChunk);

    // Buffer for incomplete lines from the Stockfish process
    FString OutputBuffer;

    // UCI protocol state management
    EUciState UciState;
    TArray<FString> CommandQueue;

    // --- Process and Thread Management ---
    
    // Handle to the Stockfish process
    FProcHandle ProcessHandle;

    // Pipes for communication with the process
    void* PipeToStockfish_Write = nullptr;  // We write to this (Stockfish's STDIN)
    void* PipeFromStockfish_Read = nullptr; // We read from this (Stockfish's STDOUT)

    // The other ends of the pipes that the child process uses. We need to store them to close them properly.
    void* PipeToStockfish_Read = nullptr;   // The end Stockfish reads from
    void* PipeFromStockfish_Write = nullptr;// The end Stockfish writes to

    // Thread for reading Stockfish output
    FRunnableThread* ReaderThread = nullptr;
    
    // Task for the reader thread
    FStockfishReader* ReaderTask = nullptr;

    friend class FStockfishReader;
};
