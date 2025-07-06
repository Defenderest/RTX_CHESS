#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RotatingPieceWidget.generated.h"

class UImage;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ARotatingPieceCaptureActor;
class UTextureRenderTarget2D;

UCLASS()
class RTX_CHESS_API URotatingPieceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Rotating Piece")
	void InitializePiece(UStaticMesh* PieceMesh, UMaterialInterface* PieceMaterial);

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UImage> PieceImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotating Piece")
	float RotationSpeed = 45.0f;

	UPROPERTY(EditAnywhere, Category = "Rotating Piece", meta = (DisplayName = "Render Target Material Base"))
	TObjectPtr<UMaterialInterface> RenderTargetMaterialBase;

protected:
	virtual void NativeConstruct() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	UPROPERTY()
	TObjectPtr<ARotatingPieceCaptureActor> CaptureActor;

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PieceMaterialInstance;
};
