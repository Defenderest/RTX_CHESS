#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChessGameMode.h" // For ETimeControlType
#include "LobbyWidget.generated.h"

class UTextBlock;
class UListView;
class UButton;

UCLASS()
class RTX_CHESS_API ULobbyWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void UpdateLobbyInfo();

    UFUNCTION(BlueprintPure, Category = "Lobby")
    FString GetLobbyIPAddress() const;

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> IpAddressText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> TimeControlText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UListView> PlayerListView;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> StartGameButton;
    
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> LeaveLobbyButton;

    UFUNCTION()
    void OnStartGameClicked();

    UFUNCTION()
    void OnLeaveLobbyClicked();
};
