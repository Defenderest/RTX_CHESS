#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "ChessPiece.h"
#include "Net/UnrealNetwork.h"
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

    // Gets the transform and FOV for a specific player color's camera.
    UFUNCTION(BlueprintPure, Category = "Chess Camera")
    bool GetCameraPerspectiveForColor(EPieceColor PlayerColor, FTransform& OutTransform, float& OutFOV) const;

    float GetRotationSpeed() const;
    float GetMinPitchOffset() const;
    float GetMaxPitchOffset() const;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /**
     * Actor whose position and rotation will be used for the white player's camera.
     * You can use an Empty Actor or a Target Point.
     */
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Replicated, Category = "Chess Camera|Perspectives")
    AActor* WhitePerspectiveActor;

    /** Field of View (FOV) for the white player's camera. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera|Perspectives", meta = (ClampMin = "10", UIMin = "10", ClampMax = "170", UIMax = "170"))
    float WhitePlayerFOV = 90.f;

    /**
     * Actor whose position and rotation will be used for the black player's camera.
     * You can use an Empty Actor or a Target Point.
     */
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Replicated, Category = "Chess Camera|Perspectives")
    AActor* BlackPerspectiveActor;

    /** Field of View (FOV) for the black player's camera. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera|Perspectives", meta = (ClampMin = "10", UIMin = "10", ClampMax = "170", UIMax = "170"))
    float BlackPlayerFOV = 90.f;

    /** Camera rotation speed (in degrees per second) for manual control. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera|Controls", meta = (ClampMin = "1.0", UIMin = "1.0"))
    float ManualRotationSpeed = 45.f;

    /** Minimum vertical offset (Pitch) from the base position in degrees (looking down). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera|Controls", meta = (UIMin = "-89.0", UIMax = "0.0"))
    float MinPitchOffset = -25.0f;

    /** Maximum vertical offset (Pitch) from the base position in degrees (looking up). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera|Controls", meta = (UIMin = "0.0", UIMax = "89.0"))
    float MaxPitchOffset = 25.0f;
};
