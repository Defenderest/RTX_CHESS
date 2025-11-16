#include "KingPiece.h"
#include "ChessBoard.h" // Для AChessBoard и его методов
#include "ChessGameState.h" // Потенциально для проверки шаха при рокировке

AKingPiece::AKingPiece()
{
    TypeOfPiece = EPieceType::King;
    // bHasMoved теперь инициализируется в AChessPiece::InitializePiece
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

TArray<FIntPoint> AKingPiece::GetValidMoves(const AChessGameState* GameState, const AChessBoard* Board) const
{
    TArray<FIntPoint> ValidMoves;
    if (!Board || !GameState)
    {
        UE_LOG(LogTemp, Error, TEXT("AKingPiece::GetValidMoves: Board or GameState is null."));
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
            AChessPiece* PieceAtTarget = GameState->GetPieceAtGridPosition(TargetPos);
            if (!PieceAtTarget || PieceAtTarget->GetPieceColor() != PieceColor)
            {
                // Клетка либо пуста, либо занята фигурой противника
                // TODO: Добавить проверку, не ставит ли этот ход короля под шах
                ValidMoves.Add(TargetPos);
            }
        }
    }

    // --- Логика рокировки ---
    const EPieceColor OpponentColor = (PieceColor == EPieceColor::White) ? EPieceColor::Black : EPieceColor::White;

    // 1. Король не должен был двигаться и не должен быть под шахом
    if (!HasMoved() && !Board->IsSquareAttackedBy(CurrentPos, OpponentColor))
    {
        // --- Рокировка в сторону короля (короткая) ---
        const bool bCanCastleKingside = (PieceColor == EPieceColor::White) ? GameState->bCanWhiteCastleKingSide : GameState->bCanBlackCastleKingSide;
        if (bCanCastleKingside)
        {
            // Проверяем, что клетки между королем и ладьей пусты
            bool bPathIsClear = true;
            for (int32 X = CurrentPos.X + 1; X < Board->GetBoardSize().X - 1; ++X)
            {
                if (GameState->GetPieceAtGridPosition(FIntPoint(X, CurrentPos.Y)) != nullptr)
                {
                    bPathIsClear = false;
                    break;
                }
            }

            // Проверяем, что клетки, через которые проходит король, не под атакой
            if (bPathIsClear)
            {
                const FIntPoint KingPassThrough1(CurrentPos.X + 1, CurrentPos.Y);
                const FIntPoint KingPassThrough2(CurrentPos.X + 2, CurrentPos.Y);
                if (!Board->IsSquareAttackedBy(KingPassThrough1, OpponentColor) &&
                    !Board->IsSquareAttackedBy(KingPassThrough2, OpponentColor))
                {
                    ValidMoves.Add(KingPassThrough2); // Добавляем ход рокировки
                }
            }
        }

        // --- Рокировка в сторону ферзя (длинная) ---
        const bool bCanCastleQueenside = (PieceColor == EPieceColor::White) ? GameState->bCanWhiteCastleQueenSide : GameState->bCanBlackCastleQueenSide;
        if (bCanCastleQueenside)
        {
            // Проверяем, что клетки между королем и ладьей пусты
            bool bPathIsClear = true;
            for (int32 X = CurrentPos.X - 1; X > 0; --X)
            {
                if (GameState->GetPieceAtGridPosition(FIntPoint(X, CurrentPos.Y)) != nullptr)
                {
                    bPathIsClear = false;
                    break;
                }
            }

            // Проверяем, что клетки, через которые проходит король, не под атакой
            if (bPathIsClear)
            {
                const FIntPoint KingPassThrough1(CurrentPos.X - 1, CurrentPos.Y);
                const FIntPoint KingPassThrough2(CurrentPos.X - 2, CurrentPos.Y);
                if (!Board->IsSquareAttackedBy(KingPassThrough1, OpponentColor) &&
                    !Board->IsSquareAttackedBy(KingPassThrough2, OpponentColor))
                {
                    ValidMoves.Add(KingPassThrough2); // Добавляем ход рокировки
                }
            }
        }
    }

    return ValidMoves;
}

void AKingPiece::NotifyMoveCompleted_Implementation()
{
    if (!bHasMoved) // Устанавливаем флаг только если он еще не был установлен
    {
        bHasMoved = true;
        UE_LOG(LogTemp, Log, TEXT("AKingPiece: King at (%d, %d) has completed its first move."), GetBoardPosition().X, GetBoardPosition().Y);
    }
    Super::NotifyMoveCompleted_Implementation(); // Вызываем базовую реализацию
}
