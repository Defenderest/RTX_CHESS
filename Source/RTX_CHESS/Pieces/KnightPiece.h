#pragma once

#include "CoreMinimal.h"
#include "ChessPiece.h"
#include "KnightPiece.generated.h"

// Forward declarations
class AChessBoard;
class AChessGameState;

UCLASS()
class RTX_CHESS_API AKnightPiece : public AChessPiece
{
    GENERATED_BODY()

public:
    AKnightPiece();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Переопределяет GetValidMoves из AChessPiece.
    // Возвращает массив допустимых ходов для Коня с учетом текущего состояния доски.
    // Конь движется "L"-образно: две клетки в одном направлении (горизонтально или вертикально) и затем одну клетку перпендикулярно.
    virtual TArray<FIntPoint> GetValidMoves(const AChessGameState* GameState, const AChessBoard* Board) const override;
};
