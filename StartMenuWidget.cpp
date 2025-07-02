
#include "StartMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "ChessGameMode.h"
#include "ChessPlayerController.h"

void UStartMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    GameLevelName = "Cigar_room";
}

void UStartMenuWidget::OnStartPlayerVsPlayerClicked()
{
    OnStartGame(false);
}

void UStartMenuWidget::OnStartPlayerVsBotClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("UStartMenuWidget::OnStartPlayerVsBotClicked() - Button press detected!"));
    OnStartGame(true);
}

void UStartMenuWidget::OnStartGame(bool bIsBotGame)
{
    UE_LOG(LogTemp, Log, TEXT("OnStartGame called. bIsBotGame: %s"), bIsBotGame ? TEXT("true") : TEXT("false"));
    if (GameLevelName.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT("GameLevelName is not set!"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Attempting to open level: %s"), *GameLevelName.ToString());
    FString Options = bIsBotGame ? "?bIsBotGame=true" : "";
    UGameplayStatics::OpenLevel(GetWorld(), GameLevelName, true, Options);
    // HideMenu() убран, так как загрузка уровня сама уничтожит виджет.
}

void UStartMenuWidget::OnStartOnlineGameClicked()
{
    // TODO: Implement online game logic
    UE_LOG(LogTemp, Warning, TEXT("Online game functionality is not implemented yet."));
    OnStartGame(false);
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
