// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChessPiece.h"
#include "ChessClock.generated.h"

UCLASS()
class RTX_CHESS_API AChessClock : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AChessClock();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Updates the rotation of the clock hands based on the current time
	void UpdateClockHands();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ClockBodyMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> WhiteMinuteHandMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> WhiteSecondHandMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BlackMinuteHandMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BlackSecondHandMesh;

	// Total remaining time for each player in seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Clock", meta = (AllowPrivateAccess = "true"))
	float WhitePlayerTimeSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Clock", meta = (AllowPrivateAccess = "true"))
	float BlackPlayerTimeSeconds;

	// The player whose clock is currently running
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Clock", meta = (AllowPrivateAccess = "true"))
	EPieceColor ActivePlayerColor;

	// Whether the clock is currently running
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Clock", meta = (AllowPrivateAccess = "true"))
	bool bIsClockRunning;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
