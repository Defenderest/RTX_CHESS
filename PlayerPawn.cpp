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
		// Получаем мировое вращение контроллера (камеры) в виде кватерниона
		const FQuat ControlRotation = Controller->GetControlRotation().Quaternion();

		// Получаем мировое преобразование компонента сетки
		const FTransform MeshComponentTransform = PlayerMesh->GetComponentTransform();
		
		// Преобразуем мировое вращение камеры в локальное пространство компонента сетки.
		// Это даст нам вращение "взгляда" относительно ориентации самого персонажа.
		const FQuat LocalHeadRotation = MeshComponentTransform.InverseTransformRotation(ControlRotation);
		
		// Сохраняем результат в виде FRotator.
		HeadLookRotation = LocalHeadRotation.Rotator();
		
		// Обнуляем крен (Roll), чтобы голова не наклонялась вбок.
		HeadLookRotation.Roll = 0.f;
	}
	else
	{
		// Если контроллера нет, сбрасываем вращение
		HeadLookRotation = FRotator::ZeroRotator;
	}
}
