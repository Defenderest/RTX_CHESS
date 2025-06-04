#pragma once

#include "CoreMinimal.h"
#include "ChessPiece.h"
#include "PawnPiece.generated.h"

// Forward declarations
class AChessBoard;

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
    virtual TArray<FIntPoint> GetValidMoves(const AChessBoard* Board) const override;

    // Флаг, указывающий, совершила ли пешка свой первый ход (важно для двойного хода)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pawn Logic")
    bool bHasMoved;

    // Вызывается после того, как пешка совершила ход, чтобы обновить bHasMoved
    void NotifyMoveCompleted();
};
