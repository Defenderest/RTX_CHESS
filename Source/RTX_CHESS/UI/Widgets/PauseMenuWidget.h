#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;

UCLASS()
class RTX_CHESS_API UPauseMenuWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    UPROPERTY(meta = (BindWidget))
    UButton* ResumeButton;

    // Эта переменная автоматически связывается с виджетом UButton с именем "SettingsButton" в вашем Blueprint виджета.
    // Убедитесь, что имя кнопки в UMG Editor совпадает с именем этой переменной.
    UPROPERTY(meta = (BindWidget))
    UButton* SettingsButton;

    UPROPERTY(meta = (BindWidget))
    UButton* ExitToMenuButton;

    UPROPERTY(meta = (BindWidget))
    UButton* QuitGameButton;

    UFUNCTION()
    void OnResumeClicked();

    UFUNCTION()
    void OnSettingsClicked();

    UFUNCTION()
    void OnExitToMenuClicked();

    UFUNCTION()
    void OnQuitGameClicked();

    /** Имя уровня главного меню для загрузки. Можно изменить в Blueprint. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    FName MainMenuLevelName = "MainMenu";
};
