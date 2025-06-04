#pragma once

#include "CoreMinimal.h"
#include "ChessPiece.h"
#include "QueenPiece.generated.h"

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

    // Переопределение логики перемещения для ферзя
    // virtual bool CanMoveTo(int32 TargetX, int32 TargetY /*, AChessBoard* Board */) override;
};
