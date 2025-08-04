#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PlayerProfile.h"
#include "ChessSaveGame.generated.h"

UCLASS()
class RTX_CHESS_API UChessSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "SaveGame")
	FPlayerProfile PlayerProfile;

	UPROPERTY(VisibleAnywhere, Category = "SaveGame")
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = "SaveGame")
	uint32 UserIndex;

	UChessSaveGame();
};
