#include "Controllers/ChessPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Core/ChessGameInstance.h"
#include "Core/ChessPlayerState.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Pieces/ChessPiece.h"
#include "Board/ChessBoard.h"
#include "Core/ChessGameMode.h"
#include "Managers/StockfishManager.h"
#include "Actors/GameCameraActor.h"
#include "Actors/MenuCameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Widgets/StartMenuWidget.h"
#include "UI/Widgets/PromotionMenuWidget.h"
#include "Pieces/PawnPiece.h"
#include "Blueprint/UserWidget.h"
#include "UI/Widgets/PauseMenuWidget.h"
#include "UI/Widgets/PlayerInfoWidget.h"
#include "UI/Widgets/GameOverWidget.h"
#include "Components/AudioComponent.h"
#include "Core/ChessGameState.h"
#include "Engine/Engine.h"
#include "Controllers/ChessPlayerCameraManager.h"
#include "TimerManager.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "GameFramework/PlayerState.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UI/Widgets/LobbyWidget.h"

DEFINE_LOG_CATEGORY(LogCameraManagement);

AChessPlayerController::AChessPlayerController()
{
    PlayerCameraManagerClass = AChessPlayerCameraManager::StaticClass();
    bAutoManageActiveCameraTarget = false;
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    PlayerColor = EPieceColor::White;
    SelectedPiece = nullptr;
    ChessBoard = nullptr;
    bIsInputModeSetForGame = false;
    bHasGameStarted_Client = false;
    bShowDebugInfo = false;
    MenuMusicComponent = nullptr;
    CaptureEffect = nullptr;
    PauseMenuWidgetInstance = nullptr;
    GraphicsSettingsWidgetInstance = nullptr;
    PlayerProfileWidgetInstance = nullptr;
    GameOverWidgetInstance = nullptr;
    PlayerInfoWidgetInstance = nullptr;

    // Mobile touch initialization
    bIsTouchDragging = false;
    PreviousTouchLocation = FVector2D::ZeroVector;
    LastTapTime = 0.0;
    LastTapLocation = FVector2D::ZeroVector;

    // Set default highlight colors
    ValidMoveHighlightColor = FLinearColor(0.1f, 0.5f, 0.1f, 1.0f); // Dark Green
    SelectedPieceHighlightColor = FLinearColor(0.2f, 0.2f, 0.8f, 1.0f); // Blue
}

void AChessPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (ChessMappingContext)
        {
            Subsystem->AddMappingContext(ChessMappingContext, 0);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessPlayerController::BeginPlay: ChessMappingContext not assigned! Please assign it in the Player Controller Blueprint."));
        }
    }

    ChessBoard = Cast<AChessBoard>(UGameplayStatics::GetActorOfClass(GetWorld(), AChessBoard::StaticClass()));
    if (!ChessBoard)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessPlayerController::BeginPlay: ChessBoard actor not found!"));
    }

    // Defer the decision of what to show until the GameState is definitely available.
    // This helps avoid race conditions where the Player Controller's BeginPlay runs
    // before the GameState is fully initialized or replicated.
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AChessPlayerController::DetermineInitialUI, 0.1f, false);
    
    // If we are a client, send our profile to the server
    if (IsLocalController() && GetNetMode() == NM_Client)
    {
        if (UChessGameInstance* GameInstance = GetGameInstance<UChessGameInstance>())
        {
            Server_SetPlayerProfile(GameInstance->GetPlayerProfile());
        }
    }
    
    // If we are the host, check if we need to create a lobby.
    if (HasAuthority())
    {
        if (const AGameModeBase* GM = GetWorld()->GetAuthGameMode())
        {
            const FString Options = GM->OptionsString;
            const bool bIsLobby = UGameplayStatics::ParseOption(Options, TEXT("Lobby")).Contains(TEXT("1"));
            if (bIsLobby)
            {
                if (AChessGameState* GS = GetWorld()->GetGameState<AChessGameState>())
                {
                    const FString TimeControlOption = UGameplayStatics::ParseOption(Options, TEXT("TimeControl"));
                    const ETimeControlType TCType = static_cast<ETimeControlType>(FCString::Atoi(*TimeControlOption));
                    
                    GS->SetIsInLobby(true);
                    GS->SetLobbyTimeControl(TCType);
                }
            }
        }
    }
}

void AChessPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Bind legacy touch events for mobile support (in case Enhanced Input mappings are missing for touch)
    InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AChessPlayerController::OnTouchStarted);
    InputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AChessPlayerController::OnTouchMoved);
    InputComponent->BindTouch(EInputEvent::IE_Released, this, &AChessPlayerController::OnTouchEnded);

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
    {
        if (ClickAction)
        {
            EnhancedInput->BindAction(ClickAction, ETriggerEvent::Started, this, &AChessPlayerController::OnClickStarted);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessPlayerController::SetupInputComponent: ClickAction not assigned! Please assign it in the Player Controller Blueprint."));
        }

        if (LookAction)
        {
            EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AChessPlayerController::HandleLook);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessPlayerController::SetupInputComponent: LookAction not assigned! Please assign it in the Player Controller Blueprint."));
        }

        if (MoveCameraAction)
        {
            EnhancedInput->BindAction(MoveCameraAction, ETriggerEvent::Triggered, this, &AChessPlayerController::HandleCameraMove);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessPlayerController::SetupInputComponent: MoveCameraAction not assigned! Please assign it in the Player Controller Blueprint."));
        }

        if (PauseAction)
        {
            EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &AChessPlayerController::TogglePauseMenu);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessPlayerController::SetupInputComponent: PauseAction not assigned! Please assign it in the Player Controller Blueprint."));
        }

        if (PlayerInfoAction)
        {
            EnhancedInput->BindAction(PlayerInfoAction, ETriggerEvent::Started, this, &AChessPlayerController::TogglePlayerInfoWidget);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessPlayerController::SetupInputComponent: PlayerInfoAction not assigned! Please assign it in the Player Controller Blueprint."));
        }

        if (ToggleDebugAction)
        {
            EnhancedInput->BindAction(ToggleDebugAction, ETriggerEvent::Started, this, &AChessPlayerController::ToggleDebugInfo);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessPlayerController::SetupInputComponent: ToggleDebugAction not assigned! Please assign it in the Player Controller Blueprint."));
        }
    }
}

void AChessPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Synchronize ControlRotation Yaw with the actual camera rotation.
    // Pitch is handled separately to avoid looking at the ceiling at start.
    if (bIsInputModeSetForGame)
    {
        // OPTIMIZATION: This code block should only run when the player is actively rotating the camera (Right Mouse Button held).
        // This prevents unnecessary calculations every frame when the camera is static.
        bool bIsRotateDown = IsInputKeyDown(EKeys::RightMouseButton);
#if PLATFORM_ANDROID || PLATFORM_IOS
        bIsRotateDown = bIsRotateDown || IsInputKeyDown(EKeys::LeftMouseButton);
#endif

        if (PlayerCameraManager && bIsRotateDown)
        {
            FRotator CurrentControlRotation = GetControlRotation();

            // CORRECT METHOD FOR YAW SYNCHRONIZATION:
            // 1. Get the vector the camera is looking at.
            const FVector CameraForward = PlayerCameraManager->GetCameraRotation().Vector();
            // 2. Convert this vector to rotation. This method correctly calculates Yaw even if Pitch is present.
            const FRotator CameraDirectionAsRotator = CameraForward.Rotation();
            // 3. Synchronize head Yaw with camera Yaw.
            CurrentControlRotation.Yaw = CameraDirectionAsRotator.Yaw;

            // Limit vertical rotation (Pitch) for realistic head movement.
            // -45 degrees down and +30 degrees up is a good range for a seated person.
            CurrentControlRotation.Pitch = FMath::Clamp(CurrentControlRotation.Pitch, -45.0f, 30.0f);

            // Explicitly zero out Roll to prevent head tilting/twisting.
            CurrentControlRotation.Roll = 0.0f;
            
            SetControlRotation(CurrentControlRotation);
        }
    }

    // --- On-Screen Debug Information ---
    if (bShowDebugInfo && GEngine)
    {
        // OPTIMIZATION: Update debug info less frequently than every frame,
        // as this is a resource-intensive operation.
        static float DebugInfoTimer = 0.0f;
        DebugInfoTimer += DeltaTime;

        if (DebugInfoTimer > 0.1f) // Update ~10 times per second
        {
            DebugInfoTimer = 0.0f;
            
            AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
            if (GameState)
            {
                FString GamePhaseStr = UEnum::GetValueAsString(GameState->GetGamePhase());
                FString CurrentTurnStr = (GameState->GetCurrentTurnColor() == EPieceColor::White) ? TEXT("White") : TEXT("Black");
                
                GEngine->AddOnScreenDebugMessage(0, 0.f, FColor::Yellow, FString::Printf(TEXT("Game Phase: %s"), *GamePhaseStr));
                GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Yellow, FString::Printf(TEXT("Current Turn: %s"), *CurrentTurnStr));

                // Display Time
                const int32 WhiteTimeInt = FMath::CeilToInt(GameState->WhiteTimeSeconds);
                const FString WhiteTimeStr = (WhiteTimeInt < 0) ? TEXT("Unlimited") : FString::Printf(TEXT("%02d:%02d"), WhiteTimeInt / 60, WhiteTimeInt % 60);
                GEngine->AddOnScreenDebugMessage(5, 0.f, FColor::White, FString::Printf(TEXT("White Time: %s"), *WhiteTimeStr));

                const int32 BlackTimeInt = FMath::CeilToInt(GameState->BlackTimeSeconds);
                const FString BlackTimeStr = (BlackTimeInt < 0) ? TEXT("Unlimited") : FString::Printf(TEXT("%02d:%02d"), BlackTimeInt / 60, BlackTimeInt % 60);
                GEngine->AddOnScreenDebugMessage(6, 0.f, FColor::Black, FString::Printf(TEXT("Black Time: %s"), *BlackTimeStr));

                // Display Profiles
                const FString WhiteProfileStr = FString::Printf(TEXT("White: %s (%d) [%s]"), *GameState->WhitePlayerProfile.PlayerName, GameState->WhitePlayerProfile.EloRating, *GameState->WhitePlayerProfile.Country);
                GEngine->AddOnScreenDebugMessage(7, 0.f, FColor::White, WhiteProfileStr);

                const FString BlackProfileStr = FString::Printf(TEXT("Black: %s (%d) [%s]"), *GameState->BlackPlayerProfile.PlayerName, GameState->BlackPlayerProfile.EloRating, *GameState->BlackPlayerProfile.Country);
                GEngine->AddOnScreenDebugMessage(8, 0.f, FColor::Black, BlackProfileStr);
            }
            else
            {
                GEngine->AddOnScreenDebugMessage(0, 0.f, FColor::Red, TEXT("Game State is NULL"));
            }

            FString MyColorStr = (PlayerColor == EPieceColor::White) ? TEXT("White") : TEXT("Black");
            GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Cyan, FString::Printf(TEXT("My Player Color: %s"), *MyColorStr));

            FString SelectedPieceStr = SelectedPiece ? GetNameSafe(SelectedPiece) : TEXT("None");
            GEngine->AddOnScreenDebugMessage(3, 0.f, FColor::Green, FString::Printf(TEXT("Selected Piece: %s"), *SelectedPieceStr));

            // Display FPS
            const float FPS = 1.0f / DeltaTime;
            GEngine->AddOnScreenDebugMessage(9, 0.f, FColor::Green, FString::Printf(TEXT("FPS: %.1f"), FPS));

            // --- Stockfish Debug Info ---
            AChessGameMode* GameMode = GetChessGameMode();
            if (GameMode && GameMode->GetCurrentGameModeType() == EGameModeType::PlayerVsBot)
            {
                UStockfishManager* SFManager = GameMode->GetStockfishManager();
                if (SFManager)
                {
                    // NOTE: The new async StockfishManager doesn't expose internal state for debug logs.
                    // We can only confirm that the manager object exists.
                    GEngine->AddOnScreenDebugMessage(4, 0.f, FColor::Orange, TEXT("Stockfish Manager: Present"));
                }
                else
                {
                    GEngine->AddOnScreenDebugMessage(4, 0.f, FColor::Red, TEXT("Stockfish Manager is NULL"));
                }
            }
            else if (GameMode)
            {
                 GEngine->AddOnScreenDebugMessage(4, 0.f, FColor::White, TEXT("Game Mode: Player vs Player"));
            }
        }
    }
    // --- End Debug Info ---
}

