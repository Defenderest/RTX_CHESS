#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "MenuCameraActor.generated.h"

UCLASS()
class RTX_CHESS_API AMenuCameraActor : public ACameraActor
{
    GENERATED_BODY()

public:
    AMenuCameraActor();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    /** The speed at which the camera rotates. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Camera")
    float RotationSpeed = 10.0f;

    /** The maximum yaw angle from the initial rotation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu Camera")
    float MaxYawOffset = 45.0f;

private:
    /** Initial rotation of the camera when the game starts. */
    FRotator InitialRotation;

    /** Direction of rotation. 1 for clockwise, -1 for counter-clockwise. */
    int32 RotationDirection = 1;
};
