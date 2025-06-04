#pragma once

#include "CoreMinimal.h"
#include "ChessPiece.h"
#include "RookPiece.generated.h"

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

    // Переопределение логики перемещения для ладьи
    // virtual bool CanMoveTo(int32 TargetX, int32 TargetY /*, AChessBoard* Board */) override;
};
