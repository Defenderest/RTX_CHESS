#include "GameCameraActor.h"

AGameCameraActor::AGameCameraActor()
{
    // WhitePlayerCameraSetup использует значения по умолчанию из структуры FCameraSetup

    // Переопределяем значения для перспективы черных
    BlackPlayerCameraSetup.LocationOffset = FVector(0.f, 1200.f, 1000.f);
    BlackPlayerCameraSetup.Rotation = FRotator(-45.f, 180.f, 0.f);
    BlackPlayerCameraSetup.FOV = 90.f;
}

const FCameraSetup& AGameCameraActor::GetCameraSetupForColor(EPieceColor PlayerColor) const
{
    return (PlayerColor == EPieceColor::White) ? WhitePlayerCameraSetup : BlackPlayerCameraSetup;
}
