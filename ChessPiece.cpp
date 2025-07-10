#include "ChessPiece.h"
#include "ChessBoard.h" // Для использования AChessBoard в GetValidMoves
#include "Components/StaticMeshComponent.h" // Для UStaticMeshComponent
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h" // Для UStaticMesh
#include "Materials/MaterialInterface.h" // Для UMaterialInterface
#include "Net/UnrealNetwork.h" // Для репликации
#include "Kismet/GameplayStatics.h" // Для GetActorOfClass

AChessPiece::AChessPiece()
{
    PrimaryActorTick.bCanEverTick = true; // Включаем Tick для анимации движения

    // Включаем репликацию для этого актора.
    bReplicates = true;
    // Фигуры всегда важны, отключаем отсечение по расстоянию.
    bAlwaysRelevant = true;

    // Создаем корневой компонент сцены для обеспечения стабильной точки привязки.
    USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
    RootComponent = SceneComponent;

    // Создаем компонент меша и присоединяем его к корневому компоненту.
    PieceMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PieceMesh"));
    PieceMeshComponent->SetupAttachment(RootComponent);
    
    // Настраиваем коллизию меша так, чтобы он был интерактивным.
    // Он должен быть видимым для трассировки курсора (ECC_Visibility).
    PieceMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PieceMeshComponent->SetCollisionObjectType(ECC_WorldDynamic); // Используем тип WorldDynamic для интерактивных объектов
    PieceMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
    PieceMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // Блокируем канал Visibility для обнаружения кликов
    PieceMeshComponent->CanCharacterStepUpOn = ECB_No;
    PieceMeshComponent->SetSimulatePhysics(false); // Физика не нужна


    PieceColor = EPieceColor::White;
    TypeOfPiece = EPieceType::Pawn;
    BoardPosition = FIntPoint(-1, -1); // Невалидная начальная позиция
    bHasMoved = false; // Инициализация по умолчанию

    // Инициализация переменных для движения
    bIsMoving = false;
    MoveLerpAlpha = 0.0f;
}

void AChessPiece::BeginPlay()
{
    Super::BeginPlay();

    // Мы должны установить материал на клиенте при его создании.
    // OnRep_PieceProperties не гарантирует срабатывание для фигур,
    // чей цвет совпадает со значением по умолчанию (белый).
    // BeginPlay на клиенте вызывается после получения начальных реплицируемых свойств.
    if (GetNetMode() == NM_Client)
    {
        SetupMaterial();
    }
}

void AChessPiece::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsMoving)
    {
        // MoveLerpAlpha будет идти от 0 до 1. MoveSpeed регулирует скорость.
        MoveLerpAlpha = FMath::Clamp(MoveLerpAlpha + DeltaTime * MoveSpeed, 0.f, 1.f);

        FVector CurrentLocation;
        if (TypeOfPiece == EPieceType::Knight)
        {
            // Параболическая интерполяция для коня (прыжок)
            CurrentLocation = FMath::Lerp(StartWorldLocation, TargetWorldLocation, MoveLerpAlpha);
            // Добавляем высоту дуги. Sin(0) = 0, Sin(PI) = 0. Максимум в Sin(PI/2)=1.
            CurrentLocation.Z += KnightArcHeight * FMath::Sin(PI * MoveLerpAlpha);
        }
        else
        {
            // Линейная интерполяция для остальных фигур
            CurrentLocation = FMath::Lerp(StartWorldLocation, TargetWorldLocation, MoveLerpAlpha);
        }

        SetActorLocation(CurrentLocation);

        if (MoveLerpAlpha >= 1.0f)
        {
            // Точно устанавливаем в конце, чтобы избежать погрешностей
            SetActorLocation(TargetWorldLocation);
            bIsMoving = false;
        }
    }
}

void AChessPiece::AnimateMoveTo(const FVector& TargetLocation)
{
    if (GetActorLocation().Equals(TargetLocation, 1.f)) return;

    StartWorldLocation = GetActorLocation();
    TargetWorldLocation = TargetLocation;
    MoveLerpAlpha = 0.f;
    bIsMoving = true;
}

void AChessPiece::InitializePiece(EPieceColor InColor, EPieceType InType, FIntPoint InBoardPosition)
{
    PieceColor = InColor;
    TypeOfPiece = InType;
    BoardPosition = InBoardPosition;
    bHasMoved = false; // Сбрасываем флаг при инициализации/рестарте

    // Устанавливаем материал
    SetupMaterial();

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

TArray<FIntPoint> AChessPiece::GetValidMoves(const AChessGameState* GameState, const AChessBoard* Board) const
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

bool AChessPiece::HasMoved() const
{
    return bHasMoved;
}

void AChessPiece::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AChessPiece, PieceColor);
    DOREPLIFETIME(AChessPiece, TypeOfPiece);
    DOREPLIFETIME(AChessPiece, BoardPosition);
    DOREPLIFETIME(AChessPiece, bHasMoved);
}

void AChessPiece::OnRep_PieceProperties()
{
    // Эта функция вызывается на клиентах, когда реплицируются PieceColor или TypeOfPiece.
    SetupMaterial();
}

void AChessPiece::OnRep_BoardPosition()
{
    // Эта функция вызывается на клиентах, когда реплицируется BoardPosition.
    // Мы должны визуально переместить фигуру в новое место.
    if (AChessBoard* Board = Cast<AChessBoard>(UGameplayStatics::GetActorOfClass(GetWorld(), AChessBoard::StaticClass())))
    {
        const FVector TargetLocation = Board->GridToWorldPosition(BoardPosition);
        AnimateMoveTo(TargetLocation);
    }
}

void AChessPiece::SetupMaterial()
{
    // Предотвращаем повторное создание материала, если он уже есть
    if (DynamicMaterialInstance)
    {
        return;
    }

    if (PieceMeshComponent)
    {
        UMaterialInterface* BaseMaterial = (PieceColor == EPieceColor::White) ? WhiteMaterial : BlackMaterial;

        if (BaseMaterial)
        {
            DynamicMaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
            PieceMeshComponent->SetMaterial(0, DynamicMaterialInstance);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessPiece::SetupMaterial: Base material not set for %s %s. Cannot create dynamic material instance."),
                (PieceColor == EPieceColor::White ? TEXT("White") : TEXT("Black")),
                *UEnum::GetValueAsString(TypeOfPiece));
        }
    }
}
