#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RotatingPieceWidget.generated.h"

class UImage;
class UTexture2D;

UCLASS()
class RTX_CHESS_API URotatingPieceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Rotating Piece")
	void SetPieceTexture(UTexture2D* NewPieceTexture);

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UImage> PieceImage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance", meta = (DisplayName = "Default Piece Texture"))
	TObjectPtr<UTexture2D> DefaultPieceTexture;

protected:
	virtual void NativeConstruct() override;
};
