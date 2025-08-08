#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "ChessGameMode.h" // For ETimeControlType
#include "StartMenuWidget.generated.h"

class UWidgetSwitcher;
class UPanelWidget;
class USlider;
class UComboBoxString;

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
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnOnlineGameClicked();

    /** Вызывается при нажатии кнопки "Настройки" в главном меню. */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnSettingsClicked();

    /** Вызывается при нажатии кнопки "Профиль" в главном меню. */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnProfileClicked();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnExitClicked();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnConfirmBotSettingsAndStartClicked();

    /**
     * Вызывается для возврата на главный экран из любого подменю (например, настроек бота или сетевой игры).
     * Убедитесь, что кнопка "Назад" в ваших подменю вызывает эту функцию в Blueprint.
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnBackToMainMenuClicked();

    /** Вызывается при нажатии на кнопку "Создать игру" в меню сетевой игры. */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnHostGameClicked();

    /** Вызывается при нажатии на кнопку "Присоединиться к игре" в меню сетевой игры. */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnJoinGameClicked();

    /** Вызывается при изменении значения слайдера выбора цвета. */
    UFUNCTION()
    void OnPlayerColorSliderChanged(float Value);

    /** Вызывается при изменении значения комбо-бокса выбора времени. */
    UFUNCTION()
    void OnTimeControlChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidgetSwitcher> MainMenuSwitcher;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> MainMenuPanel;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> BotSettingsPanel;

    /***************************************************************************************************
     *
     *  ПОЛНАЯ ИНСТРУКЦИЯ ПО НАСТРОЙКЕ ONLINE MENUPANEL В РЕДАКТОРЕ UNREAL
     *
     *  Эта инструкция поможет вам создать и настроить с нуля панель для сетевой игры.
     *  Выполните эти шаги в вашем виджет-блюпринте (например, WBP_StartMenu).
     *
     *  --- ШАГ 1: Создание панели ---
     *  1. Откройте виджет и перейдите на вкладку "Designer".
     *  2. В панели "Hierarchy" найдите ваш `MainMenuSwitcher` (UWidgetSwitcher).
     *  3. В панели "Palette" найдите `Canvas Panel` и перетащите ее ВНУТРЬ `MainMenuSwitcher`.
     *  4. Переименуйте эту новую `Canvas Panel` для ясности, например, в `OnlineMenuPanel_Canvas`.
     *
     *  --- ШАГ 2: Привязка панели к переменной C++ ---
     *  1. Выделите только что созданную `Canvas Panel` (`OnlineMenuPanel_Canvas`).
     *  2. На панели "Details" (справа) установите галочку "Is Variable".
     *  3. В поле для имени переменной введите `OnlineMenuPanel`.
     *  4. Нажмите Enter. Теперь эта панель привязана к переменной C++ `OnlineMenuPanel`.
     *
     *  --- ШАГ 3: Создание поля для ввода IP-адреса (НОВЫЙ МЕТОД) ---
     *  1. В панели "Palette" найдите `Editable Text Box` и перетащите его на вашу `OnlineMenuPanel_Canvas`.
     *  2. Разместите его в удобном месте. На панели "Details" можно задать текст по умолчанию в разделе "Content" -> "Text". Например, "127.0.0.1:7777". Это поле используется только для присоединения к игре.
     *  3. Выделите этот `Editable Text Box`.
     *  4. На панели "Details" (в самом верху) найдите поле для имени виджета.
     *  5. Введите в это поле точное имя `SessionNameInput` и нажмите Enter.
     *  6. Галочку "Is Variable" ставить НЕ НУЖНО. Поиск виджета произойдет автоматически по этому имени.
     *
     *  --- ШАГ 4: Создание кнопок ---
     *  Для каждой кнопки выполните следующие действия:
     *
     *  A. КНОПКА "СОЗДАТЬ ИГРУ" (Host Game):
     *      1. Перетащите `Button` из "Palette" на `OnlineMenuPanel_Canvas`.
     *      2. Перетащите `Text` из "Palette" на эту кнопку, чтобы добавить надпись. Измените текст на "Создать игру".
     *      3. Выделите кнопку. На панели "Details" прокрутите вниз до раздела "Events".
     *      4. Нажмите на `+` рядом с событием `On Clicked`.
     *      5. Откроется "Event Graph". Из синего контакта ноды события вытяните связь и найдите вашу C++ функцию `OnHostGameClicked`. Выберите ее.
     *
     *  B. КНОПКА "ПРИСОЕДИНИТЬСЯ" (Join Game):
     *      1. Повторите шаги 1-3 для новой кнопки с текстом "Присоединиться".
     *      2. На шаге 5 в "Event Graph" вызовите функцию `OnJoinGameClicked`.
     *
     *  C. КНОПКА "НАЗАД" (Back):
     *      1. Повторите шаги 1-3 для новой кнопки с текстом "Назад".
     *      2. На шаге 5 в "Event Graph" вызовите функцию `OnBackToMainMenuClicked`.
     *
     *  --- ШАГ 5: Завершение ---
     *  1. Нажмите "Compile" и "Save" в редакторе виджетов.
     *  2. Не забудьте в `MainMenuPanel` настроить кнопку "Сетевая игра", чтобы она по клику вызывала `OnOnlineGameClicked` и открывала эту новую панель.
     *
     ***************************************************************************************************/
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> OnlineMenuPanel;

    /** Поле для ввода IP-адреса сервера для присоединения. Находится в NativeConstruct по имени. */
    UPROPERTY()
    TObjectPtr<UEditableTextBox> SessionNameInput;

    /** Слайдер для выбора цвета игрока в игре против бота. Находится в NativeConstruct по имени "PlayerColorSlider". */
    UPROPERTY()
    TObjectPtr<USlider> ColorSelectionSlider;

    /** Комбо-бокс для выбора контроля времени в игре с ботом. Привязывается автоматически по имени. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> BotTimeControlComboBox;

    /** Комбо-бокс для выбора контроля времени в сетевой игре. Привязывается автоматически по имени. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> OnlineTimeControlComboBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> SkillLevelSlider;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    FName GameLevelName;

private:
    void HideMenu();

    // Хранит выбранный режим контроля времени
    ETimeControlType SelectedTimeControl;
};
