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
    // Пример:
    /*
    if (IsValidGridPosition(GridPosition))
    {
        // Если используется локальный PieceGrid:
        // PieceGrid.Add(GridPosition, Piece);

        if (Piece)
        {
            // Piece->SetBoardPosition(GridPosition); // Это также должно обновить мировое положение
        }
    }
    */
    UE_LOG(LogTemp, Log, TEXT("AChessBoard::SetPieceAtGridPosition called for (%d, %d)."), GridPosition.X, GridPosition.Y);
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
    // Предполагаем, что местоположение актора является углом (0,0) сетки
    // И что клетки центрированы.
    // Смещение для центрирования клетки:
    float HalfTile = TileSize / 2.0f;
    // Вычисляем X и Y относительно начала координат актора доски
    float XPos = (GridPosition.X * TileSize) + HalfTile;
    float YPos = (GridPosition.Y * TileSize) + HalfTile;
    
    // Z позиция может быть фиксированной или основываться на Z актора.
    // Этот расчет предполагает, что доска лежит в плоскости XY относительно ее начала координат.
    // Скорректируйте, если локальное начало координат или ориентация вашей доски отличаются.
    return GetActorLocation() + FVector(XPos, YPos, GetActorLocation().Z); // Используем Z актора для высоты
}

FIntPoint AChessBoard::WorldToGridPosition(const FVector& WorldPosition) const
{
    // Обратно GridToWorldPosition.
    // Этот расчет предполагает,
    // что доска лежит в плоскости XY относительно ее начала координат.
    FVector LocalPosition = WorldPosition - GetActorLocation();
    int32 GridX = FMath::FloorToInt(LocalPosition.X / TileSize);
    int32 GridY = FMath::FloorToInt(LocalPosition.Y / TileSize);
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
