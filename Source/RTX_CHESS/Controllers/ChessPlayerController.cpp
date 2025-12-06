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

    // Устанавливаем цвета подсветки по умолчанию
    ValidMoveHighlightColor = FLinearColor(0.1f, 0.5f, 0.1f, 1.0f); // Темно-зеленый
    SelectedPieceHighlightColor = FLinearColor(0.2f, 0.2f, 0.8f, 1.0f); // Синий
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
            UE_LOG(LogTemp, Warning, TEXT("AChessPlayerController::BeginPlay: ChessMappingContext не назначен! Пожалуйста, назначьте его в Blueprint контроллера игрока."));
        }
    }

    ChessBoard = Cast<AChessBoard>(UGameplayStatics::GetActorOfClass(GetWorld(), AChessBoard::StaticClass()));
    if (!ChessBoard)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessPlayerController::BeginPlay: ChessBoard actor not found!"));
    }

    // Откладываем решение о том, что показывать, до тех пор, пока GameState точно не будет доступен.
    // Это помогает избежать гонок состояний, когда BeginPlay контроллера игрока запускается
    // до того, как GameState полностью инициализирован или реплицирован.
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AChessPlayerController::DetermineInitialUI, 0.1f, false);
    
    // Если мы клиент, отправляем наш профиль на сервер
    if (IsLocalController() && GetNetMode() == NM_Client)
    {
        if (UChessGameInstance* GameInstance = GetGameInstance<UChessGameInstance>())
        {
            Server_SetPlayerProfile(GameInstance->GetPlayerProfile());
        }
    }
    
    // Если мы хост, проверяем, нужно ли нам создавать лобби.
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

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
    {
        if (ClickAction)
        {
            EnhancedInput->BindAction(ClickAction, ETriggerEvent::Started, this, &AChessPlayerController::OnClickStarted);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessPlayerController::SetupInputComponent: ClickAction не назначен! Пожалуйста, назначьте его в Blueprint контроллера игрока."));
        }

        if (LookAction)
        {
            EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AChessPlayerController::HandleLook);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessPlayerController::SetupInputComponent: LookAction не назначен! Пожалуйста, назначьте его в Blueprint контроллера игрока."));
        }

        if (MoveCameraAction)
        {
            EnhancedInput->BindAction(MoveCameraAction, ETriggerEvent::Triggered, this, &AChessPlayerController::HandleCameraMove);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessPlayerController::SetupInputComponent: MoveCameraAction не назначен! Пожалуйста, назначьте его в Blueprint контроллера игрока."));
        }

        if (PauseAction)
        {
            EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &AChessPlayerController::TogglePauseMenu);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessPlayerController::SetupInputComponent: PauseAction не назначен! Пожалуйста, назначьте его в Blueprint контроллера игрока."));
        }

        if (PlayerInfoAction)
        {
            EnhancedInput->BindAction(PlayerInfoAction, ETriggerEvent::Started, this, &AChessPlayerController::TogglePlayerInfoWidget);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessPlayerController::SetupInputComponent: PlayerInfoAction не назначен! Пожалуйста, назначьте его в Blueprint контроллера игрока."));
        }

        if (ToggleDebugAction)
        {
            EnhancedInput->BindAction(ToggleDebugAction, ETriggerEvent::Started, this, &AChessPlayerController::ToggleDebugInfo);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessPlayerController::SetupInputComponent: ToggleDebugAction не назначен! Пожалуйста, назначьте его в Blueprint контроллера игрока."));
        }
    }
}

void AChessPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Синхронизируем рыскание (Yaw) ControlRotation с реальным вращением камеры.
    // Тангаж (Pitch) будет управляться отдельно, чтобы избежать взгляда в потолок при старте.
    if (bIsInputModeSetForGame)
    {
        // ОПТИМИЗАЦИЯ: Этот блок кода должен выполняться, только когда игрок активно вращает камеру (зажата ПКМ).
        // Это предотвращает ненужные вычисления в каждом кадре, когда камера статична.
        if (PlayerCameraManager && IsInputKeyDown(EKeys::RightMouseButton))
        {
            FRotator CurrentControlRotation = GetControlRotation();

            // ПРАВИЛЬНЫЙ СПОСОБ СИНХРОНИЗАЦИИ YAW:
            // 1. Получаем вектор, куда смотрит камера.
            const FVector CameraForward = PlayerCameraManager->GetCameraRotation().Vector();
            // 2. Преобразуем этот вектор во вращение. Этот метод правильно вычисляет Yaw, даже если есть Pitch.
            const FRotator CameraDirectionAsRotator = CameraForward.Rotation();
            // 3. Синхронизируем Yaw головы с Yaw камеры.
            CurrentControlRotation.Yaw = CameraDirectionAsRotator.Yaw;

            // Ограничиваем вертикальное вращение (тангаж) для реалистичного движения головы.
            // -45 градусов вниз и +30 градусов вверх - хороший диапазон для сидящего человека.
            CurrentControlRotation.Pitch = FMath::Clamp(CurrentControlRotation.Pitch, -45.0f, 30.0f);

            // Явно обнуляем крен (Roll), чтобы предотвратить искажения и скручивание головы.
            CurrentControlRotation.Roll = 0.0f;
            
            SetControlRotation(CurrentControlRotation);
        }
    }

    // --- Отладочная информация на экране ---
    if (bShowDebugInfo && GEngine)
    {
        // ОПТИМИЗАЦИЯ: Обновляем отладочную информацию реже, чем каждый кадр,
        // так как это очень ресурсоемкая операция.
        static float DebugInfoTimer = 0.0f;
        DebugInfoTimer += DeltaTime;

        if (DebugInfoTimer > 0.1f) // Обновление ~10 раз в секунду
        {
            DebugInfoTimer = 0.0f;
            
            AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
            if (GameState)
            {
                FString GamePhaseStr = UEnum::GetValueAsString(GameState->GetGamePhase());
                FString CurrentTurnStr = (GameState->GetCurrentTurnColor() == EPieceColor::White) ? TEXT("White") : TEXT("Black");
                
                GEngine->AddOnScreenDebugMessage(0, 0.f, FColor::Yellow, FString::Printf(TEXT("Game Phase: %s"), *GamePhaseStr));
                GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Yellow, FString::Printf(TEXT("Current Turn: %s"), *CurrentTurnStr));

                // Отображение времени
                const int32 WhiteTimeInt = FMath::CeilToInt(GameState->WhiteTimeSeconds);
                const FString WhiteTimeStr = (WhiteTimeInt < 0) ? TEXT("Unlimited") : FString::Printf(TEXT("%02d:%02d"), WhiteTimeInt / 60, WhiteTimeInt % 60);
                GEngine->AddOnScreenDebugMessage(5, 0.f, FColor::White, FString::Printf(TEXT("White Time: %s"), *WhiteTimeStr));

                const int32 BlackTimeInt = FMath::CeilToInt(GameState->BlackTimeSeconds);
                const FString BlackTimeStr = (BlackTimeInt < 0) ? TEXT("Unlimited") : FString::Printf(TEXT("%02d:%02d"), BlackTimeInt / 60, BlackTimeInt % 60);
                GEngine->AddOnScreenDebugMessage(6, 0.f, FColor::Black, FString::Printf(TEXT("Black Time: %s"), *BlackTimeStr));

                // Отображение профилей
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

            // Отображение FPS
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
    // --- Конец отладочной информации ---
}

void AChessPlayerController::TogglePauseMenu()
{
    // Не позволяем открывать меню паузы, если мы не в игре или ждем выбора фигуры для пешки.
    AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
    if (!GameState) return;

    const EGamePhase CurrentPhase = GameState->GetGamePhase();
    if (CurrentPhase == EGamePhase::WaitingToStart || CurrentPhase == EGamePhase::AwaitingPromotion)
    {
        // Не открывать меню паузы в этих состояниях. Можно добавить и другие, например GameOver, если он появится.
        return;
    }

    // Если меню уже открыто, закрываем его
    if (PauseMenuWidgetInstance && PauseMenuWidgetInstance->IsInViewport())
    {
        PauseMenuWidgetInstance->RemoveFromParent();
    }
    else // Иначе, открываем
    {
        if (PauseMenuWidgetClass)
        {
            if (!PauseMenuWidgetInstance)
            {
                PauseMenuWidgetInstance = CreateWidget<UPauseMenuWidget>(this, PauseMenuWidgetClass);
            }
            
            if (PauseMenuWidgetInstance)
            {
                PauseMenuWidgetInstance->AddToViewport(10); // Высокий Z-order, чтобы быть поверх всего
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AChessPlayerController: PauseMenuWidgetClass не назначен в Blueprint!"));
        }
    }
    UpdateInputMode();
}

void AChessPlayerController::ToggleGraphicsSettingsMenu()
{
    // Если меню уже открыто, закрываем его
    if (GraphicsSettingsWidgetInstance && GraphicsSettingsWidgetInstance->IsInViewport())
    {
        GraphicsSettingsWidgetInstance->RemoveFromParent();
    }
    else // Иначе, открываем
    {
        if (GraphicsSettingsWidgetClass)
        {
            if (!GraphicsSettingsWidgetInstance)
            {
                GraphicsSettingsWidgetInstance = CreateWidget<UUserWidget>(this, GraphicsSettingsWidgetClass);
            }
            
            if (GraphicsSettingsWidgetInstance)
            {
                GraphicsSettingsWidgetInstance->AddToViewport(10); // Высокий Z-order, чтобы быть поверх всего
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AChessPlayerController: GraphicsSettingsWidgetClass не назначен в Blueprint!"));
        }
    }
    UpdateInputMode();
}

void AChessPlayerController::TogglePlayerInfoWidget()
{
    UE_LOG(LogTemp, Log, TEXT("Player Info Widget toggled via key press."));

    // Не позволяем открывать этот виджет, если мы в главном меню.
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

    // Если виджет уже показан, скрываем его.
    if (PlayerInfoWidgetInstance && PlayerInfoWidgetInstance->IsInViewport())
    {
        PlayerInfoWidgetInstance->RemoveFromParent();
    }
    else // Иначе, показываем.
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
            UE_LOG(LogTemp, Error, TEXT("AChessPlayerController: PlayerInfoWidgetClass не назначен в Blueprint!"));
        }
    }
    // Мы не вызываем UpdateInputMode(), так как этот виджет - просто оверлей и не должен менять режим ввода.
}

void AChessPlayerController::ToggleProfileWidget()
{
    // Если виджет профиля уже открыт, закрываем его и показываем главное меню.
    if (PlayerProfileWidgetInstance && PlayerProfileWidgetInstance->IsInViewport())
    {
        PlayerProfileWidgetInstance->RemoveFromParent();

        // Показываем главное меню снова, если оно существует.
        if (StartMenuWidgetInstance)
        {
            StartMenuWidgetInstance->SetVisibility(ESlateVisibility::Visible);
        }
    }
    else // Иначе, открываем виджет профиля и скрываем главное меню.
    {
        // Скрываем главное меню, если оно сейчас на экране.
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
                PlayerProfileWidgetInstance->AddToViewport(11); // Z-order выше, чем у других меню
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AChessPlayerController: PlayerProfileWidgetClass не назначен в Blueprint!"));
        }
    }
    UpdateInputMode();
}

