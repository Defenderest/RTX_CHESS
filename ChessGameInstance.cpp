#include "ChessGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

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
}

void UChessGameInstance::Init()
{
	Super::Init();
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
}

void UChessGameInstance::HostSession(const FString& SessionName, FName LevelName)
{
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[HostSession] HostSession ABORTED: SessionInterface is not valid."));
        return;
    }
    if (SessionName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[HostSession] HostSession ABORTED: SessionName is empty."));
        return;
    }

    LevelNameToHost = LevelName;
    SessionNameToCreate = SessionName;
    UE_LOG(LogTemp, Log, TEXT("[HostSession] --- Starting Host Process ---"));
    UE_LOG(LogTemp, Log, TEXT("[HostSession] Caching LevelNameToHost: %s and SessionNameToCreate: %s"), *LevelNameToHost.ToString(), *SessionNameToCreate);

    auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
    if (ExistingSession != nullptr)
    {
        UE_LOG(LogTemp, Log, TEXT("[HostSession] Found an existing session named '%s'. Destroying it before creating a new one..."), *ExistingSession->SessionName.ToString());
        OnDestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);
        SessionInterface->DestroySession(NAME_GameSession);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[HostSession] No existing session found. Proceeding to create a new one."));
        CreateSession(SessionNameToCreate);
    }
}

void UChessGameInstance::FindAndJoinSession(const FString& SessionName)
{
    UE_LOG(LogTemp, Log, TEXT("[HostSession] FindAndJoinSession triggered for session: '%s'"), *SessionName);

    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[HostSession] FindAndJoinSession ABORTED: SessionInterface is not valid."));
        return;
    }
    if (SessionName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HostSession] FindAndJoinSession ABORTED: SessionName is empty."));
        return;
    }

    if (bIsFindingSessions)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HostSession] FindAndJoinSession IGNORED: A session search is already in progress."));
        return;
    }

    auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
    if (ExistingSession != nullptr && ExistingSession->SessionState == EOnlineSessionState::InProgress)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HostSession] FindAndJoinSession ABORTED: Already in an active session ('%s')."), *ExistingSession->SessionName.ToString());
        return;
    }

    SessionNameToFind = SessionName;
    FindSessionRetryCount = 0;
    bIsFindingSessions = true;
    UE_LOG(LogTemp, Log, TEXT("[HostSession] Starting search for session '%s'. Retry count reset."), *SessionNameToFind);

    GetWorld()->GetTimerManager().ClearTimer(FindSessionTimerHandle);
    
    UE_LOG(LogTemp, Log, TEXT("[HostSession] Waiting 3.0s before starting first session search..."));
    GetWorld()->GetTimerManager().SetTimer(FindSessionTimerHandle, this, &UChessGameInstance::FindSessions, 3.0f, false);
}

void UChessGameInstance::FindSessions()
{
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[HostSession] FindSessions ABORTED: SessionInterface is not valid."));
        bIsFindingSessions = false; // Reset state since we can't proceed
        return;
    }
    
    FindSessionRetryCount++;
    UE_LOG(LogTemp, Log, TEXT("[HostSession] --- Starting session search (Attempt %d/%d) ---"), FindSessionRetryCount, MAX_FIND_SESSION_RETRIES);

    SessionSearch = MakeShareable(new FOnlineSessionSearch());
    SessionSearch->bIsLanQuery = true;
    SessionSearch->MaxSearchResults = 20;
    SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, false, EOnlineComparisonOp::Equals);

    UE_LOG(LogTemp, Log, TEXT("[HostSession] SessionSearch object created. IsLANQuery=%d. MaxResults=%d. SEARCH_PRESENCE=false."), SessionSearch->bIsLanQuery, SessionSearch->MaxSearchResults);

    OnFindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);
	UE_LOG(LogTemp, Log, TEXT("[HostSession] OnFindSessionsComplete delegate handle bound."));

    // GameInstance does not have a local player directly. We get it from the world.
    ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
    if (LocalPlayer)
    {
	    UE_LOG(LogTemp, Log, TEXT("[HostSession] Issuing FindSessions call for player %d..."), LocalPlayer->GetControllerId());
	    SessionInterface->FindSessions(LocalPlayer->GetControllerId(), SessionSearch.ToSharedRef());
    }
    else
    {
	    UE_LOG(LogTemp, Error, TEXT("[HostSession] FindSessions FAILED: Cannot get LocalPlayer."));
        bIsFindingSessions = false; // Reset state
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle); // Clean up delegate
    }
}

void UChessGameInstance::JoinSession(const FOnlineSessionSearchResult& SearchResult)
{
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[HostSession] JoinSession failed: SessionInterface is not valid."));
        return;
    }
    OnJoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
    
    ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
    if (LocalPlayer)
    {
	    UE_LOG(LogTemp, Log, TEXT("[HostSession] Player %d joining session..."), LocalPlayer->GetControllerId());
	    SessionInterface->JoinSession(LocalPlayer->GetControllerId(), NAME_GameSession, SearchResult);
    }
     else
    {
	    UE_LOG(LogTemp, Error, TEXT("[HostSession] JoinSession failed: Cannot get LocalPlayer."));
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
    UE_LOG(LogTemp, Log, TEXT("[HostSession] SessionSettings configured. ROOM_NAME_KEY = %s"), *SessionName);
	
    ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
    if(LocalPlayer)
    {
	    UE_LOG(LogTemp, Log, TEXT("[HostSession] Creating LAN session with name: %s for player %d"), *SessionName, LocalPlayer->GetControllerId());
	    SessionInterface->CreateSession(LocalPlayer->GetControllerId(), NAME_GameSession, *SessionSettings);
    }
    else
    {
	    UE_LOG(LogTemp, Error, TEXT("[HostSession] CreateSession failed: Cannot get LocalPlayer."));
    }
}

void UChessGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    UE_LOG(LogTemp, Log, TEXT("[HostSession] OnCreateSessionComplete called. SessionName: %s, Success: %d"), *SessionName.ToString(), bWasSuccessful);
    if (bWasSuccessful)
    {
        UE_LOG(LogTemp, Log, TEXT("[HostSession] Session '%s' created successfully. Traveling to map '%s' as listen server..."), *SessionName.ToString(), *LevelNameToHost.ToString());
        GetWorld()->ServerTravel(LevelNameToHost.ToString() + TEXT("?listen"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[HostSession] Failed to create session."));
    }

    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
    }
}

void UChessGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    UE_LOG(LogTemp, Log, TEXT("[HostSession] OnDestroySessionComplete called. SessionName: %s, Success: %d"), *SessionName.ToString(), bWasSuccessful);
    if (bWasSuccessful)
    {
        CreateSession(SessionNameToCreate);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[HostSession] Failed to destroy session '%s'."), *SessionName.ToString());
    }

    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
    }
}

void UChessGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
    UE_LOG(LogTemp, Log, TEXT("[HostSession] OnFindSessionsComplete received. bWasSuccessful: %d"), bWasSuccessful);

    if (bWasSuccessful && SessionSearch.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("[HostSession] Search was successful. Found %d total sessions."), SessionSearch->SearchResults.Num());
        
        bool bFoundMatch = false;
        if (SessionSearch->SearchResults.Num() > 0)
        {
            for (const FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
            {
                FString RoomName;
                if (SearchResult.Session.SessionSettings.Get(FName(TEXT("ROOM_NAME_KEY")), RoomName))
                {
                    UE_LOG(LogTemp, Log, TEXT("[HostSession]   - Checking session: RoomName='%s' | Desired: '%s' | Owner: '%s' | Ping: %dms"), *RoomName, *SessionNameToFind, *SearchResult.Session.OwningUserName, SearchResult.PingInMs);
                    if (RoomName == SessionNameToFind)
                    {
                        UE_LOG(LogTemp, Log, TEXT("[HostSession] >>> Found a matching session! RoomName: '%s'."), *RoomName);
                        bIsFindingSessions = false; // Search process is complete.
                        GetWorld()->GetTimerManager().ClearTimer(FindSessionTimerHandle);
                        JoinSession(SearchResult);
                        bFoundMatch = true;
                        break;
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[HostSession]   - Found a session without ROOM_NAME_KEY. SessionId: %s"), *SearchResult.GetSessionIdStr());
                }
            }
        }
        
        if (!bFoundMatch)
        {
            UE_LOG(LogTemp, Warning, TEXT("[HostSession] No matching session found in the results."));
            if (FindSessionRetryCount < MAX_FIND_SESSION_RETRIES)
            {
                UE_LOG(LogTemp, Log, TEXT("[HostSession] Will retry search in 2.0 seconds..."));
                // bIsFindingSessions remains true for the retry
                GetWorld()->GetTimerManager().SetTimer(FindSessionTimerHandle, this, &UChessGameInstance::FindSessions, 2.0f, false);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[HostSession] All %d search attempts failed. Could not find session '%s'. Giving up."), MAX_FIND_SESSION_RETRIES, *SessionNameToFind);
                bIsFindingSessions = false; // Search process is complete.
                GetWorld()->GetTimerManager().ClearTimer(FindSessionTimerHandle);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[HostSession] FindSessions RPC call failed. bWasSuccessful=%d, SessionSearch.IsValid()=%d"), bWasSuccessful, SessionSearch.IsValid());
        bIsFindingSessions = false; // Search process is complete.
        GetWorld()->GetTimerManager().ClearTimer(FindSessionTimerHandle);
    }
    
    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
        UE_LOG(LogTemp, Log, TEXT("[HostSession] OnFindSessionsComplete delegate handle cleared."));
    }
}

void UChessGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    UE_LOG(LogTemp, Log, TEXT("[HostSession] OnJoinSessionComplete called. SessionName: %s, Result: %s"), *SessionName.ToString(), *GetJoinSessionResultString(Result));

    if (Result == EOnJoinSessionCompleteResult::Success && SessionInterface.IsValid())
    {
        FString ConnectString;
        if (SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
        {
            UE_LOG(LogTemp, Log, TEXT("[HostSession] Join successful. Resolved connect string: %s"), *ConnectString);
            UE_LOG(LogTemp, Log, TEXT("[HostSession] Traveling to host..."));
            APlayerController* PlayerController = GetFirstLocalPlayerController();
            if (PlayerController)
            {
            	PlayerController->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[HostSession] ClientTravel FAILED: Could not get PlayerController."));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[HostSession] Could not get connect string for session '%s'."), *SessionName.ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[HostSession] Failed to join session '%s'. Reason: %s"), *SessionName.ToString(), *GetJoinSessionResultString(Result));
    }
    
    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
    }
}
