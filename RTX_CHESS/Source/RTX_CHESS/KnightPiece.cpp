#include "KnightPiece.h"

AKnightPiece::AKnightPiece()
{
    // TypeOfPiece = EPieceType::Knight;
}

void AKnightPiece::BeginPlay()
{
    Super::BeginPlay();
}

void AKnightPiece::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// bool AKnightPiece::CanMoveTo(int32 TargetX, int32 TargetY /*, AChessBoard* Board */)
// {
//     // Логика ходов коня
//     return false;
// }
