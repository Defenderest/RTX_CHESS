#include "Pieces/KnightPiece.h"
#include "Board/ChessBoard.h" // Для AChessBoard
#include "Core/ChessGameState.h" // Для AChessGameState

AKnightPiece::AKnightPiece()
{
    TypeOfPiece = EPieceType::Knight;
}

void AKnightPiece::BeginPlay()
{
    Super::BeginPlay();
}

void AKnightPiece::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

TArray<FIntPoint> AKnightPiece::GetValidMoves(const AChessGameState* GameState, const AChessBoard* Board) const
{
    TArray<FIntPoint> ValidMoves;
    if (!Board || !GameState) return ValidMoves;

    FIntPoint CurrentPos = GetBoardPosition();

    // Все 8 возможных "L"-образных ходов коня
    FIntPoint PossibleMoves[] = {
        FIntPoint(CurrentPos.X + 1, CurrentPos.Y + 2),
        FIntPoint(CurrentPos.X + 1, CurrentPos.Y - 2),
        FIntPoint(CurrentPos.X - 1, CurrentPos.Y + 2),
        FIntPoint(CurrentPos.X - 1, CurrentPos.Y - 2),
        FIntPoint(CurrentPos.X + 2, CurrentPos.Y + 1),
        FIntPoint(CurrentPos.X + 2, CurrentPos.Y - 1),
        FIntPoint(CurrentPos.X - 2, CurrentPos.Y + 1),
        FIntPoint(CurrentPos.X - 2, CurrentPos.Y - 1)
    };

    for (const FIntPoint& TargetPos : PossibleMoves)
    {
        if (Board->IsValidGridPosition(TargetPos))
        {
            AChessPiece* PieceAtTarget = GameState->GetPieceAtGridPosition(TargetPos);
            if (!PieceAtTarget || PieceAtTarget->GetPieceColor() != PieceColor)
            {
                ValidMoves.Add(TargetPos); // Клетка свободна или занята фигурой противника
            }
        }
    }

    return ValidMoves;
}

bool AKnightPiece::IsAttackingSquare(const FIntPoint& TargetSquare, const AChessGameState* GameState, const AChessBoard* Board) const
{
    FIntPoint CurrentPos = GetBoardPosition();
    int32 Dx = FMath::Abs(CurrentPos.X - TargetSquare.X);
    int32 Dy = FMath::Abs(CurrentPos.Y - TargetSquare.Y);

    // Конь атакует клетку, если разница координат (1,2) или (2,1)
    return (Dx == 1 && Dy == 2) || (Dx == 2 && Dy == 1);
}
