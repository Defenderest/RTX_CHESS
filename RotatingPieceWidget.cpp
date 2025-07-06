#include "RotatingPieceWidget.h"
#include "Components/Viewport.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"

void URotatingPieceWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // We create a new world for our viewport.
    if (!IsValid(ViewportWorld))
    {
        ViewportWorld = UWorld::CreateWorld(EWorldType::Inactive, false);
    }
    
    if (Viewport && IsValid(ViewportWorld))
    {
        // NOTE: In UE5, UViewport widget no longer supports rendering a separate UWorld directly.
        // The functions SetWorld, SetViewLocation, and SetViewRotation have been removed.
        // This code is modified to compile, but it will not render the piece.
        // A refactor using a SceneCapture2D component is required to restore functionality.

        // Spawn a camera to look at the piece.
        const FVector CameraLocation(150.f, 0.f, 40.f); // Adjust location for a good view
        const FRotator CameraRotation(0.f, -180.f, 0.f); // Look at the center
        CameraActor = ViewportWorld->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), CameraLocation, CameraRotation);
    }
}

void URotatingPieceWidget::InitializePiece(UStaticMesh* PieceMesh, UMaterialInterface* PieceMaterial)
{
    if (!IsValid(ViewportWorld) || !PieceMesh)
    {
        return;
    }

    // If a piece actor already exists, destroy it first.
    if (IsValid(PieceActor))
    {
        PieceActor->Destroy();
        PieceActor = nullptr;
    }
    
    // Spawn the static mesh actor that will represent our chess piece.
    PieceActor = ViewportWorld->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    if (IsValid(PieceActor) && PieceActor->GetStaticMeshComponent())
    {
        PieceActor->GetStaticMeshComponent()->SetStaticMesh(PieceMesh);
        if (PieceMaterial)
        {
            PieceActor->GetStaticMeshComponent()->SetMaterial(0, PieceMaterial);
        }
    }
}

void URotatingPieceWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (IsValid(PieceActor))
    {
        // Rotate the actor around the Z axis (yaw).
        PieceActor->AddActorWorldRotation(FRotator(0.f, RotationSpeed * InDeltaTime, 0.f));
    }
}

void URotatingPieceWidget::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);

    // Clean up the world we created.
    if (IsValid(ViewportWorld))
    {
        ViewportWorld->DestroyWorld(false);
        ViewportWorld = nullptr;
    }
}
