#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Pieces/ChessPiece.h" // For EPieceType
#include "PromotionMenuWidget.generated.h"

// Forward declarations
class UImage;
class UTexture2D;
class UBorder;
class UBackgroundBlur;

// Делегат, который будет вызван, когда игрок выберет фигуру для превращения
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPromotionPieceSelected, EPieceType, SelectedPieceType);

/**
 * Виджет для меню превращения пешки.
 * Позволяет игроку выбрать, в какую фигуру превратить пешку.
 */
UCLASS()
class RTX_CHESS_API UPromotionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Этот делегат вызывается, когда игрок выбирает фигуру.
	UPROPERTY(BlueprintAssignable, Category = "Promotion")
	FOnPromotionPieceSelected OnPromotionPieceSelected;

protected:
	virtual void NativeConstruct() override;
	
	// Эти функции должны быть привязаны к событиям OnClicked кнопок в виджете Blueprint.
	UFUNCTION(BlueprintCallable, Category = "Promotion")
	void OnQueenSelected();

	UFUNCTION(BlueprintCallable, Category = "Promotion")
	void OnRookSelected();

	UFUNCTION(BlueprintCallable, Category = "Promotion")
	void OnBishopSelected();

	UFUNCTION(BlueprintCallable, Category = "Promotion")
	void OnKnightSelected();

	// --- WIDGETS TO BIND ---
	// The user must place Image widgets in the Blueprint and name them accordingly.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UBackgroundBlur* BackgroundBlur;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UBorder* BackgroundBorder;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* QueenDisplayImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* RookDisplayImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* BishopDisplayImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* KnightDisplayImage;

	// --- CONTENT TO SET IN BLUEPRINT ---
	UPROPERTY(EditDefaultsOnly, Category = "Promotion Content")
	UTexture2D* QueenTexture;
	UPROPERTY(EditDefaultsOnly, Category = "Promotion Content")
	UTexture2D* RookTexture;
	UPROPERTY(EditDefaultsOnly, Category = "Promotion Content")
	UTexture2D* BishopTexture;
	UPROPERTY(EditDefaultsOnly, Category = "Promotion Content")
	UTexture2D* KnightTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Promotion Content", meta = (DisplayName = "Image Size"))
	FVector2D ImageSize = FVector2D(64.f, 64.f);

	UPROPERTY(EditDefaultsOnly, Category = "Promotion Content", meta = (DisplayName = "Blur Strength"))
	float BlurStrength = 5.0f;
};
