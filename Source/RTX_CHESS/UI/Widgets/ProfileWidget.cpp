#include "ProfileWidget.h"
#include "PlayerProfile.h"
#include "ChessGameInstance.h"
#include "ChessPlayerController.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UProfileWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Привязываем функцию сохранения к нажатию кнопки
    if (SaveButton)
    {
        SaveButton->OnClicked.AddDynamic(this, &UProfileWidget::OnSaveClicked);
    }

    // Заполняем поля данными из профиля при открытии виджета
    const FPlayerProfile Profile = GetCurrentProfile();
    if (PlayerNameInput)
    {
        PlayerNameInput->SetText(FText::FromString(Profile.PlayerName));
    }
    
    if (EloRatingText)
    {
        EloRatingText->SetText(GetEloRating());
    }
    if (CountryText)
    {
        CountryText->SetText(GetCountry());
    }
    if (GameStatsText)
    {
        GameStatsText->SetText(GetGameStats());
    }
}

FText UProfileWidget::GetPlayerName() const
{
    return FText::FromString(GetCurrentProfile().PlayerName);
}

FText UProfileWidget::GetEloRating() const
{
    return FText::FromString(FString::Printf(TEXT("Рейтинг: %d"), GetCurrentProfile().EloRating));
}

FText UProfileWidget::GetCountry() const
{
    const FString& Country = GetCurrentProfile().Country;
    return FText::FromString(FString::Printf(TEXT("Страна: %s"), Country.IsEmpty() ? TEXT("Не указана") : *Country));
}

FText UProfileWidget::GetGameStats() const
{
    const FPlayerProfile& Profile = GetCurrentProfile();
    return FText::FromString(FString::Printf(TEXT("Статистика: %d (В) / %d (П) / %d (Н)"), Profile.GamesWon, Profile.GamesLost, Profile.GamesDrawn));
}

void UProfileWidget::OnSaveClicked()
{
    UChessGameInstance* GameInstance = GetGameInstance<UChessGameInstance>();
    AChessPlayerController* PlayerController = GetOwningPlayer<AChessPlayerController>();

    if (!GameInstance || !PlayerController)
    {
        UE_LOG(LogTemp, Error, TEXT("ProfileWidget: GameInstance или PlayerController не найдены."));
        return;
    }

    if (!PlayerNameInput || PlayerNameInput->GetText().IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ProfileWidget: Имя игрока не может быть пустым."));
        return;
    }
    
    FPlayerProfile UpdatedProfile = GetCurrentProfile();
    UpdatedProfile.PlayerName = PlayerNameInput->GetText().ToString();
    
    GameInstance->UpdatePlayerProfile(UpdatedProfile);
    GameInstance->SavePlayerProfile();
    PlayerController->Server_SetPlayerProfile(UpdatedProfile);

    UE_LOG(LogTemp, Log, TEXT("Профиль игрока '%s' сохранен."), *UpdatedProfile.PlayerName);

    // Закрываем виджет после сохранения.
    PlayerController->ToggleProfileWidget();
}

FPlayerProfile UProfileWidget::GetCurrentProfile() const
{
    if (UChessGameInstance* GameInstance = GetGameInstance<UChessGameInstance>())
    {
        return GameInstance->GetPlayerProfile();
    }
    return FPlayerProfile();
}
