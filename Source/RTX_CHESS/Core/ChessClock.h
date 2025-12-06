#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pieces/ChessPiece.h"
#include "ChessClock.generated.h"

UENUM(BlueprintType)
enum class EClockHandRotationAxis : uint8
{
	Pitch UMETA(DisplayName = "Pitch (Rotation around Y-axis)"),
	Yaw   UMETA(DisplayName = "Yaw (Rotation around Z-axis)"),
	Roll  UMETA(DisplayName = "Roll (Rotation around X-axis)")
};

UCLASS()
class RTX_CHESS_API AChessClock : public AActor
{
	GENERATED_BODY()
	
public:	
	AChessClock();

protected:
	virtual void BeginPlay() override;

	// Updates the rotation of the clock hands based on the current time
	void UpdateClockHands();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ClockBodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> WhiteMinuteHandPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> WhiteSecondHandPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> BlackMinuteHandPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> BlackSecondHandPivot;
	
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

	// Axis of rotation for the clock hands
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Clock|Visuals", meta = (AllowPrivateAccess = "true"))
	EClockHandRotationAxis HandRotationAxis;

	// The local position for the pivot point of the white player's clock hands
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Clock|Visuals", meta = (AllowPrivateAccess = "true"))
	FVector WhiteClockHandsPivotLocation;

	// The local position for the pivot point of the black player's clock hands
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Clock|Visuals", meta = (AllowPrivateAccess = "true"))
	FVector BlackClockHandsPivotLocation;

	// The local offset for the minute hand mesh from its pivot point
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Clock|Visuals", meta = (AllowPrivateAccess = "true"))
	FVector MinuteHandMeshOffset;

	// The local offset for the second hand mesh from its pivot point
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Clock|Visuals", meta = (AllowPrivateAccess = "true"))
	FVector SecondHandMeshOffset;

	// Whether the clock is currently running
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Clock", meta = (AllowPrivateAccess = "true"))
	bool bIsClockRunning;

public:	
	virtual void Tick(float DeltaTime) override;

};
