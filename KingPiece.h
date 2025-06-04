#pragma once

#include "CoreMinimal.h"
#include "ChessPiece.h"
#include "KingPiece.generated.h"

// Forward declarations
class AChessBoard;

UCLASS()
class RTX_CHESS_API AKingPiece : public AChessPiece
{
    GENERATED_BODY()

public:
    AKingPiece();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Переопределяет GetValidMoves из AChessPiece.
    // Возвращает массив допустимых ходов для Короля с учетом текущего состояния доски.
    // Король движется на одну клетку в любом направлении (горизонтально, вертикально или диагонально).
    // Также включает логику для рокировки.
    virtual TArray<FIntPoint> GetValidMoves(const AChessBoard* Board) const override;

    // bHasMoved теперь наследуется от AChessPiece

    // Вызывается после того, как король совершил ход, чтобы обновить bHasMoved
    virtual void NotifyMoveCompleted_Implementation() override;
};
