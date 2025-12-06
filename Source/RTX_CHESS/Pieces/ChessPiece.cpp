#include "Pieces/ChessPiece.h"
#include "Board/ChessBoard.h" // Для использования AChessBoard в GetValidMoves
#include "Core/ChessGameMode.h"
#include "Components/StaticMeshComponent.h" // Для UStaticMeshComponent
#include "Components/SceneComponent.h"
#include "Particles/ParticleSystemComponent.h"
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
    SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
    RootComponent = SceneRootComponent;

    // Создаем компонент меша и присоединяем его к корневому компоненту.
    PieceMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PieceMesh"));
    PieceMeshComponent->SetupAttachment(RootComponent);

    // Создаем компонент эффекта сгорания и присоединяем его к мешу.
    BurnEffectComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("BurnEffectComponent"));
    BurnEffectComponent->SetupAttachment(PieceMeshComponent);
    BurnEffectComponent->bAutoActivate = false;
    
    // Настраиваем коллизию меша для взаимодействия и физики.
    PieceMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    PieceMeshComponent->SetCollisionObjectType(ECC_PhysicsBody); // Устанавливаем как физический объект
    PieceMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
    PieceMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // Для кликов
    PieceMeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); // Для столкновений с доской
    PieceMeshComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block); // Для столкновений с другими фигурами
    PieceMeshComponent->CanCharacterStepUpOn = ECB_No;
    PieceMeshComponent->SetSimulatePhysics(false); // Физика включается только при взятии
    PieceMeshComponent->GetBodyInstance()->bUseCCD = true; // Для предотвращения прохождения сквозь объекты

    // Устанавливаем физические свойства для красивого падения
    if (FBodyInstance* BodyInstance = PieceMeshComponent->GetBodyInstance())
    {
        BodyInstance->bOverrideMass = true;
        BodyInstance->SetMassOverride(MassInKg);
        BodyInstance->LinearDamping = LinearDamping;
        BodyInstance->AngularDamping = AngularDamping;
        BodyInstance->COMNudge = CenterOfMassOffset;
    }


    PieceColor = EPieceColor::White;
    TypeOfPiece = EPieceType::Pawn;
    BoardPosition = FIntPoint(-1, -1); // Невалидная начальная позиция
    bHasMoved = false; // Инициализация по умолчанию
    bIsCaptured = false;

    // Инициализация переменных для движения
    bIsMoving = false;
    MoveLerpAlpha = 0.0f;
    TimeSpentStationary = 0.f;
    bIsBurning = false;
}

void AChessPiece::HandleCapture(AChessPiece* CapturingPiece, const FVector& BoardCenter, float ImpulseStrength, float TorqueStrength)
{
    if (bIsCaptured || !CapturingPiece)
    {
        return; // Уже обработано или невалидный ввод
    }
    bIsCaptured = true;

    // --- Вычисляем комбинированное направление отталкивания ---
    // 1. Основное направление - от атакующей фигуры.
    FVector DirectionFromAttacker = GetActorLocation() - CapturingPiece->GetActorLocation();
    DirectionFromAttacker.Z = 0; // Игнорируем разницу в высоте для основного направления

    // Предохранитель на случай, если фигуры окажутся в одной точке
    if (DirectionFromAttacker.IsNearlyZero())
    {
        DirectionFromAttacker = GetActorForwardVector(); // Просто толкаем вперед
    }
    DirectionFromAttacker.Normalize();

    // 2. Корректирующее направление - от центра доски, чтобы гарантировать вылет за пределы.
    FVector DirectionFromCenter = GetActorLocation() - BoardCenter;
    DirectionFromCenter.Z = 0; // Также работаем в 2D
    DirectionFromCenter.Normalize();

    // Смешиваем два направления с весами. Основной толчок (70%) идет от атакующего, 
    // остальное (30%) - от центра доски для коррекции.
    FVector CombinedDirection = (DirectionFromAttacker * 0.7f + DirectionFromCenter * 0.3f).GetSafeNormal();

    // Добавляем небольшой случайный импульс вверх, чтобы фигура подпрыгнула
    CombinedDirection.Z = FMath::RandRange(0.4f, 0.6f);
    CombinedDirection.Normalize();

    Multicast_ApplyPhysicsAndImpulse(CombinedDirection, ImpulseStrength, TorqueStrength);
}

