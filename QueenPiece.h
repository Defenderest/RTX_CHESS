#pragma once

#include "CoreMinimal.h"
#include "ChessPiece.h"
#include "QueenPiece.generated.h"

// Forward declarations
class AChessBoard;
class AChessGameState;

UCLASS()
class RTX_CHESS_API AQueenPiece : public AChessPiece
{
    GENERATED_BODY()

public:
    AQueenPiece();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Переопределяет GetValidMoves из AChessPiece.
    // Возвращает массив допустимых ходов для Ферзя с учетом текущего состояния доски.
    // Ферзь движется по горизонтали, вертикали или диагонали на любое количество свободных клеток.
    virtual TArray<FIntPoint> GetValidMoves(const AChessGameState* GameState, const AChessBoard* Board) const override;
};
