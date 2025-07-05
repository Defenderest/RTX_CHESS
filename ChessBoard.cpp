#include "ChessBoard.h"
#include "ChessPiece.h" // Необходимо для AChessPiece
#include "ChessGameState.h" // Потенциально необходимо для GetPieceAtGridPosition
#include "Kismet/GameplayStatics.h" // Потенциально необходимо для GetGameState
#include "Components/StaticMeshComponent.h" // Для UStaticMeshComponent
#include "DrawDebugHelpers.h" // Для отладочной отрисовки

AChessBoard::AChessBoard()
{
    PrimaryActorTick.bCanEverTick = true;

    BoardMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoardMesh"));
    RootComponent = BoardMeshComponent;

    // Настройка доски по умолчанию
    BoardSize = FIntPoint(8, 8); // Стандартная доска 8x8
    TileSize = 100.0f;           // Пример: 100 юнитов Unreal на клетку
    PieceZOffsetOnBoard = 5.0f;  // Небольшое смещение над доской, чтобы фигуры не проваливались
}

#include "Kismet/KismetSystemLibrary.h"

void AChessBoard::BeginPlay()
{
    Super::BeginPlay();

    // Автоматический расчет TileSize был удален, так как он некорректно работал с мешами,
    // у которых есть рамка. Он использовал полный размер меша, что приводило к завышенному
    // значению TileSize и смещению фигур.
    // Теперь TileSize должен быть задан вручную в редакторе Blueprint для актора AChessBoard.
    UE_LOG(LogTemp, Log, TEXT("AChessBoard: Using manually set TileSize: %f. Ensure this is set correctly in the Blueprint."), TileSize);

    InitializeBoard();
}

void AChessBoard::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

#if WITH_EDITOR
    if (bDrawDebugGrid)
    {
        DrawDebugGrid();
    }
#endif
}

void AChessBoard::InitializeBoard()
{
    UE_LOG(LogTemp, Log, TEXT("AChessBoard::InitializeBoard called. Board is ready for pieces to be spawned by GameMode."));
    // Логика спавна фигур была перенесена в AChessGameMode::SpawnInitialPieces
    // для централизации и гибкости.
}

void AChessBoard::SpawnPieces()
{
    // Эта функция оставлена пустой и больше не используется.
    // Логика была перенесена в AChessGameMode для централизации.
    // Это предотвращает дублирование логики спавна.
}

FIntPoint AChessBoard::GetBoardSize() const
{
    return BoardSize;
}

bool AChessBoard::IsValidGridPosition(const FIntPoint& GridPosition) const
{
    return GridPosition.X >= 0 && GridPosition.X < BoardSize.X &&
        GridPosition.Y >= 0 && GridPosition.Y < BoardSize.Y;
}

AChessPiece* AChessBoard::GetPieceAtGridPosition(const FIntPoint& GridPosition) const
{
    // Эта функция в идеале должна запрашивать AChessGameState для получения фигуры на этой позиции.
    // Пока возвращаем nullptr в качестве заглушки.
    // Если вы поддерживаете локальный PieceGrid на доске (и он синхронизирован), вы можете использовать его.
    if (GetWorld())
    {
        if (AChessGameState* GS = GetWorld()->GetGameState<AChessGameState>())
        {
            return GS->GetPieceAtGridPosition(GridPosition);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessBoard::GetPieceAtGridPosition: ChessGameState is null."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessBoard::GetPieceAtGridPosition: GetWorld() is null."));
    }
    return nullptr;
}

void AChessBoard::SetPieceAtGridPosition(AChessPiece* Piece, const FIntPoint& GridPosition)
{
    // Эта функция обычно обновляет AChessGameState.
    // Если доска поддерживает собственную сетку фигур (PieceGrid), обновите ее здесь.
    // Убедитесь, что внутренняя позиция фигуры на доске также обновлена.
    if (IsValidGridPosition(GridPosition) && Piece)
    {
        // Устанавливаем позицию фигуры на доске
        FVector WorldPos = GridToWorldPosition(GridPosition);
        Piece->AnimateMoveTo(WorldPos);

        // Обновляем внутреннюю позицию фигуры
        Piece->SetBoardPosition(GridPosition);

        UE_LOG(LogTemp, Log, TEXT("AChessBoard::SetPieceAtGridPosition: Piece set at (%d, %d) world pos (%f, %f, %f)"),
            GridPosition.X, GridPosition.Y, WorldPos.X, WorldPos.Y, WorldPos.Z);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessBoard::SetPieceAtGridPosition: Invalid position (%d, %d) or null piece"),
            GridPosition.X, GridPosition.Y);
    }
}

void AChessBoard::ClearSquare(const FIntPoint& GridPosition)
{
    // Эта функция обычно обновляет AChessGameState или локальный PieceGrid.
    // Пример:
    /*
    if (IsValidGridPosition(GridPosition))
    {
        // Если используется локальный PieceGrid:
        // PieceGrid.Add(GridPosition, nullptr);
    }
    */
    UE_LOG(LogTemp, Log, TEXT("AChessBoard::ClearSquare called for (%d, %d)."), GridPosition.X, GridPosition.Y);
}

