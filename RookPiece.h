#pragma once

#include "CoreMinimal.h"
#include "ChessPiece.h"
#include "RookPiece.generated.h"

// Forward declarations
class AChessBoard;

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
    virtual TArray<FIntPoint> GetValidMoves(const AChessBoard* Board) const override;

    // Флаг, указывающий, совершила ли ладья свой первый ход (важно для рокировки)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rook Logic")
    bool bHasMoved;
    
    // Вызывается после того, как ладья совершила ход, чтобы обновить bHasMoved
    void NotifyMoveCompleted();
};
