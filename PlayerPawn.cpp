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

	if (Controller)
	{
		// Получаем вращение контроллера (камеры)
		const FRotator ControlRotation = Controller->GetControlRotation();

		// Получаем вращение самого пеона
		const FRotator ActorRotation = GetActorRotation();

		// Вычисляем дельту вращения для головы. Нам нужны только Pitch и Yaw.
		// Результат будет в координатах, относительных к пеону.
		HeadLookRotation = FRotator(ControlRotation.Pitch, ControlRotation.Yaw, 0.f) - ActorRotation;
		HeadLookRotation.Normalize();
	}
	else
	{
		// Если контроллера нет, сбрасываем вращение
		HeadLookRotation = FRotator::ZeroRotator;
	}
}
