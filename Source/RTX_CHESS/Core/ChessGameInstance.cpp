#include "Core/ChessGameInstance.h"
#include "UI/Data/GraphicsSettingsData.h"
#include "Core/ChessGameMode.h" 
#include "Core/ChessSaveGame.h"
#include "GameFramework/GameUserSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Blueprint/UserWidget.h"
#include "Modules/ModuleManager.h"
#include "Framework/Application/SlateApplication.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineExternalUIInterface.h"

#include "GameFramework/PlayerState.h"
#include "Engine/Texture2D.h"

// --- Steam Includes ---
#pragma push_macro("h")
#undef h
#include "steam/steam_api.h"
#pragma pop_macro("h")
// ----------------------

// Helper function to convert EOnJoinSessionCompleteResult::Type to FString
FString GetJoinSessionResultString(EOnJoinSessionCompleteResult::Type Result)
{
    switch (Result)
    {
    case EOnJoinSessionCompleteResult::Success: return TEXT("Success");
    case EOnJoinSessionCompleteResult::SessionIsFull: return TEXT("Session is full");
    case EOnJoinSessionCompleteResult::SessionDoesNotExist: return TEXT("Session does not exist");
    case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress: return TEXT("Could not retrieve address");
    case EOnJoinSessionCompleteResult::AlreadyInSession: return TEXT("Already in session");
    case EOnJoinSessionCompleteResult::UnknownError:
    default:
        return TEXT("Unknown error");
    }
}

const int32 UChessGameInstance::MAX_FIND_SESSION_RETRIES;

UChessGameInstance::UChessGameInstance()
{
	FindSessionRetryCount = 0;
	bIsFindingSessions = false;
	bIsHost = false;
    StagedTimeControl = ETimeControlType::Unlimited;
}

void UChessGameInstance::Init()
{
	Super::Init();

	if (GEngine)
	{
		UGameUserSettings* UserSettings = GEngine->GetGameUserSettings();
		if (UserSettings)
		{
			UserSettings->SetFrameRateLimit(0); // 0 = без ограничений
			UserSettings->SetVSyncEnabled(false);
			UserSettings->ApplySettings(false);
		}
	}
	
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		UE_LOG(LogTemp, Log, TEXT("[Init] Found OnlineSubsystem: %s"), *Subsystem->GetSubsystemName().ToString());
		SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			UE_LOG(LogTemp, Log, TEXT("[Init] SessionInterface is valid. Binding delegates."));
			OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UChessGameInstance::OnCreateSessionComplete);
			OnDestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UChessGameInstance::OnDestroySessionComplete);
			OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UChessGameInstance::OnFindSessionsComplete);
			OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UChessGameInstance::OnJoinSessionComplete);
			OnSessionUserInviteAcceptedDelegate = FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &UChessGameInstance::OnSessionUserInviteAccepted);
			OnSessionUserInviteAcceptedDelegateHandle = SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegate);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[Init] Failed to get SessionInterface from Subsystem: %s"), *Subsystem->GetSubsystemName().ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Init] Failed to get OnlineSubsystem. Online functionality will be disabled."));
	}

	LoadPlayerProfile();

	if (!IsRunningDedicatedServer())
	{
		// Применяем сохраненные настройки графики поверх настроек по умолчанию
		ApplyGraphicsSettings();

		// Настраиваем загрузочный экран для переходов между картами
		FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UChessGameInstance::BeginLoadingScreen);
		FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UChessGameInstance::EndLoadingScreen);
	}
}

void UChessGameInstance::Shutdown()
{
	Super::Shutdown();
	
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegateHandle);
	}
	
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
}

