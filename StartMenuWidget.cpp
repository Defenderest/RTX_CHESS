
#include "StartMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "ChessGameMode.h"
#include "ChessPlayerController.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Slider.h"
#include "Components/EditableTextBox.h"

void UStartMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    GameLevelName = "Cigar_room";
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
    if (AChessPlayerController* PlayerController = GetOwningPlayer<AChessPlayerController>())
    {
        if (SessionNameInput && !SessionNameInput->GetText().IsEmpty())
        {
            const FString SessionName = SessionNameInput->GetText().ToString();
            HideMenu();
            PlayerController->HostSession(SessionName);
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
    if (AChessPlayerController* PlayerController = GetOwningPlayer<AChessPlayerController>())
    {
        if (SessionNameInput && !SessionNameInput->GetText().IsEmpty())
        {
            const FString SessionName = SessionNameInput->GetText().ToString();
            HideMenu();
            PlayerController->FindAndJoinSession(SessionName);
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

    if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        PlayerController->bShowMouseCursor = true; // Показываем курсор
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // Не блокировать мышь в окне
        InputMode.SetHideCursorDuringCapture(false); // Не прятать курсор при вращении камеры
        PlayerController->SetInputMode(InputMode);
    }
}
