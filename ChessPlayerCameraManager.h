#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "ChessPiece.h" // Включаем для доступа к EPieceColor
#include "ChessPlayerCameraManager.generated.h"

UCLASS()
class RTX_CHESS_API AChessPlayerCameraManager : public APlayerCameraManager
{
    GENERATED_BODY()

public:
    AChessPlayerCameraManager();

    // Обновляет вид каждый кадр
    virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

    // Публичная функция для переключения перспективы камеры
    UFUNCTION(BlueprintCallable, Category = "Chess Camera")
    void SwitchToPlayerPerspective(EPieceColor NewPerspective);

    /** Добавляет вращение к камере. Вызывается из PlayerController. */
    void AddCameraRotationInput(FVector2D RotationInput);

protected:
    // Скорость интерполяции камеры при смене перспективы
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera", meta = (ClampMin = "0.1", UIMin = "0.1"))
    float CameraInterpolationSpeed = 5.0f;


private:
    // Целевые параметры камеры, к которым будет происходить интерполяция
    FVector TargetCameraLocation;
    FRotator TargetCameraRotation;
    float TargetCameraFOV;

    // Текущее смещение вращения камеры, заданное игроком
    FRotator CurrentRotationOffset;

    // Флаг, указывающий, нужно ли интерполировать камеру
    bool bShouldInterpolateCamera = false;

    // Инициализирует начальное положение камеры
    virtual void BeginPlay() override;
};
