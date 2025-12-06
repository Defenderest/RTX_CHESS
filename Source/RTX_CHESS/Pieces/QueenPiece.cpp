#include "Pieces/QueenPiece.h"
#include "Board/ChessBoard.h" // Для AChessBoard
#include "Core/ChessGameState.h" // Для AChessGameState

AQueenPiece::AQueenPiece()
{
    TypeOfPiece = EPieceType::Queen;
}

void AQueenPiece::BeginPlay()
{
    Super::BeginPlay();
}

void AQueenPiece::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

TArray<FIntPoint> AQueenPiece::GetValidMoves(const AChessGameState* GameState, const AChessBoard* Board) const
{
    TArray<FIntPoint> ValidMoves;
    if (!Board || !GameState) return ValidMoves;

    FIntPoint CurrentPos = GetBoardPosition();

    // Ферзь комбинирует ходы Ладьи (горизонтальные/вертикальные) и Слона (диагональные)
    // Определяем все 8 направлений (горизонтальные, вертикальные, диагональные)
    int32 Dirs[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}, // Горизонтальные и вертикальные
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}  // Диагональные
    };

    for (int i = 0; i < 8; ++i)
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
                break; // Дальше по этой линии двигаться нельзя
            }
            else
            {
                ValidMoves.Add(TargetPos); // Свободная клетка
            }
        }
    }

    return ValidMoves;
}
