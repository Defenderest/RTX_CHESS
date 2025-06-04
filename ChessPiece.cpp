#include "ChessPiece.h"
#include "ChessBoard.h" // Для использования AChessBoard в GetValidMoves
#include "Components/StaticMeshComponent.h" // Для UStaticMeshComponent
#include "Engine/StaticMesh.h" // Для UStaticMesh
#include "Materials/MaterialInterface.h" // Для UMaterialInterface

AChessPiece::AChessPiece()
{
    PrimaryActorTick.bCanEverTick = false; // Фигуры обычно не тикают каждый кадр

    PieceMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PieceMesh"));
    RootComponent = PieceMeshComponent;
    PieceMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // Для кликов
    PieceMeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
    PieceMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);

    PieceColor = EPieceColor::White;
    TypeOfPiece = EPieceType::Pawn;
    BoardPosition = FIntPoint(-1, -1); // Невалидная начальная позиция
    bHasMoved = false; // Инициализация по умолчанию
}

void AChessPiece::BeginPlay()
{
    Super::BeginPlay();
}

void AChessPiece::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AChessPiece::InitializePiece(EPieceColor InColor, EPieceType InType, FIntPoint InBoardPosition)
{
    PieceColor = InColor;
    TypeOfPiece = InType;
    BoardPosition = InBoardPosition;
    bHasMoved = false; // Сбрасываем флаг при инициализации/рестарте

    // Устанавливаем материал в зависимости от цвета
    if (PieceMeshComponent)
    {
        if (PieceColor == EPieceColor::White && WhiteMaterial)
        {
            PieceMeshComponent->SetMaterial(0, WhiteMaterial);
        }
        else if (PieceColor == EPieceColor::Black && BlackMaterial)
        {
            PieceMeshComponent->SetMaterial(0, BlackMaterial);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessPiece::InitializePiece: Material not set for %s %s. WhiteMaterial: %s, BlackMaterial: %s"),
                (PieceColor == EPieceColor::White ? TEXT("White") : TEXT("Black")),
                *UEnum::GetValueAsString(TypeOfPiece),
                WhiteMaterial ? *WhiteMaterial->GetName() : TEXT("null"),
                BlackMaterial ? *BlackMaterial->GetName() : TEXT("null"));
        }
    }

    UE_LOG(LogTemp, Log, TEXT("AChessPiece: Initialized %s %s at (%d, %d)"),
           (PieceColor == EPieceColor::White ? TEXT("White") : TEXT("Black")),
           *UEnum::GetValueAsString(TypeOfPiece),
           BoardPosition.X, BoardPosition.Y);
}

EPieceColor AChessPiece::GetPieceColor() const
{
    return PieceColor;
}

EPieceType AChessPiece::GetPieceType() const
{
    return TypeOfPiece;
}

FIntPoint AChessPiece::GetBoardPosition() const
{
    return BoardPosition;
}

void AChessPiece::SetBoardPosition(const FIntPoint& NewPosition)
{
    BoardPosition = NewPosition;
    // Мировое положение актора будет обновлено GameMode или ChessBoard
    // UE_LOG(LogTemp, Log, TEXT("AChessPiece: Set %s %s to new board position (%d, %d)"),
    //        (PieceColor == EPieceColor::White ? TEXT("White") : TEXT("Black")),
    //        *UEnum::GetValueAsString(TypeOfPiece),
    //        BoardPosition.X, BoardPosition.Y);
}

void AChessPiece::SetPieceMesh(UStaticMesh* NewMesh)
{
    if (PieceMeshComponent)
    {
        PieceMeshComponent->SetStaticMesh(NewMesh);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessPiece::SetPieceMesh: PieceMeshComponent is null for %s."), *GetName());
    }
}

TArray<FIntPoint> AChessPiece::GetValidMoves(const AChessBoard* Board) const
{
    // Базовая реализация: фигура не имеет допустимых ходов.
    // Эта функция должна быть переопределена в дочерних классах для каждого типа фигуры.
    UE_LOG(LogTemp, Warning, TEXT("AChessPiece::GetValidMoves: Base implementation called for %s %s at (%d, %d). This should be overridden in derived classes. No valid moves returned."),
           (PieceColor == EPieceColor::White ? TEXT("White") : TEXT("Black")),
           *UEnum::GetValueAsString(TypeOfPiece),
           BoardPosition.X, BoardPosition.Y);
    return TArray<FIntPoint>();
}

void AChessPiece::OnSelected_Implementation()
{
    // Логика по умолчанию для выбора фигуры (может быть переопределена в Blueprint)
    UE_LOG(LogTemp, Log, TEXT("AChessPiece: %s %s at (%d, %d) selected."),
           (PieceColor == EPieceColor::White ? TEXT("White") : TEXT("Black")),
           *UEnum::GetValueAsString(TypeOfPiece),
           BoardPosition.X, BoardPosition.Y);
}

void AChessPiece::OnDeselected_Implementation()
{
    // Логика по умолчанию для снятия выбора с фигуры (может быть переопределена в Blueprint)
    UE_LOG(LogTemp, Log, TEXT("AChessPiece: %s %s at (%d, %d) deselected."),
           (PieceColor == EPieceColor::White ? TEXT("White") : TEXT("Black")),
           *UEnum::GetValueAsString(TypeOfPiece),
           BoardPosition.X, BoardPosition.Y);
}

void AChessPiece::OnCaptured_Implementation()
{
    // Логика по умолчанию для захвата фигуры (может быть переопределена в Blueprint)
    UE_LOG(LogTemp, Log, TEXT("AChessPiece: %s %s at (%d, %d) captured."),
           (PieceColor == EPieceColor::White ? TEXT("White") : TEXT("Black")),
           *UEnum::GetValueAsString(TypeOfPiece),
           BoardPosition.X, BoardPosition.Y);
    // Здесь можно добавить логику для скрытия фигуры, воспроизведения анимации и т.д.
}

void AChessPiece::NotifyMoveCompleted_Implementation()
{
    // Реализация по умолчанию. Может быть переопределена в дочерних классах.
    // Основная логика установки bHasMoved будет в переопределенных методах.
    // UE_LOG(LogTemp, Log, TEXT("AChessPiece: %s %s at (%d, %d) notified of move completion (base)."),
    //        (PieceColor == EPieceColor::White ? TEXT("White") : TEXT("Black")),
    //        *UEnum::GetValueAsString(TypeOfPiece),
    //        BoardPosition.X, BoardPosition.Y);
}
