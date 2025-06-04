#pragma once

#include "CoreMinimal.h"
#include "ChessPiece.h"
#include "BishopPiece.generated.h"

// Forward declarations
class AChessBoard;

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
    virtual TArray<FIntPoint> GetValidMoves(const AChessBoard* Board) const override;

    // Уведомляет пешку о том, что она сделала ход (для первого хода)
    UFUNCTION(BlueprintCallable, Category = "Chess Piece|Pawn")
    void NotifyMoveCompleted();

protected:
    // Флаг, указывающий, делала ли пешка уже ход (для двойного первого хода)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chess Piece|Pawn", meta = (AllowPrivateAccess = "true"))
    bool bHasMoved;
};
