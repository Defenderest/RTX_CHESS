#include "PawnPiece.h"
#include "ChessBoard.h" // Для AChessBoard
#include "ChessGameState.h" // Для AChessGameState

APawnPiece::APawnPiece()
{
    // Устанавливаем тип фигуры в конструкторе
    TypeOfPiece = EPieceType::Pawn;
    // bHasMoved теперь инициализируется в AChessPiece::InitializePiece
}

void APawnPiece::BeginPlay()
{
    Super::BeginPlay();
}

void APawnPiece::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

TArray<FIntPoint> APawnPiece::GetValidMoves(const AChessGameState* GameState, const AChessBoard* Board) const
{
    TArray<FIntPoint> ValidMoves;
    if (!Board || !GameState) return ValidMoves;

    FIntPoint CurrentPos = GetBoardPosition();
    int32 Direction = (PieceColor == EPieceColor::White) ? 1 : -1; // 1 для белых (вверх), -1 для черных (вниз)

    // 1. Ход вперед на 1 клетку
    FIntPoint OneStepForward = FIntPoint(CurrentPos.X, CurrentPos.Y + Direction);
    if (Board->IsValidGridPosition(OneStepForward) && !GameState->GetPieceAtGridPosition(OneStepForward))
    {
        ValidMoves.Add(OneStepForward);

        // 2. Ход вперед на 2 клетки (только если это первый ход пешки)
        FIntPoint TwoStepsForward = FIntPoint(CurrentPos.X, CurrentPos.Y + 2 * Direction);
        if (!bHasMoved && Board->IsValidGridPosition(TwoStepsForward) && !GameState->GetPieceAtGridPosition(TwoStepsForward))
        {
            ValidMoves.Add(TwoStepsForward);
        }
    }

    // 3. Ходы для взятия по диагонали
    FIntPoint DiagonalLeft = FIntPoint(CurrentPos.X - 1, CurrentPos.Y + Direction);
    FIntPoint DiagonalRight = FIntPoint(CurrentPos.X + 1, CurrentPos.Y + Direction);

    // Проверяем взятие влево
    if (Board->IsValidGridPosition(DiagonalLeft))
    {
        AChessPiece* PieceAtDiagonalLeft = GameState->GetPieceAtGridPosition(DiagonalLeft);
        if (PieceAtDiagonalLeft && PieceAtDiagonalLeft->GetPieceColor() != PieceColor)
        {
            ValidMoves.Add(DiagonalLeft);
        }
    }

    // Проверяем взятие вправо
    if (Board->IsValidGridPosition(DiagonalRight))
    {
        AChessPiece* PieceAtDiagonalRight = GameState->GetPieceAtGridPosition(DiagonalRight);
        if (PieceAtDiagonalRight && PieceAtDiagonalRight->GetPieceColor() != PieceColor)
        {
            ValidMoves.Add(DiagonalRight);
        }
    }

    // 4. Взятие на проходе (En Passant)
    if (GameState)
    {
        FIntPoint EnPassantTarget = GameState->GetEnPassantTargetSquare();
        APawnPiece* PawnToCaptureEP = GameState->GetEnPassantPawnToCapture();

        // Проверяем, является ли диагональный ход ходом на клетку для взятия на проходе
        // И что пешка для взятия на проходе существует и имеет противоположный цвет
        if (PawnToCaptureEP && PawnToCaptureEP->GetPieceColor() != PieceColor)
        {
            if (Board->IsValidGridPosition(DiagonalLeft) && DiagonalLeft == EnPassantTarget &&
                PawnToCaptureEP->GetBoardPosition() == FIntPoint(CurrentPos.X - 1, CurrentPos.Y))
            {
                ValidMoves.Add(DiagonalLeft);
            }
            if (Board->IsValidGridPosition(DiagonalRight) && DiagonalRight == EnPassantTarget &&
                 PawnToCaptureEP->GetBoardPosition() == FIntPoint(CurrentPos.X + 1, CurrentPos.Y))
            {
                ValidMoves.Add(DiagonalRight);
            }
        }
    }

    return ValidMoves;
}

void APawnPiece::NotifyMoveCompleted_Implementation()
{
    if (!bHasMoved) // Устанавливаем флаг только если он еще не был установлен
    {
        bHasMoved = true;
        UE_LOG(LogTemp, Log, TEXT("APawnPiece: Pawn at (%d, %d) has completed its first move."), GetBoardPosition().X, GetBoardPosition().Y);
    }
    Super::NotifyMoveCompleted_Implementation(); // Вызываем базовую реализацию, если она что-то делает
}
