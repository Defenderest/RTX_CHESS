#include "Pieces/BishopPiece.h"
#include "Board/ChessBoard.h" // Для AChessBoard
#include "Core/ChessGameState.h" // Для AChessGameState

ABishopPiece::ABishopPiece()
{
    TypeOfPiece = EPieceType::Bishop;
}

void ABishopPiece::BeginPlay()
{
    Super::BeginPlay();
}

void ABishopPiece::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

TArray<FIntPoint> ABishopPiece::GetValidMoves(const AChessGameState* GameState, const AChessBoard* Board) const
{
    TArray<FIntPoint> ValidMoves;
    if (!Board || !GameState) return ValidMoves;

    FIntPoint CurrentPos = GetBoardPosition();

    // Определяем все 4 диагональных направления
    int32 Dirs[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

    for (int i = 0; i < 4; ++i)
    {
        int32 Dx = Dirs[i][0];
        int32 Dy = Dirs[i][1];

        for (int32 Step = 1; Step < Board->GetBoardSize().X; ++Step) // Максимальное количество шагов по диагонали
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
