#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameOverWidget.generated.h"

class UTextBlock;
class UButton;

/**
 * Виджет, отображаемый по окончании игры (победа, поражение, ничья).
 */
UCLASS()
class RTX_CHESS_API UGameOverWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

public:
    /** Устанавливает основной текст результата игры (напр., "Победа белых"). */
    UFUNCTION(BlueprintCallable, Category = "Game Over Screen")
    void SetResultText(const FText& Text);

    /** Устанавливает текст с причиной окончания игры (напр., "Мат"). */
    UFUNCTION(BlueprintCallable, Category = "Game Over Screen")
    void SetReasonText(const FText& Text);

protected:
    /** Вызывается при нажатии кнопки "В главное меню". */
    UFUNCTION()
    void OnBackToMenuClicked();
    
    /** Текст, показывающий результат игры. Привяжите в Blueprint по имени ResultText. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ResultText;

    /** Текст, показывающий причину. Привяжите в Blueprint по имени ReasonText. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ReasonText;

    /** Кнопка для возврата в главное меню. Привяжите в Blueprint по имени BackToMenuButton. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> BackToMenuButton;
};