void AChessPiece::Multicast_ApplyPhysicsAndImpulse_Implementation(const FVector& ImpulseDirection, float ImpulseStrength, float TorqueStrength)
{
    if (PieceMeshComponent)
    {
        // Включаем симуляцию физики
        PieceMeshComponent->SetSimulatePhysics(true);
        // Применяем импульс
        PieceMeshComponent->AddImpulse(ImpulseDirection * ImpulseStrength);

        // Применяем крутящий момент, чтобы фигура упала на бок
        const FVector TorqueAxis = FVector::CrossProduct(ImpulseDirection, GetActorUpVector()).GetSafeNormal();
        PieceMeshComponent->AddTorqueInRadians(TorqueAxis * TorqueStrength);
        
        // Отключаем коллизию с каналом Visibility, чтобы нельзя было кликнуть на летящую фигуру
        PieceMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
        // Отключаем коллизию с другими фигурами, чтобы избежать застревания
        PieceMeshComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);

        // Запускаем таймер самоуничтожения
        GetWorldTimerManager().SetTimer(SelfDestructTimerHandle, this, &AChessPiece::SelfDestruct, 5.0f, false);
    }
}

void AChessPiece::Multicast_StartBurnEffect_Implementation(float Duration, const FVector& Scale)
{
    if (BurnEffectComponent && BurnEffect)
    {
        BurnEffectComponent->SetTemplate(BurnEffect);
        BurnEffectComponent->SetRelativeScale3D(Scale); // Используем RelativeScale для большей надежности с присоединенными компонентами
        BurnEffectComponent->Activate(true);
    }

    if (PieceMeshComponent)
    {
        PieceMeshComponent->SetSimulatePhysics(false);
        // При желании можно сделать фигуру невидимой, чтобы эффект горения был чище, раскомментировав строку ниже
        // PieceMeshComponent->SetVisibility(false); 
    }

    // Отменяем старый таймер самоуничтожения (который был на 5 секунд)
    GetWorldTimerManager().ClearTimer(SelfDestructTimerHandle);
    // Устанавливаем новый короткий таймер на уничтожение, используя длительность из GameMode
    GetWorldTimerManager().SetTimer(SelfDestructTimerHandle, this, &AChessPiece::SelfDestruct, Duration, false);
}

void AChessPiece::SelfDestruct()
{
    if (IsValid(this))
    {
        Destroy();
    }
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

    // Серверная логика для обнаружения, когда захваченная фигура должна загореться
    if (HasAuthority() && bIsCaptured && !bIsBurning && PieceMeshComponent && PieceMeshComponent->IsSimulatingPhysics())
    {
        // Проверяем, остановилась ли фигура
        if (PieceMeshComponent->GetPhysicsLinearVelocity().SizeSquared() < 10.f) // Пороговое значение для "покоя"
        {
            TimeSpentStationary += DeltaTime;
            // Если фигура стоит на месте более 0.25с, запускаем эффект сгорания
            if (TimeSpentStationary > 0.25f)
            {
                bIsBurning = true; // Помечаем, чтобы не делать этого снова
                
                if (const AChessGameMode* GameMode = GetWorld()->GetAuthGameMode<AChessGameMode>())
                {
                    const float Duration = GameMode->GetBurnEffectDuration();
                    const FVector Scale = GameMode->GetBurnEffectScale();
                    Multicast_StartBurnEffect(Duration, Scale);
                }
                else
                {
                    // Запасной вариант, если GameMode не найден
                    Multicast_StartBurnEffect(1.5f, FVector(1.0f));
                }
            }
        }
        else
        {
            // Сбрасываем таймер, если фигура снова движется (например, катится)
            TimeSpentStationary = 0.f;
        }
    }

    // Если фигура захвачена и симулирует физику, обновляем трансформацию актора, чтобы он следовал за мешем.
    // Это нужно, чтобы другие компоненты (например, эффекты) оставались в правильной позиции.
    if (bIsCaptured && PieceMeshComponent->IsSimulatingPhysics())
    {
        SetActorLocationAndRotation(PieceMeshComponent->GetComponentLocation(), PieceMeshComponent->GetComponentRotation());
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
    if (PieceMeshComponent)
    {
        PieceMeshComponent->SetRenderCustomDepth(true);
    }
}

void AChessPiece::OnDeselected_Implementation()
{
    if (DynamicMaterialInstance)
    {
        DynamicMaterialInstance->SetScalarParameterValue(FName("Highlight"), 0.0f);
    }
    if (PieceMeshComponent)
    {
        PieceMeshComponent->SetRenderCustomDepth(false);
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
    DOREPLIFETIME(AChessPiece, bIsCaptured);
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
