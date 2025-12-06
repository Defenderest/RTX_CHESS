#include "Actors/PlayerPawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Animation/AnimInstance.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h" // Для вывода отладочных сообщений на экран

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

    // Корректируем начальный поворот модели, чтобы она стояла прямо и смотрела вперед.
    // Это необходимо, если модель была создана в программе с другой системой координат (например, Y-Up вместо Z-Up).
    PlayerMesh->SetRelativeRotation(InitialMeshRotation);
}

void APlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Эта логика должна выполняться только на компьютере игрока, который управляет этим персонажем.
	if (IsLocallyControlled())
	{
		if (Controller)
		{
			// Получаем вращение камеры/контроллера.
			const FRotator ControlRotation = Controller->GetControlRotation();

			// Мы хотим, чтобы голова поворачивалась вверх-вниз (Pitch) и влево-вправо (Yaw),
			// но не наклонялась вбок (Roll), так как это выглядит неестественно.
			// Создаем новое вращение, обнулив компонент Roll.
			const FRotator TargetHeadRotation = FRotator(ControlRotation.Pitch, ControlRotation.Yaw, 0.0f);

			// Присваиваем рассчитанное вращение. Это значение будет реплицировано другим игрокам.
			// В AnimGraph мы будем использовать это значение напрямую в "World Space".
			HeadTargetWorldRotation = TargetHeadRotation;
			
			// Лог для локально управляемого пеона. Будет желтым.
			if (GEngine)
			{
				//const FString DebugText = FString::Printf(TEXT("LOCAL HeadTargetWorldRotation: %s"), *HeadTargetWorldRotation.ToString());
				//GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Yellow, DebugText);
			}
		}
		else
		{
			// Лог, если нет контроллера. Будет красным.
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(1, 5.f, FColor::Red, TEXT("LOCAL PAWN HAS NO CONTROLLER"));
			}
		}
	}
	else
	{
		// Для удаленных пеонов мы просто выводим текущее реплицированное значение.
		if (GEngine)
		{
			const FString DebugText = FString::Printf(TEXT("REMOTE HeadTargetWorldRotation: %s"), *HeadTargetWorldRotation.ToString());
			GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Cyan, DebugText);
		}
	}
}

void APlayerPawn::PlayMoveAnimation()
{
	if (PlayerMesh && MoveAnimation)
	{
		// Используем PlayAnimation, так как это проще и работает с UAnimationAsset.
		// Для более сложных сценариев со смешиванием лучше использовать монтажи.
		PlayerMesh->PlayAnimation(MoveAnimation, false);
	}
}

void APlayerPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APlayerPawn, HeadTargetWorldRotation);
}