void AChessPlayerController::ReturnToMainMenu()
{
    UChessGameInstance* GI = GetGameInstance<UChessGameInstance>();
    // Если мы хост, мы должны уничтожить сессию.
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
    
    // Используем тот же уровень, что и в StartMenuWidget, для консистентности.
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
            GameOverWidgetInstance->AddToViewport(20); // Наивысший Z-order, чтобы быть поверх всего
            UpdateInputMode();
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AChessPlayerController: GameOverWidgetClass не назначен в Blueprint!"));
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
            // Убираем стартовое меню, если оно вдруг есть
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
    // Просто возвращаемся в главное меню.
    // ReturnToMainMenu() уже обрабатывает уничтожение сессии для хоста.
    ReturnToMainMenu();
}

bool AChessPlayerController::IsHost() const
{
    // Простой способ проверить, является ли игрок хостом - это проверить NetMode.
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

    // Сначала пытаемся использовать камеру, указанную в свойстве Blueprint через TSoftObjectPtr.
    if (MenuCameraActor.IsValid())
    {
        UE_LOG(LogCameraManagement, Log, TEXT("Attempting to load Menu Camera from Soft Ptr reference."));
        // Принудительно загружаем объект, на который указывает Soft Ptr.
        // Это необходимо, так как на момент вызова BeginPlay объект может быть еще не загружен.
        CameraToSet = MenuCameraActor.LoadSynchronous();
    }

    // Если камера не была задана в Blueprint, ищем первую попавшуюся на сцене как запасной вариант.
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
            StartMenuWidgetInstance->AddToViewport(10); // Высокий Z-order, чтобы быть поверх всего
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
    // Эта функция зарезервирована, но в настоящее время не используется.
    // Вращение камеры обрабатывается в HandleCameraMove.
}

