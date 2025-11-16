#include "LobbyWidget.h"
#include "ChessPlayerController.h"
#include "ChessGameState.h"
#include "ChessGameInstance.h"
#include "Components/TextBlock.h"
#include "Components/ListView.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"

void ULobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (StartGameButton)
    {
        StartGameButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnStartGameClicked);
    }
    if (LeaveLobbyButton)
    {
        LeaveLobbyButton->OnClicked.AddDynamic(this, &ULobbyWidget::OnLeaveLobbyClicked);
    }

    UpdateLobbyInfo();
}

void ULobbyWidget::UpdateLobbyInfo()
{
    AChessPlayerController* PC = GetOwningPlayer<AChessPlayerController>();
    AChessGameState* GS = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;

    if (!PC || !GS)
    {
        return;
    }
    
    // --- Update Lobby IP ---
    if (IpAddressText)
    {
        IpAddressText->SetText(FText::FromString(GetLobbyIPAddress()));
    }

    // --- Update Time Control ---
    if (TimeControlText)
    {
        FString TCString = "Unlimited";
        switch(GS->LobbyTimeControl)
        {
            case ETimeControlType::Bullet_1_0: TCString = "Bullet (1|0)"; break;
            case ETimeControlType::Blitz_3_2: TCString = "Blitz (3|2)"; break;
            case ETimeControlType::Rapid_10_0: TCString = "Rapid (10|0)"; break;
            default: break;
        }
        TimeControlText->SetText(FText::FromString(TCString));
    }
    
    // --- Update Player List ---
    if (PlayerListView)
    {
        PlayerListView->ClearListItems();
        for (APlayerState* PS : GS->PlayerArray)
        {
            if (PS)
            {
                PlayerListView->AddItem(PS);
            }
        }
    }
    
    // --- Update Button Visibility ---
    if (StartGameButton)
    {
        // Только хост может начать игру.
        StartGameButton->SetVisibility(PC->IsHost() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

FString ULobbyWidget::GetLobbyIPAddress() const
{
    if (UChessGameInstance* GI = GetGameInstance<UChessGameInstance>())
    {
        return GI->GetSessionHostAddress();
    }
    return TEXT("N/A");
}

void ULobbyWidget::OnStartGameClicked()
{
    if (AChessPlayerController* PC = GetOwningPlayer<AChessPlayerController>())
    {
        PC->Server_RequestStartGame();
    }
}

void ULobbyWidget::OnLeaveLobbyClicked()
{
    if (AChessPlayerController* PC = GetOwningPlayer<AChessPlayerController>())
    {
        PC->LeaveLobby();
    }
}
