#include "ChessPlayerCameraManager.h"
#include "ChessBoard.h"
#include "Kismet/GameplayStatics.h"
#include "GameCameraActor.h"
#include "GameFramework/PlayerController.h"
#include "ChessPlayerController.h"
#include "ChessGameState.h"

AChessPlayerCameraManager::AChessPlayerCameraManager()
{
    CurrentRotationOffset = FRotator::ZeroRotator;
}

void AChessPlayerCameraManager::BeginPlay()
{
    Super::BeginPlay();
}

void AChessPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
    Super::UpdateViewTarget(OutVT, DeltaTime);

    // Если мы не управляем игровой камерой (например, в меню), выходим.
    // Это позволяет другим системам (например, SetViewTargetWithBlend) работать как ожидается.
    if (!bIsControllingGameCamera)
    {
        return;
    }

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
    AChessPlayerController* ChessPC = Cast<AChessPlayerController>(GetOwningPlayerController());
    if (ChessPC)
    {
        AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
        // В режиме "Игрок против Бота" камера должна оставаться на перспективе человека,
        // даже во время хода бота. Это предотвращает "тряску" камеры от переключения туда-сюда.
        if (GameState && GameState->GetCurrentGameModeType() == EGameModeType::PlayerVsBot)
        {
            if (NewPerspective != ChessPC->GetPlayerColor())
            {
                // Не переключаем камеру с вида игрока-человека.
                return;
            }
        }
    }

    // Находим камеру в мире. Это единственный надежный способ получить ссылку на нее.
    AGameCameraActor* GameCamera = Cast<AGameCameraActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameCameraActor::StaticClass()));

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

void AChessPlayerCameraManager::StartControllingGameCamera()
{
    bIsControllingGameCamera = true;
}

void AChessPlayerCameraManager::StopControllingGameCamera()
{
    bIsControllingGameCamera = false;
    bShouldInterpolateCamera = false; // Также останавливаем любую интерполяцию
    CurrentRotationOffset = FRotator::ZeroRotator; // Сбрасываем вращение
}

void AChessPlayerCameraManager::AddCameraRotationInput(FVector2D RotationInput)
{
    if (RotationInput.IsNearlyZero() || GetWorld() == nullptr)
    {
        return;
    }

    AGameCameraActor* GameCamera = Cast<AGameCameraActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameCameraActor::StaticClass()));

    if (!GameCamera)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessPlayerCameraManager::AddCameraRotationInput: GameCameraActor not found. Cannot get rotation limits."));
        return;
    }

    const float CurrentRotationSpeed = GameCamera->GetRotationSpeed();
    const float CurrentMinPitchOffset = GameCamera->GetMinPitchOffset();
    const float CurrentMaxPitchOffset = GameCamera->GetMaxPitchOffset();

    // RotationInput.X (Mouse X) -> Yaw (рыскание)
    // RotationInput.Y (Mouse Y) -> Pitch (тангаж)
    const float YawChange = RotationInput.X * CurrentRotationSpeed;
    // Инвертируем Y для привычного управления (движение мыши вверх -> камера вверх)
    const float PitchChange = -RotationInput.Y * CurrentRotationSpeed;

    CurrentRotationOffset.Yaw += YawChange;
    CurrentRotationOffset.Pitch += PitchChange;

    // Ограничиваем Pitch, чтобы не смотреть "под себя" или "в небо"
    CurrentRotationOffset.Pitch = FMath::Clamp(CurrentRotationOffset.Pitch, CurrentMinPitchOffset, CurrentMaxPitchOffset);

    // Оставляем Roll без изменений
    CurrentRotationOffset.Roll = 0;
}

