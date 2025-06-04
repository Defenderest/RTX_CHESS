#pragma once

#include "CoreMinimal.h"
#include "ChessPiece.h"
#include "KingPiece.generated.h"

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

    // Переопределение логики перемещения для короля
    // virtual bool CanMoveTo(int32 TargetX, int32 TargetY /*, AChessBoard* Board */) override;
};
