#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PlayerPawn.generated.h"

class USkeletalMeshComponent;

UCLASS()
class RTX_CHESS_API APlayerPawn : public APawn
{
    GENERATED_BODY()

public:
    APlayerPawn();

	// Вызывается каждый кадр
	virtual void Tick(float DeltaTime) override;

protected:
    // Визуальное представление игрока.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> PlayerMesh;

	// Рассчитанное вращение для головы. Используется в AnimBP для поворота головы.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pawn")
	FRotator HeadLookRotation;
};
