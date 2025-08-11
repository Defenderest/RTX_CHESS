#include "PlayerListEntryWidget.h"
#include "Components/TextBlock.h"
#include "ChessPlayerState.h"

void UPlayerListEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    AChessPlayerState* PlayerState = Cast<AChessPlayerState>(ListItemObject);
    if (!PlayerState)
    {
        return;
    }

    if (PlayerNameText)
    {
        const FPlayerProfile& Profile = PlayerState->GetPlayerProfile();
        PlayerNameText->SetText(FText::FromString(Profile.PlayerName));
    }
    
    if (PlayerPingText)
    {
        PlayerPingText->SetText(FText::AsNumber(static_cast<int32>(PlayerState->GetPingInMilliseconds())));
    }
}
