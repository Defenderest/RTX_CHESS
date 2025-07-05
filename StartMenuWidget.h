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
     * Вызывается при нажатии кнопки "Сетевая игра" в главном меню.
     * Переключает виджет на панель для создания/присоединения к игре.
     * ИНСТРУКЦИЯ ДЛЯ РЕДАКТОРА UNREAL:
     * 1. В главном меню (`MainMenuPanel`) найдите или создайте кнопку "Сетевая игра" (`Button`).
     * 2. Для события `OnClicked` этой кнопки вызовите эту функцию (`OnOnlineGameClicked`).
     * 3. Старые кнопки "Создать игру" и "Присоединиться" из главного меню можно удалить.
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnOnlineGameClicked();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnExitClicked();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnConfirmBotSettingsAndStartClicked();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnBackToMainMenuClicked();

    /**
     * Вызывается при нажатии на кнопку "Создать игру" в меню сетевой игры.
     * ИНСТРУКЦИЯ ДЛЯ РЕДАКТОРА UNREAL:
     * 1. В `OnlineMenuPanel` добавьте кнопку "Создать игру" (`Button`).
     * 2. Для события `OnClicked` этой кнопки вызовите эту функцию (`OnHostGameClicked`).
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnHostGameClicked();

    /**
     * Вызывается при нажатии на кнопку "Присоединиться к игре" в меню сетевой игры.
     * ИНСТРУКЦИЯ ДЛЯ РЕДАКТОРА UNREAL:
     * 1. В `OnlineMenuPanel` добавьте кнопку "Присоединиться к игре" (`Button`).
     * 2. Для события `OnClicked` этой кнопки вызовите эту функцию (`OnJoinGameClicked`).
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnJoinGameClicked();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidgetSwitcher> MainMenuSwitcher;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> MainMenuPanel;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> BotSettingsPanel;

    /**
     * Панель, содержащая элементы для сетевой игры (ввод имени комнаты, кнопки "Создать" и "Присоединиться").
     * ИНСТРУКЦИЯ ДЛЯ РЕДАКТОРА UNREAL:
     * Добавьте в `MainMenuSwitcher` новую панель (например, `CanvasPanel`) и привяжите ее к этому свойству.
     */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> OnlineMenuPanel;

    /**
     * Поле для ввода имени комнаты.
     * ИНСТРУКЦИЯ ДЛЯ РЕДАКТОРА UNREAL:
     * Добавьте на `OnlineMenuPanel` виджет `EditableTextBox` и привяжите его к этому свойству.
     */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<class UEditableTextBox> SessionNameInput;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> SkillLevelSlider;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    FName GameLevelName;

private:
    void HideMenu();
};
