#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChessPiece.h" // For EPieceType
#include "PromotionMenuWidget.generated.h"

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
    // Эти функции должны быть привязаны к событиям OnClicked кнопок в виджете Blueprint.
    UFUNCTION(BlueprintCallable, Category = "Promotion")
    void OnQueenSelected();

    UFUNCTION(BlueprintCallable, Category = "Promotion")
    void OnRookSelected();

    UFUNCTION(BlueprintCallable, Category = "Promotion")
    void OnBishopSelected();

    UFUNCTION(BlueprintCallable, Category = "Promotion")
    void OnKnightSelected();
};
