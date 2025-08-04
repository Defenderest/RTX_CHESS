#include "QueenPiece.h"

AQueenPiece::AQueenPiece()
{
    // TypeOfPiece = EPieceType::Queen;
}

void AQueenPiece::BeginPlay()
{
    Super::BeginPlay();
}

void AQueenPiece::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// bool AQueenPiece::CanMoveTo(int32 TargetX, int32 TargetY /*, AChessBoard* Board */)
// {
//     // Логика ходов ферзя
//     return false;
// }
