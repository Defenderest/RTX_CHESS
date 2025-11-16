
#include "StartMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "ChessGameMode.h"
#include "ChessPlayerController.h"
#include "ChessGameInstance.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Slider.h"
#include "Components/ComboBoxString.h"

void UStartMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SelectedTimeControl = ETimeControlType::Unlimited;
    GameLevelName = TEXT("/Game/Cigar_room/Maps/Cigar_room");

    // Альтернативный способ получения виджета: поиск по имени.
    // Этот метод не требует привязки через "Is Variable" или BindWidget.
    SessionNameInput = Cast<UEditableTextBox>(GetWidgetFromName(TEXT("SessionNameInput")));
    if (!SessionNameInput)
    {
        // Это не критическая ошибка, а предупреждение для разработчика.
        UE_LOG(LogTemp, Warning, TEXT("UStartMenuWidget: Не удалось найти EditableTextBox с именем 'SessionNameInput'. Убедитесь, что виджет с таким именем существует на панели OnlineMenuPanel."));
    }

    ColorSelectionSlider = Cast<USlider>(GetWidgetFromName(TEXT("PlayerColorSlider")));
    if (ColorSelectionSlider)
    {
        ColorSelectionSlider->OnValueChanged.AddDynamic(this, &UStartMenuWidget::OnPlayerColorSliderChanged);
        // Вызываем один раз при запуске, чтобы установить начальное значение из слайдера
        OnPlayerColorSliderChanged(ColorSelectionSlider->GetValue());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UStartMenuWidget: Не удалось найти Slider с именем 'PlayerColorSlider'. Убедитесь, что виджет с таким именем существует на панели BotSettingsPanel."));
    }

    if (BotTimeControlComboBox)
    {
        BotTimeControlComboBox->ClearOptions();
        BotTimeControlComboBox->AddOption(TEXT("Unlimited"));
        BotTimeControlComboBox->AddOption(TEXT("Bullet (1|0)"));
        BotTimeControlComboBox->AddOption(TEXT("Blitz (3|2)"));
        BotTimeControlComboBox->AddOption(TEXT("Rapid (10|0)"));
        BotTimeControlComboBox->SetSelectedIndex(0);
        BotTimeControlComboBox->OnSelectionChanged.AddDynamic(this, &UStartMenuWidget::OnTimeControlChanged);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UStartMenuWidget: Виджет с именем 'BotTimeControlComboBox' не найден или не привязан. Контроль времени для игры с ботом будет недоступен."));
    }

    if (OnlineTimeControlComboBox)
    {
        OnlineTimeControlComboBox->ClearOptions();
        OnlineTimeControlComboBox->AddOption(TEXT("Unlimited"));
        OnlineTimeControlComboBox->AddOption(TEXT("Bullet (1|0)"));
        OnlineTimeControlComboBox->AddOption(TEXT("Blitz (3|2)"));
        OnlineTimeControlComboBox->AddOption(TEXT("Rapid (10|0)"));
        OnlineTimeControlComboBox->SetSelectedIndex(0);
        OnlineTimeControlComboBox->OnSelectionChanged.AddDynamic(this, &UStartMenuWidget::OnTimeControlChanged);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UStartMenuWidget: Виджет с именем 'OnlineTimeControlComboBox' не найден или не привязан. Контроль времени для сетевой игры будет недоступен."));
    }
}

void UStartMenuWidget::OnStartPlayerVsPlayerClicked()
{
    if (GameLevelName.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT("GameLevelName is not set!"));
        return;
    }
    // Для PvP игры параметры могут быть пустыми или явно указывать режим.
    FString Options = FString::Printf(TEXT("?bIsBotGame=false?TimeControl=%d"), static_cast<int32>(SelectedTimeControl));
    UE_LOG(LogTemp, Log, TEXT("Attempting to open level for PvP: %s"), *GameLevelName.ToString());
    UGameplayStatics::OpenLevel(GetWorld(), GameLevelName, true, Options);
}

void UStartMenuWidget::OnStartPlayerVsBotClicked()
{
    UE_LOG(LogTemp, Log, TEXT("UStartMenuWidget: Switching to bot settings menu."));
    if (MainMenuSwitcher && BotSettingsPanel)
    {
        MainMenuSwitcher->SetActiveWidget(BotSettingsPanel);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("MainMenuSwitcher or BotSettingsPanel is not bound in StartMenuWidget Blueprint!"));
    }
}

