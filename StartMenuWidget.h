
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StartMenuWidget.generated.h"

UCLASS()
class RTX_CHESS_API UStartMenuWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnStartPlayerVsPlayerClicked();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnStartPlayerVsBotClicked();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnStartOnlineGameClicked();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void OnExitClicked();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    FName GameLevelName;

private:
    void HideMenu();
    void OnStartGame(bool bIsBotGame);
};
