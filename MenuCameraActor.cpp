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

    FRotator CurrentRotation = GetActorRotation();
    float YawChange = RotationSpeed * DeltaTime * RotationDirection;
    FRotator NewRotation = CurrentRotation + FRotator(0.f, YawChange, 0.f);

    // Normalize the angles to keep them within a predictable range.
    float DeltaYaw = FMath::FindDeltaAngleDegrees(InitialRotation.Yaw, NewRotation.Yaw);

    if (RotationDirection == 1 && DeltaYaw > MaxYawOffset)
    {
        RotationDirection = -1;
        // Clamp to the max offset to prevent overshooting
        NewRotation.Yaw = InitialRotation.Yaw + MaxYawOffset;
    }
    else if (RotationDirection == -1 && DeltaYaw < -MaxYawOffset)
    {
        RotationDirection = 1;
        // Clamp to the min offset to prevent overshooting
        NewRotation.Yaw = InitialRotation.Yaw - MaxYawOffset;
    }

    SetActorRotation(NewRotation);
}