void UChessGameInstance::BeginLoadingScreen(const FString& MapName)
{
	UE_LOG(LogTemp, Log, TEXT("--- BeginLoadingScreen triggered for map: %s ---"), *MapName);
	if (IsRunningDedicatedServer() || LoadingScreenWidgetInstance)
	{
		return;
	}

	if (LoadingScreenWidgetClass)
	{
		UE_LOG(LogTemp, Log, TEXT("Showing loading screen."));
		LoadingScreenWidgetInstance = CreateWidget<UUserWidget>(this, LoadingScreenWidgetClass);
		if (LoadingScreenWidgetInstance)
		{
			LoadingScreenWidgetInstance->AddToViewport(9999);
			// Принудительно отрисовываем кадр, чтобы загрузочный экран появился до блокировки потока
			FSlateApplication::Get().Tick();
		}
	}
	else
	{
		// --- ЧЕТКАЯ ДИАГНОСТИКА ---
		// Если мы видим это сообщение, значит, в настройках проекта указан не тот GameInstance.
		// Нужно указать Blueprint-класс, в котором задан LoadingScreenWidgetClass.
		const FString ErrorMessage = FString::Printf(
			TEXT("!!! ACTION REQUIRED: LoadingScreenWidgetClass is NOT SET on GameInstance of class '%s'. Go to Project Settings -> Maps & Modes and set 'Game Instance Class' to your Blueprint, e.g., 'BP_ChessGameInstance'."),
			*GetClass()->GetName()
		);
		UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMessage);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Red, ErrorMessage);
		}
	}
}

void UChessGameInstance::EndLoadingScreen(UWorld* InLoadedWorld)
{
	if (LoadingScreenWidgetInstance)
	{
		UE_LOG(LogTemp, Log, TEXT("Hiding loading screen."));
		LoadingScreenWidgetInstance->RemoveFromParent();
		LoadingScreenWidgetInstance = nullptr;
	}
}

void UChessGameInstance::HostSession(const FString& SessionName, FName LevelName, ETimeControlType TimeControl)
{
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[NetworkSession] HostSession ABORTED: SessionInterface is not valid."));
        return;
    }
    if (SessionName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[NetworkSession] HostSession ABORTED: SessionName is empty."));
        return;
    }

    StagedTimeControl = TimeControl;
    LevelNameToHost = LevelName;
    SessionNameToCreate = SessionName;
    bIsHost = true; // Mark this instance as the host
    UE_LOG(LogTemp, Log, TEXT("[NetworkSession] --- Starting Host Process ---"));
    UE_LOG(LogTemp, Log, TEXT("[NetworkSession] Caching LevelNameToHost: %s and SessionNameToCreate: %s"), *LevelNameToHost.ToString(), *SessionNameToCreate);

    auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
    if (ExistingSession != nullptr)
    {
        UE_LOG(LogTemp, Log, TEXT("[NetworkSession] Found an existing session named '%s'. Destroying it before creating a new one..."), *ExistingSession->SessionName.ToString());
        OnDestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);
        SessionInterface->DestroySession(NAME_GameSession);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[NetworkSession] No existing session found. Proceeding to create a new one."));
        CreateSession(SessionNameToCreate);
    }
}

void UChessGameInstance::FindAndJoinSession(const FString& SessionName)
{
    UE_LOG(LogTemp, Log, TEXT("[NetworkSession] FindAndJoinSession triggered for session: '%s'"), *SessionName);

    if (bIsHost)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NetworkSession] FindAndJoinSession IGNORED: This client is the host and cannot join another session."));
        return;
    }

    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[NetworkSession] FindAndJoinSession ABORTED: SessionInterface is not valid."));
        return;
    }
    if (SessionName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[NetworkSession] FindAndJoinSession ABORTED: SessionName is empty."));
        return;
    }

    if (bIsFindingSessions)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NetworkSession] FindAndJoinSession IGNORED: A session search is already in progress."));
        return;
    }

    auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
    if (ExistingSession != nullptr && ExistingSession->SessionState == EOnlineSessionState::InProgress)
    {
        UE_LOG(LogTemp, Warning, TEXT("[NetworkSession] FindAndJoinSession ABORTED: Already in an active session ('%s')."), *ExistingSession->SessionName.ToString());
        return;
    }

    SessionNameToFind = SessionName;
    FindSessionRetryCount = 0;
    bIsFindingSessions = true;
    UE_LOG(LogTemp, Log, TEXT("[NetworkSession] Starting search for session '%s'. Retry count reset."), *SessionNameToFind);

    GetWorld()->GetTimerManager().ClearTimer(FindSessionTimerHandle);
    
    UE_LOG(LogTemp, Log, TEXT("[NetworkSession] Waiting 3.0s before starting first session search..."));
    GetWorld()->GetTimerManager().SetTimer(FindSessionTimerHandle, this, &UChessGameInstance::FindSessions, 3.0f, false);
}

