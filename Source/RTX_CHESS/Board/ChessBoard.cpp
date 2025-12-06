#include "Board/ChessBoard.h"
#include "Pieces/ChessPiece.h" // Необходимо для AChessPiece
#include "Core/ChessGameState.h" // Потенциально необходимо для GetPieceAtGridPosition
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

        UE_LOG(LogTemp, Log, TEXT("AChessBoard::SetPieceAtGridPosition: Piece set at %s world pos %s"),
            *GridPosition.ToString(), *WorldPos.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessBoard::SetPieceAtGridPosition: Invalid position %s or null piece"),
            *GridPosition.ToString());
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
    UE_LOG(LogTemp, Log, TEXT("AChessBoard::ClearSquare called for %s."), *GridPosition.ToString());
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

    // Проверяем каждую фигуру атакующего цвета
    for (AChessPiece* Piece : GS->ActivePieces)
    {
        if (Piece && Piece->GetPieceColor() == AttackingColor)
        {
            // --- ИСПРАВЛЕНИЕ ВЫЛЕТА ИЗ-ЗА РЕКУРСИИ ---
            // Вылет был вызван бесконечной рекурсией: KingA->GetValidMoves вызывает IsSquareAttackedBy,
            // который вызывает GetValidMoves для вражеского KingB, что в свою очередь снова вызывает IsSquareAttackedBy, создавая цикл.
            // Чтобы разорвать этот цикл, мы обрабатываем атаку короля отдельно, не вызывая для него GetValidMoves.
            if (Piece->GetPieceType() == EPieceType::King)
            {
                FIntPoint KingPos = Piece->GetBoardPosition();
                // Король атакует все 8 смежных клеток.
                if (FMath::Abs(KingPos.X - SquarePosition.X) <= 1 && FMath::Abs(KingPos.Y - SquarePosition.Y) <= 1)
                {
                    // Убедимся, что это не та же клетка, на которой стоит король, так как фигура не атакует саму себя.
                    if (KingPos != SquarePosition)
                    {
                        return true;
                    }
                }
            }
            else
            {
                // Для всех остальных фигур мы предполагаем, что вызов GetValidMoves безопасен.
                const TArray<FIntPoint> ValidMoves = Piece->GetValidMoves(GS, this);
                if (ValidMoves.Contains(SquarePosition))
                {
                    return true; // Найдена фигура, атакующая клетку
                }
            }
        }
    }

    return false; // Ни одна фигура не атакует клетку
}
