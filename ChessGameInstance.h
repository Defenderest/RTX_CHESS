#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "ChessGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionCreatedDelegate, bool, bWasSuccessful);

UCLASS()
class RTX_CHESS_API UChessGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UChessGameInstance();

	virtual void Init() override;

	UFUNCTION(BlueprintCallable, Exec, Category = "Network")
	void HostSession(const FString& SessionName, FName LevelName);

	UFUNCTION(BlueprintCallable, Exec, Category = "Network")
	void FindAndJoinSession(const FString& SessionName);

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnSessionCreatedDelegate OnSessionCreated;

protected:
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	// --- Callbacks ---
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	// --- Internal Methods ---
	void CreateSession(const FString& SessionName);
	void FindSessions();
	void JoinSession(const FOnlineSessionSearchResult& SearchResult);

private:
    // --- Session Data ---
    FName LevelNameToHost;
    FString SessionNameToFind;
    FString SessionNameToCreate;
    int32 FindSessionRetryCount;
    FTimerHandle FindSessionTimerHandle;
    bool bIsFindingSessions;
    bool bIsHost;

    static const int32 MAX_FIND_SESSION_RETRIES = 3;
	
	// --- Delegate Handles ---
    FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
    FOnDestroySessionCompleteDelegate OnDestroySessionCompleteDelegate;
    FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
    FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;

    FDelegateHandle OnCreateSessionCompleteDelegateHandle;
    FDelegateHandle OnDestroySessionCompleteDelegateHandle;
    FDelegateHandle OnFindSessionsCompleteDelegateHandle;
    FDelegateHandle OnJoinSessionCompleteDelegateHandle;
};
