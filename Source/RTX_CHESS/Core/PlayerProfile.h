#pragma once

#include "CoreMinimal.h"
#include "PlayerProfile.generated.h"

USTRUCT(BlueprintType)
struct FPlayerProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	FString PlayerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	int32 EloRating;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	FString Country;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	int32 GamesWon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	int32 GamesLost;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	int32 GamesDrawn;

	FPlayerProfile()
		: PlayerName(TEXT("Player"))
		, EloRating(1200)
		, Country(TEXT("US"))
		, GamesWon(0)
		, GamesLost(0)
		, GamesDrawn(0)
	{
	}
};
