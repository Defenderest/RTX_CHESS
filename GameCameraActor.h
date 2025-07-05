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
};