void AChessPlayerController::TogglePauseMenu()
{
    // Do not allow opening the pause menu if we are not in game or awaiting pawn promotion choice.
    AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
    if (!GameState) return;

    const EGamePhase CurrentPhase = GameState->GetGamePhase();
    if (CurrentPhase == EGamePhase::WaitingToStart || CurrentPhase == EGamePhase::AwaitingPromotion)
    {
        // Do not open the pause menu in these states. Others can be added, e.g., GameOver, if needed.
        return;
    }

    // If menu is already open, close it
    if (PauseMenuWidgetInstance && PauseMenuWidgetInstance->IsInViewport())
    {
        PauseMenuWidgetInstance->RemoveFromParent();
    }
    else // Otherwise, open it
    {
        if (PauseMenuWidgetClass)
        {
            if (!PauseMenuWidgetInstance)
            {
                PauseMenuWidgetInstance = CreateWidget<UPauseMenuWidget>(this, PauseMenuWidgetClass);
            }
            
            if (PauseMenuWidgetInstance)
            {
                PauseMenuWidgetInstance->AddToViewport(10); // High Z-order to be on top of everything
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AChessPlayerController: PauseMenuWidgetClass not assigned in Blueprint!"));
        }
    }
    UpdateInputMode();
}

void AChessPlayerController::ToggleGraphicsSettingsMenu()
{
    // If menu is already open, close it
    if (GraphicsSettingsWidgetInstance && GraphicsSettingsWidgetInstance->IsInViewport())
    {
        GraphicsSettingsWidgetInstance->RemoveFromParent();
    }
    else // Otherwise, open it
    {
        if (GraphicsSettingsWidgetClass)
        {
            if (!GraphicsSettingsWidgetInstance)
            {
                GraphicsSettingsWidgetInstance = CreateWidget<UUserWidget>(this, GraphicsSettingsWidgetClass);
            }
            
            if (GraphicsSettingsWidgetInstance)
            {
                GraphicsSettingsWidgetInstance->AddToViewport(10); // High Z-order to be on top of everything
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AChessPlayerController: GraphicsSettingsWidgetClass not assigned in Blueprint!"));
        }
    }
    UpdateInputMode();
}

void AChessPlayerController::TogglePlayerInfoWidget()
{
    UE_LOG(LogTemp, Log, TEXT("Player Info Widget toggled via key press."));

    // Do not allow opening this widget if we are in the main menu.
    AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
    if (!GameState)
    {
        UE_LOG(LogTemp, Warning, TEXT("TogglePlayerInfoWidget: GameState is null, cannot toggle widget."));
        return;
    }

    const EGamePhase CurrentPhase = GameState->GetGamePhase();
    if (CurrentPhase == EGamePhase::WaitingToStart)
    {
        UE_LOG(LogTemp, Log, TEXT("TogglePlayerInfoWidget: Widget is disabled in the main menu (WaitingToStart phase)."));
        return;
    }

    // If widget is already shown, hide it.
    if (PlayerInfoWidgetInstance && PlayerInfoWidgetInstance->IsInViewport())
    {
        PlayerInfoWidgetInstance->RemoveFromParent();
    }
    else // Otherwise, show it.
    {
        if (PlayerInfoWidgetClass)
        {
            if (!PlayerInfoWidgetInstance)
            {
                PlayerInfoWidgetInstance = CreateWidget<UPlayerInfoWidget>(this, PlayerInfoWidgetClass);
            }
            
            if (PlayerInfoWidgetInstance)
            {
                PlayerInfoWidgetInstance->AddToViewport(5); // Z-Order
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AChessPlayerController: PlayerInfoWidgetClass not assigned in Blueprint!"));
        }
    }
    // We do not call UpdateInputMode() because this widget is just an overlay and should not change input mode.
}

void AChessPlayerController::ToggleProfileWidget()
{
    // If profile widget is already open, close it and show main menu.
    if (PlayerProfileWidgetInstance && PlayerProfileWidgetInstance->IsInViewport())
    {
        PlayerProfileWidgetInstance->RemoveFromParent();

        // Show main menu again if it exists.
        if (StartMenuWidgetInstance)
        {
            StartMenuWidgetInstance->SetVisibility(ESlateVisibility::Visible);
        }
    }
    else // Otherwise, open profile widget and hide main menu.
    {
        // Hide main menu if it is currently on screen.
        if (StartMenuWidgetInstance && StartMenuWidgetInstance->IsInViewport())
        {
            StartMenuWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
        }

        if (PlayerProfileWidgetClass)
        {
            if (!PlayerProfileWidgetInstance)
            {
                PlayerProfileWidgetInstance = CreateWidget<UUserWidget>(this, PlayerProfileWidgetClass);
            }
            
            if (PlayerProfileWidgetInstance)
            {
                PlayerProfileWidgetInstance->AddToViewport(11); // Z-order higher than other menus
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AChessPlayerController: PlayerProfileWidgetClass not assigned in Blueprint!"));
        }
    }
    UpdateInputMode();
}

void AChessPlayerController::ReturnToMainMenu()
{
    UChessGameInstance* GI = GetGameInstance<UChessGameInstance>();
    // If we are host, we must destroy the session.
    if (GI && IsHost())
    {
        IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
        if (Subsystem)
        {
            IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
            if (SessionInterface.IsValid())
            {
                SessionInterface->DestroySession(NAME_GameSession);
            }
        }
    }
    
    // Use the same level as in StartMenuWidget for consistency.
    const FName MainMenuLevelName = FName(TEXT("/Game/Cigar_room/Maps/Cigar_room"));
    UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}

void AChessPlayerController::Client_ShowGameOverScreen_Implementation(const FText& ResultText, const FText& ReasonText)
{
    if (GameOverWidgetClass)
    {
        if (!GameOverWidgetInstance)
        {
            GameOverWidgetInstance = CreateWidget<UGameOverWidget>(this, GameOverWidgetClass);
        }

        if (GameOverWidgetInstance && !GameOverWidgetInstance->IsInViewport())
        {
            GameOverWidgetInstance->SetResultText(ResultText);
            GameOverWidgetInstance->SetReasonText(ReasonText);
            GameOverWidgetInstance->AddToViewport(20); // Highest Z-order to be on top of everything
            UpdateInputMode();
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AChessPlayerController: GameOverWidgetClass not assigned in Blueprint!"));
    }
}

void AChessPlayerController::ShowLobbyUI()
{
    if (IsLocalController() && LobbyWidgetClass)
    {
        if (!LobbyWidgetInstance)
        {
            LobbyWidgetInstance = CreateWidget<ULobbyWidget>(this, LobbyWidgetClass);
        }
        if (LobbyWidgetInstance && !LobbyWidgetInstance->IsInViewport())
        {
            // Remove start menu if it exists
            if (StartMenuWidgetInstance && StartMenuWidgetInstance->IsInViewport())
            {
                StartMenuWidgetInstance->RemoveFromParent();
                StartMenuWidgetInstance = nullptr;
            }

            LobbyWidgetInstance->AddToViewport(10);
            UpdateInputMode();
        }
    }
    else if (IsLocalController())
    {
        UE_LOG(LogTemp, Error, TEXT("LobbyWidgetClass is not set in BP_ChessPlayerController!"));
    }
}

void AChessPlayerController::HideLobbyUI()
{
    if (LobbyWidgetInstance && LobbyWidgetInstance->IsInViewport())
    {
        LobbyWidgetInstance->RemoveFromParent();
        LobbyWidgetInstance = nullptr;
        UpdateInputMode();
    }
}

void AChessPlayerController::LeaveLobby()
{
    // Simply return to main menu.
    // ReturnToMainMenu() already handles session destruction for host.
    ReturnToMainMenu();
}

bool AChessPlayerController::IsHost() const
{
    // Simple way to check if player is host - check NetMode.
    return GetNetMode() == NM_ListenServer;
}

void AChessPlayerController::ToggleDebugInfo()
{
    bShowDebugInfo = !bShowDebugInfo;
}


void AChessPlayerController::SetGameCamera()
{
    AGameCameraActor* GameCamera = Cast<AGameCameraActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameCameraActor::StaticClass()));
    if (GameCamera)
    {
        UE_LOG(LogCameraManagement, Log, TEXT("Switching to Game Camera: %s"), *GetNameSafe(GameCamera));
        SetViewTargetWithBlend(GameCamera, 0.5f);
        if (AChessPlayerCameraManager* CamManager = Cast<AChessPlayerCameraManager>(PlayerCameraManager))
        {
            CamManager->StartControllingGameCamera();
        }
    }
    else
    {
        UE_LOG(LogCameraManagement, Warning, TEXT("GameCameraActor not found in world. Cannot switch to game camera."));
    }
}

void AChessPlayerController::SetMenuCamera()
{
    AMenuCameraActor* CameraToSet = nullptr;

    // First, try to use the camera specified in the Blueprint property via TSoftObjectPtr.
    if (MenuCameraActor.IsValid())
    {
        UE_LOG(LogCameraManagement, Log, TEXT("Attempting to load Menu Camera from Soft Ptr reference."));
        // Force load the object pointed to by Soft Ptr.
        // This is necessary because the object might not be loaded yet when BeginPlay is called.
        CameraToSet = MenuCameraActor.LoadSynchronous();
    }

    // If the camera was not set in Blueprint, look for the first one in the scene as a fallback.
    if (!CameraToSet)
    {
        UE_LOG(LogCameraManagement, Log, TEXT("Menu Camera not loaded from properties. Searching for one in the world."));
        CameraToSet = Cast<AMenuCameraActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AMenuCameraActor::StaticClass()));
    }

    if (CameraToSet)
    {
        UE_LOG(LogCameraManagement, Log, TEXT("Switching to Menu Camera: %s"), *GetNameSafe(CameraToSet));
        SetViewTargetWithBlend(CameraToSet, 0.5f);
        if (AChessPlayerCameraManager* CamManager = Cast<AChessPlayerCameraManager>(PlayerCameraManager))
        {
            CamManager->StopControllingGameCamera();
        }
    }
    else
    {
        UE_LOG(LogCameraManagement, Warning, TEXT("MenuCameraActor not found either in properties or in the world! Falling back to game camera."));
        SetGameCamera();
    }
}

void AChessPlayerController::ShowStartMenu()
{
    if (IsLocalController() && StartMenuWidgetClass)
    {
        if (!StartMenuWidgetInstance)
        {
            StartMenuWidgetInstance = CreateWidget<UStartMenuWidget>(this, StartMenuWidgetClass);
        }

        if (StartMenuWidgetInstance)
        {
            StartMenuWidgetInstance->AddToViewport(10); // High Z-order to be on top of everything
            UpdateInputMode();
            SetMenuCamera();

            if (MenuMusic && !MenuMusicComponent)
            {
                MenuMusicComponent = UGameplayStatics::CreateSound2D(this, MenuMusic);
                if (MenuMusicComponent)
                {
                    MenuMusicComponent->Play();
                }
            }
        }
    }
}

void AChessPlayerController::HandleLook(const FInputActionValue& Value)
{
    // This function is reserved but currently unused.
    // Camera rotation is handled in HandleCameraMove.
}

void AChessPlayerController::HandleCameraMove(const FInputActionValue& Value)
{
    // Do not handle camera input if we are not in game mode (e.g. in a menu)
    if (!bIsInputModeSetForGame) return;

    bool bIsRotateDown = IsInputKeyDown(EKeys::RightMouseButton);
#if PLATFORM_ANDROID || PLATFORM_IOS
    bIsRotateDown = bIsRotateDown || IsInputKeyDown(EKeys::LeftMouseButton);
#endif

    // Rotate camera only if Right Mouse Button is held (or LMB on mobile)
    if (bIsRotateDown)
    {
        const FVector2D LookAxisVector = Value.Get<FVector2D>();

        // Add pitch input directly to ControlRotation.
        // Yaw is synchronized with the camera in the Tick function.
        // Invert Y axis because moving mouse up usually corresponds to negative value.
        AddPitchInput(LookAxisVector.Y * -1.0f);
    
        if (AChessPlayerCameraManager* CamManager = Cast<AChessPlayerCameraManager>(PlayerCameraManager))
        {
            // This function rotates the camera itself.
            CamManager->AddCameraRotationInput(LookAxisVector);
        }
    }
}

AChessGameMode* AChessPlayerController::GetChessGameMode() const
{
    UWorld* World = GetWorld();
    if (World)
    {
        return Cast<AChessGameMode>(World->GetAuthGameMode());
    }
    return nullptr;
}

void AChessPlayerController::SetPlayerColorChoiceForBotGame(int32 ChoiceIndex)
{
    if (AChessGameMode* GM = GetChessGameMode())
    {
        GM->SetPlayerColorForBotGameFromInt(ChoiceIndex);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessPlayerController::SetPlayerColorChoiceForBotGame: Could not get ChessGameMode."));
    }
}

void AChessPlayerController::SetInputModeForGame()
{
    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);
    bShowMouseCursor = true;
    bIsInputModeSetForGame = true;
}