FVector AChessBoard::GridToWorldPosition(const FIntPoint& GridPosition) const
{
    // Вычисляем общее смещение, чтобы центрировать сетку относительно pivot'а актора.
    // Это предполагает, что pivot актора находится в центре доски.
    const float HalfBoardWidth = (BoardSize.X * TileSize) / 2.0f;
    const float HalfBoardHeight = (BoardSize.Y * TileSize) / 2.0f;

    // Вычисляем позицию в локальном пространстве доски.
    // Ось X инвертирована, чтобы соответствовать стандартной ориентации шахмат (файл 'a' слева).
    // Мы смещаем начало координат в правый нижний угол для оси X.
    const float LocalX = HalfBoardWidth - (GridPosition.X * TileSize) - (TileSize / 2.0f);
    const float LocalY = (GridPosition.Y * TileSize) + (TileSize / 2.0f) - HalfBoardHeight;
    const float LocalZ = PieceZOffsetOnBoard;

    const FVector LocalPosition(LocalX, LocalY, LocalZ);

    // Преобразуем локальную позицию в мировую, используя трансформацию актора.
    return GetActorTransform().TransformPosition(LocalPosition);
}

FIntPoint AChessBoard::WorldToGridPosition(const FVector& WorldPosition) const
{
    // Преобразуем мировую позицию в локальное пространство актора доски.
    const FVector LocalPosition = GetActorTransform().InverseTransformPosition(WorldPosition);

    // Вычисляем общее смещение, чтобы центрировать сетку относительно pivot'а актора.
    const float HalfBoardWidth = (BoardSize.X * TileSize) / 2.0f;
    const float HalfBoardHeight = (BoardSize.Y * TileSize) / 2.0f;

    // Преобразуем локальные координаты в координаты сетки.
    // Ось X инвертирована, поэтому мы вычисляем ее обратным образом.
    int32 GridX = FMath::FloorToInt((HalfBoardWidth - LocalPosition.X) / TileSize);
    int32 GridY = FMath::FloorToInt((LocalPosition.Y + HalfBoardHeight) / TileSize);

    // Ограничиваем значения в пределах доски.
    GridX = FMath::Clamp(GridX, 0, BoardSize.X - 1);
    GridY = FMath::Clamp(GridY, 0, BoardSize.Y - 1);

    return FIntPoint(GridX, GridY);
}

#if WITH_EDITOR
void AChessBoard::DrawDebugGrid() const
{
    if (!GetWorld()) return;

    for (int32 x = 0; x < BoardSize.X; ++x)
    {
        for (int32 y = 0; y < BoardSize.Y; ++y)
        {
            const FIntPoint GridPos(x, y);
            const FVector WorldPos = GridToWorldPosition(GridPos);
            // Рисуем небольшую красную сферу в центре каждой клетки
            DrawDebugSphere(GetWorld(), WorldPos, TileSize / 8, 12, FColor::Red, false, -1.f, 0, 1.f);
        }
    }
}
#endif

bool AChessBoard::IsSquareAttackedBy(const FIntPoint& SquarePosition, EPieceColor AttackingColor) const
{
    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("AChessBoard::IsSquareAttackedBy: GetWorld() is null."));
        return false;
    }

    AChessGameState* GS = GetWorld()->GetGameState<AChessGameState>();
    if (!GS)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessBoard::IsSquareAttackedBy: ChessGameState is null."));
        return false;
    }

    for (AChessPiece* Piece : GS->ActivePieces)
    {
        if (Piece && Piece->GetPieceColor() == AttackingColor)
        {
            const EPieceType Type = Piece->GetPieceType();
            
            if (Type == EPieceType::King)
            {
                // Специальная проверка для короля, чтобы избежать рекурсии.
                // Король атакует 8 соседних клеток.
                const FIntPoint KingPos = Piece->GetBoardPosition();
                const int32 KingMoveOffsets[][2] = {
                    {0, 1}, {1, 1}, {1, 0}, {1, -1},
                    {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}
                };
                for (const auto& Offset : KingMoveOffsets)
                {
                    if (KingPos + FIntPoint(Offset[0], Offset[1]) == SquarePosition)
                    {
                        return true; // Нашли атакующую клетку, выходим.
                    }
                }
            }
            else 
            {
                TArray<FIntPoint> AttackingMoves;
                if (Type == EPieceType::Pawn)
                {
                    // Пешки атакуют по диагонали вперед.
                    const FIntPoint PawnPos = Piece->GetBoardPosition();
                    const int32 Direction = (Piece->GetPieceColor() == EPieceColor::White) ? 1 : -1;

                    const FIntPoint AttackLeft(PawnPos.X - 1, PawnPos.Y + Direction);
                    if (IsValidGridPosition(AttackLeft)) AttackingMoves.Add(AttackLeft);

                    const FIntPoint AttackRight(PawnPos.X + 1, PawnPos.Y + Direction);
                    if (IsValidGridPosition(AttackRight)) AttackingMoves.Add(AttackRight);
                }
                else
                {
                    // Для всех остальных фигур (Ферзь, Ладья, Конь, Слон)
                    // их атакующие ходы совпадают с их обычными ходами.
                    AttackingMoves = Piece->GetValidMoves(GS, this);
                }

                if (AttackingMoves.Contains(SquarePosition))
                {
                    return true;
                }
            }
        }
    }
    return false;
}