void UStartMenuWidget::OnConfirmBotSettingsAndStartClicked()
{
    if (GameLevelName.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT("GameLevelName is not set!"));
        return;
    }

    int32 SkillLevel = 20; // Значение по умолчанию, если слайдер не привязан
    if (SkillLevelSlider)
    {
        SkillLevel = FMath::RoundToInt(SkillLevelSlider->GetValue());
    }

    // Получаем выбор цвета из слайдера
    int32 ColorChoiceIndex = 1; // 1 = Random по умолчанию
    if (ColorSelectionSlider)
    {
        ColorChoiceIndex = FMath::RoundToInt(ColorSelectionSlider->GetValue());
    }

    FString Options = FString::Printf(TEXT("?bIsBotGame=true?SkillLevel=%d?ColorChoice=%d?TimeControl=%d"), SkillLevel, ColorChoiceIndex, static_cast<int32>(SelectedTimeControl));
    UE_LOG(LogTemp, Log, TEXT("Attempting to open level for PvE: %s with options: %s"), *GameLevelName.ToString(), *Options);
    UGameplayStatics::OpenLevel(GetWorld(), GameLevelName, true, Options);
}

void UStartMenuWidget::OnBackToMainMenuClicked()
{
    if (MainMenuSwitcher && MainMenuPanel)
    {
        MainMenuSwitcher->SetActiveWidget(MainMenuPanel);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("MainMenuSwitcher or MainMenuPanel is not bound in StartMenuWidget Blueprint!"));
    }
}

void UStartMenuWidget::OnOnlineGameClicked()
{
    if (MainMenuSwitcher && OnlineMenuPanel)
    {
        MainMenuSwitcher->SetActiveWidget(OnlineMenuPanel);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("MainMenuSwitcher or OnlineMenuPanel is not bound in StartMenuWidget Blueprint!"));
    }
}

void UStartMenuWidget::OnSettingsClicked()
{
    if (AChessPlayerController* PlayerController = Cast<AChessPlayerController>(GetOwningPlayer()))
    {
        PlayerController->ToggleGraphicsSettingsMenu();
    }
}

void UStartMenuWidget::OnProfileClicked()
{
    if (AChessPlayerController* PlayerController = Cast<AChessPlayerController>(GetOwningPlayer()))
    {
        PlayerController->ToggleProfileWidget();
    }
}

void UStartMenuWidget::OnHostGameClicked()
{
    if (UChessGameInstance* GameInstance = Cast<UChessGameInstance>(GetGameInstance()))
    {
        // Имя сессии жестко закодировано, так как поле ввода теперь используется для IP-адреса клиента.
        const FString SessionName = TEXT("ChessGameSession");
        HideMenu();
        GameInstance->HostSession(SessionName, GameLevelName, SelectedTimeControl);
    }
}

void UStartMenuWidget::OnJoinGameClicked()
{
    if (SessionNameInput && !SessionNameInput->GetText().IsEmpty())
    {
        const FString IpAddress = SessionNameInput->GetText().ToString();
        HideMenu();

        if (APlayerController* PlayerController = GetOwningPlayer())
        {
            UE_LOG(LogTemp, Log, TEXT("Attempting to join game by IP: %s"), *IpAddress);
            PlayerController->ClientTravel(IpAddress, ETravelType::TRAVEL_Absolute);
        }
        else
        {
             UE_LOG(LogTemp, Error, TEXT("Could not get PlayerController to join by IP."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("IP address field is empty. Cannot join game."));
        // При желании можно показать пользователю сообщение в UI
    }
}

void UStartMenuWidget::OnPlayerColorSliderChanged(float Value)
{
    if (AChessPlayerController* PlayerController = GetOwningPlayer<AChessPlayerController>())
    {
        // Округляем значение до ближайшего целого (0, 1 или 2)
        const int32 ChoiceIndex = FMath::RoundToInt(Value);
        PlayerController->SetPlayerColorChoiceForBotGame(ChoiceIndex);
    }
}

void UStartMenuWidget::OnTimeControlChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (SelectedItem == TEXT("Bullet (1|0)"))
    {
        SelectedTimeControl = ETimeControlType::Bullet_1_0;
    }
    else if (SelectedItem == TEXT("Blitz (3|2)"))
    {
        SelectedTimeControl = ETimeControlType::Blitz_3_2;
    }
    else if (SelectedItem == TEXT("Rapid (10|0)"))
    {
        SelectedTimeControl = ETimeControlType::Rapid_10_0;
    }
    else // Unlimited
    {
        SelectedTimeControl = ETimeControlType::Unlimited;
    }
}

void UStartMenuWidget::OnExitClicked()
{
    FGenericPlatformMisc::RequestExit(false);
}

void UStartMenuWidget::HideMenu()
{
    this->RemoveFromParent();

    if (AChessPlayerController* PlayerController = GetOwningPlayer<AChessPlayerController>())
    {
        PlayerController->SetInputModeForGame();
        PlayerController->bShowMouseCursor = true;
    }
}
