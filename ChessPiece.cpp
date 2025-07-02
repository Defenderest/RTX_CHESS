#include "ChessPiece.h"
#include "ChessBoard.h" // Для использования AChessBoard в GetValidMoves
#include "Components/StaticMeshComponent.h" // Для UStaticMeshComponent
#include "Components/SphereComponent.h" // Для USphereComponent
#include "Engine/StaticMesh.h" // Для UStaticMesh
#include "Materials/MaterialInterface.h" // Для UMaterialInterface

AChessPiece::AChessPiece()
{
    PrimaryActorTick.bCanEverTick = false; // Фигуры обычно не тикают каждый кадр

    // Создаем компонент-сферу и делаем его корневым
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    RootComponent = CollisionSphere;
    CollisionSphere->SetSphereRadius(40.f); 
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
    CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block); 
    CollisionSphere->CanCharacterStepUpOn = ECB_No;
    // Явно отключаем симуляцию физики, чтобы фигура не падала под действием гравитации.
    CollisionSphere->SetSimulatePhysics(false);

    // Создаем компонент меша и прикрепляем его к сфере
    PieceMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PieceMesh"));
    PieceMeshComponent->SetupAttachment(RootComponent);
    // Устанавливаем относительное положение меша в (0,0,0), чтобы он был идеально центрирован
    // относительно корневого компонента (CollisionSphere).
    PieceMeshComponent->SetRelativeLocation(FVector::ZeroVector);
    
    // Коллизия меша настраивается для кликов (��рассировка видимости)
    // и не должна мешать основной коллизии сферы для размещения.
    PieceMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PieceMeshComponent->SetCollisionObjectType(ECC_Pawn); // Или другой тип, который вы используете для интерактивных объектов
    PieceMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
    PieceMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // Реагировать на трассировку видимости для кликов
    PieceMeshComponent->CanCharacterStepUpOn = ECB_No;


    PieceColor = EPieceColor::White;
    TypeOfPiece = EPieceType::Pawn;
    BoardPosition = FIntPoint(-1, -1); // Невалидная начальная позиция
    bHasMoved = false; // Инициализация по умолчанию
}

void AChessPiece::BeginPlay()
{
    Super::BeginPlay();
    // Отладочный лог был перенесен в GameMode для большей точности,
    // так как BeginPlay может вызываться до полной инициализации фигуры.
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

    // Устанавливаем материал в зависимости от цвета и создаем динамический инстанс для подсветки
    if (PieceMeshComponent)
    {
        UMaterialInterface* BaseMaterial = nullptr;
        if (PieceColor == EPieceColor::White && WhiteMaterial)
        {
            BaseMaterial = WhiteMaterial;
        }
        else if (PieceColor == EPieceColor::Black && BlackMaterial)
        {
            BaseMaterial = BlackMaterial;
        }

        if (BaseMaterial)
        {
            DynamicMaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
            PieceMeshComponent->SetMaterial(0, DynamicMaterialInstance);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessPiece::InitializePiece: Base material not set for %s %s. Cannot create dynamic material instance."),
                (PieceColor == EPieceColor::White ? TEXT("White") : TEXT("Black")),
                *UEnum::GetValueAsString(TypeOfPiece));
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
    if (DynamicMaterialInstance)
    {
        DynamicMaterialInstance->SetScalarParameterValue(FName("Highlight"), 1.0f);
    }
}

void AChessPiece::OnDeselected_Implementation()
{
    if (DynamicMaterialInstance)
    {
        DynamicMaterialInstance->SetScalarParameterValue(FName("Highlight"), 0.0f);
    }
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