void UChessGameInstance::FindSessions()
{
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[NetworkSession] FindSessions ABORTED: SessionInterface is not valid."));
        bIsFindingSessions = false; // Reset state since we can't proceed
        return;
    }
    
    FindSessionRetryCount++;
    UE_LOG(LogTemp, Log, TEXT("[NetworkSession] --- Starting session search (Attempt %d/%d) ---"), FindSessionRetryCount, MAX_FIND_SESSION_RETRIES);

    SessionSearch = MakeShareable(new FOnlineSessionSearch());
    SessionSearch->bIsLanQuery = false;
    SessionSearch->MaxSearchResults = 20; // Increase max results to find more Steam lobbies
    SessionSearch->QuerySettings.Set(FName(TEXT("ROOM_NAME_KEY")), SessionNameToFind, EOnlineComparisonOp::Equals);

    UE_LOG(LogTemp, Log, TEXT("[NetworkSession] SessionSearch object created. IsLANQuery=%d. MaxResults=%d. Searching for ROOM_NAME_KEY='%s'."), SessionSearch->bIsLanQuery, SessionSearch->MaxSearchResults, *SessionNameToFind);

    OnFindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);
    UE_LOG(LogTemp, Log, TEXT("[NetworkSession] OnFindSessionsComplete delegate handle bound."));

    // GameInstance does not have a local player directly. We get it from the world.
    ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
    if (LocalPlayer)
    {
        UE_LOG(LogTemp, Log, TEXT("[NetworkSession] Issuing FindSessions call for player %d..."), LocalPlayer->GetControllerId());
        SessionInterface->FindSessions(LocalPlayer->GetControllerId(), SessionSearch.ToSharedRef());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[NetworkSession] FindSessions FAILED: Cannot get LocalPlayer."));
        bIsFindingSessions = false; // Reset state
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle); // Clean up delegate
    }
}

void UChessGameInstance::JoinFoundSession(const FOnlineSessionSearchResult& SearchResult)
{
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[NetworkSession] JoinSession failed: SessionInterface is not valid."));
        return;
    }
    OnJoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
    
    ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
    if (LocalPlayer)
    {
        UE_LOG(LogTemp, Log, TEXT("[NetworkSession] Player %d joining session..."), LocalPlayer->GetControllerId());
        SessionInterface->JoinSession(LocalPlayer->GetControllerId(), NAME_GameSession, SearchResult);
    }
     else
    {
        UE_LOG(LogTemp, Error, TEXT("[NetworkSession] JoinSession failed: Cannot get LocalPlayer."));
    }
}

void UChessGameInstance::CreateSession(const FString& SessionName)
{
    if (!SessionInterface.IsValid()) return;

    OnCreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);

    TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
    SessionSettings->bIsLANMatch = false;
    SessionSettings->NumPublicConnections = 2;
    SessionSettings->bShouldAdvertise = true;
    SessionSettings->bUsesPresence = true;
    SessionSettings->bUseLobbiesIfAvailable = true;
    SessionSettings->bAllowJoinInProgress = true;
    SessionSettings->Set(FName(TEXT("ROOM_NAME_KEY")), SessionName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    UE_LOG(LogTemp, Log, TEXT("[NetworkSession] SessionSettings configured for STEAM. ROOM_NAME_KEY = %s"), *SessionName);
	
    ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
    if(LocalPlayer)
    {
        UE_LOG(LogTemp, Log, TEXT("[NetworkSession] Creating session with name: %s for player %d"), *SessionName, LocalPlayer->GetControllerId());
        SessionInterface->CreateSession(LocalPlayer->GetControllerId(), NAME_GameSession, *SessionSettings);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[NetworkSession] CreateSession failed: Cannot get LocalPlayer."));
    }
}

void UChessGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    UE_LOG(LogTemp, Log, TEXT("[NetworkSession] OnCreateSessionComplete called. SessionName: %s, Success: %d"), *SessionName.ToString(), bWasSuccessful);
        
    // Broadcast the result to any subscribed UI widgets
    OnSessionCreated.Broadcast(bWasSuccessful);

    if (bWasSuccessful)
    {
        FString TravelURL = LevelNameToHost.ToString();
        TravelURL += TEXT("?listen");
        
        // Добавляем параметр TimeControl в URL, используя сохраненное значение
        TravelURL += FString::Printf(TEXT("?TimeControl=%d"), static_cast<int32>(StagedTimeControl));
        
        // Добавляем флаг, что это лобби
        TravelURL += TEXT("?Lobby=1");

        UE_LOG(LogTemp, Log, TEXT("[NetworkSession] Session '%s' created successfully. Traveling to '%s'..."), *SessionName.ToString(), *TravelURL);

        // --- Steam Session Created ---
        SessionHostAddress = TEXT("Steam Session");
        if (GEngine)
        {
             GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Green, TEXT("Steam Lobby Created! Open Overlay (Shift+Tab) to invite friends."));
             UE_LOG(LogTemp, Log, TEXT("Steam Lobby Created. Ready for invites."));
        }

        // --- Open Invite UI (Steam/External) ---
        IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
        if (OnlineSub)
        {
            IOnlineExternalUIPtr ExternalUI = OnlineSub->GetExternalUIInterface();
            if (ExternalUI.IsValid())
            {
                ExternalUI->ShowInviteUI(0, SessionName);
            }
        }

        GetWorld()->ServerTravel(TravelURL);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[NetworkSession] Failed to create session."));
        bIsHost = false; // Reset host status on failure
    }

    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
    }
}

void UChessGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    UE_LOG(LogTemp, Log, TEXT("[NetworkSession] OnDestroySessionComplete called. SessionName: %s, Success: %d"), *SessionName.ToString(), bWasSuccessful);
        
    bIsHost = false; // We are no longer a host after destroying a session.

    if (bWasSuccessful)
    {
        CreateSession(SessionNameToCreate);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[NetworkSession] Failed to destroy session '%s'."), *SessionName.ToString());
    }

    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
    }
}

void UChessGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
    UE_LOG(LogTemp, Log, TEXT("[NetworkSession] OnFindSessionsComplete received. bWasSuccessful: %d"), bWasSuccessful);

    if (bWasSuccessful && SessionSearch.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("[NetworkSession] Search query completed. Found %d matching sessions."), SessionSearch->SearchResults.Num());
        
        bool bFoundMatch = false;
        if (SessionSearch->SearchResults.Num() > 0)
        {
            const FOnlineSessionSearchResult& SearchResult = SessionSearch->SearchResults[0];
            FString RoomName;
            SearchResult.Session.SessionSettings.Get(FName(TEXT("ROOM_NAME_KEY")), RoomName);
            
            UE_LOG(LogTemp, Log, TEXT("[NetworkSession] >>> Found a matching session via query! RoomName: '%s', Owner: '%s'. Joining..."), *RoomName, *SearchResult.Session.OwningUserName);
            
            bIsFindingSessions = false; // Search process is complete.
            GetWorld()->GetTimerManager().ClearTimer(FindSessionTimerHandle);
            JoinFoundSession(SearchResult);
            bFoundMatch = true;
        }
        
        if (!bFoundMatch)
        {
            UE_LOG(LogTemp, Warning, TEXT("[NetworkSession] No session found matching the name '%s'."), *SessionNameToFind);
            if (FindSessionRetryCount < MAX_FIND_SESSION_RETRIES)
            {
                UE_LOG(LogTemp, Log, TEXT("[NetworkSession] Will retry search in 2.0 seconds..."));
                // bIsFindingSessions remains true for the retry
                GetWorld()->GetTimerManager().SetTimer(FindSessionTimerHandle, this, &UChessGameInstance::FindSessions, 2.0f, false);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[NetworkSession] All %d search attempts failed. Could not find session '%s'. Giving up."), MAX_FIND_SESSION_RETRIES, *SessionNameToFind);
                bIsFindingSessions = false; // Search process is complete.
                GetWorld()->GetTimerManager().ClearTimer(FindSessionTimerHandle);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[NetworkSession] FindSessions RPC call failed. bWasSuccessful=%d, SessionSearch.IsValid()=%d"), bWasSuccessful, SessionSearch.IsValid());
        bIsFindingSessions = false; // Search process is complete.
        GetWorld()->GetTimerManager().ClearTimer(FindSessionTimerHandle);
    }
    
    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
        UE_LOG(LogTemp, Log, TEXT("[NetworkSession] OnFindSessionsComplete delegate handle cleared."));
    }
}

