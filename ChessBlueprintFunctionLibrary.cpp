#include "ChessBlueprintFunctionLibrary.h"
#include "ChessGameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UChessBlueprintFunctionLibrary::ShowChessGraphicsSettings(const UObject* WorldContextObject)
{
	UChessGameInstance* GameInstance = Cast<UChessGameInstance>(UGameplayStatics::GetGameInstance(WorldContextObject));
	if (GameInstance)
	{
		GameInstance->ShowGraphicsSettingsMenu();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ShowChessGraphicsSettings: Could not get a valid UChessGameInstance. Make sure your Game Instance class is set correctly in Project Settings."));
	}
}
