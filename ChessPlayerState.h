#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "PlayerProfile.h"
#include "ChessPlayerState.generated.h"

UCLASS()
class RTX_CHESS_API AChessPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AChessPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Player Profile")
	const FPlayerProfile& GetPlayerProfile() const;

	// Called on server to update the profile
	void SetPlayerProfile(const FPlayerProfile& NewProfile);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_PlayerProfile)
	FPlayerProfile PlayerProfile;

	UFUNCTION()
	void OnRep_PlayerProfile();
};
