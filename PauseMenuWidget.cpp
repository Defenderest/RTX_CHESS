#include "PauseMenuWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "ChessPlayerController.h"
#include "ChessBlueprintFunctionLibrary.h"

void UPauseMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (ResumeButton)
    {
        ResumeButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnResumeClicked);
    }
    if (SettingsButton)
    {
        SettingsButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnSettingsClicked);
    }
    if (ExitToMenuButton)
    {
        ExitToMenuButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnExitToMenuClicked);
    }
    if (QuitGameButton)
    {
        QuitGameButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnQuitGameClicked);
    }
}

void UPauseMenuWidget::OnResumeClicked()
{
    if (AChessPlayerController* PlayerController = Cast<AChessPlayerController>(GetOwningPlayer()))
    {
        PlayerController->TogglePauseMenu();
    }
}

void UPauseMenuWidget::OnSettingsClicked()
{
    UChessBlueprintFunctionLibrary::ShowChessGraphicsSettings(this);
}

void UPauseMenuWidget::OnExitToMenuClicked()
{
    UGameplayStatics::OpenLevel(GetWorld(), MainMenuLevelName);
}

void UPauseMenuWidget::OnQuitGameClicked()
{
    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        // Используем команду "quit" для корректного выхода из игры
        PlayerController->ConsoleCommand("quit");
    }
}

FReply UPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // Позволяет закрывать меню паузы по клавише Escape, даже когда фокус на UI.
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        OnResumeClicked();
        return FReply::Handled();
    }
    
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
