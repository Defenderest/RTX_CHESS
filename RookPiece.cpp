#include "RookPiece.h"

ARookPiece::ARookPiece()
{
    // TypeOfPiece = EPieceType::Rook;
}

void ARookPiece::BeginPlay()
{
    Super::BeginPlay();
}

void ARookPiece::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// bool ARookPiece::CanMoveTo(int32 TargetX, int32 TargetY /*, AChessBoard* Board */)
// {
//     // Логика ходов ладьи
//     return false;
// }
