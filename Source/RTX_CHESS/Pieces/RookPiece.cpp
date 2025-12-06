#include "Pieces/RookPiece.h"
#include "Board/ChessBoard.h" // Для AChessBoard
#include "Core/ChessGameState.h" // Для AChessGameState

ARookPiece::ARookPiece()
{
    TypeOfPiece = EPieceType::Rook;
    // bHasMoved теперь инициализируется в AChessPiece::InitializePiece
}

void ARookPiece::BeginPlay()
{
    Super::BeginPlay();
}

void ARookPiece::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

TArray<FIntPoint> ARookPiece::GetValidMoves(const AChessGameState* GameState, const AChessBoard* Board) const
{
    TArray<FIntPoint> ValidMoves;
    if (!Board || !GameState) return ValidMoves;

    FIntPoint CurrentPos = GetBoardPosition();

    // Определяем все 4 горизонтальных/вертикальных направления
    int32 Dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    for (int i = 0; i < 4; ++i)
    {
        int32 Dx = Dirs[i][0];
        int32 Dy = Dirs[i][1];

        for (int32 Step = 1; Step < Board->GetBoardSize().X; ++Step) // Максимальное количество шагов
        {
            FIntPoint TargetPos = FIntPoint(CurrentPos.X + Dx * Step, CurrentPos.Y + Dy * Step);

            if (!Board->IsValidGridPosition(TargetPos))
            {
                break; // Вышли за пределы доски
            }

            AChessPiece* PieceAtTarget = GameState->GetPieceAtGridPosition(TargetPos);

            if (PieceAtTarget)
            {
                if (PieceAtTarget->GetPieceColor() != PieceColor)
                {
                    ValidMoves.Add(TargetPos); // Можно взять фигуру противника
                }
                break; // Дальше по этой линии двигаться нельзя (своя фигура или захваченная)
            }
            else
            {
                ValidMoves.Add(TargetPos); // Свободная клетка
            }
        }
    }

    return ValidMoves;
}

void ARookPiece::NotifyMoveCompleted_Implementation()
{
    if (!bHasMoved) // Устанавливаем флаг только если он еще не был установлен
    {
        bHasMoved = true;
        UE_LOG(LogTemp, Log, TEXT("ARookPiece: Rook at (%d, %d) has completed its first move."), GetBoardPosition().X, GetBoardPosition().Y);
    }
    Super::NotifyMoveCompleted_Implementation(); // Вызываем базовую реализацию
}
