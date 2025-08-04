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

    /** Сообщает менеджеру камеры, что он должен начать применять свою игровую логику (интерполяцию, вращение игрока). */
    void StartControllingGameCamera();

    /** Сообщает менеджеру камеры прекратить свою логику и вернуться к поведению по умолчанию (например, для меню). */
    void StopControllingGameCamera();

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

    // Флаг, указывающий, что мы управляем камерой для игры в шахматы, а не для меню.
    bool bIsControllingGameCamera = false;

    // Инициализирует начальное положение камеры
    virtual void BeginPlay() override;
};