void AChessPlayerController::SetInputModeForUI()
{
    FInputModeUIOnly InputMode;
    SetInputMode(InputMode);
    bShowMouseCursor = true;
    bIsInputModeSetForGame = false;
}

void AChessPlayerController::UpdateInputMode()
{
    UE_LOG(LogTemp, Log, TEXT("--- UpdateInputMode: CALLED ---"));

    const bool bStartMenuVisible = StartMenuWidgetInstance && StartMenuWidgetInstance->IsInViewport();
    const bool bPauseMenuVisible = PauseMenuWidgetInstance && PauseMenuWidgetInstance->IsInViewport();
    const bool bSettingsMenuVisible = GraphicsSettingsWidgetInstance && GraphicsSettingsWidgetInstance->IsInViewport();
    const bool bProfileMenuVisible = PlayerProfileWidgetInstance && PlayerProfileWidgetInstance->IsInViewport();
    const bool bPromotionMenuVisible = PromotionMenuWidgetInstance && PromotionMenuWidgetInstance->IsInViewport();
    const bool bGameOverScreenVisible = GameOverWidgetInstance && GameOverWidgetInstance->IsInViewport();
    const bool bLobbyVisible = LobbyWidgetInstance && LobbyWidgetInstance->IsInViewport();

    UE_LOG(LogTemp, Log, TEXT("UpdateInputMode: Menu visibility states: Start=%d, Pause=%d, Settings=%d, Profile=%d, Promotion=%d, GameOver=%d, Lobby=%d"),
        (int32)bStartMenuVisible, (int32)bPauseMenuVisible, (int32)bSettingsMenuVisible, (int32)bProfileMenuVisible, (int32)bPromotionMenuVisible, (int32)bGameOverScreenVisible, (int32)bLobbyVisible);

    if (bStartMenuVisible || bPauseMenuVisible || bSettingsMenuVisible || bProfileMenuVisible || bPromotionMenuVisible || bGameOverScreenVisible || bLobbyVisible)
    {
        UE_LOG(LogTemp, Log, TEXT("UpdateInputMode: At least one menu is visible. Setting input mode to UI_ONLY."));
        SetInputModeForUI();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("UpdateInputMode: No menus are visible. Setting input mode to GAME_AND_UI."));
        SetInputModeForGame();
    }
}

