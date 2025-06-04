#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ChessPiece.h" // Для EPieceColor
#include "ChessGameState.generated.h"

UCLASS()
class RTX_CHESS_API AChessGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AChessGameState();

    // Заготовки для реплицируемых переменных состояния игры
    // UPROPERTY(ReplicatedUsing = OnRep_CurrentTurn)
    // EPieceColor CurrentTurnColor;

    // UFUNCTION()
    // void OnRep_CurrentTurn();

    // UPROPERTY(Replicated)
    // TArray<AChessPiece*> AllPieces; // Потребует аккуратного управления

    // virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
