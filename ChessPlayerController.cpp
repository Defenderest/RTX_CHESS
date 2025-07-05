#include "ChessPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "ChessPiece.h"
#include "ChessBoard.h"
#include "ChessGameMode.h"
#include "StockfishManager.h"
#include "GameCameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "StartMenuWidget.h"
#include "PromotionMenuWidget.h"
#include "PawnPiece.h"
#include "Blueprint/UserWidget.h"
#include "ChessGameState.h"
#include "Engine/Engine.h"
#include "ChessPlayerCameraManager.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"

AChessPlayerController::AChessPlayerController()
{
    bAutoManageActiveCameraTarget = false;
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    PlayerColor = EPieceColor::White;
    SelectedPiece = nullptr;
    ChessBoard = nullptr;
    bIsInputModeSetForGame = false;
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

    SetCamera();

    if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
    {
        SessionInterface = Subsystem->GetSessionInterface();
        if (SessionInterface.IsValid())
        {
            OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &AChessPlayerController::OnCreateSessionComplete);
            OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &AChessPlayerController::OnFindSessionsComplete);
            OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &AChessPlayerController::OnJoinSessionComplete);
        }
    }

    ChessBoard = Cast<AChessBoard>(UGameplayStatics::GetActorOfClass(GetWorld(), AChessBoard::StaticClass()));
    if (!ChessBoard)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessPlayerController::BeginPlay: ChessBoard actor not found!"));
    }
    
    // Показываем меню только если игра еще не началась.
    // GameMode отвечает за запуск игры (и смену состояния) при перезагрузке уровня с опциями.
    AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
    if (!GameState)
    {
        UE_LOG(LogTemp, Fatal, TEXT("AChessPlayerController::BeginPlay: AChessGameState is NULL! Check GameMode Override in World Settings. It must be set to AChessGameMode or a Blueprint based on it."));
        return;
    }

    if (GameState->GetGamePhase() == EGamePhase::WaitingToStart)
    {
        ShowStartMenu();
    }
    else
    {
        // Если игра уже идет, убеждаемся, что ввод настроен для игры.
        SetInputModeForGame();
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
    }
}

void AChessPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // --- Отладочная информация на экране ---
    if (GEngine)
    {
        AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
        if (GameState)
        {
            FString GamePhaseStr = UEnum::GetValueAsString(GameState->GetGamePhase());
            FString CurrentTurnStr = (GameState->GetCurrentTurnColor() == EPieceColor::White) ? TEXT("White") : TEXT("Black");
            
            GEngine->AddOnScreenDebugMessage(0, 0.f, FColor::Yellow, FString::Printf(TEXT("Game Phase: %s"), *GamePhaseStr));
            GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Yellow, FString::Printf(TEXT("Current Turn: %s"), *CurrentTurnStr));
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(0, 0.f, FColor::Red, TEXT("Game State is NULL"));
        }

        FString MyColorStr = (PlayerColor == EPieceColor::White) ? TEXT("White") : TEXT("Black");
        GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Cyan, FString::Printf(TEXT("My Player Color: %s"), *MyColorStr));

        FString SelectedPieceStr = SelectedPiece ? GetNameSafe(SelectedPiece) : TEXT("None");
        GEngine->AddOnScreenDebugMessage(3, 0.f, FColor::Green, FString::Printf(TEXT("Selected Piece: %s"), *SelectedPieceStr));

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
    // --- Конец отладочной информации ---
}

void AChessPlayerController::SetCamera()
{
    AGameCameraActor* GameCamera = Cast<AGameCameraActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameCameraActor::StaticClass()));
    if (GameCamera)
    {
        SetViewTargetWithBlend(GameCamera, 0.5f);
    }
}

void AChessPlayerController::ShowStartMenu()
{
    if (StartMenuWidgetClass)
    {
        if (!StartMenuWidgetInstance)
        {
            StartMenuWidgetInstance = CreateWidget<UStartMenuWidget>(this, StartMenuWidgetClass);
        }

        if (StartMenuWidgetInstance)
        {
            StartMenuWidgetInstance->AddToViewport();
            SetInputModeForUI();
        }
    }
}

