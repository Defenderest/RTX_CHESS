#pragma once

#include "CoreMinimal.h"
#include "ChessPiece.h"
#include "BishopPiece.generated.h"

// Forward declarations
class AChessBoard;
class AChessGameState;

UCLASS()
class RTX_CHESS_API ABishopPiece : public AChessPiece
{
    GENERATED_BODY()

public:
    ABishopPiece();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Переопределяет GetValidMoves из AChessPiece.
    // Возвращает массив допустимых ходов для Слона с учетом текущего состояния доски.
    // Слон движется по диагонали на любое количество свободных клеток.
    virtual TArray<FIntPoint> GetValidMoves(const AChessGameState* GameState, const AChessBoard* Board) const override;
};