void AChessPlayerController::HandleCameraMove(const FInputActionValue& Value)
{
    // Не обрабатываем ввод для камеры, если мы не в игровом режиме (например, в меню)
    if (!bIsInputModeSetForGame) return;

    // Вращаем камеру, только если зажата правая кнопка мыши
    if (IsInputKeyDown(EKeys::RightMouseButton))
    {
        const FVector2D LookAxisVector = Value.Get<FVector2D>();

        // Добавляем ввод для тангажа (Pitch) напрямую в ControlRotation.
        // Рыскание (Yaw) синхронизируется с камерой в функции Tick.
        // Инвертируем ось Y, так как движение мыши вверх обычно соответствует отрицательному значению.
        AddPitchInput(LookAxisVector.Y * -1.0f);
    
        if (AChessPlayerCameraManager* CamManager = Cast<AChessPlayerCameraManager>(PlayerCameraManager))
        {
            // Эта функция вращает саму камеру.
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
    bHasGameStarted_Client = true; // Устанавливаем флаг, что игра началась на клиенте
    SetupGameUI();

    // Устанавливаем игровую камеру и ее перспективу здесь.
    // Этот RPC вызывается с задержкой из GameMode, что дает время для репликации
    // PlayerColor. Это предотвращает "дрожание" камеры при старте за черных.
    // OnRep_PlayerColor исправит перспективу, если цвет придет с опозданием.
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
    // Эта функция вызывается на клиенте, когда свойство PlayerColor реплицируется с сервера.
    FString MyColorStr = (PlayerColor == EPieceColor::White) ? TEXT("White") : TEXT("Black");
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("The server has assigned you the color: %s"), *MyColorStr));
    }
    
    // Переключаем камеру на правильную перспективу, ТОЛЬКО ЕСЛИ игра уже началась (т.е. Client_GameStarted был вызван).
    // Это предотвращает переключение камеры, пока мы находимся в главном меню, и исправляет перспективу,
    // если Client_GameStarted был вызван до репликации цвета.
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
    // Если виджет лобби уже отображается, не нужно ничего делать.
    // Это предотвращает повторное открытие главного меню поверх лобби.
    if (LobbyWidgetInstance && LobbyWidgetInstance->IsInViewport())
    {
        return;
    }

    AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
    if (!GameState)
    {
        // Если GameState все еще недействителен, это серьезная проблема.
        UE_LOG(LogTemp, Fatal, TEXT("AChessPlayerController::DetermineInitialUI: AChessGameState is NULL! Check GameMode Override in World Settings."));
        return;
    }

    if (GameState->GetGamePhase() == EGamePhase::WaitingToStart)
    {
        // Мы находимся в главном меню или на экране ожидания
        ShowStartMenu();
    }
    else
    {
        // Игра уже идет, настраиваем игровой интерфейс
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
    // Камера теперь устанавливается в Client_GameStarted, чтобы гарантировать,
    // что цвет игрока уже реплицирован.
}

