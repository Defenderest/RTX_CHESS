#include "UI/Widgets/LobbyWidget.h"
#include "Controllers/ChessPlayerController.h"
#include "Core/ChessGameState.h"
#include "Core/ChessGameInstance.h"
#include "Core/ChessPlayerState.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
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

void ULobbyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    UpdateLobbyInfo();
}

void ULobbyWidget::UpdateLobbyInfo()
{
    AChessPlayerController* PC = GetOwningPlayer<AChessPlayerController>();
    AChessGameState* GS = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
    UChessGameInstance* GI = Cast<UChessGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));

    if (!PC || !GS || !GI) return;
    
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

    // --- Пошук та оновлення даних гравців (Білі) ---
    if (WhitePlayerNameText) WhitePlayerNameText->SetText(FText::FromString(GS->WhitePlayerProfile.PlayerName));
    if (WhitePlayerRatingText) WhitePlayerRatingText->SetText(FText::AsNumber(GS->WhitePlayerProfile.EloRating));
    
    // --- Пошук та оновлення даних гравців (Чорні) ---
    if (BlackPlayerNameText) BlackPlayerNameText->SetText(FText::FromString(GS->BlackPlayerProfile.PlayerName));
    if (BlackPlayerRatingText) BlackPlayerRatingText->SetText(FText::AsNumber(GS->BlackPlayerProfile.EloRating));

    // --- Завантаження аватарів через Steam ---
    for (APlayerState* PS : GS->PlayerArray)
    {
        if (AChessPlayerState* ChessPS = Cast<AChessPlayerState>(PS))
        {
            UTexture2D* Avatar = GI->GetSteamAvatar(PS);
            if (Avatar)
            {
                if (GS->WhitePlayerProfile.PlayerName == ChessPS->GetPlayerProfile().PlayerName && WhitePlayerAvatar)
                {
                    WhitePlayerAvatar->SetBrushFromTexture(Avatar);
                }
                else if (GS->BlackPlayerProfile.PlayerName == ChessPS->GetPlayerProfile().PlayerName && BlackPlayerAvatar)
                {
                    BlackPlayerAvatar->SetBrushFromTexture(Avatar);
                }
            }
        }
    }

    if (GamePhaseText)
    {
        FString PhaseStr = UEnum::GetValueAsString(GS->GetGamePhase());
        PhaseStr.Split(TEXT("::"), nullptr, &PhaseStr);
        GamePhaseText->SetText(FText::FromString(PhaseStr));
    }

    if (FullmoveText)
    {
        FString FullHistory = "";
        for (const FString& Move : GS->MoveHistory)
        {
            FullHistory += Move + "\n";
        }
        
        if (FullHistory.IsEmpty())
        {
            FullHistory = "No moves yet.";
        }

        FullmoveText->SetText(FText::FromString(FullHistory));
    }
    
    // --- Керування кнопками ---
    if (StartGameButton)
    {
        // Показуємо старт лише в лобі і лише хосту
        StartGameButton->SetVisibility((PC->IsHost() && GS->bIsInLobby) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
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