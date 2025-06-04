#pragma once

#include "CoreMinimal.h"
#include "ChessPiece.h"
#include "PawnPiece.generated.h" // Исправлено на PawnPiece.generated.h

// Forward declarations
class AChessBoard;

UCLASS()
class RTX_CHESS_API APawnPiece : public AChessPiece // Исправлено на APawnPiece
{
    GENERATED_BODY()

public:
    APawnPiece(); // Исправлено на APawnPiece

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Переопределяет GetValidMoves из AChessPiece.
    // Возвращает массив допустимых ходов для Пешки с учетом текущего состояния доски.
    virtual TArray<FIntPoint> GetValidMoves(const AChessBoard* Board) const override;

    // Уведомляет пешку о том, что она сделала ход (для первого хода)
    UFUNCTION(BlueprintCallable, Category = "Chess Piece|Pawn")
    void NotifyMoveCompleted();

protected:
    // Флаг, указывающий, делала ли пешка уже ход (для двойного первого хода)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chess Piece|Pawn", meta = (AllowPrivateAccess = "true"))
    bool bHasMoved;
};
