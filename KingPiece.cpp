#include "KingPiece.h"

AKingPiece::AKingPiece()
{
    // PrimaryActorTick.bCanEverTick = true; // Уже установлено в базовом классе
    // TypeOfPiece = EPieceType::King; // Установите тип фигуры
}

void AKingPiece::BeginPlay()
{
    Super::BeginPlay();
}

void AKingPiece::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// bool AKingPiece::CanMoveTo(int32 TargetX, int32 TargetY /*, AChessBoard* Board */)
// {
//     // Логика ходов короля
//     // return Super::CanMoveTo(TargetX, TargetY, Board); // Или полностью своя логика
//     return false;
// }
