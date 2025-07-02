#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "GameCameraActor.generated.h"

/**
 * A simple camera actor class to be placed in the level.
 * The PlayerController will find this camera and use it.
 */
UCLASS()
class RTX_CHESS_API AGameCameraActor : public ACameraActor
{
    GENERATED_BODY()
};
