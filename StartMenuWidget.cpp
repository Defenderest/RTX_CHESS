
#include "StartMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "ChessGameMode.h"
#include "ChessPlayerController.h"
#include "ChessGameInstance.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Slider.h"

void UStartMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    GameLevelName = TEXT("/Game/Cigar_room/Maps/Cigar_room");

    // Альтернативный способ получения виджета: поиск по имени.
    // Этот метод не требует привязки через "Is Variable" или BindWidget.
    SessionNameInput = Cast<UEditableTextBox>(GetWidgetFromName(TEXT("SessionNameInput")));
    if (!SessionNameInput)
    {
        // Это не критическая ошибка, а предупреждение для разработчика.
        UE_LOG(LogTemp, Warning, TEXT("UStartMenuWidget: Не удалось найти EditableTextBox с именем 'SessionNameInput'. Убедитесь, что виджет с таким именем существует на панели OnlineMenuPanel."));
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
    FString Options = TEXT("?bIsBotGame=false");
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

    FString Options = FString::Printf(TEXT("?bIsBotGame=true?SkillLevel=%d"), SkillLevel);
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

void UStartMenuWidget::OnHostGameClicked()
{
    if (UChessGameInstance* GameInstance = Cast<UChessGameInstance>(GetGameInstance()))
    {
        if (SessionNameInput && !SessionNameInput->GetText().IsEmpty())
        {
            const FString SessionName = SessionNameInput->GetText().ToString();
            HideMenu();
            GameInstance->HostSession(SessionName, GameLevelName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Session name is empty. Cannot host game."));
            // При желании можно показать пользователю сообщение в UI
        }
    }
}

void UStartMenuWidget::OnJoinGameClicked()
{
    if (UChessGameInstance* GameInstance = Cast<UChessGameInstance>(GetGameInstance()))
    {
        if (SessionNameInput && !SessionNameInput->GetText().IsEmpty())
        {
            const FString SessionName = SessionNameInput->GetText().ToString();
            HideMenu();
            GameInstance->FindAndJoinSession(SessionName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Session name is empty. Cannot join game."));
            // При желании можно показать пользователю сообщение в UI
        }
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
