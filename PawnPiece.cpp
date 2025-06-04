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

TArray<FIntPoint> APawnPiece::GetValidMoves(const AChessBoard* Board) const
{
    TArray<FIntPoint> ValidMoves;
    if (!Board) return ValidMoves;

    FIntPoint CurrentPos = GetBoardPosition();
    int32 Direction = (PieceColor == EPieceColor::White) ? 1 : -1; // 1 для белых (вверх), -1 для черных (вниз)

    // 1. Ход вперед на 1 клетку
    FIntPoint OneStepForward = FIntPoint(CurrentPos.X, CurrentPos.Y + Direction);
    if (Board->IsValidGridPosition(OneStepForward) && !Board->GetPieceAtGridPosition(OneStepForward))
    {
        ValidMoves.Add(OneStepForward);

        // 2. Ход вперед на 2 клетки (только если это первый ход пешки)
        FIntPoint TwoStepsForward = FIntPoint(CurrentPos.X, CurrentPos.Y + 2 * Direction);
        if (!bHasMoved && Board->IsValidGridPosition(TwoStepsForward) && !Board->GetPieceAtGridPosition(TwoStepsForward))
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
        AChessPiece* PieceAtDiagonalLeft = Board->GetPieceAtGridPosition(DiagonalLeft);
        if (PieceAtDiagonalLeft && PieceAtDiagonalLeft->GetPieceColor() != PieceColor)
        {
            ValidMoves.Add(DiagonalLeft);
        }
    }

    // Проверяем взятие вправо
    if (Board->IsValidGridPosition(DiagonalRight))
    {
        AChessPiece* PieceAtDiagonalRight = Board->GetPieceAtGridPosition(DiagonalRight);
        if (PieceAtDiagonalRight && PieceAtDiagonalRight->GetPieceColor() != PieceColor)
        {
            ValidMoves.Add(DiagonalRight);
        }
    }

    // TODO: Добавить логику для "взятия на проходе" (En Passant)

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
