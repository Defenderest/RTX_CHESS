#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/PlatformProcess.h"
#include "UObject/NoExportTypes.h"
#include "StockfishManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBestMoveReceived, const FString&, BestMove);

UENUM(BlueprintType)
enum class EUciState : uint8
{
    NotConnected,
    UciSent,
    Ready
};

class UStockfishManager;

class RTX_CHESS_API FStockfishReader : public FRunnable
{
public:
    FStockfishReader(void* InReadPipe, UStockfishManager* InManager);
    virtual ~FStockfishReader();

    // FRunnable interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;

private:
    void* ReadPipe;
    UStockfishManager* Manager;
    bool bStopTask;
};

UCLASS(BlueprintType, Blueprintable)
class RTX_CHESS_API UStockfishManager : public UObject
{
    GENERATED_BODY()

public:
    UStockfishManager();

    // UObject interface
    virtual void BeginDestroy() override;

    UFUNCTION(BlueprintCallable, Category = "Stockfish")
    void LaunchStockfish();

    UFUNCTION(BlueprintCallable, Category = "Stockfish")
    void Shutdown();

    UFUNCTION(BlueprintCallable, Category = "Stockfish")
    void RequestBestMove(const FString& FEN, int32 SkillLevel = 10, int32 SearchTimeMsec = 1000);

    UFUNCTION(BlueprintCallable, Category = "Stockfish")
    void SendCommand(const FString& Command);

    UFUNCTION(BlueprintPure, Category = "Stockfish")
    bool IsReady() const;

    UFUNCTION(BlueprintPure, Category = "Stockfish")
    bool IsRunning() const;

    UPROPERTY(BlueprintAssignable, Category = "Stockfish")
    FOnBestMoveReceived OnBestMoveReceived;

    // Функция для обработки вывода Stockfish (вызывается из потока чтения)
    void HandleStockfishOutput(const FString& OutputChunk);

private:
    void WriteCommandToPipe(const FString& Command);
    void ProcessCommandQueue();
    void HandleParsedLine(const FString& Line);
    void CleanupResources();

    // Процесс Stockfish
    FProcHandle ProcessHandle;
    void* PipeToStockfish_Write;
    void* PipeFromStockfish_Read;

    // Поток для чтения вывода
    FRunnableThread* ReaderThread;
    FStockfishReader* ReaderTask;

    // Состояние UCI
    EUciState UciState;
    bool bIsInitialized;

    // Буферы и очереди
    TArray<FString> CommandQueue;
    FString OutputBuffer;
};