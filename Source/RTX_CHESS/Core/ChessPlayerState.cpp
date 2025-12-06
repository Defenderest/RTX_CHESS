#include "Core/ChessPlayerState.h"
#include "Net/UnrealNetwork.h"

AChessPlayerState::AChessPlayerState()
{
	// Default values are set in FPlayerProfile constructor
}

void AChessPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AChessPlayerState, PlayerProfile);
}

const FPlayerProfile& AChessPlayerState::GetPlayerProfile() const
{
	return PlayerProfile;
}

void AChessPlayerState::SetPlayerProfile(const FPlayerProfile& NewProfile)
{
	if (HasAuthority())
	{
		PlayerProfile = NewProfile;
		// Also update the base PlayerState name
		SetPlayerName(NewProfile.PlayerName);
		// OnRep will be called automatically on clients.
		// For the server, we call it manually to ensure UI updates if needed.
		OnRep_PlayerProfile();
	}
}

void AChessPlayerState::OnRep_PlayerProfile()
{
	// Update base player name on client as well when profile replicates.
	SetPlayerName(PlayerProfile.PlayerName);
	// In the future, we would broadcast an event here to update UI.
}