void AChessPlayerController::Client_GameStarted_Implementation()
{
    bHasGameStarted_Client = true; // Set flag that game has started on client
    SetupGameUI();

    // Set game camera and its perspective here.
    // This RPC is called with a delay from GameMode, allowing time for PlayerColor
    // to replicate. This prevents camera "jitters" at start when playing as Black.
    // OnRep_PlayerColor will fix perspective if color arrives late.
    SetGameCamera();
    if (AChessPlayerCameraManager* CamManager = Cast<AChessPlayerCameraManager>(PlayerCameraManager))
    {
        CamManager->SwitchToPlayerPerspective(PlayerColor);
    }
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan, TEXT("Match has started! Good luck."));
    }
}

void AChessPlayerController::OnRep_PlayerColor()
{
    // This function is called on the client when PlayerColor property is replicated from the server.
    FString MyColorStr = (PlayerColor == EPieceColor::White) ? TEXT("White") : TEXT("Black");
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("The server has assigned you the color: %s"), *MyColorStr));
    }
    
    // Switch camera to correct perspective ONLY IF the game has already started (i.e., Client_GameStarted was called).
    // This prevents camera switching while in the main menu and fixes perspective
    // if Client_GameStarted was called before color replication.
    if (bHasGameStarted_Client)
    {
        if (AChessPlayerCameraManager* CamManager = Cast<AChessPlayerCameraManager>(PlayerCameraManager))
        {
            FString ColorStr = (PlayerColor == EPieceColor::White) ? TEXT("White") : TEXT("Black");
            UE_LOG(LogCameraManagement, Log, TEXT("OnRep_PlayerColor: Game has started. Switching camera perspective to %s's view."), *ColorStr);
            CamManager->SwitchToPlayerPerspective(PlayerColor);
        }
    }
    else
    {
        UE_LOG(LogCameraManagement, Log, TEXT("OnRep_PlayerColor: Received color, but game has not started yet on client. Deferring camera switch."));
    }
}

void AChessPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AChessPlayerController, PlayerColor);
}

void AChessPlayerController::DetermineInitialUI()
{
    // If lobby widget is already shown, do nothing.
    // This prevents re-opening the main menu over the lobby.
    if (LobbyWidgetInstance && LobbyWidgetInstance->IsInViewport())
    {
        return;
    }

    AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
    if (!GameState)
    {
        // If GameState is still invalid, this is a serious issue.
        UE_LOG(LogTemp, Fatal, TEXT("AChessPlayerController::DetermineInitialUI: AChessGameState is NULL! Check GameMode Override in World Settings."));
        return;
    }

    if (GameState->GetGamePhase() == EGamePhase::WaitingToStart)
    {
        // We are in the main menu or waiting screen.
        ShowStartMenu();
    }
    else
    {
        // Game is already in progress, setup game UI.
        SetupGameUI();
    }
}

