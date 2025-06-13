#include "ChessBoard.h"
#include "ChessPiece.h" // Необходимо для AChessPiece
#include "ChessGameState.h" // Потенциально необходимо для GetPieceAtGridPosition
#include "Kismet/GameplayStatics.h" // Потенциально необходимо для GetGameState
#include "Components/StaticMeshComponent.h" // Для UStaticMeshComponent

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

void AChessBoard::BeginPlay()
{
    Super::BeginPlay();
    InitializeBoard(); // Инициализируем доску при начале игры
}

void AChessBoard::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AChessBoard::InitializeBoard()
{
    // TODO: Реализуйте логику инициализации доски, если это необходимо
    // Это может включать очистку существующих визуальных элементов,
    // сброс внутреннего состояния и т.д.
    // Пока это заглушка.
    UE_LOG(LogTemp, Log, TEXT("AChessBoard::InitializeBoard called."));

    // Пример: Если бы вы использовали PieceGrid TMap:
    // PieceGrid.Empty();
    // for (int32 x = 0; x < BoardSize.X; ++x)
    // {
    //     for (int32 y = 0; y < BoardSize.Y; ++y)
    //     {
    //         PieceGrid.Add(FIntPoint(x,y), nullptr);
    //     }
    // }
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
        Piece->SetActorLocation(WorldPos);

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
    // Эта логика предполагает, что pivot (опорная точка) актора доски находится в ее геометрическом центре.
    // Сначала вычисляем позицию в локальном пространстве доски.

    // Общий размер доски в локальном пространстве (без учета масштаба актора)
    const float TotalBoardWidth = BoardSize.X * TileSize;
    const float TotalBoardHeight = BoardSize.Y * TileSize;

    // Локальная позиция левого нижнего угла сетки (0,0)
    const float StartX = -TotalBoardWidth / 2.0f;
    const float StartY = -TotalBoardHeight / 2.0f;

    // Локальная позиция центра целевой клетки
    const float LocalX = StartX + (GridPosition.X * TileSize) + (TileSize / 2.0f);
    const float LocalY = StartY + (GridPosition.Y * TileSize) + (TileSize / 2.0f);
    const float LocalZ = PieceZOffsetOnBoard; // Смещение по Z также в локальном пространстве

    const FVector LocalPosition(LocalX, LocalY, LocalZ);

    // Преобразуем локальную позицию в мировую, используя трансформацию актора (учитывает location, rotation, scale)
    return GetActorTransform().TransformPosition(LocalPosition);
}

FIntPoint AChessBoard::WorldToGridPosition(const FVector& WorldPosition) const
{
    // Преобразуем мировую позицию в локальное пространство актора доски
    const FVector LocalPosition = GetActorTransform().InverseTransformPosition(WorldPosition);

    // Общий размер доски в локальном пространстве
    const float TotalBoardWidth = BoardSize.X * TileSize;
    const float TotalBoardHeight = BoardSize.Y * TileSize;

    // Локальная позиция левого нижнего угла сетки
    const float StartX = -TotalBoardWidth / 2.0f;
    const float StartY = -TotalBoardHeight / 2.0f;

    // Относительная позиция от левого нижнего угла
    const float RelativeX = LocalPosition.X - StartX;
    const float RelativeY = LocalPosition.Y - StartY;

    // Преобразуем в координаты сетки, используя оригинальный TileSize (т.к. мы в локальном пространстве)
    int32 GridX = FMath::FloorToInt(RelativeX / TileSize);
    int32 GridY = FMath::FloorToInt(RelativeY / TileSize);

    // Ограничиваем значения в пределах доски
    GridX = FMath::Clamp(GridX, 0, BoardSize.X - 1);
    GridY = FMath::Clamp(GridY, 0, BoardSize.Y - 1);

    return FIntPoint(GridX, GridY);
}

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
            // Для пешек, их "атакующие" ходы - это диагональные ходы, даже если там нет фигуры для взятия.
            // GetValidMoves для пешки уже включает диагональные ходы только если там есть фигура противника.
            // Нам нужно проверить "потенциальные" атаки.
            // Однако, для IsSquareAttackedBy, стандартное GetValidMoves обычно подходит,
            // так как оно показывает, куда фигура МОЖЕТ пойти и взять.
            // Если для пешки нужно особое правило "атаки пустой клетки по диагонали", это потребует доработки GetValidMoves или отдельного метода.
            // Пока будем использовать стандартный GetValidMoves.
            TArray<FIntPoint> AttackingMoves = Piece->GetValidMoves(this);

            // Специальная логика для пешек: они атакуют диагонали, даже если там нет фигуры для взятия (для проверки шаха)
            if (Piece->GetPieceType() == EPieceType::Pawn)
            {
                AttackingMoves.Empty(); // Очистим стандартные ходы и добавим только атакующие
                FIntPoint PawnPos = Piece->GetBoardPosition();
                int32 Direction = (Piece->GetPieceColor() == EPieceColor::White) ? 1 : -1;

                FIntPoint AttackLeft(PawnPos.X - 1, PawnPos.Y + Direction);
                if (IsValidGridPosition(AttackLeft)) AttackingMoves.Add(AttackLeft);

                FIntPoint AttackRight(PawnPos.X + 1, PawnPos.Y + Direction);
                if (IsValidGridPosition(AttackRight)) AttackingMoves.Add(AttackRight);
            }

            if (AttackingMoves.Contains(SquarePosition))
            {
                return true;
            }
        }
    }
    return false;
}
