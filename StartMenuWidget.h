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

    /**
     * Вызывается при нажатии на кнопку "Создать сетевую игру".
     * ИНСТРУКЦИЯ ДЛЯ РЕДАКТОРА UNREAL:
     * 1. Откройте ваш Blueprint виджета (напр., WBP_StartMenu).
     * 2. Вероятно, вы увидите ошибку компиляции из-за удаленной функции. Это ожидаемо.
     * 3. В дизайнере удалите старую кнопку "Online Game" или в графе (Graph) удалите ее событие OnClicked.
     * 4. Добавьте новую кнопку "Создать игру" (Host Game).
     * 5. В графе для события OnClicked новой кнопки вызовите эту функцию (OnHostOnlineGameClicked).
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnHostOnlineGameClicked();

    /**
     * Вызывается при нажатии на кнопку "Присоединиться к сетевой игре".
     * ИНСТРУКЦИЯ ДЛЯ РЕДАКТОРА UNREAL:
     * 1. Добавьте вторую кнопку "Присоединиться" (Join Game).
     * 2. В графе для события OnClicked этой кнопки вызовите эту функцию (OnFindAndJoinOnlineGameClicked).
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnFindAndJoinOnlineGameClicked();

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
