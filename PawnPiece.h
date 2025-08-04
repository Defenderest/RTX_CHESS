#pragma once

#include "CoreMinimal.h"
#include "ChessPiece.h"
#include "PawnPiece.generated.h"

// Forward declarations
class AChessBoard;
class AChessGameState; // Добавлено для доступа к состоянию игры (например, для en passant)

UCLASS()
class RTX_CHESS_API APawnPiece : public AChessPiece
{
    GENERATED_BODY()

public:
    APawnPiece();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Переопределяет GetValidMoves из AChessPiece.
    // Возвращает массив допустимых ходов для Пешки с учетом текущего состояния доски.
    // Логика включает движение вперед, первый двойной ход, взятие по диагонали и ан пассан (опционально).
    virtual TArray<FIntPoint> GetValidMoves(const AChessGameState* GameState, const AChessBoard* Board) const override;

    // bHasMoved теперь наследуется от AChessPiece
    virtual void NotifyMoveCompleted_Implementation() override;
};
