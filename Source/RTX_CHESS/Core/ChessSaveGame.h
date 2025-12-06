#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PlayerProfile.h"
#include "UI/Data/GraphicsSettingsData.h" // Добавляем структуру с настройками
#include "ChessSaveGame.generated.h"

UCLASS()
class RTX_CHESS_API UChessSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "SaveGame")
	FPlayerProfile PlayerProfile;

	/** Сохраненные настройки графики. */
	UPROPERTY(VisibleAnywhere, Category = "SaveGame")
	FGraphicsSettingsData GraphicsSettings;

	UPROPERTY(VisibleAnywhere, Category = "SaveGame")
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = "SaveGame")
	uint32 UserIndex;

	UChessSaveGame();
};
