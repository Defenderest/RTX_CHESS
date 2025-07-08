#include "ChessPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "ChessPiece.h"
#include "ChessBoard.h"
#include "ChessGameMode.h"
#include "StockfishManager.h"
#include "GameCameraActor.h"
#include "MenuCameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "StartMenuWidget.h"
#include "PromotionMenuWidget.h"
#include "PawnPiece.h"
#include "Blueprint/UserWidget.h"
#include "Components/AudioComponent.h"
#include "ChessGameState.h"
#include "Engine/Engine.h"
#include "ChessPlayerCameraManager.h"
#include "TimerManager.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "GameFramework/PlayerState.h"

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
    MenuMusicComponent = nullptr;
    MenuCameraActor = nullptr;

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
        SetGameCamera();
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

void AChessPlayerController::SetGameCamera()
{
    AGameCameraActor* GameCamera = Cast<AGameCameraActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AGameCameraActor::StaticClass()));
    if (GameCamera)
    {
        SetViewTargetWithBlend(GameCamera, 0.5f);
    }
}

void AChessPlayerController::SetMenuCamera()
{
    // Сначала пытаемся использовать камеру, указанную в свойстве Blueprint.
    AMenuCameraActor* CameraToSet = MenuCameraActor;

    // Если она не задана, ищем камеру на сцене, как и раньше.
    if (!CameraToSet)
    {
        CameraToSet = Cast<AMenuCameraActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AMenuCameraActor::StaticClass()));
    }

    if (CameraToSet)
    {
        SetViewTargetWithBlend(CameraToSet, 0.5f);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessPlayerController::SetMenuCamera: MenuCameraActor not found either in properties or in the world! Falling back to game camera."));
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
            StartMenuWidgetInstance->AddToViewport();
            SetInputModeForUI();
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

void AChessPlayerController::Client_GameStarted_Implementation()
{
    if (StartMenuWidgetInstance && StartMenuWidgetInstance->IsInViewport())
    {
        StartMenuWidgetInstance->RemoveFromParent();
        StartMenuWidgetInstance = nullptr; // Очищаем указатель
    }

    if (MenuMusicComponent && MenuMusicComponent->IsPlaying())
    {
        MenuMusicComponent->Stop();
    }
    MenuMusicComponent = nullptr;

    SetInputModeForGame();
    SetGameCamera();
    
    // После установки основной игровой камеры, переключаемся на перспективу игрока
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
    
    // Переключаем камеру на правильную перспективу, ТОЛЬКО ЕСЛИ ИГРА УЖЕ ИДЕТ.
    // В главном меню у нас своя камера. Если игра еще не началась, перспектива
    // будет установлена при вызове Client_GameStarted.
    AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
    if (GameState && GameState->GetGamePhase() != EGamePhase::WaitingToStart)
    {
        if (AChessPlayerCameraManager* CamManager = Cast<AChessPlayerCameraManager>(PlayerCameraManager))
        {
            CamManager->SwitchToPlayerPerspective(PlayerColor);
        }
    }
}

void AChessPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AChessPlayerController, PlayerColor);
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

    // --- 2. ВОССТАНОВЛЕННЫЙ МЕТОД: Математическое определение клетки на доске ---
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

bool AChessPlayerController::Server_RequestValidMoves_Validate(AChessPiece* ForPiece)
{
    return ForPiece != nullptr;
}

void AChessPlayerController::Server_RequestValidMoves_Implementation(AChessPiece* ForPiece)
{
    if (!ForPiece) return;

    // We must get the GameState and Board from the world on the server to ensure we use the authoritative versions.
    AChessGameState* GameState = GetWorld()->GetGameState<AChessGameState>();
    AChessBoard* Board = Cast<AChessBoard>(UGameplayStatics::GetActorOfClass(GetWorld(), AChessBoard::StaticClass()));

    if (GameState && Board)
    {
        TArray<FIntPoint> ValidMoves = ForPiece->GetValidMoves(GameState, Board);
        Client_ReceiveValidMoves(ValidMoves);
    }
}

void AChessPlayerController::Client_ReceiveValidMoves_Implementation(const TArray<FIntPoint>& Moves)
{
    // It's possible the player deselected the piece while waiting for the server's response.
    if (!SelectedPiece || !ChessBoard)
    {
        return;
    }

    LastValidMoves = Moves;

    if (LastValidMoves.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Client_ReceiveValidMoves: Server returned 0 valid moves for piece %s."), *GetNameSafe(SelectedPiece));
    }
    
    for (const FIntPoint& Move : LastValidMoves)
    {
        ChessBoard->HighlightSquare(Move, ValidMoveHighlightColor);
    }
    
    // Also highlight the piece that these moves are for.
    ChessBoard->HighlightSquare(SelectedPiece->GetBoardPosition(), SelectedPieceHighlightColor);
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

    // Запрашиваем валидные ходы с сервера вместо того, чтобы считать их на клиенте.
    // Это предотвращает проблемы с рассинхронизацией состояния.
    Server_RequestValidMoves(SelectedPiece);
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
    if (ChessBoard)
    {
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
