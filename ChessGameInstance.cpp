#include "ChessGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

const int32 UChessGameInstance::MAX_FIND_SESSION_RETRIES;

UChessGameInstance::UChessGameInstance()
{
	FindSessionRetryCount = 0;
}

void UChessGameInstance::Init()
{
	Super::Init();
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &UChessGameInstance::OnCreateSessionComplete);
			OnDestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UChessGameInstance::OnDestroySessionComplete);
			OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UChessGameInstance::OnFindSessionsComplete);
			OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UChessGameInstance::OnJoinSessionComplete);
		}
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
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[HostSession] FindAndJoinSession ABORTED: SessionInterface is not valid."));
        return;
    }
    if (SessionName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HostSession] FindAndJoinSession called with empty SessionName."));
        return;
    }

    SessionNameToFind = SessionName;
    FindSessionRetryCount = 0;
    UE_LOG(LogTemp, Log, TEXT("[HostSession] FindAndJoinSession called. Will search for '%s'. Resetting retry count."), *SessionNameToFind);

    GetWorld()->GetTimerManager().ClearTimer(FindSessionTimerHandle);
    
    UE_LOG(LogTemp, Log, TEXT("[HostSession] Waiting 1.5s before starting first session search..."));
    GetWorld()->GetTimerManager().SetTimer(FindSessionTimerHandle, this, &UChessGameInstance::FindSessions, 1.5f, false);
}

void UChessGameInstance::FindSessions()
{
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[HostSession] FindSessions failed: SessionInterface is not valid."));
        return;
    }
    
    FindSessionRetryCount++;
    UE_LOG(LogTemp, Log, TEXT("[HostSession] --- Starting session search attempt %d of %d... ---"), FindSessionRetryCount, MAX_FIND_SESSION_RETRIES);

    SessionSearch = MakeShareable(new FOnlineSessionSearch());
    SessionSearch->bIsLanQuery = true;
    SessionSearch->MaxSearchResults = 20;

    UE_LOG(LogTemp, Log, TEXT("[HostSession] SessionSearch object created. IsLANQuery=%d. No query filters."), SessionSearch->bIsLanQuery);

    OnFindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);

    // GameInstance does not have a local player directly. We get it from the world.
    ULocalPlayer* LocalPlayer = GetFirstGamePlayer();
    if (LocalPlayer)
    {
	    UE_LOG(LogTemp, Log, TEXT("[HostSession] Finding all LAN sessions for player %d..."), LocalPlayer->GetControllerId());
	    SessionInterface->FindSessions(LocalPlayer->GetControllerId(), SessionSearch.ToSharedRef());
    }
    else
    {
	    UE_LOG(LogTemp, Error, TEXT("[HostSession] FindSessions failed: Cannot get LocalPlayer."));
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
    UE_LOG(LogTemp, Log, TEXT("[HostSession] OnFindSessionsComplete called. Success: %d"), bWasSuccessful);

    if (bWasSuccessful && SessionSearch.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("[HostSession] Found %d raw search results."), SessionSearch->SearchResults.Num());
        
        bool bFoundMatch = false;
        if (SessionSearch->SearchResults.Num() > 0)
        {
            for (const FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
            {
                FString RoomName;
                if (SearchResult.Session.SessionSettings.Get(FName(TEXT("ROOM_NAME_KEY")), RoomName))
                {
                    UE_LOG(LogTemp, Log, TEXT("[HostSession] Checking session with RoomName: '%s' against desired '%s'"), *RoomName, *SessionNameToFind);
                    if (RoomName == SessionNameToFind)
                    {
                        UE_LOG(LogTemp, Log, TEXT("[HostSession] !!! Found matching session: '%s'. Joining..."), *RoomName);
                        GetWorld()->GetTimerManager().ClearTimer(FindSessionTimerHandle);
                        JoinSession(SearchResult);
                        bFoundMatch = true;
                        break;
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("[HostSession] Found a session without ROOM_NAME_KEY. Owner: %s"), *SearchResult.GetSessionIdStr());
                }
            }
        }
        
        if (!bFoundMatch)
        {
            if (FindSessionRetryCount < MAX_FIND_SESSION_RETRIES)
            {
                UE_LOG(LogTemp, Warning, TEXT("[HostSession] No matching session found. Retrying in 2.0s..."));
                GetWorld()->GetTimerManager().SetTimer(FindSessionTimerHandle, this, &UChessGameInstance::FindSessions, 2.0f, false);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[HostSession] All %d search attempts failed. Could not find a session named '%s'. Giving up."), MAX_FIND_SESSION_RETRIES, *SessionNameToFind);
                GetWorld()->GetTimerManager().ClearTimer(FindSessionTimerHandle);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[HostSession] Session search failed on attempt %d. bWasSuccessful=%d, SessionSearch.IsValid()=%d"), FindSessionRetryCount, bWasSuccessful, SessionSearch.IsValid());
        GetWorld()->GetTimerManager().ClearTimer(FindSessionTimerHandle);
    }
    
    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
    }
}

void UChessGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    UE_LOG(LogTemp, Log, TEXT("[HostSession] OnJoinSessionComplete called. SessionName: %s, Result: %d"), *SessionName.ToString(), static_cast<int32>(Result));

    if (Result == EOnJoinSessionCompleteResult::Success && SessionInterface.IsValid())
    {
        FString ConnectString;
        if (SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
        {
            UE_LOG(LogTemp, Log, TEXT("[HostSession] Successfully joined session '%s'. Traveling to: %s"), *SessionName.ToString(), *ConnectString);
            APlayerController* PlayerController = GetFirstLocalPlayerController();
            if (PlayerController)
            {
            	PlayerController->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[HostSession] Could not get connect string for session '%s'."), *SessionName.ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[HostSession] Failed to join session '%s'. Error code: %d"), *SessionName.ToString(), static_cast<int32>(Result));
    }
    
    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
    }
}
