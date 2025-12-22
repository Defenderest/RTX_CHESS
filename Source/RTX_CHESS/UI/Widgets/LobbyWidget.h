#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/ChessGameMode.h" // For ETimeControlType
#include "LobbyWidget.generated.h"

class UTextBlock;
class UListView;
class UButton;
class UImage;

UCLASS()
class RTX_CHESS_API ULobbyWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void UpdateLobbyInfo();

    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION(BlueprintPure, Category = "Lobby")
    FString GetLobbyIPAddress() const;

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TimeControlText;

    // --- Поля для статистики матчу ---
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> WhitePlayerAvatar;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> WhitePlayerNameText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> WhitePlayerRatingText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> BlackPlayerAvatar;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> BlackPlayerNameText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> BlackPlayerRatingText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> GamePhaseText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> FullmoveText;
    // --------------------------------------

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> StartGameButton;
    
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> LeaveLobbyButton;

    UFUNCTION()
    void OnStartGameClicked();

    UFUNCTION()
    void OnLeaveLobbyClicked();
};
