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

private:
    void* ReadPipe;
    void* WritePipe;
    FProcHandle ProcessHandle;
};
