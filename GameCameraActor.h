#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "ChessPiece.h" // Для EPieceColor
#include "GameCameraActor.generated.h"

// Структура для хранения настроек камеры для одной перспективы
USTRUCT(BlueprintType)
struct FCameraSetup
{
    GENERATED_BODY()

    // Смещение камеры относительно центра доски
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Setup")
    FVector LocationOffset = FVector(0.f, -1200.f, 1000.f);

    // Поворот камеры
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Setup")
    FRotator Rotation = FRotator(-45.f, 0.f, 0.f);

    // Поле зрения камеры
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Setup")
    float FOV = 90.f;
};


/**
 * A camera actor that holds different perspective settings for the chess game.
 * The PlayerController will find this camera and use it.
 */
UCLASS()
class RTX_CHESS_API AGameCameraActor : public ACameraActor
{
	GENERATED_BODY()

public:
    AGameCameraActor();

	// Возвращает настройки камеры для данного цвета игрока.
	const FCameraSetup& GetCameraSetupForColor(EPieceColor PlayerColor) const;

protected:
    // Настройки камеры для перспективы белого игрока
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera")
    FCameraSetup WhitePlayerCameraSetup;

    // Настройки камеры для перспективы черного игрока
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera")
    FCameraSetup BlackPlayerCameraSetup;
};
