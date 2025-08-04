#include "ChessGameInstance.h"
#include "ChessGameMode.h" // For ETimeControlType
#include "ChessSaveGame.h"
#include "GameFramework/GameUserSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "MoviePlayer.h"
#include "Blueprint/UserWidget.h"

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

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UChessGameInstance::BeginLoadingScreen);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UChessGameInstance::EndLoadingScreen);
}

void UChessGameInstance::Shutdown()
{
	Super::Shutdown();
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
}

void UChessGameInstance::BeginLoadingScreen(const FString& InMapName)
{
	if (IsRunningDedicatedServer())
	{
		return;
	}

	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
	
	if (LoadingScreenWidgetClass)
	{
		LoadingScreen.WidgetLoadingScreen = CreateWidget<UUserWidget>(this, LoadingScreenWidgetClass)->TakeWidget();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadingScreenWidgetClass is not set in ChessGameInstance Blueprint. Falling back to default loading screen."));
		LoadingScreen.WidgetLoadingScreen = FLoadingScreenAttributes::NewTestLoadingScreenWidget();
	}

	GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
}

void UChessGameInstance::EndLoadingScreen(UWorld* InLoadedWorld)
{
	// Эта функция вызывается после загрузки карты.
	// Экран загрузки скроется автоматически благодаря bAutoCompleteWhenLoadingCompletes = true.
}

void UChessGameInstance::ShowGraphicsSettingsMenu()
{
	if (MainGraphicsSettingsWidgetClass)
	{
		UUserWidget* SettingsWidget = CreateWidget<UUserWidget>(this, MainGraphicsSettingsWidgetClass);
		if (SettingsWidget)
		{
			SettingsWidget->AddToViewport();
			// Как правило, сам виджет должен обрабатывать переключение режима ввода.
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MainGraphicsSettingsWidgetClass is not set in ChessGameInstance Blueprint. Cannot show settings menu."));
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
    SessionSearch->bIsLanQuery = true;
    SessionSearch->MaxSearchResults = 1; // We are looking for a specific session
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

void UChessGameInstance::JoinSession(const FOnlineSessionSearchResult& SearchResult)
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
    SessionSettings->bIsLANMatch = true;
    SessionSettings->NumPublicConnections = 2;
    SessionSettings->bShouldAdvertise = true;
    SessionSettings->bUsesPresence = false;
    SessionSettings->bUseLobbiesIfAvailable = false;
    SessionSettings->bAllowJoinInProgress = true;
    SessionSettings->Set(FName(TEXT("ROOM_NAME_KEY")), SessionName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    UE_LOG(LogTemp, Log, TEXT("[NetworkSession] SessionSettings configured. ROOM_NAME_KEY = %s"), *SessionName);
	
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

        UE_LOG(LogTemp, Log, TEXT("[NetworkSession] Session '%s' created successfully. Traveling to '%s'..."), *SessionName.ToString(), *TravelURL);

        // --- Get and display local IP address, prioritizing VPN addresses ---
        FString DisplayIP = TEXT("Not Found");
        TArray<TSharedPtr<FInternetAddr>> Addresses;
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalAdapterAddresses(Addresses);

        // First, look for a Radmin VPN IP (typically in 26.x.x.x range)
        for (const auto& Addr : Addresses)
        {
            if (Addr.IsValid())
            {
                const FString CurrentIP = Addr->ToString(false);
                if (!CurrentIP.StartsWith(TEXT("127.")) && CurrentIP.StartsWith(TEXT("26.")))
                {
                    DisplayIP = CurrentIP;
                    break;
                }
            }
        }

        // If no Radmin IP was found, find the first valid non-loopback IPv4
        if (DisplayIP == TEXT("Not Found"))
        {
            for (const auto& Addr : Addresses)
            {
                if (Addr.IsValid() && Addr->GetProtocolType() == FNetworkProtocolTypes::IPv4)
                {
                    const FString CurrentIP = Addr->ToString(false);
                    if (!CurrentIP.StartsWith(TEXT("127.")))
                    {
                        DisplayIP = CurrentIP;
                        break;
                    }
                }
            }
        }
        
        if (DisplayIP != TEXT("Not Found"))
        {
            if (GEngine)
            {
                // The default Unreal port is 7777.
                GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Green, FString::Printf(TEXT("Server started. Share this IP with LAN/VPN players: %s:7777"), *DisplayIP));
                UE_LOG(LogTemp, Log, TEXT("Displaying Server IP for LAN/VPN: %s:7777"), *DisplayIP);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Could not determine a suitable local IP address to display."));
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
            JoinSession(SearchResult);
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

void UChessGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    UE_LOG(LogTemp, Log, TEXT("[NetworkSession] OnJoinSessionComplete called. SessionName: %s, Result: %s"), *SessionName.ToString(), *GetJoinSessionResultString(Result));

    if (Result == EOnJoinSessionCompleteResult::Success && SessionInterface.IsValid())
    {
        FString ConnectString;
        if (SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
        {
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

void UChessGameInstance::UpdatePlayerProfile(const FPlayerProfile& NewProfile)
{
	if (CurrentSaveGame)
	{
		CurrentSaveGame->PlayerProfile = NewProfile;
		SavePlayerProfile();
	}
}
