#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "ChessPlayerCameraManager.generated.h"

UCLASS()
class RTX_CHESS_API AChessPlayerCameraManager : public APlayerCameraManager
{
    GENERATED_BODY()

public:
    AChessPlayerCameraManager();

    // Здесь вы можете переопределить функции стандартного PlayerCameraManager
    // Например, для изменения логики обновления камеры:
    // virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

    // Также можно добавить свои свойства, настраиваемые в Blueprint
    // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera")
    // float CustomCameraDistance;
};
