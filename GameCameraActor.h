#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "ChessPiece.h" // Для EPieceColor
#include "GameCameraActor.generated.h"

/**
 * A camera actor that holds references to perspective actors for the chess game.
 * The PlayerController will find this camera and use it.
 */
UCLASS()
class RTX_CHESS_API AGameCameraActor : public ACameraActor
{
	GENERATED_BODY()

public:
    AGameCameraActor();

    // Получает transform и FOV для камеры определенного цвета игрока.
    UFUNCTION(BlueprintPure, Category = "Chess Camera")
    bool GetCameraPerspectiveForColor(EPieceColor PlayerColor, FTransform& OutTransform, float& OutFOV) const;

    float GetRotationSpeed() const;
    float GetMinPitchOffset() const;
    float GetMaxPitchOffset() const;

protected:
    /**
     * Actor, чье положение и поворот будут использоваться для камеры белого игрока.
     * Вы можете использовать пустой Actor (Empty Actor) или Target Point.
     */
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Chess Camera|Perspectives")
    AActor* WhitePerspectiveActor;

    /** Поле зрения (FOV) для камеры белого игрока. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera|Perspectives", meta = (ClampMin = "10", UIMin = "10", ClampMax = "170", UIMax = "170"))
    float WhitePlayerFOV = 90.f;

    /**
     * Actor, чье положение и поворот будут использоваться для камеры черного игрока.
     * Вы можете использовать пустой Actor (Empty Actor) или Target Point.
     */
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Chess Camera|Perspectives")
    AActor* BlackPerspectiveActor;

    /** Поле зрения (FOV) для камеры черного игрока. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera|Perspectives", meta = (ClampMin = "10", UIMin = "10", ClampMax = "170", UIMax = "170"))
    float BlackPlayerFOV = 90.f;

    /** Скорость вращения камеры (в градусах в секунду) при ручном управлении. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera|Controls", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float ManualRotationSpeed = 45.f;

    /** Минимальное смещение по вертикали (Pitch) от базового положения в градусах (взгляд вниз). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera|Controls", meta = (UIMin = "-89.0", UIMax = "0.0"))
    float MinPitchOffset = -25.0f;

    /** Максимальное смещение по вертикали (Pitch) от базового положения в градусах (взгляд вверх). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera|Controls", meta = (UIMin = "0.0", UIMax = "89.0"))
    float MaxPitchOffset = 25.0f;
};
