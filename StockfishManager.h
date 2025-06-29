// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "StockfishManager.generated.h"

/**
 * 
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
