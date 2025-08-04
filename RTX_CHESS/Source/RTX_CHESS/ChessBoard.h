#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChessBoard.generated.h"

UCLASS()
class RTX_CHESS_API AChessBoard : public AActor
{
    GENERATED_BODY()

public:
    AChessBoard();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Здесь будет логика доски и ее представление
};
