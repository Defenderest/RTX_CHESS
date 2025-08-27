// Fill out your copyright notice in the Description page of Project Settings.


#include "ChessClock.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AChessClock::AChessClock()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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
	
}

// Called every frame
void AChessClock::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
