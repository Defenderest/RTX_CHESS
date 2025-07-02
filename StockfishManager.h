#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformProcess.h" // Для FProcHandle
#include "UObject/NoExportTypes.h"
#include "StockfishManager.generated.h"

/**
 * Manages the Stockfish chess engine process.
 */

UCLASS()
class RTX_CHESS_API UStockfishManager : public UObject
{
	GENERATED_BODY()
	
public:
    UStockfishManager();
    ~UStockfishManager();

    void StartEngine();
    void StopEngine();
    FString GetBestMove(const FString& FEN);

    UFUNCTION(BlueprintPure, Category = "Stockfish")
    bool IsEngineRunning() const;

    UFUNCTION(BlueprintPure, Category = "Stockfish")
    FString GetLastBestMove() const;

    UFUNCTION(BlueprintPure, Category = "Stockfish")
    int32 GetSearchTimeMsec() const;

private:
    void* ReadPipe;
    void* WritePipe;
    FProcHandle ProcessHandle;

    bool bIsEngineRunningPrivate;
    FString LastBestMovePrivate;
    int32 SearchTimeMsecPrivate;
};