void AChessPlayerController::SetPlayerColor(EPieceColor NewColor)
{
    // Эта функция должна вызываться только на сервере (в GameMode).
    if (HasAuthority())
    {
        PlayerColor = NewColor;

        // OnRep-функции не вызываются на сервере, поэтому мы вызываем ее вручную
        // для локального контроллера сервера (хоста в listen-server игре).
        // Мы не должны вызывать ее для прокси-контроллера клиента на сервере.
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
    // --- 1. Предварительные проверки состояния игры ---
    AChessGameState* GameState = GetWorld()->GetGameState<AChessGameState>();
    if (!GameState)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnClickStarted ABORTED: GameState is NULL."));
        return;
    }

    if (GameState->GetCurrentTurnColor() != PlayerColor)
    {
        UE_LOG(LogTemp, Log, TEXT("OnClickStarted ABORTED: Not player's turn."));
        return;
    }

    const EGamePhase CurrentPhase = GameState->GetGamePhase();
    if (CurrentPhase == EGamePhase::AwaitingPromotion)
    {
        UE_LOG(LogTemp, Log, TEXT("OnClickStarted ABORTED: Awaiting promotion."));
        return;
    }
    if (CurrentPhase != EGamePhase::InProgress && CurrentPhase != EGamePhase::Check)
    {
        UE_LOG(LogTemp, Log, TEXT("OnClickStarted ABORTED: Cannot move in current game phase: %s"), *UEnum::GetValueAsString(CurrentPhase));
        return;
    }

    if (!ChessBoard)
    {
        UE_LOG(LogTemp, Error, TEXT("OnClickStarted ABORTED: ChessBoard reference is NULL."));
        return;
    }

    // --- 2. ВОЗВРАЩЕНИЕ К КЛАССИЧЕСКОМУ МЕТОДУ: Определение клетки по курсору мыши ---
    FHitResult HitResult;
    // Используем канал ECC_Visibility, так как меши фигур и доска блокируют его.
    if (!GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
    {
        // Кликнули в пустое место, не на доску и не на фигуру
        UE_LOG(LogTemp, Log, TEXT("OnClickStarted: Clicked on empty space. Clearing selection."));
        ClearSelectionAndHighlights();
        return;
    }

    // --- 3. Основная логика выбора и хода, основанная на состоянии ---
    const FIntPoint HitGridPosition = ChessBoard->WorldToGridPosition(HitResult.Location);
    if (!ChessBoard->IsValidGridPosition(HitGridPosition))
    {
        UE_LOG(LogTemp, Log, TEXT("OnClickStarted: Clicked outside of valid board grid. Clearing selection."));
        ClearSelectionAndHighlights();
        return;
    }

    // Для определения фигуры используем GameState, так как это авторитетный источник.
    AChessPiece* PieceOnSquare = GameState->GetPieceAtGridPosition(HitGridPosition);
    UE_LOG(LogTemp, Log, TEXT("OnClickStarted: Clicked on grid (%d, %d). Piece on square: %s"), HitGridPosition.X, HitGridPosition.Y, *GetNameSafe(PieceOnSquare));

    if (PieceOnSquare) // Кликнули на клетку, где стоит фигура
    {
        if (PieceOnSquare->GetPieceColor() == PlayerColor) // Это наша фигура
        {
            // Логика выбора/перевыбора/отмены выбора обрабатывается в этой функции
            HandlePieceSelection(PieceOnSquare);
        }
        else // Это фигура противника
        {
            if (SelectedPiece) // Если у нас уже выбрана фигура, это попытка взятия
            {
                HandleBoardClick(HitGridPosition);
            }
            else // Если ничего не выбрано, клик по врагу ничего не делает
            {
                UE_LOG(LogTemp, Log, TEXT("OnClickStarted: Clicked enemy piece with no selection. No action."));
            }
        }
    }
    else // Кликнули на пустую клетку
    {
        if (SelectedPiece) // Если у нас выбрана фигура, это попытка хода
        {
            HandleBoardClick(HitGridPosition);
        }
        else // Если ничего не выбрано, клик по пустой клетке ничего не делает, но сбрасывает подсветку
        {
            UE_LOG(LogTemp, Log, TEXT("OnClickStarted: Clicked empty square with no selection. Clearing highlights."));
            ClearSelectionAndHighlights();
        }
    }
}


bool AChessPlayerController::Server_AttemptMove_Validate(AChessPiece* PieceToMove, const FIntPoint& TargetGridPosition)
{
    // Простая валидация, чтобы предотвратить отправку некорректных данных от клиента.
    return PieceToMove != nullptr;
}

void AChessPlayerController::Server_AttemptMove_Implementation(AChessPiece* PieceToMove, const FIntPoint& TargetGridPosition)
{
    AChessGameMode* GameMode = GetChessGameMode();
    if (GameMode)
    {
        // GameMode->AttemptMove выполнит ход, если он валиден.
        // Если ход невалиден, он просто вернет false, и положение фигуры на сервере не изменится.
        // Репликация положения актора позаботится о том, чтобы на клиенте фигура вернулась на место.
        GameMode->AttemptMove(PieceToMove, TargetGridPosition, this);
    }
}

void AChessPlayerController::Server_RequestStartGame_Implementation()
{
    // Только хост может запустить игру
    if (IsHost())
    {
        if (AChessGameState* GS = GetWorld()->GetGameState<AChessGameState>())
        {
            // Проверяем, что в лобби есть два игрока
            if (GS->PlayerArray.Num() >= 2)
            {
                // Выходим из состояния лобби
                GS->SetIsInLobby(false);

                // Начинаем игру (это вызовет Client_GameStarted на всех клиентах)
                if (AChessGameMode* GM = GetChessGameMode())
                {
                    GM->StartNewGame();
                }
            }
            else
            {
                // Недостаточно игроков
                UE_LOG(LogTemp, Warning, TEXT("Cannot start game: Not enough players in the lobby."));
            }
        }
    }
}


void AChessPlayerController::Server_SetPlayerProfile_Implementation(const FPlayerProfile& Profile)
{
    // Вызывается на сервере, когда клиент отправляет информацию о своем профиле.
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

    // Если мы кликаем на уже выделенную фигуру, снимаем выделение
    if (SelectedPiece == PieceToSelect)
    {
        ClearSelectionAndHighlights();
        return;
    }

    // Если была выбрана другая фигура, сначала очищаем старое выделение
    if (SelectedPiece)
    {
        ClearSelectionAndHighlights();
    }

    SelectedPiece = PieceToSelect;
    SelectedPiece->OnSelected();

    // Вычисляем и отображаем валидные ходы локально на клиенте для мгновенной обратной связи.
    // Сервер все равно проверит ход при его совершении.
    AChessGameState* GameState = GetWorld()->GetGameState<AChessGameState>();
    if (GameState)
    {
        // Сначала получаем все псевдо-легальные ходы для этой фигуры
        const TArray<FIntPoint> PseudoLegalMoves = SelectedPiece->GetValidMoves(GameState, ChessBoard);
        
        LastValidMoves.Empty(); // Очищаем старый список

        // Фильтруем ходы, чтобы оставить только те, которые не оставляют короля под шахом
        for (const FIntPoint& Move : PseudoLegalMoves)
        {
            // IsMoveLegal симулирует ход и проверяет, не находится ли король под шахом.
            if (GameState->IsMoveLegal(SelectedPiece, Move, ChessBoard))
            {
                LastValidMoves.Add(Move);
            }
        }

        // Подсвечиваем саму выбранную фигуру
        ChessBoard->HighlightSquare(SelectedPiece->GetBoardPosition(), SelectedPieceHighlightColor);
        // Подсвечиваем все валидные ходы
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
                    // Отключаем коллизию, чтобы индикаторы не мешали кликам
                    IndicatorComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    // Регистрируем компонент, чтобы он появился в мире
                    IndicatorComponent->RegisterComponent();
                    // Прикрепляем к доске, чтобы он был частью ее иерархии
                    IndicatorComponent->AttachToComponent(ChessBoard->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
                    // Устанавливаем положение в центре клетки
                    IndicatorComponent->SetWorldLocation(ChessBoard->GridToWorldPosition(Move));
                    
                    ValidMoveIndicatorComponents.Add(IndicatorComponent);
                }
            }
            else
            {
                // Если меш не задан, используем старый метод подсветки цветом
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

        // Снимаем выделение только ПОСЛЕ того, как отправили валидный ход на сервер.
        // Это обеспечивает правильную обратную связь для игрока.
        ClearSelectionAndHighlights();
    }
    // Если игрок кликнул на невалидную клетку, мы больше не будем снимать выделение.
    // Это позволит ему выбрать другую клетку без необходимости заново выбирать фигуру.
}

void AChessPlayerController::ClearSelectionAndHighlights()
{
    // Уничтожаем и очищаем все индикаторы ходов
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
        // Эта функция теперь будет убирать только подсветку выбранной фигуры
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
                // Привязываем обработчик к событию выбора
                PromotionMenuWidgetInstance->OnPromotionPieceSelected.AddDynamic(this, &AChessPlayerController::HandlePromotionSelection);
            }
        }

        if (PromotionMenuWidgetInstance && !PromotionMenuWidgetInstance->IsInViewport())
        {
            PawnAwaitingPromotion = PawnForPromotion; // Сохраняем пешку для отправки на сервер
            PromotionMenuWidgetInstance->AddToViewport(10); // Высокий Z-order, чтобы быть поверх всего
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
    
    // Скрываем меню выбора и обновляем режим ввода
    if (PromotionMenuWidgetInstance)
    {
        PromotionMenuWidgetInstance->RemoveFromParent();
    }
    UpdateInputMode();
    PawnAwaitingPromotion = nullptr;
}