void AChessPlayerController::SetupGameUI()
{
    if (StartMenuWidgetInstance && StartMenuWidgetInstance->IsInViewport())
    {
        StartMenuWidgetInstance->RemoveFromParent();
        StartMenuWidgetInstance = nullptr;
    }

    if (MenuMusicComponent && MenuMusicComponent->IsPlaying())
    {
        MenuMusicComponent->Stop();
    }
    MenuMusicComponent = nullptr;

    UpdateInputMode();
    // Camera is now set in Client_GameStarted to ensure that PlayerColor is already replicated.
}

void AChessPlayerController::SetPlayerColor(EPieceColor NewColor)
{
    // This function should only be called on the server (in GameMode).
    if (HasAuthority())
    {
        PlayerColor = NewColor;

        // OnRep functions are not called on the server, so we call it manually
        // for the server's local controller (host in listen-server game).
        // We should not call it for client proxy controllers on the server.
        if (IsLocalController())
        {
            OnRep_PlayerColor();
        }
    }
}

EPieceColor AChessPlayerController::GetPlayerColor() const
{
    return PlayerColor;
}

void AChessPlayerController::OnClickStarted()
{
    // Determine what we clicked on
    FHitResult HitResult;
    bool bHit = GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

#if PLATFORM_ANDROID || PLATFORM_IOS
    if (!bHit)
    {
        bHit = GetHitResultUnderFinger(ETouchIndex::Touch1, ECC_Visibility, false, HitResult);
    }
#endif

    // Process the hit (or lack thereof)
    ProcessHit(HitResult);
}

void AChessPlayerController::ProcessHit(const FHitResult& HitResult)
{
    // --- 1. Preliminary Game State Checks ---
    AChessGameState* GameState = GetWorld()->GetGameState<AChessGameState>();
    if (!GameState)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProcessHit ABORTED: GameState is NULL."));
        return;
    }

    if (GameState->GetCurrentTurnColor() != PlayerColor)
    {
        UE_LOG(LogTemp, Log, TEXT("ProcessHit ABORTED: Not player's turn."));
        return;
    }

    const EGamePhase CurrentPhase = GameState->GetGamePhase();
    if (CurrentPhase == EGamePhase::AwaitingPromotion)
    {
        UE_LOG(LogTemp, Log, TEXT("ProcessHit ABORTED: Awaiting promotion."));
        return;
    }
    if (CurrentPhase != EGamePhase::InProgress && CurrentPhase != EGamePhase::Check)
    {
        UE_LOG(LogTemp, Log, TEXT("ProcessHit ABORTED: Cannot move in current game phase: %s"), *UEnum::GetValueAsString(CurrentPhase));
        return;
    }

    if (!ChessBoard)
    {
        UE_LOG(LogTemp, Error, TEXT("ProcessHit ABORTED: ChessBoard reference is NULL."));
        return;
    }

    // Use ECC_Visibility channel as piece meshes and board block it.
    if (!HitResult.bBlockingHit)
    {
        // Clicked on empty space, not on board or piece
        UE_LOG(LogTemp, Log, TEXT("ProcessHit: Clicked on empty space. Clearing selection."));
        ClearSelectionAndHighlights();
        return;
    }

    // --- 3. Main Selection and Move Logic Based on State ---
    const FIntPoint HitGridPosition = ChessBoard->WorldToGridPosition(HitResult.Location);
    if (!ChessBoard->IsValidGridPosition(HitGridPosition))
    {
        UE_LOG(LogTemp, Log, TEXT("ProcessHit: Clicked outside of valid board grid. Clearing selection."));
        ClearSelectionAndHighlights();
        return;
    }

    // Use GameState to determine piece, as it is the authoritative source.
    AChessPiece* PieceOnSquare = GameState->GetPieceAtGridPosition(HitGridPosition);
    UE_LOG(LogTemp, Log, TEXT("ProcessHit: Clicked on grid (%d, %d). Piece on square: %s"), HitGridPosition.X, HitGridPosition.Y, *GetNameSafe(PieceOnSquare));

    if (PieceOnSquare) // Clicked on a square with a piece
    {
        if (PieceOnSquare->GetPieceColor() == PlayerColor) // It's our piece
        {
            // Logic for selection/re-selection/deselection is handled in this function
            HandlePieceSelection(PieceOnSquare);
        }
        else // It's an enemy piece
        {
            if (SelectedPiece) // If we have a piece selected, this is a capture attempt
            {
                HandleBoardClick(HitGridPosition);
            }
            else // If nothing selected, clicking enemy does nothing
            {
                UE_LOG(LogTemp, Log, TEXT("ProcessHit: Clicked enemy piece with no selection. No action."));
            }
        }
    }
    else // Clicked on an empty square
    {
        if (SelectedPiece) // If we have a piece selected, this is a move attempt
        {
            HandleBoardClick(HitGridPosition);
        }
        else // If nothing selected, clicking empty square does nothing but clear highlights
        {
            UE_LOG(LogTemp, Log, TEXT("ProcessHit: Clicked empty square with no selection. Clearing highlights."));
            ClearSelectionAndHighlights();
        }
    }
}

void AChessPlayerController::OnTouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location)
{
    // Only handle first finger
    if (FingerIndex != ETouchIndex::Touch1) return;

    PreviousTouchLocation = FVector2D(Location.X, Location.Y);
    bIsTouchDragging = false;
}

void AChessPlayerController::OnTouchMoved(const ETouchIndex::Type FingerIndex, const FVector Location)
{
    // Only handle first finger
    if (FingerIndex != ETouchIndex::Touch1) return;

    // Determine if we dragged enough to count as a move/camera rotation
    FVector2D CurrentTouchLocation(Location.X, Location.Y);
    FVector2D Delta = CurrentTouchLocation - PreviousTouchLocation;

    // Threshold to start dragging (avoid jitter on taps)
    if (!bIsTouchDragging && Delta.SizeSquared() < 100.0f) // 10 pixels threshold squared (approx)
    {
        return;
    }
    
    bIsTouchDragging = true;

    // Apply sensitivity factor.
    // Touch screen coords can be large, so we scale down.
    const float TouchSensitivity = 0.2f; 
    FVector2D LocalRotationInput = Delta * TouchSensitivity;

    // Use the existing camera rotation logic
    // Pitch (Y axis of screen moves pitch)
    AddPitchInput(LocalRotationInput.Y * -1.0f);
    
    if (AChessPlayerCameraManager* CamManager = Cast<AChessPlayerCameraManager>(PlayerCameraManager))
    {
        CamManager->AddCameraRotationInput(LocalRotationInput);
    }

    PreviousTouchLocation = CurrentTouchLocation;
}

