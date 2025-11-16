#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PlayerPawn.generated.h"

class USkeletalMeshComponent;
class UAnimationAsset;

UCLASS()
class RTX_CHESS_API APlayerPawn : public APawn
{
	GENERATED_BODY()

public:
	APlayerPawn();

	// Вызывается каждый кадр
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Воспроизводит анимацию хода.
	void PlayMoveAnimation();

protected:
	// Визуальное представление игрока.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> PlayerMesh;

	// Анимация, которая будет воспроизводиться при совершении хода.
	// Назначьте UAnimMontage или UAnimSequence в редакторе.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimationAsset> MoveAnimation;

	// Начальный поворот для PlayerMesh, чтобы скорректировать ориентацию модели.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pawn", meta = (DisplayName = "Initial Mesh Rotation"))
	FRotator InitialMeshRotation = FRotator(90.f, -90.f, 0.f);

	// Целевое вращение для головы в мировом пространстве. Используется в AnimBP для поворота головы в сторону взгляда игрока.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Pawn", meta = (DisplayName = "Head Target World Rotation"))
	FRotator HeadTargetWorldRotation;
};
