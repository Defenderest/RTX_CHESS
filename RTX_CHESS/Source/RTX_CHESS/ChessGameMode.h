#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ChessGameMode.generated.h"

// Forward declarations (если нужны)
// class AChessPlayerController;
// class AChessGameState;

UCLASS()
class RTX_CHESS_API AChessGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AChessGameMode();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override; // GameModeBase не имеет Tick по умолчанию, но можно добавить

    // Заготовки для функций игровой логики
    // void StartNewGame();
    // void EndTurn();
    // bool IsCheck(EPieceColor PlayerColor);
    // bool IsCheckmate(EPieceColor PlayerColor);

protected:
    // UPROPERTY()
    // TSubclassOf<AChessPlayerController> ChessPlayerControllerClass;

    // UPROPERTY()
    // TSubclassOf<AChessGameState> ChessGameStateClass;
};
