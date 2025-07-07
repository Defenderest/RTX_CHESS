#include "PlayerPawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"

APlayerPawn::APlayerPawn()
{
    // Создаем корневой компонент для прикрепления других компонентов
    USceneComponent* RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootComp;

    // Создаем компонент скелетной сетки для модели игрока
    PlayerMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PlayerMesh"));
    PlayerMesh->SetupAttachment(RootComponent);
}
