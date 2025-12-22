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
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Оновлює обертання стрілок */
	void UpdateClockHands();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ClockBodyMesh;

	/** Компоненти-півоти для стрілок (їх треба обертати) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> WhiteMinuteHandPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> WhiteSecondHandPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> BlackMinuteHandPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> BlackSecondHandPivot;
	
	/** Самі моделі стрілок */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> WhiteMinuteHandMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> WhiteSecondHandMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BlackMinuteHandMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BlackSecondHandMesh;

	/** Вісь обертання стрілок */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Clock|Visuals")
	EClockHandRotationAxis HandRotationAxis;

private:
	float WhitePlayerTimeSeconds;
	float BlackPlayerTimeSeconds;
	EPieceColor ActivePlayerColor;
	bool bIsClockRunning;

public:	
	virtual void Tick(float DeltaTime) override;
};