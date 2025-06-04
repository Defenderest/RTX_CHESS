#include "PawnPiece.h"

APawnPiece::APawnPiece()
{
    // TypeOfPiece = EPieceType::Pawn;
}

void APawnPiece::BeginPlay()
{
    Super::BeginPlay();
}

void APawnPiece::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// bool APawnPiece::CanMoveTo(int32 TargetX, int32 TargetY /*, AChessBoard* Board */)
// {
//     // Логика ходов пешки
//     return false;
// }
