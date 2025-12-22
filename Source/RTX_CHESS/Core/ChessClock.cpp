#include "Core/ChessClock.h"
#include "Components/StaticMeshComponent.h"
#include "Core/ChessGameState.h"
#include "Kismet/GameplayStatics.h"

AChessClock::AChessClock()
{
	PrimaryActorTick.bCanEverTick = true;

	HandRotationAxis = EClockHandRotationAxis::Roll;
	WhitePlayerTimeSeconds = 0.f;
	BlackPlayerTimeSeconds = 0.f;
	bIsClockRunning = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	ClockBodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClockBodyMesh"));
	ClockBodyMesh->SetupAttachment(RootComponent);

	// Ініціалізація білих стрілок
	WhiteMinuteHandPivot = CreateDefaultSubobject<USceneComponent>(TEXT("WhiteMinuteHandPivot"));
	WhiteMinuteHandPivot->SetupAttachment(ClockBodyMesh);
	WhiteMinuteHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WhiteMinuteHandMesh"));
	WhiteMinuteHandMesh->SetupAttachment(WhiteMinuteHandPivot);

	WhiteSecondHandPivot = CreateDefaultSubobject<USceneComponent>(TEXT("WhiteSecondHandPivot"));
	WhiteSecondHandPivot->SetupAttachment(ClockBodyMesh);
	WhiteSecondHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WhiteSecondHandMesh"));
	WhiteSecondHandMesh->SetupAttachment(WhiteSecondHandPivot);

	// Ініціалізація чорних стрілок
	BlackMinuteHandPivot = CreateDefaultSubobject<USceneComponent>(TEXT("BlackMinuteHandPivot"));
	BlackMinuteHandPivot->SetupAttachment(ClockBodyMesh);
	BlackMinuteHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlackMinuteHandMesh"));
	BlackMinuteHandMesh->SetupAttachment(BlackMinuteHandPivot);

	BlackSecondHandPivot = CreateDefaultSubobject<USceneComponent>(TEXT("BlackSecondHandPivot"));
	BlackSecondHandPivot->SetupAttachment(ClockBodyMesh);
	BlackSecondHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlackSecondHandMesh"));
	BlackSecondHandMesh->SetupAttachment(BlackSecondHandPivot);
}

void AChessClock::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateClockHands();
}

void AChessClock::BeginPlay()
{
	Super::BeginPlay();
}

void AChessClock::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (AChessGameState* GS = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr)
	{
		WhitePlayerTimeSeconds = GS->WhiteTimeSeconds;
		BlackPlayerTimeSeconds = GS->BlackTimeSeconds;
		
		const EGamePhase CurrentPhase = GS->GetGamePhase();
		bIsClockRunning = (CurrentPhase == EGamePhase::InProgress || CurrentPhase == EGamePhase::Check);
	}
	
	UpdateClockHands();
}

void AChessClock::UpdateClockHands()
{
	auto RotateSide = [&](USceneComponent* MinPivot, USceneComponent* SecPivot, float Seconds)
	{
		if (!MinPivot || !SecPivot) return;

		// Якщо час необмежений (-1), ставимо на 0
		float DisplayTime = FMath::Max(0.f, Seconds);

		const float SecAngle = (FMath::Fmod(DisplayTime, 60.f) / 60.f) * 360.f;
		const float MinAngle = (FMath::Fmod(DisplayTime, 3600.f) / 3600.f) * 360.f;

		FRotator SecRot(0, 0, 0), MinRot(0, 0, 0);

		switch (HandRotationAxis)
		{
		case EClockHandRotationAxis::Pitch:
			SecRot.Pitch = SecAngle; MinRot.Pitch = MinAngle; break;
		case EClockHandRotationAxis::Yaw:
			SecRot.Yaw = SecAngle; MinRot.Yaw = MinAngle; break;
		case EClockHandRotationAxis::Roll:
		default:
			SecRot.Roll = SecAngle; MinRot.Roll = MinAngle; break;
		}

		SecPivot->SetRelativeRotation(SecRot);
		MinPivot->SetRelativeRotation(MinRot);
	};

	RotateSide(WhiteMinuteHandPivot, WhiteSecondHandPivot, WhitePlayerTimeSeconds);
	RotateSide(BlackMinuteHandPivot, BlackSecondHandPivot, BlackPlayerTimeSeconds);
}