#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RotatingPieceWidget.generated.h"

class UViewport;
class UStaticMesh;
class UMaterialInterface;
class AStaticMeshActor;

UCLASS()
class RTX_CHESS_API URotatingPieceWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // This function must be called from the parent widget to set up the mesh.
    UFUNCTION(BlueprintCallable, Category = "Rotating Piece")
    void InitializePiece(UStaticMesh* PieceMesh, UMaterialInterface* PieceMaterial);

    // The viewport widget that must be present in the WBP_RotatingPiece Blueprint.
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UViewport* Viewport;

    // Speed of rotation in degrees per second.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotating Piece")
    float RotationSpeed = 45.0f;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
    // The world used by the viewport to render the scene.
    UPROPERTY()
    TObjectPtr<UWorld> ViewportWorld;

    // The piece actor spawned in the viewport world.
    UPROPERTY()
    TObjectPtr<AStaticMeshActor> PieceActor;

    // The camera used to view the piece.
    UPROPERTY()
    TObjectPtr<AActor> CameraActor;
};