void UChessGameInstance::OnSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
    UE_LOG(LogTemp, Log, TEXT("[NetworkSession] OnSessionUserInviteAccepted called. Success: %d"), bWasSuccessful);

    if (bWasSuccessful)
    {
        UE_LOG(LogTemp, Log, TEXT("[NetworkSession] Invitation accepted! Joining invited session..."));
        
        // Ensure we are not the host of another session
        if (bIsHost)
        {
             UE_LOG(LogTemp, Warning, TEXT("[NetworkSession] We are currently hosting a session. Destroying it to join invite..."));
             // Ideally we should destroy the session first, but for now we proceed to join (JoinFoundSession might fail or we might need to chain logic).
             // To keep it simple and robust:
             if (SessionInterface.IsValid())
             {
                 SessionInterface->DestroySession(NAME_GameSession);
             }
             bIsHost = false;
        }

        JoinFoundSession(InviteResult);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[NetworkSession] Failed to accept invite."));
    }
}

void UChessGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    UE_LOG(LogTemp, Log, TEXT("[NetworkSession] OnJoinSessionComplete called. SessionName: %s, Result: %s"), *SessionName.ToString(), *GetJoinSessionResultString(Result));

    if (Result == EOnJoinSessionCompleteResult::Success && SessionInterface.IsValid())
    {
        FString ConnectString;
        if (SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
        {
            SessionHostAddress = ConnectString; // Store the address
            UE_LOG(LogTemp, Log, TEXT("[NetworkSession] Join successful. Resolved connect string: %s"), *ConnectString);
            UE_LOG(LogTemp, Log, TEXT("[NetworkSession] Traveling to host..."));
            APlayerController* PlayerController = GetFirstLocalPlayerController();
            if (PlayerController)
            {
            	PlayerController->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[NetworkSession] ClientTravel FAILED: Could not get PlayerController."));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[NetworkSession] Could not get connect string for session '%s'."), *SessionName.ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[NetworkSession] Failed to join session '%s'. Reason: %s"), *SessionName.ToString(), *GetJoinSessionResultString(Result));
    }
    
    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
    }
}

void UChessGameInstance::LoadPlayerProfile()
{
	CurrentSaveGame = Cast<UChessSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("ChessPlayerProfile"), 0));

	if (!CurrentSaveGame)
	{
		CurrentSaveGame = Cast<UChessSaveGame>(UGameplayStatics::CreateSaveGameObject(UChessSaveGame::StaticClass()));
		UE_LOG(LogTemp, Log, TEXT("No saved profile found. Created a new default profile."));
		SavePlayerProfile();
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Player profile loaded for '%s' (ELO: %d)."), *CurrentSaveGame->PlayerProfile.PlayerName, CurrentSaveGame->PlayerProfile.EloRating);
	}
}

void UChessGameInstance::SavePlayerProfile()
{
	if (CurrentSaveGame)
	{
		UGameplayStatics::SaveGameToSlot(CurrentSaveGame, CurrentSaveGame->SaveSlotName, CurrentSaveGame->UserIndex);
		UE_LOG(LogTemp, Log, TEXT("Player profile saved."));
	}
}

const FPlayerProfile& UChessGameInstance::GetPlayerProfile() const
{
	if (CurrentSaveGame)
	{
		return CurrentSaveGame->PlayerProfile;
	}
	
	static const FPlayerProfile DefaultProfile;
	UE_LOG(LogTemp, Warning, TEXT("UChessGameInstance::GetPlayerProfile returning default profile because CurrentSaveGame is null."));
	return DefaultProfile;
}

FString UChessGameInstance::GetSessionHostAddress() const
{
    return SessionHostAddress;
}

TSubclassOf<class UUserWidget> UChessGameInstance::GetGraphicsSettingsWidgetClass() const
{
	return GraphicsSettingsWidgetClass;
}

void UChessGameInstance::UpdatePlayerProfile(const FPlayerProfile& NewProfile)
{
	if (CurrentSaveGame)
	{
		CurrentSaveGame->PlayerProfile = NewProfile;
		SavePlayerProfile();
	}
}

