#include "ChessPlayerCameraManager.h"
#include "ChessBoard.h"
#include "Kismet/GameplayStatics.h"
#include "GameCameraActor.h"
#include "GameFramework/PlayerController.h"

AChessPlayerCameraManager::AChessPlayerCameraManager()
{
    CurrentRotationOffset = FRotator::ZeroRotator;
}

void AChessPlayerCameraManager::BeginPlay()
{
    Super::BeginPlay();
    // Устанавливаем начальную перспективу на белых. 
    // Камера плавно переместится в эту позицию при первом вызове UpdateViewTarget.
    SwitchToPlayerPerspective(EPieceColor::White);
}

void AChessPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
    Super::UpdateViewTarget(OutVT, DeltaTime);

    const FVector FinalTargetLocation = TargetCameraLocation;
    const FRotator FinalTargetRotation = TargetCameraRotation + CurrentRotationOffset;

    if (bShouldInterpolateCamera)
    {
        // Плавно интерполируем положение, поворот и FOV камеры к целевым значениям,
        // изменяя непосредственно структуру OutVT.POV.
        OutVT.POV.Location = FMath::VInterpTo(OutVT.POV.Location, FinalTargetLocation, DeltaTime, CameraInterpolationSpeed);
        OutVT.POV.Rotation = FMath::RInterpTo(OutVT.POV.Rotation, FinalTargetRotation, DeltaTime, CameraInterpolationSpeed);
        OutVT.POV.FOV = FMath::FInterpTo(OutVT.POV.FOV, TargetCameraFOV, DeltaTime, CameraInterpolationSpeed);

        // Проверяем, достигла ли камера цели (с небольшой погрешностью)
        if (OutVT.POV.Location.Equals(FinalTargetLocation, 1.0f) && OutVT.POV.Rotation.Equals(FinalTargetRotation, 1.0f))
        {
            bShouldInterpolateCamera = false; // Останавливаем интерполяцию
        }
    }
    else
    {
        // Если интерполяция не требуется, просто жестко устанавливаем целевые параметры,
        // чтобы избежать "дрейфа" камеры из-за неточностей вычислений.
        OutVT.POV.Location = FinalTargetLocation;
        OutVT.POV.Rotation = FinalTargetRotation;
        OutVT.POV.FOV = TargetCameraFOV;
    }
}

void AChessPlayerCameraManager::SwitchToPlayerPerspective(EPieceColor NewPerspective)
{
    // Находим камеру в мире
    AGameCameraActor* GameCamera = nullptr;
    if (APlayerController* PC = GetOwningPlayerController())
    {
        GameCamera = Cast<AGameCameraActor>(PC->GetViewTarget());
    }
    
    // Если камера не установлена как ViewTarget, ищем ее в мире как запасной вариант
    if (!GameCamera)
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessPlayerCameraManager: GameCameraActor is not the current view target. Searching in world..."));
        GameCamera = Cast<AGameCameraActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameCameraActor::StaticClass()));
    }

    if (!GameCamera)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessPlayerCameraManager: GameCameraActor not found in the world. Cannot get camera setups."));
        return;
    }

    // Находим шахматную доску на сцене
    AActor* ChessBoardActor = UGameplayStatics::GetActorOfClass(GetWorld(), AChessBoard::StaticClass());
    if (!ChessBoardActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessPlayerCameraManager: ChessBoard not found in the world. Cannot set camera target."));
        return;
    }

    FVector BoardCenter = ChessBoardActor->GetActorLocation();
    const FCameraSetup& TargetSetup = GameCamera->GetCameraSetupForColor(NewPerspective);

    // Устанавливаем целевые параметры камеры
    TargetCameraLocation = BoardCenter + TargetSetup.LocationOffset;
    TargetCameraRotation = TargetSetup.Rotation;
    TargetCameraFOV = TargetSetup.FOV;

    // Сбрасываем смещение вращения при смене перспективы
    CurrentRotationOffset = FRotator::ZeroRotator;

    // Включаем флаг интерполяции, чтобы UpdateViewTarget начал перемещение
    bShouldInterpolateCamera = true;
}

void AChessPlayerCameraManager::AddCameraRotationInput(FVector2D RotationInput)
{
    if (RotationInput.IsNearlyZero() || GetWorld() == nullptr)
    {
        return;
    }

    const float DeltaSeconds = GetWorld()->GetDeltaSeconds();

    // RotationInput.X (A/D) -> Yaw (рыскание)
    // RotationInput.Y (W/S) -> Pitch (тангаж)
    const float YawChange = RotationInput.X * RotationSpeed * DeltaSeconds;
    // Инвертируем Y для привычного управления (W - вверх)
    const float PitchChange = -RotationInput.Y * RotationSpeed * DeltaSeconds;

    CurrentRotationOffset.Yaw += YawChange;
    CurrentRotationOffset.Pitch += PitchChange;

    // Ограничиваем Pitch, чтобы не смотреть "под себя" или "в небо"
    CurrentRotationOffset.Pitch = FMath::Clamp(CurrentRotationOffset.Pitch, MinPitchOffset, MaxPitchOffset);

    // Оставляем Roll без изменений
    CurrentRotationOffset.Roll = 0;
}

