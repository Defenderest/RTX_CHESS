#include "Board/ChessBlueprintFunctionLibrary.h"
#include "Controllers/ChessPlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UChessBlueprintFunctionLibrary::ShowChessGraphicsSettings(const UObject* WorldContextObject)
{
	AChessPlayerController* PlayerController = Cast<AChessPlayerController>(UGameplayStatics::GetPlayerController(WorldContextObject, 0));
	if (PlayerController)
	{
		PlayerController->ToggleGraphicsSettingsMenu();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ShowChessGraphicsSettings: Could not get a valid AChessPlayerController."));
	}
}
