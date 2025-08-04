#include "ChessBoard.h"

AChessBoard::AChessBoard()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AChessBoard::BeginPlay()
{
    Super::BeginPlay();
}

void AChessBoard::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}
