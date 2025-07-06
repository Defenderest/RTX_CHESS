#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChessPiece.h" // For EPieceType
#include "PromotionMenuWidget.generated.h"

// Forward declarations
class URotatingPieceWidget;
class UStaticMesh;
class UMaterialInterface;

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
	// The user must place RotatingPieceWidget instances in the Blueprint and name them accordingly.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	URotatingPieceWidget* QueenDisplayWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	URotatingPieceWidget* RookDisplayWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	URotatingPieceWidget* BishopDisplayWidget;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	URotatingPieceWidget* KnightDisplayWidget;

	// --- CONTENT TO SET IN BLUEPRINT ---
	UPROPERTY(EditDefaultsOnly, Category = "Promotion Content")
	UStaticMesh* QueenMesh;
	UPROPERTY(EditDefaultsOnly, Category = "Promotion Content")
	UStaticMesh* RookMesh;
	UPROPERTY(EditDefaultsOnly, Category = "Promotion Content")
	UStaticMesh* BishopMesh;
	UPROPERTY(EditDefaultsOnly, Category = "Promotion Content")
	UStaticMesh* KnightMesh;
	UPROPERTY(EditDefaultsOnly, Category = "Promotion Content")
	UMaterialInterface* PieceMaterial;
};
