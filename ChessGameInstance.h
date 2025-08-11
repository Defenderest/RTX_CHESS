#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "ChessGameMode.h" // For ETimeControlType
#include "PlayerProfile.h"
#include "ChessGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionCreatedDelegate, bool, bWasSuccessful);

struct FGraphicsSettingsData; // Forward declaration

UCLASS()
class RTX_CHESS_API UChessGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UChessGameInstance();

	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintCallable, Exec, Category = "Network")
	void HostSession(const FString& SessionName, FName LevelName, ETimeControlType TimeControl);

	UFUNCTION(BlueprintCallable, Exec, Category = "Network")
	void FindAndJoinSession(const FString& SessionName);

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnSessionCreatedDelegate OnSessionCreated;

	UFUNCTION(BlueprintPure, Category = "Player Profile")
	const FPlayerProfile& GetPlayerProfile() const;

	UFUNCTION(BlueprintCallable, Category = "Player Profile")
	void UpdatePlayerProfile(const FPlayerProfile& NewProfile);

	/** Возвращает класс виджета для меню настроек графики. */
	UFUNCTION(BlueprintPure, Category = "UI")
	TSubclassOf<class UUserWidget> GetGraphicsSettingsWidgetClass() const;

	/** [Settings] Получает текущие сохраненные настройки графики. */
	UFUNCTION(BlueprintPure, Category = "Settings")
	FGraphicsSettingsData GetGraphicsSettings() const;

	/** [Settings] Обновляет настройки графики, применяет их и сохраняет на диск. */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void UpdateGraphicsSettings(const FGraphicsSettingsData& NewSettings);

	void SavePlayerProfile();

	FString GetSessionHostAddress() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> LoadingScreenWidgetClass;

	/** Виджет для меню настроек графики. Назначается в Blueprint. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> GraphicsSettingsWidgetClass;

	void BeginLoadingScreen(const FString& MapName);
	void EndLoadingScreen(UWorld* InLoadedWorld);

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
	void LoadPlayerProfile();

	/** Применяет настройки графики из CurrentSaveGame к UGameUserSettings. */
	void ApplyGraphicsSettings();

private:
    // --- Session Data ---
    ETimeControlType StagedTimeControl;
    FName LevelNameToHost;
    FString SessionNameToFind;
    FString SessionNameToCreate;
    int32 FindSessionRetryCount;
    FTimerHandle FindSessionTimerHandle;
    bool bIsFindingSessions;
    bool bIsHost;

    FString SessionHostAddress;

    UPROPERTY()
    TObjectPtr<class UUserWidget> LoadingScreenWidgetInstance;

    UPROPERTY()
	TObjectPtr<class UChessSaveGame> CurrentSaveGame;

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
