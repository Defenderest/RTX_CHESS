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
#include "Blueprint/UserWidget.h"
#include "ChessGameState.h"
#include "Engine/Engine.h"

AChessPlayerController::AChessPlayerController()
{
    bAutoManageActiveCameraTarget = false;
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    PlayerColor = EPieceColor::White;
    SelectedPiece = nullptr;
    ChessBoard = nullptr;
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
    }

    SetCamera();

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
        SetInputMode(FInputModeGameOnly());
        bShowMouseCursor = true;
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

        if (LookAction)
        {
            EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AChessPlayerController::HandleLook);
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
                FString SFStatus = SFManager->IsEngineRunning() ? TEXT("Running") : TEXT("Stopped");
                FString SFBestMove = SFManager->GetLastBestMove();
                int32 SFSearchTime = SFManager->GetSearchTimeMsec();

                GEngine->AddOnScreenDebugMessage(4, 0.f, FColor::Orange, FString::Printf(TEXT("Stockfish Status: %s"), *SFStatus));
                GEngine->AddOnScreenDebugMessage(5, 0.f, FColor::Orange, FString::Printf(TEXT("Stockfish Last Best Move: %s"), *SFBestMove));
                GEngine->AddOnScreenDebugMessage(6, 0.f, FColor::Orange, FString::Printf(TEXT("Stockfish Search Time (ms): %d"), SFSearchTime));
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
            bShowMouseCursor = true;
            SetInputMode(FInputModeUIOnly());
        }
    }
}

void AChessPlayerController::HandleLook(const FInputActionValue& Value)
{
    const FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (LookAxisVector.Y != 0.f)
    {
        AddPitchInput(LookAxisVector.Y);
    }

    if (LookAxisVector.X != 0.f)
    {
        AddYawInput(LookAxisVector.X);
    }
}

AChessGameMode* AChessPlayerController::GetChessGameMode() const
{
    return Cast<AChessGameMode>(GetWorld()->GetAuthGameMode());
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
    AChessGameState* GameState = GetWorld()->GetGameState<AChessGameState>();
    if (!GameState)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnClickStarted: GameState is NULL."));
        return;
    }

    // Проверяем, ход текущего игрока
    if (GameState->GetCurrentTurnColor() != PlayerColor)
    {
        UE_LOG(LogTemp, Log, TEXT("OnClickStarted: Not player's turn."));
        return; // Не наш ход
    }

    // Ходить можно только в фазах InProgress и Check
    const EGamePhase CurrentPhase = GameState->GetGamePhase();
    if (CurrentPhase != EGamePhase::InProgress && CurrentPhase != EGamePhase::Check)
    {
        UE_LOG(LogTemp, Log, TEXT("OnClickStarted: Cannot move in current game phase: %s"), *UEnum::GetValueAsString(CurrentPhase));
        return; // Игра неактивна
    }

    if (!ChessBoard)
    {
        UE_LOG(LogTemp, Error, TEXT("OnClickStarted: ChessBoard is not valid."));
        return;
    }

    FHitResult HitResult;
    GetHitResultUnderCursor(ECC_Visibility, true, HitResult);

    if (!HitResult.bBlockingHit)
    {
        UE_LOG(LogTemp, Log, TEXT("OnClickStarted: Clicked into empty space. Clearing selection."));
        ClearSelectionAndHighlights(); // Клик в пустоту
        return;
    }

    AActor* HitActor = HitResult.GetActor();
    UE_LOG(LogTemp, Log, TEXT("OnClickStarted: Clicked on actor: %s"), *GetNameSafe(HitActor));

    AChessPiece* ClickedPiece = Cast<AChessPiece>(HitActor);
    if (ClickedPiece)
    {
        // --- Логика клика по фигуре ---
        if (ClickedPiece->GetPieceColor() == PlayerColor)
        {
            // Кликнули на свою фигуру (выбрать/перевыбрать)
            UE_LOG(LogTemp, Log, TEXT("OnClickStarted: Handling selection of friendly piece %s."), *ClickedPiece->GetName());
            HandlePieceSelection(ClickedPiece);
        }
        else if (SelectedPiece)
        {
            // Кликнули на фигуру противника, когда наша фигура выбрана (атака)
            UE_LOG(LogTemp, Log, TEXT("OnClickStarted: Attempting to capture enemy piece %s with selected piece %s."), *ClickedPiece->GetName(), *SelectedPiece->GetName());
            HandleBoardClick(ClickedPiece->GetBoardPosition());
        }
        else
        {
            // Кликнули на фигуру противника, когда ничего не выбрано
            UE_LOG(LogTemp, Log, TEXT("OnClickStarted: Clicked enemy piece %s with no selection. Clearing."), *ClickedPiece->GetName());
            ClearSelectionAndHighlights();
        }
    }
    else if (Cast<AChessBoard>(HitActor))
    {
        // --- Логика клика по доске ---
        if (SelectedPiece)
        {
            // Фигура выбрана, и кликнули на доску (ход на пустую клетку)
            FIntPoint TargetGridPosition = ChessBoard->WorldToGridPosition(HitResult.Location);
            UE_LOG(LogTemp, Log, TEXT("OnClickStarted: Attempting to move selected piece %s to board position (%d, %d)."), *SelectedPiece->GetName(), TargetGridPosition.X, TargetGridPosition.Y);
            HandleBoardClick(TargetGridPosition);
        }
        else
        {
            // Фигура не выбрана, и кликнули на доску
            UE_LOG(LogTemp, Log, TEXT("OnClickStarted: Clicked on board with no selection. Clearing."));
            ClearSelectionAndHighlights();
        }
    }
    else
    {
        // Кликнули на что-то другое
        UE_LOG(LogTemp, Log, TEXT("OnClickStarted: Clicked on an unhandled actor. Clearing selection."));
        ClearSelectionAndHighlights();
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
    TArray<FIntPoint> ValidMoves = SelectedPiece->GetValidMoves(ChessBoard);
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

    TArray<FIntPoint> ValidMoves = SelectedPiece->GetValidMoves(ChessBoard);
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