void AChessPlayerController::OnTouchEnded(const ETouchIndex::Type FingerIndex, const FVector Location)
{
    if (FingerIndex != ETouchIndex::Touch1) return;

    // If we didn't drag, treat it as a click/tap
    if (!bIsTouchDragging)
    {
        const double CurrentTime = FPlatformTime::Seconds();
        const FVector2D CurrentTapLocation(Location.X, Location.Y);
        
        bool bIsDoubleTap = false;

        // Check for double tap
        if (LastTapTime > 0.0)
        {
            const double TimeDiff = CurrentTime - LastTapTime;
            const float DistanceSq = FVector2D::DistSquared(CurrentTapLocation, LastTapLocation);

            // Thresholds: 0.3s for time, 50 pixels for distance (2500 sq)
            if (TimeDiff < 0.3 && DistanceSq < 2500.0f)
            {
                bIsDoubleTap = true;
                TogglePauseMenu();
                
                // Reset to avoid detecting a "triple tap" as another double tap
                LastTapTime = 0.0; 
            }
        }

        if (!bIsDoubleTap)
        {
            FHitResult HitResult;
            bool bHit = GetHitResultAtScreenPosition(CurrentTapLocation, ECC_Visibility, false, HitResult);
            ProcessHit(HitResult);

            // Update for next tap check
            LastTapTime = CurrentTime;
            LastTapLocation = CurrentTapLocation;
        }
    }
    
    bIsTouchDragging = false;
}

bool AChessPlayerController::Server_AttemptMove_Validate(AChessPiece* PieceToMove, const FIntPoint& TargetGridPosition)
{
    // Simple validation to prevent sending invalid data from client.
    return PieceToMove != nullptr;
}

void AChessPlayerController::Server_AttemptMove_Implementation(AChessPiece* PieceToMove, const FIntPoint& TargetGridPosition)
{
    AChessGameMode* GameMode = GetChessGameMode();
    if (GameMode)
    {
        // GameMode->AttemptMove will execute the move if valid.
        // If invalid, it returns false, and server state remains unchanged.
        // Actor replication will ensure client piece snaps back.
        GameMode->AttemptMove(PieceToMove, TargetGridPosition, this);
    }
}

void AChessPlayerController::Server_RequestStartGame_Implementation()
{
    // Only host can start the game
    if (IsHost())
    {
        if (AChessGameState* GS = GetWorld()->GetGameState<AChessGameState>())
        {
            // Check if there are two players in lobby
            if (GS->PlayerArray.Num() >= 2)
            {
                // Exit lobby state
                GS->SetIsInLobby(false);

                // Start game (this triggers Client_GameStarted on all clients)
                if (AChessGameMode* GM = GetChessGameMode())
                {
                    GM->StartNewGame();
                }
            }
            else
            {
                // Not enough players
                UE_LOG(LogTemp, Warning, TEXT("Cannot start game: Not enough players in the lobby."));
            }
        }
    }
}


void AChessPlayerController::Server_SetPlayerProfile_Implementation(const FPlayerProfile& Profile)
{
    // Called on server when client sends their profile info.
    if (AChessPlayerState* PS = GetPlayerState<AChessPlayerState>())
    {
        PS->SetPlayerProfile(Profile);
    }
}