FGraphicsSettingsData UChessGameInstance::GetGraphicsSettings() const
{
	if (CurrentSaveGame)
	{
		return CurrentSaveGame->GraphicsSettings;
	}
	
	// Возвращаем настройки по умолчанию, если файл сохранения еще не создан
	return FGraphicsSettingsData();
}

void UChessGameInstance::UpdateGraphicsSettings(const FGraphicsSettingsData& NewSettings)
{
	if (CurrentSaveGame)
	{
		CurrentSaveGame->GraphicsSettings = NewSettings;
		SavePlayerProfile();     // Сохраняем их в файл

		// Откладываем применение настроек, чтобы UI успел отреагировать (например, закрыть меню)
		// до начала потенциально долгой операции. Это предотвращает "зависание" интерфейса.
		UWorld* World = GetWorld();
		if (World)
		{
			// Используем небольшую задержку, чтобы гарантировать, что текущий кадр с UI-логикой завершится.
			FTimerHandle DummyHandle;
			World->GetTimerManager().SetTimer(DummyHandle, this, &UChessGameInstance::ApplyGraphicsSettings, 0.1f, false);
		}
		else
		{
			// Если мир недоступен, применяем немедленно как запасной вариант.
			ApplyGraphicsSettings();
		}
	}
}

void UChessGameInstance::ApplyGraphicsSettings()
{
	if (!GEngine || !CurrentSaveGame)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyGraphicsSettings: GEngine or CurrentSaveGame is null."));
		return;
	}

	UGameUserSettings* UserSettings = GEngine->GetGameUserSettings();
	if (!UserSettings)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyGraphicsSettings: UserSettings is null."));
		return;
	}

	const FGraphicsSettingsData& Settings = CurrentSaveGame->GraphicsSettings;
	
	UserSettings->SetAntiAliasingQuality(Settings.AntiAliasingQuality);
	UserSettings->SetPostProcessingQuality(Settings.PostProcessingQuality);
	UserSettings->SetShadowQuality(Settings.ShadowQuality);
	UserSettings->SetTextureQuality(Settings.TextureQuality);
	UserSettings->SetVisualEffectQuality(Settings.EffectsQuality);
	UserSettings->SetVSyncEnabled(Settings.bUseVSync);
	
	// Применяем настройки, но не сохраняем их в файл config/user.ini,
	// так как мы управляем сохранением через наш SaveGame.
	UserSettings->ApplySettings(false);

	UE_LOG(LogTemp, Log, TEXT("Graphics settings applied. VSync: %d, Quality: AA=%d, PP=%d, SH=%d, TX=%d, FX=%d"), 
		Settings.bUseVSync,
		Settings.AntiAliasingQuality,
		Settings.PostProcessingQuality,
		Settings.ShadowQuality,
		Settings.TextureQuality,
		Settings.EffectsQuality);
}

void UChessGameInstance::TryLoadSteamInfo()
{
	if (!CurrentSaveGame) return;

	FString SteamName = GetSteamPersonaName();
	if (!SteamName.IsEmpty() && SteamName != TEXT("Player") && SteamName != CurrentSaveGame->PlayerProfile.PlayerName)
	{
		UE_LOG(LogTemp, Log, TEXT("Updating profile name from Steam: %s"), *SteamName);
		CurrentSaveGame->PlayerProfile.PlayerName = SteamName;
		SavePlayerProfile();
	}
}

FString UChessGameInstance::GetSteamPersonaName() const
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem && Subsystem->GetIdentityInterface().IsValid())
	{
		// Получаем имя текущего локального пользователя (индекс 0)
		FString NickName = Subsystem->GetIdentityInterface()->GetPlayerNickname(0);
		if (!NickName.IsEmpty())
		{
			return NickName;
		}
	}
	return TEXT("");
}

