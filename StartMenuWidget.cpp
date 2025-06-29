
#include "StartMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "ChessGameMode.h"
#include "ChessPlayerController.h"

void UStartMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Здесь можно будет привязать функции к кнопкам, если они созданы в C++
    // Но мы будем делать это в Blueprint
}

void UStartMenuWidget::OnStartPlayerVsPlayerClicked()
{
    if (AChessGameMode* GameMode = Cast<AChessGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
    {
        GameMode->StartNewGame();
        HideMenu();
    }
}

void UStartMenuWidget::OnStartPlayerVsBotClicked()
{
    if (AChessGameMode* GameMode = Cast<AChessGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
    {
        GameMode->StartBotGame();
        HideMenu();
    }
}

void UStartMenuWidget::OnStartOnlineGameClicked()
{
    // TODO: Implement online game logic
    UE_LOG(LogTemp, Warning, TEXT("Online game functionality is not implemented yet."));
    // Пока что можно просто скрыть меню и начать обычную игру
    if (AChessGameMode* GameMode = Cast<AChessGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
    {
        GameMode->StartNewGame();
        HideMenu();
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
