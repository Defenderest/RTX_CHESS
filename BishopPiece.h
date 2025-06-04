#pragma once

#include "CoreMinimal.h"
#include "ChessPiece.h"
#include "BishopPiece.generated.h"

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

    // Переопределение логики перемещения для слона
    // virtual bool CanMoveTo(int32 TargetX, int32 TargetY /*, AChessBoard* Board */) override;
};
