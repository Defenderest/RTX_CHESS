#include "BishopPiece.h"

ABishopPiece::ABishopPiece()
{
    // TypeOfPiece = EPieceType::Bishop;
}

void ABishopPiece::BeginPlay()
{
    Super::BeginPlay();
}

void ABishopPiece::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// bool ABishopPiece::CanMoveTo(int32 TargetX, int32 TargetY /*, AChessBoard* Board */)
// {
//     // Логика ходов слона
//     return false;
// }
