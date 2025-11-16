#include "GameCameraActor.h"
#include "Net/UnrealNetwork.h"

AGameCameraActor::AGameCameraActor()
{
    bReplicates = true;
    // Конструктор теперь пуст, так как настройки задаются через ссылки на акторы.
}

bool AGameCameraActor::GetCameraPerspectiveForColor(EPieceColor PlayerColor, FTransform& OutTransform, float& OutFOV) const
{
    AActor* PerspectiveActor = (PlayerColor == EPieceColor::White) ? WhitePerspectiveActor : BlackPerspectiveActor;
    
    if (PerspectiveActor)
    {
        OutTransform = PerspectiveActor->GetActorTransform();
        OutFOV = (PlayerColor == EPieceColor::White) ? WhitePlayerFOV : BlackPlayerFOV;
        return true;
    }

    // Если актор не назначен, возвращаем false
    return false;
}

float AGameCameraActor::GetRotationSpeed() const
{
    return ManualRotationSpeed;
}

float AGameCameraActor::GetMinPitchOffset() const
{
    return MinPitchOffset;
}

float AGameCameraActor::GetMaxPitchOffset() const
{
    return MaxPitchOffset;
}

void AGameCameraActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AGameCameraActor, WhitePerspectiveActor);
    DOREPLIFETIME(AGameCameraActor, BlackPerspectiveActor);
}
