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

	FPlayerProfile()
		: PlayerName(TEXT("Player"))
		, EloRating(1200)
		, Country(TEXT("US"))
	{
	}
};
