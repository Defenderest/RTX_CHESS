#include "ChessPlayerCameraManager.h"
#include "ChessBoard.h"
#include "Kismet/GameplayStatics.h"

AChessPlayerCameraManager::AChessPlayerCameraManager()
{
    // Устанавливаем стандартные значения по умолчанию, если они не переопределены в Blueprint
    WhitePlayerCameraSetup.LocationOffset = FVector(0.f, -1200.f, 1000.f);
    WhitePlayerCameraSetup.Rotation = FRotator(-45.f, 0.f, 0.f);
    WhitePlayerCameraSetup.FOV = 90.f;

    BlackPlayerCameraSetup.LocationOffset = FVector(0.f, 1200.f, 1000.f);
    BlackPlayerCameraSetup.Rotation = FRotator(-45.f, 180.f, 0.f);
    BlackPlayerCameraSetup.FOV = 90.f;
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

    if (bShouldInterpolateCamera)
    {
        // Плавно инте��полируем положение, поворот и FOV камеры к целевым значениям,
        // изменяя непосредственно структуру OutVT.POV.
        OutVT.POV.Location = FMath::VInterpTo(OutVT.POV.Location, TargetCameraLocation, DeltaTime, CameraInterpolationSpeed);
        OutVT.POV.Rotation = FMath::RInterpTo(OutVT.POV.Rotation, TargetCameraRotation, DeltaTime, CameraInterpolationSpeed);
        OutVT.POV.FOV = FMath::FInterpTo(OutVT.POV.FOV, TargetCameraFOV, DeltaTime, CameraInterpolationSpeed);

        // Проверяем, достигла ли камера цели (с небольшой погрешностью)
        if (OutVT.POV.Location.Equals(TargetCameraLocation, 1.0f) && OutVT.POV.Rotation.Equals(TargetCameraRotation, 1.0f))
        {
            bShouldInterpolateCamera = false; // Останавливаем интерполяцию
        }
    }
    else
    {
        // Если интерполяция не требуется, просто жестко устанавливаем целевые параметры,
        // чтобы избежать "дрейфа" камеры из-за неточностей вычислений.
        OutVT.POV.Location = TargetCameraLocation;
        OutVT.POV.Rotation = TargetCameraRotation;
        OutVT.POV.FOV = TargetCameraFOV;
    }
}

void AChessPlayerCameraManager::SwitchToPlayerPerspective(EPieceColor NewPerspective)
{
    // Находим шахматную доску на сцене
    AActor* ChessBoardActor = UGameplayStatics::GetActorOfClass(GetWorld(), AChessBoard::StaticClass());
    if (!ChessBoardActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessPlayerCameraManager: ChessBoard not found in the world. Cannot set camera target."));
        return;
    }

    FVector BoardCenter = ChessBoardActor->GetActorLocation();
    const FCameraSetup& TargetSetup = (NewPerspective == EPieceColor::White) ? WhitePlayerCameraSetup : BlackPlayerCameraSetup;

    // Устанавливаем целевые параметры камеры
    TargetCameraLocation = BoardCenter + TargetSetup.LocationOffset;
    TargetCameraRotation = TargetSetup.Rotation;
    TargetCameraFOV = TargetSetup.FOV;

    // Включаем флаг интерполяции, чтобы UpdateViewTarget начал перемещение
    bShouldInterpolateCamera = true;
}

