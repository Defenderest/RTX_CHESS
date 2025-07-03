#include "ChessPiece.h"
#include "ChessBoard.h" // Для использования AChessBoard в GetValidMoves
#include "Components/StaticMeshComponent.h" // Для UStaticMeshComponent
#include "Engine/StaticMesh.h" // Для UStaticMesh
#include "Materials/MaterialInterface.h" // Для UMaterialInterface

AChessPiece::AChessPiece()
{
    PrimaryActorTick.bCanEverTick = true; // Включаем Tick для анимации движения

    // Создаем компонент меша и делаем его корневым компонентом.
    PieceMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PieceMesh"));
    RootComponent = PieceMeshComponent;
    
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
    // Отладочный лог был перенесен в GameMode для большей точности,
    // так как BeginPlay может вызываться до полной инициализации фигуры.
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
