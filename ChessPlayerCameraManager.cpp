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

    // Шаг 1: Обработка смены перспективы (интерполяция к базовой позиции)
    if (bShouldInterpolateCamera)
    {
        // Интерполируем к БАЗОВЫМ целевым параметрам, без смещения от игрока.
        OutVT.POV.Location = FMath::VInterpTo(OutVT.POV.Location, TargetCameraLocation, DeltaTime, CameraInterpolationSpeed);
        OutVT.POV.Rotation = FMath::RInterpTo(OutVT.POV.Rotation, TargetCameraRotation, DeltaTime, CameraInterpolationSpeed);
        OutVT.POV.FOV = FMath::FInterpTo(OutVT.POV.FOV, TargetCameraFOV, DeltaTime, CameraInterpolationSpeed);

        // Проверяем, достигла ли камера базовой цели.
        if (OutVT.POV.Location.Equals(TargetCameraLocation, 1.0f) && OutVT.POV.Rotation.Equals(TargetCameraRotation, 1.0f))
        {
            bShouldInterpolateCamera = false; // Останавливаем интерполяцию
        }
    }
    else
    {
        // Если интерполяция не требуется, жестко устанавливаем базовые параметры.
        OutVT.POV.Location = TargetCameraLocation;
        OutVT.POV.Rotation = TargetCameraRotation;
        OutVT.POV.FOV = TargetCameraFOV;
    }

    // Шаг 2: ВСЕГДА применяем смещение вращения от игрока поверх текущего поворота камеры.
    OutVT.POV.Rotation += CurrentRotationOffset;
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

    // Получаем целевые transform и FOV из GameCameraActor
    FTransform TargetTransform;
    float NewFOV;
    if (GameCamera->GetCameraPerspectiveForColor(NewPerspective, TargetTransform, NewFOV))
    {
        // Устанавливаем целевые параметры камеры напрямую из transform'а актора-цели
        TargetCameraLocation = TargetTransform.GetLocation();
        TargetCameraRotation = TargetTransform.GetRotation().Rotator();
        TargetCameraFOV = NewFOV;

        // Сбрасываем смещение вращения при смене перспективы
        CurrentRotationOffset = FRotator::ZeroRotator;

        // Включаем флаг интерполяции, чтобы UpdateViewTarget начал перемещение
        bShouldInterpolateCamera = true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AChessPlayerCameraManager: Perspective actor for color %s is not set in GameCameraActor. Cannot switch perspective."),
            (NewPerspective == EPieceColor::White ? TEXT("White") : TEXT("Black")));
    }
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