void AChessPlayerController::HandleLook(const FInputActionValue& Value)
{
    // Вращаем камеру, только если зажата правая кнопка мыши
    if (IsInputKeyDown(EKeys::RightMouseButton))
    {
        const FVector2D LookAxisVector = Value.Get<FVector2D>();
    
        // Получаем Camera Manager и вызываем его функцию вращения
        if (AChessPlayerCameraManager* CamManager = Cast<AChessPlayerCameraManager>(PlayerCameraManager))
        {
            CamManager->AddCameraRotationInput(LookAxisVector);
        }
    }
}

void AChessPlayerController::HandleCameraMove(const FInputActionValue& Value)
{
    // Вращаем камеру, только если зажата правая кнопка мыши
    if (IsInputKeyDown(EKeys::RightMouseButton))
    {
        const FVector2D MoveVector = Value.Get<FVector2D>();
    
        // Получаем Camera Manager и вызываем его функцию вращения
        if (AChessPlayerCameraManager* CamManager = Cast<AChessPlayerCameraManager>(PlayerCameraManager))
        {
            CamManager->AddCameraRotationInput(MoveVector);
        }
    }
}

AChessGameMode* AChessPlayerController::GetChessGameMode() const
{
    return Cast<AChessGameMode>(GetWorld()->GetAuthGameMode());
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

void AChessPlayerController::SetPlayerColor(EPieceColor NewColor)
{
    PlayerColor = NewColor;
}

EPieceColor AChessPlayerController::GetPlayerColor() const
{
    return PlayerColor;
}

void AChessPlayerController::OnClickStarted()
{
    UE_LOG(LogTemp, Warning, TEXT("<<<<< OnClickStarted FIRED! Left Mouse Button Click Detected. >>>>>"));

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
        // Игрок должен сначала выбрать фигуру в меню, игнорируем клики по доске.
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

    // --- 2. НОВЫЙ МЕТОД: Математическое определение клетки на доске ---
    // Этот метод не зависит от коллизии доски или фигур, что делает его более надежным.
    FVector WorldLocation, WorldDirection;
    if (!DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
    {
        UE_LOG(LogTemp, Warning, TEXT("OnClickStarted: Failed to deproject mouse position."));
        ClearSelectionAndHighlights();
        return;
    }

    // Определяем плоскость доски. Нормаль - это "верх" актора доски.
    const FPlane BoardPlane(ChessBoard->GetActorLocation(), ChessBoard->GetActorUpVector());

    // Находим точку пересечения луча от камеры с плоскостью доски
    const FVector IntersectionPoint = FMath::LinePlaneIntersection(
        WorldLocation,
        WorldLocation + WorldDirection * 10000.f, // Луч достаточной длины
        BoardPlane
    );

    // --- 3. Основная логика выбора и хода, основанная на состоянии ---
    const FIntPoint HitGridPosition = ChessBoard->WorldToGridPosition(IntersectionPoint);
    if (!ChessBoard->IsValidGridPosition(HitGridPosition))
    {
        UE_LOG(LogTemp, Log, TEXT("OnClickStarted: Clicked outside of valid board grid. Clearing selection."));
        ClearSelectionAndHighlights();
        return;
    }

    AChessPiece* PieceOnSquare = ChessBoard->GetPieceAtGridPosition(HitGridPosition);
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
        else // Если ничего не выбрано, клик по пустой клетке ничего не делает
        {
            UE_LOG(LogTemp, Log, TEXT("OnClickStarted: Clicked empty square with no selection. No action."));
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

    // Подсветка допустимых ходов
    AChessGameState* GameState = GetWorld()->GetGameState<AChessGameState>();
    TArray<FIntPoint> ValidMoves = SelectedPiece->GetValidMoves(GameState, ChessBoard);
    for (const FIntPoint& Move : ValidMoves)
    {
        ChessBoard->HighlightSquare(Move, FLinearColor::Green);
    }
    // Подсветка текущей клетки
    ChessBoard->HighlightSquare(SelectedPiece->GetBoardPosition(), FLinearColor::Blue);
}

void AChessPlayerController::HandleBoardClick(const FIntPoint& GridPosition)
{
    if (!SelectedPiece || !ChessBoard)
    {
        return;
    }

    AChessGameState* GameState = GetWorld()->GetGameState<AChessGameState>();
    TArray<FIntPoint> ValidMoves = SelectedPiece->GetValidMoves(GameState, ChessBoard);
    if (ValidMoves.Contains(GridPosition))
    {
        Server_AttemptMove(SelectedPiece, GridPosition);
    }
    
    // В любом случае (даже если ход невалиден) снимаем выделение после попытки хода
    ClearSelectionAndHighlights();
}

void AChessPlayerController::ClearSelectionAndHighlights()
{
    if (ChessBoard)
    {
        ChessBoard->ClearAllHighlights();
    }
    if (SelectedPiece)
    {
        SelectedPiece->OnDeselected();
        SelectedPiece = nullptr;
    }
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
            PromotionMenuWidgetInstance->AddToViewport();
            SetInputModeForUI();
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
    SetInputModeForGame(); // Возвращаем управление в игру
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

void AChessPlayerController::HostSession(const FString& SessionName)
{
    if (!SessionInterface.IsValid() || SessionName.IsEmpty())
    {
        return;
    }

    // Использовать NAME_GameSession в качестве локального имени сессии - это нормально.
    auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
    if (ExistingSession != nullptr)
    {
        SessionInterface->DestroySession(NAME_GameSession);
    }

    SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);

    TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
    SessionSettings->bIsLANMatch = true;
    SessionSettings->NumPublicConnections = 2;
    SessionSettings->bShouldAdvertise = true;
    SessionSettings->bUsesPresence = false;
    SessionSettings->bAllowJoinInProgress = true;
    // Устанавливаем наше кастомное свойство с именем комнаты
    SessionSettings->Set(FName(TEXT("ROOM_NAME_KEY")), SessionName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);


    UE_LOG(LogTemp, Log, TEXT("Creating LAN session with name: %s"), *SessionName);
    SessionInterface->CreateSession(*GetLocalPlayer()->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings);
}

void AChessPlayerController::FindAndJoinSession(const FString& SessionName)
{
    if (SessionName.IsEmpty())
    {
        return;
    }
    FindSessions(SessionName);
}

void AChessPlayerController::FindSessions(const FString& SessionName)
{
    if (!SessionInterface.IsValid())
    {
        return;
    }
    
    SessionSearch = MakeShareable(new FOnlineSessionSearch());
    SessionSearch->bIsLanQuery = true;
    SessionSearch->MaxSearchResults = 10;
    // Ищем по нашему кастомному свойству с именем комнаты
    SessionSearch->QuerySettings.Set(FName(TEXT("ROOM_NAME_KEY")), SessionName, EOnlineComparisonOp::Equals);

    SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);

    UE_LOG(LogTemp, Log, TEXT("Finding LAN sessions with name: %s"), *SessionName);
    SessionInterface->FindSessions(*GetLocalPlayer()->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef());
}

void AChessPlayerController::JoinSession(const FOnlineSessionSearchResult& SearchResult)
{
    if (!SessionInterface.IsValid())
    {
        return;
    }
    SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
    UE_LOG(LogTemp, Log, TEXT("Joining session..."));
    SessionInterface->JoinSession(*GetLocalPlayer()->GetPreferredUniqueNetId(), NAME_GameSession, SearchResult);
}


void AChessPlayerController::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    if (bWasSuccessful)
    {
        UE_LOG(LogTemp, Log, TEXT("Session '%s' created successfully. Traveling to map as listen server..."), *SessionName.ToString());
        GetWorld()->ServerTravel(TEXT("/Game/Maps/Cigar_room?listen"), true);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create session."));
    }
}

void AChessPlayerController::OnFindSessionsComplete(bool bWasSuccessful)
{
    if (bWasSuccessful && SessionSearch.IsValid())
    {
        if (SessionSearch->SearchResults.Num() > 0)
        {
            UE_LOG(LogTemp, Log, TEXT("Found %d sessions. Joining the first one."), SessionSearch->SearchResults.Num());
            JoinSession(SessionSearch->SearchResults[0]);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Could not find any sessions to join."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Session search failed."));
    }
}

void AChessPlayerController::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if (Result == EOnJoinSessionCompleteResult::Success && SessionInterface.IsValid())
    {
        FString ConnectString;
        if (SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
        {
            UE_LOG(LogTemp, Log, TEXT("Successfully joined session '%s'. Traveling to: %s"), *SessionName.ToString(), *ConnectString);
            ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Could not get connect string for session '%s'."), *SessionName.ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to join session '%s'. Error: %d"), *SessionName.ToString(), static_cast<int32>(Result));
    }
}