bool AChessPlayerController::Server_CompletePawnPromotion_Validate(APawnPiece* PawnToPromote, EPieceType PromoteToType)
{
    // Простая валидация: пешка должна существовать, а тип фигуры быть допустимым для превращения.
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
        // Воспроизводим звук локально для этого игрока
        UGameplayStatics::PlaySound2D(this, SoundToPlay);
    }
}

void AChessPlayerController::Client_PlayCaptureEffect_Implementation(AChessPiece* CapturedPiece, const FVector& Location, const FVector& Scale, const FVector& CellBoundingBox, float Lifetime, float Density)
{
    // 1. Показываем дым
    if (CaptureEffect)
    {
        // Спавним систему Niagara, применяя указанный масштаб.
        UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            CaptureEffect,
            Location,
            FRotator::ZeroRotator,
            Scale, // Устанавливаем общий масштаб компонента.
            false, // bAutoDestroy - управляем временем жизни вручную
            true   // bAutoActivate
        );

        if (NiagaraComponent)
        {
            // Устанавливаем дополнительные параметры, если они используются в эффекте Niagara.
            NiagaraComponent->SetVariableFloat(TEXT("User.Density"), Density);
            
            // Если передан валидный размер ограничивающего объема, устанавливаем его.
            // Это требует, чтобы сам эффект Niagara был настроен на использование этого параметра.
            if (!CellBoundingBox.IsZero())
            {
                NiagaraComponent->SetVariableVec3(TEXT("User.CellBoundingBox"), CellBoundingBox);
                UE_LOG(LogTemp, Log, TEXT("Spawned CaptureEffect with Scale=%s, Density=%f, and CellBoundingBox=%s"), *Scale.ToString(), Density, *CellBoundingBox.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("Spawned CaptureEffect with Scale=%s and Density=%f"), *Scale.ToString(), Density);
            }

            // Устанавливаем таймер на удаление дыма.
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

    // 2. Сразу же скрываем взятую фигуру
    if (CapturedPiece)
    {
        CapturedPiece->SetActorHiddenInGame(true);
    }
}
