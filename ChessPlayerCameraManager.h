#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "ChessPiece.h" // Включаем для доступа к EPieceColor
#include "ChessPlayerCameraManager.generated.h"

// Структура для хранения настроек камеры для одной пе��спективы
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

    /** Добавляет смещение к камере для панорамирования. Вызывается из PlayerController. */
    void AddCameraPanInput(FVector2D PanInput);

protected:
    // Настройки камеры для перспективы белого игрока
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera")
    FCameraSetup WhitePlayerCameraSetup;

    // Настройки камеры для перспективы черного игрока
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera")
    FCameraSetup BlackPlayerCameraSetup;

    // Скорость интерполяции камеры при смене перспективы
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera", meta = (ClampMin = "0.1", UIMin = "0.1"))
    float CameraInterpolationSpeed = 5.0f;

    // Скорость панорамирования камеры
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera")
    float PanSpeed = 500.0f;

    // Максимальное расстояние панорамирования от центральной точки
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Camera")
    float MaxPanDistance = 1000.0f;

private:
    // Целевые параметры камеры, к которым будет происходить интерполяция
    FVector TargetCameraLocation;
    FRotator TargetCameraRotation;
    float TargetCameraFOV;

    // Текущее смещение камеры из-за панорамирования
    FVector CurrentPanOffset;

    // Флаг, указывающий, нужно ли интерполировать камеру
    bool bShouldInterpolateCamera = false;

    // Инициализирует начальное положение камеры
    virtual void BeginPlay() override;
};
