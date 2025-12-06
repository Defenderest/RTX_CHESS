#include "Core/ChessClock.h"
#include "Components/StaticMeshComponent.h"

AChessClock::AChessClock()
{
	PrimaryActorTick.bCanEverTick = true;

	WhitePlayerTimeSeconds = 300.f;
	BlackPlayerTimeSeconds = 300.f;
	ActivePlayerColor = EPieceColor::White;
	bIsClockRunning = true;

	HandRotationAxis = EClockHandRotationAxis::Roll;

	WhiteClockHandsPivotLocation = FVector::ZeroVector;
	BlackClockHandsPivotLocation = FVector::ZeroVector;
	MinuteHandMeshOffset = FVector::ZeroVector;
	SecondHandMeshOffset = FVector::ZeroVector;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	ClockBodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClockBodyMesh"));
	ClockBodyMesh->SetupAttachment(RootComponent);

	WhiteMinuteHandPivot = CreateDefaultSubobject<USceneComponent>(TEXT("WhiteMinuteHandPivot"));
	WhiteMinuteHandPivot->SetupAttachment(ClockBodyMesh);

	WhiteMinuteHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WhiteMinuteHandMesh"));
	WhiteMinuteHandMesh->SetupAttachment(WhiteMinuteHandPivot);

	WhiteSecondHandPivot = CreateDefaultSubobject<USceneComponent>(TEXT("WhiteSecondHandPivot"));
	WhiteSecondHandPivot->SetupAttachment(ClockBodyMesh);

	WhiteSecondHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WhiteSecondHandMesh"));
	WhiteSecondHandMesh->SetupAttachment(WhiteSecondHandPivot);

	BlackMinuteHandPivot = CreateDefaultSubobject<USceneComponent>(TEXT("BlackMinuteHandPivot"));
	BlackMinuteHandPivot->SetupAttachment(ClockBodyMesh);

	BlackMinuteHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlackMinuteHandMesh"));
	BlackMinuteHandMesh->SetupAttachment(BlackMinuteHandPivot);

	BlackSecondHandPivot = CreateDefaultSubobject<USceneComponent>(TEXT("BlackSecondHandPivot"));
	BlackSecondHandPivot->SetupAttachment(ClockBodyMesh);

	BlackSecondHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlackSecondHandMesh"));
	BlackSecondHandMesh->SetupAttachment(BlackSecondHandPivot);

	WhiteMinuteHandPivot->SetRelativeLocation(WhiteClockHandsPivotLocation);
	WhiteSecondHandPivot->SetRelativeLocation(WhiteClockHandsPivotLocation);

	BlackMinuteHandPivot->SetRelativeLocation(BlackClockHandsPivotLocation);
	BlackSecondHandPivot->SetRelativeLocation(BlackClockHandsPivotLocation);

	WhiteMinuteHandMesh->SetRelativeLocation(MinuteHandMeshOffset);
	WhiteSecondHandMesh->SetRelativeLocation(SecondHandMeshOffset);
	
	BlackMinuteHandMesh->SetRelativeLocation(MinuteHandMeshOffset);
	BlackSecondHandMesh->SetRelativeLocation(SecondHandMeshOffset);
}

void AChessClock::BeginPlay()
{
	Super::BeginPlay();
	UpdateClockHands();
}

void AChessClock::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsClockRunning)
	{
		if (ActivePlayerColor == EPieceColor::White)
		{
			WhitePlayerTimeSeconds = FMath::Max(0.f, WhitePlayerTimeSeconds - DeltaTime);
		}
		else
		{
			BlackPlayerTimeSeconds = FMath::Max(0.f, BlackPlayerTimeSeconds - DeltaTime);
		}
	}
	
	UpdateClockHands();
}

void AChessClock::UpdateClockHands()
{
	if (WhiteMinuteHandPivot && WhiteSecondHandPivot)
	{
		const float SecondHandAngle = (FMath::Fmod(WhitePlayerTimeSeconds, 60.f) / 60.f) * -360.f;
		const float MinuteHandAngle = (FMath::Fmod(WhitePlayerTimeSeconds, 3600.f) / 3600.f) * -360.f;

		FRotator SecondHandRotator;
		FRotator MinuteHandRotator;

		switch (HandRotationAxis)
		{
		case EClockHandRotationAxis::Pitch:
			SecondHandRotator = FRotator(SecondHandAngle, 0.f, 0.f);
			MinuteHandRotator = FRotator(MinuteHandAngle, 0.f, 0.f);
			break;
		case EClockHandRotationAxis::Yaw:
			SecondHandRotator = FRotator(0.f, SecondHandAngle, 0.f);
			MinuteHandRotator = FRotator(0.f, MinuteHandAngle, 0.f);
			break;
		case EClockHandRotationAxis::Roll:
		default:
			SecondHandRotator = FRotator(0.f, 0.f, SecondHandAngle);
			MinuteHandRotator = FRotator(0.f, 0.f, MinuteHandAngle);
			break;
		}

		WhiteSecondHandPivot->SetRelativeRotation(SecondHandRotator);
		WhiteMinuteHandPivot->SetRelativeRotation(MinuteHandRotator);
	}

	if (BlackMinuteHandPivot && BlackSecondHandPivot)
	{
		const float SecondHandAngle = (FMath::Fmod(BlackPlayerTimeSeconds, 60.f) / 60.f) * -360.f;
		const float MinuteHandAngle = (FMath::Fmod(BlackPlayerTimeSeconds, 3600.f) / 3600.f) * -360.f;

		FRotator SecondHandRotator;
		FRotator MinuteHandRotator;

		switch (HandRotationAxis)
		{
		case EClockHandRotationAxis::Pitch:
			SecondHandRotator = FRotator(SecondHandAngle, 0.f, 0.f);
			MinuteHandRotator = FRotator(MinuteHandAngle, 0.f, 0.f);
			break;
		case EClockHandRotationAxis::Yaw:
			SecondHandRotator = FRotator(0.f, SecondHandAngle, 0.f);
			MinuteHandRotator = FRotator(0.f, MinuteHandAngle, 0.f);
			break;
		case EClockHandRotationAxis::Roll:
		default:
			SecondHandRotator = FRotator(0.f, 0.f, SecondHandAngle);
			MinuteHandRotator = FRotator(0.f, 0.f, MinuteHandAngle);
			break;
		}
		
		BlackSecondHandPivot->SetRelativeRotation(SecondHandRotator);
		BlackMinuteHandPivot->SetRelativeRotation(MinuteHandRotator);
	}
}