void AChessPlayerController::HandlePieceSelection(AChessPiece* PieceToSelect)
{
    if (!PieceToSelect || !ChessBoard)
    {
        return;
    }

    // If clicking on already selected piece, deselect it
    if (SelectedPiece == PieceToSelect)
    {
        ClearSelectionAndHighlights();
        return;
    }

    // If another piece was selected, clear old selection first
    if (SelectedPiece)
    {
        ClearSelectionAndHighlights();
    }

    SelectedPiece = PieceToSelect;
    SelectedPiece->OnSelected();

    // Calculate and display valid moves locally on client for instant feedback.
    // Server will still verify the move when executed.
    AChessGameState* GameState = GetWorld()->GetGameState<AChessGameState>();
    if (GameState)
    {
        // First get all pseudo-legal moves for this piece
        const TArray<FIntPoint> PseudoLegalMoves = SelectedPiece->GetValidMoves(GameState, ChessBoard);
        
        LastValidMoves.Empty(); // Clear old list

        // Filter moves to keep only those that don't leave king in check
        for (const FIntPoint& Move : PseudoLegalMoves)
        {
            // IsMoveLegal simulates move and checks for check.
            if (GameState->IsMoveLegal(SelectedPiece, Move, ChessBoard))
            {
                LastValidMoves.Add(Move);
            }
        }

        // Highlight the selected piece itself
        ChessBoard->HighlightSquare(SelectedPiece->GetBoardPosition(), SelectedPieceHighlightColor);
        // Highlight all valid moves
        for (const FIntPoint& Move : LastValidMoves)
        {
            if (ValidMoveIndicatorMesh)
            {
                UStaticMeshComponent* IndicatorComponent = NewObject<UStaticMeshComponent>(ChessBoard);
                if (IndicatorComponent)
                {
                    IndicatorComponent->SetStaticMesh(ValidMoveIndicatorMesh);
                    if (ValidMoveIndicatorMaterial)
                    {
                        IndicatorComponent->SetMaterial(0, ValidMoveIndicatorMaterial);
                    }
                    IndicatorComponent->SetWorldScale3D(ValidMoveIndicatorScale);
                    // Disable collision so indicators don't block clicks
                    IndicatorComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    // Register component so it appears in world
                    IndicatorComponent->RegisterComponent();
                    // Attach to board so it's part of its hierarchy
                    IndicatorComponent->AttachToComponent(ChessBoard->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
                    // Set location to center of square
                    IndicatorComponent->SetWorldLocation(ChessBoard->GridToWorldPosition(Move));
                    
                    ValidMoveIndicatorComponents.Add(IndicatorComponent);
                }
            }
            else
            {
                // If mesh not set, use old color highlight method
                ChessBoard->HighlightSquare(Move, ValidMoveHighlightColor);
            }
        }
    }
}

void AChessPlayerController::HandleBoardClick(const FIntPoint& GridPosition)
{
    if (!SelectedPiece || !ChessBoard)
    {
        return;
    }

    if (LastValidMoves.Contains(GridPosition))
    {
        Server_AttemptMove(SelectedPiece, GridPosition);

        // Deselect only AFTER sending valid move to server.
        // This ensures correct feedback for player.
        ClearSelectionAndHighlights();
    }
    // If player clicked invalid square, we do NOT deselect.
    // This allows choosing another square without re-selecting the piece.
}

void AChessPlayerController::ClearSelectionAndHighlights()
{
    // Destroy and clear all move indicators
    for (UStaticMeshComponent* Indicator : ValidMoveIndicatorComponents)
    {
        if (Indicator && !Indicator->IsBeingDestroyed())
        {
            Indicator->DestroyComponent();
        }
    }
    ValidMoveIndicatorComponents.Empty();

    if (ChessBoard)
    {
        // This function now only clears highlight of the selected piece
        ChessBoard->ClearAllHighlights();
    }
    if (SelectedPiece)
    {
        SelectedPiece->OnDeselected();
        SelectedPiece = nullptr;
    }
    LastValidMoves.Empty();
}

void AChessPlayerController::Client_ShowPromotionMenu_Implementation(APawnPiece* PawnForPromotion)
{
    if (PromotionMenuWidgetClass)
    {
        if (!PromotionMenuWidgetInstance)
        {
            PromotionMenuWidgetInstance = CreateWidget<UPromotionMenuWidget>(this, PromotionMenuWidgetClass);
            if (PromotionMenuWidgetInstance)
            {
                // Bind handler to selection event
                PromotionMenuWidgetInstance->OnPromotionPieceSelected.AddDynamic(this, &AChessPlayerController::HandlePromotionSelection);
            }
        }

        if (PromotionMenuWidgetInstance && !PromotionMenuWidgetInstance->IsInViewport())
        {
            PawnAwaitingPromotion = PawnForPromotion; // Save pawn to send to server
            PromotionMenuWidgetInstance->AddToViewport(10); // High Z-order to be on top of everything
            UpdateInputMode();
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AChessPlayerController: PromotionMenuWidgetClass is not set in the Blueprint!"));
    }
}

void AChessPlayerController::HandlePromotionSelection(EPieceType SelectedType)
{
    if (PawnAwaitingPromotion)
    {
        Server_CompletePawnPromotion(PawnAwaitingPromotion, SelectedType);
    }
    
    // Hide selection menu and update input mode
    if (PromotionMenuWidgetInstance)
    {
        PromotionMenuWidgetInstance->RemoveFromParent();
    }
    UpdateInputMode();
    PawnAwaitingPromotion = nullptr;
}


bool AChessPlayerController::Server_CompletePawnPromotion_Validate(APawnPiece* PawnToPromote, EPieceType PromoteToType)
{
    // Simple validation: pawn must exist, and type must be valid for promotion.
    return PawnToPromote != nullptr && (PromoteToType == EPieceType::Queen || PromoteToType == EPieceType::Rook || PromoteToType == EPieceType::Bishop || PromoteToType == EPieceType::Knight);
}

void AChessPlayerController::Server_CompletePawnPromotion_Implementation(APawnPiece* PawnToPromote, EPieceType PromoteToType)
{
    AChessGameMode* GameMode = GetChessGameMode();
    if (GameMode)
    {
        GameMode->CompletePawnPromotion(PawnToPromote, PromoteToType);
    }
}

void AChessPlayerController::Client_PlaySound_Implementation(EChessSoundType SoundType)
{
    USoundBase* SoundToPlay = nullptr;
    switch (SoundType)
    {
    case EChessSoundType::Move:      SoundToPlay = MoveSound;      break;
    case EChessSoundType::Capture:   SoundToPlay = CaptureSound;   break;
    case EChessSoundType::Castle:    SoundToPlay = CastleSound;    break;
    case EChessSoundType::Check:     SoundToPlay = CheckSound;     break;
    case EChessSoundType::Checkmate: SoundToPlay = CheckmateSound; break;
    case EChessSoundType::GameStart: SoundToPlay = GameStartSound; break;
    }

    if (SoundToPlay)
    {
        // Play sound locally for this player
        UGameplayStatics::PlaySound2D(this, SoundToPlay);
    }
}

void AChessPlayerController::Client_PlayCaptureEffect_Implementation(AChessPiece* CapturedPiece, const FVector& Location, const FVector& Scale, const FVector& CellBoundingBox, float Lifetime, float Density)
{
    // 1. Show smoke
    if (CaptureEffect)
    {
        // Spawn Niagara system, applying specified scale.
        UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            CaptureEffect,
            Location,
            FRotator::ZeroRotator,
            Scale, // Set overall component scale.
            false, // bAutoDestroy - manage lifetime manually
            true   // bAutoActivate
        );

        if (NiagaraComponent)
        {
            // Set additional parameters if used in Niagara effect.
            NiagaraComponent->SetVariableFloat(TEXT("User.Density"), Density);
            
            // If valid bounding box size passed, set it.
            // Requires Niagara effect to be configured to use this parameter.
            if (!CellBoundingBox.IsZero())
            {
                NiagaraComponent->SetVariableVec3(TEXT("User.CellBoundingBox"), CellBoundingBox);
                UE_LOG(LogTemp, Log, TEXT("Spawned CaptureEffect with Scale=%s, Density=%f, and CellBoundingBox=%s"), *Scale.ToString(), Density, *CellBoundingBox.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("Spawned CaptureEffect with Scale=%s and Density=%f"), *Scale.ToString(), Density);
            }

            // Set timer to destroy smoke.
            if (Lifetime > 0.f)
            {
                TWeakObjectPtr<UNiagaraComponent> WeakEmitter = NiagaraComponent;
                FTimerHandle TimerHandle;
                GetWorld()->GetTimerManager().SetTimer(TimerHandle, [WeakEmitter]()
                {
                    if (WeakEmitter.IsValid())
                    {
                        WeakEmitter->DestroyComponent();
                    }
                }, Lifetime, false);
            }
        }
    }

    // 2. Immediately hide captured piece
    if (CapturedPiece)
    {
        CapturedPiece->SetActorHiddenInGame(true);
    }
}
