#pragma once

#include "CoreMinimal.h"
#include "ChessPiece.h"
#include "RookPiece.generated.h"

// Forward declarations
class AChessBoard;
class AChessGameState;

UCLASS()
class RTX_CHESS_API ARookPiece : public AChessPiece
{
    GENERATED_BODY()

public:
    ARookPiece();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Переопределяет GetValidMoves из AChessPiece.
    // Возвращает массив допустимых ходов для Ладьи с учетом текущего состояния доски.
    // Ладья движется по горизонтали или вертикали на любое количество свободных клеток.
    virtual TArray<FIntPoint> GetValidMoves(const AChessGameState* GameState, const AChessBoard* Board) const override;

    // bHasMoved теперь наследуется от AChessPiece
    
    // Вызывается после того, как ладья совершила ход, чтобы обновить bHasMoved
    virtual void NotifyMoveCompleted_Implementation() override;
};
