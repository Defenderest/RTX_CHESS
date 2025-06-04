#pragma once

#include "CoreMinimal.h"
#include "ChessPiece.h"
#include "KnightPiece.generated.h"

UCLASS()
class RTX_CHESS_API AKnightPiece : public AChessPiece
{
    GENERATED_BODY()

public:
    AKnightPiece();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Переопределение логики перемещения для коня
    // virtual bool CanMoveTo(int32 TargetX, int32 TargetY /*, AChessBoard* Board */) override;
};
