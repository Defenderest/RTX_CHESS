#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StartMenuWidget.generated.h"

class UWidgetSwitcher;
class UPanelWidget;
class USlider;

UCLASS()
class RTX_CHESS_API UStartMenuWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnStartPlayerVsPlayerClicked();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnStartPlayerVsBotClicked();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnStartOnlineGameClicked();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnExitClicked();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnConfirmBotSettingsAndStartClicked();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnBackToMainMenuClicked();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidgetSwitcher> MainMenuSwitcher;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> MainMenuPanel;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> BotSettingsPanel;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> SkillLevelSlider;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    FName GameLevelName;

private:
    void HideMenu();
};
