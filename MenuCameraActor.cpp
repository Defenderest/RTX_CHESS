#include "MenuCameraActor.h"
#include "Kismet/KismetMathLibrary.h"

AMenuCameraActor::AMenuCameraActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AMenuCameraActor::BeginPlay()
{
    Super::BeginPlay();
    InitialRotation = GetActorRotation();
}

void AMenuCameraActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Используем осцилляцию на основе синусоиды, чтобы избежать "борьбы" за управление вращением камеры
    // с другими частями системы, такими как PlayerCameraManager.
    // Этот метод рассчитывает вращение абсолютно на основе времени, а не инкрементально,
    // что делает его устойчивым к внешним изменениям и предотвращает дрожание.
    const float CurrentTime = GetWorld()->GetTimeSeconds();
    const float YawOffset = FMath::Sin(CurrentTime * RotationSpeed) * MaxYawOffset;

    FRotator NewRotation = InitialRotation;
    NewRotation.Yaw += YawOffset;

    SetActorRotation(NewRotation);
}
