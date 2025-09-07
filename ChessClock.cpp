// Fill out your copyright notice in the Description page of Project Settings.


#include "ChessClock.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AChessClock::AChessClock()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Initialize clock properties
	WhitePlayerTimeSeconds = 300.f;
	BlackPlayerTimeSeconds = 300.f;
	ActivePlayerColor = EPieceColor::White;
	bIsClockRunning = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	ClockBodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClockBodyMesh"));
	ClockBodyMesh->SetupAttachment(RootComponent);

	WhiteMinuteHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WhiteMinuteHandMesh"));
	WhiteMinuteHandMesh->SetupAttachment(ClockBodyMesh);

	WhiteSecondHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WhiteSecondHandMesh"));
	WhiteSecondHandMesh->SetupAttachment(ClockBodyMesh);

	BlackMinuteHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlackMinuteHandMesh"));
	BlackMinuteHandMesh->SetupAttachment(ClockBodyMesh);

	BlackSecondHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlackSecondHandMesh"));
	BlackSecondHandMesh->SetupAttachment(ClockBodyMesh);
}

// Called when the game starts or when spawned
void AChessClock::BeginPlay()
{
	Super::BeginPlay();
	UpdateClockHands();
}

// Called every frame
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
	// Update White Player's clock hands
	if (WhiteMinuteHandMesh && WhiteSecondHandMesh)
	{
		// 6 degrees per second
		const float SecondHandPitch = (FMath::Fmod(WhitePlayerTimeSeconds, 60.f) / 60.f) * -360.f;
		// 6 degrees per minute, moves smoothly
		const float MinuteHandPitch = (FMath::Fmod(WhitePlayerTimeSeconds, 3600.f) / 3600.f) * -360.f;

		WhiteSecondHandMesh->SetRelativeRotation(FRotator(SecondHandPitch, 0.f, 0.f));
		WhiteMinuteHandMesh->SetRelativeRotation(FRotator(MinuteHandPitch, 0.f, 0.f));
	}

	// Update Black Player's clock hands
	if (BlackMinuteHandMesh && BlackSecondHandMesh)
	{
		const float SecondHandPitch = (FMath::Fmod(BlackPlayerTimeSeconds, 60.f) / 60.f) * -360.f;
		const float MinuteHandPitch = (FMath::Fmod(BlackPlayerTimeSeconds, 3600.f) / 3600.f) * -360.f;
		
		BlackSecondHandMesh->SetRelativeRotation(FRotator(SecondHandPitch, 0.f, 0.f));
		BlackMinuteHandMesh->SetRelativeRotation(FRotator(MinuteHandPitch, 0.f, 0.f));
	}
}
