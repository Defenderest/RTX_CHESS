#include "KingPiece.h"
#include "ChessBoard.h" // Для AChessBoard и его методов
#include "ChessGameState.h" // Потенциально для проверки шаха при рокировке

AKingPiece::AKingPiece()
{
    TypeOfPiece = EPieceType::King;
    bHasMoved = false; // Король изначально не делал ход
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

TArray<FIntPoint> AKingPiece::GetValidMoves(const AChessBoard* Board) const
{
    TArray<FIntPoint> ValidMoves;
    if (!Board)
    {
        UE_LOG(LogTemp, Error, TEXT("AKingPiece::GetValidMoves: Board is null."));
        return ValidMoves;
    }

    FIntPoint CurrentPos = GetBoardPosition();

    // Все 8 возможных направлений для короля
    const int32 KingMoveOffsets[][2] = {
        {0, 1}, {1, 1}, {1, 0}, {1, -1},
        {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}
    };

    for (const auto& Offset : KingMoveOffsets)
    {
        FIntPoint TargetPos(CurrentPos.X + Offset[0], CurrentPos.Y + Offset[1]);

        if (Board->IsValidGridPosition(TargetPos))
        {
            AChessPiece* PieceAtTarget = Board->GetPieceAtGridPosition(TargetPos);
            if (!PieceAtTarget || PieceAtTarget->GetPieceColor() != PieceColor)
            {
                // Клетка либо пуста, либо занята фигурой противника
                // TODO: Добавить проверку, не ставит ли этот ход короля под шах
                ValidMoves.Add(TargetPos);
            }
        }
    }

    // TODO: Реализовать логику рокировки
    // 1. Король не должен был двигаться (bHasMoved == false)
    // 2. Соответствующая ладья не должна была двигаться
    // 3. Между королем и ладьей не должно быть фигур
    // 4. Король не должен быть под шахом
    // 5. Клетки, через которые проходит король, не должны быть под атакой

    return ValidMoves;
}

void AKingPiece::NotifyMoveCompleted()
{
    if (!bHasMoved)
    {
        bHasMoved = true;
        UE_LOG(LogTemp, Log, TEXT("AKingPiece: King at (%d, %d) has completed its first move."), GetBoardPosition().X, GetBoardPosition().Y);
    }
}