UTexture2D* UChessGameInstance::GetSteamAvatar(APlayerState* PlayerState)
{
    if (!PlayerState) return nullptr;

    // Проверяем, запущен ли Steam вообще
    if (!SteamAPI_Init())
    {
        // Не спамим логами, если Steam выключен
        return nullptr;
    }

    const FUniqueNetIdRepl& UniqueNetIdRepl = PlayerState->GetUniqueId();
    if (!UniqueNetIdRepl.IsValid())
    {
        // Для ботов это нормально, для игроков - нет
        return nullptr;
    }

    // Получаем "сырой" ID
    uint64 ID = 0;
    
    // Пытаемся достать ID безопасно
    if (UniqueNetIdRepl.GetUniqueNetId().IsValid())
    {
        // ВАЖНО: В Unreal 5 формат хранения ID может отличаться, но для Steam обычно это uint64
        // Пробуем строковое преобразование для диагностики (ЗАКОММЕНТИРОВАНО ИЗ-ЗА ОШИБКИ ЛИНКОВКИ)
        // FString IdStr = UniqueNetIdRepl.ToString();
        // UE_LOG(LogTemp, Log, TEXT("[GetSteamAvatar] Player %s has ID String: %s"), *PlayerState->GetPlayerName(), *IdStr);
        
        // Хаки для получения uint64 из FUniqueNetId (зависит от версии UE и подсистемы)
        // Если это SteamNetId, то Bytes хранят uint64
        if (UniqueNetIdRepl.GetUniqueNetId()->GetSize() >= sizeof(uint64))
        {
            ID = *((uint64*)UniqueNetIdRepl.GetUniqueNetId()->GetBytes());
        }
    }

    CSteamID SteamID;
    
    // Логика определения: Это Я или Другой?
    if (ID > 0)
    {
        SteamID = CSteamID(ID);
    }
    else
    {
        // Если ID = 0 (например, локальный тест без сети), пробуем взять СВОЙ ID
        // Но только если это локальный игрок!
        // Проверка:
        bool bIsLocal = false;
        if (PlayerState->GetOwner() == GetFirstLocalPlayerController())
        {
            bIsLocal = true;
        }

        if (bIsLocal)
        {
             SteamID = SteamUser()->GetSteamID();
             // UE_LOG(LogTemp, Log, TEXT("[GetSteamAvatar] ID was 0, but verified as Local Player. Using SteamUser()->GetSteamID(): %llu"), SteamID.ConvertToUint64());
        }
        else
        {
             // Если это удаленный игрок и ID=0, мы ничего не можем сделать
             return nullptr;
        }
    }

    // Запрашиваем аватар
    int PictureHandle = SteamFriends()->GetMediumFriendAvatar(SteamID);

    if (PictureHandle == 0) 
    {
        // Это ОЧЕНЬ частая ситуация. Steam еще не скачал картинку.
        // Он вернет 0, а когда скачает - пришлет коллбек (мы его не слушаем, просто пробуем снова через 0.5с в виджете)
        // UE_LOG(LogTemp, Warning, TEXT("[GetSteamAvatar] Avatar for %llu not ready yet (Handle=0). Will retry."), SteamID.ConvertToUint64());
        return nullptr;
    }
    
    if (PictureHandle == -1)
    {
         UE_LOG(LogTemp, Error, TEXT("[GetSteamAvatar] Failed to get avatar handle for %llu"), SteamID.ConvertToUint64());
         return nullptr;
    }

    uint32 Width = 0;
    uint32 Height = 0;
    if (SteamUtils()->GetImageSize(PictureHandle, &Width, &Height))
    {
        if (Width > 0 && Height > 0)
        {
            uint8* AvatarRGBA = new uint8[Width * Height * 4];
            if (SteamUtils()->GetImageRGBA(PictureHandle, AvatarRGBA, Width * Height * 4))
            {
                UTexture2D* AvatarTexture = UTexture2D::CreateTransient(Width, Height, PF_R8G8B8A8);
                if (AvatarTexture)
                {
                    FTexture2DMipMap& Mip = AvatarTexture->GetPlatformData()->Mips[0];
                    void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
                    FMemory::Memcpy(Data, AvatarRGBA, Width * Height * 4);
                    Mip.BulkData.Unlock();
                    AvatarTexture->UpdateResource();
                    
                    UE_LOG(LogTemp, Log, TEXT("[GetSteamAvatar] SUCCESS! Avatar loaded for %s (%llu). Size: %dx%d"), *PlayerState->GetPlayerName(), SteamID.ConvertToUint64(), Width, Height);
                }
                delete[] AvatarRGBA;
                return AvatarTexture;
            }
            delete[] AvatarRGBA;
        }
    }

    return nullptr;
}
