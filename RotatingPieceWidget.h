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
	void InitializePiece(UTexture2D* PieceTexture);

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<UImage> PieceImage;
};
