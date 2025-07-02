#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "StockfishManager.generated.h"

class FRunnableThread;
class FStockfishTask;

UCLASS()
class RTX_CHESS_API UStockfishManager : public UObject
{
    GENERATED_BODY()

public:
    UStockfishManager();
    virtual void BeginDestroy() override;

    void StartEngine();
    void StopEngine();
    FString GetBestMove(const FString& FEN);

    UFUNCTION(BlueprintCallable, Category = "Stockfish")
    FString GetBestMoveForNewGame(const TArray<FString>& Moves);

    UFUNCTION(BlueprintPure, Category = "Stockfish")
    bool IsEngineRunning() const;

    UFUNCTION(BlueprintPure, Category = "Stockfish")
    FString GetLastBestMove() const;

    UFUNCTION(BlueprintPure, Category = "Stockfish")
    int32 GetSearchTimeMsec() const;

    void SetSkillLevel(int32 NewSkillLevel);
    int32 GetSkillLevel() const;

private:
    // The background thread that will run Stockfish
    FRunnableThread* StockfishThread;

    // The task object that contains the logic for the thread
    FStockfishTask* StockfishTask;

    // Thread-safe members
    mutable FCriticalSection DataMutex;
    FString LastBestMovePrivate;
    bool bIsEngineRunningPrivate;
    int32 SearchTimeMsecPrivate;
    int32 SkillLevelPrivate;

    friend class FStockfishTask;
};
