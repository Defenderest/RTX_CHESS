#include "PlayerPawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"

APlayerPawn::APlayerPawn()
{
	// Разрешаем этому пеону тикать каждый кадр.
	PrimaryActorTick.bCanEverTick = true;

    // Создаем корневой компонент для прикрепления других компонентов
    USceneComponent* RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootComp;

    // Создаем компонент скелетной сетки для модели игрока
    PlayerMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PlayerMesh"));
    PlayerMesh->SetupAttachment(RootComponent);
}

void APlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Controller && PlayerMesh)
	{
		// Просто получаем мировое вращение контроллера (камеры).
		// AnimBP будет настроен для использования этого мирового вращения.
		HeadLookRotation = Controller->GetControlRotation();
	}
	else
	{
		// Если контроллера нет, сбрасываем вращение
		HeadLookRotation = FRotator::ZeroRotator;
	}
}
