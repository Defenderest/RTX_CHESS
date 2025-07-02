#include "ChessPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "ChessPiece.h"
#include "ChessBoard.h"
#include "ChessGameMode.h"
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
            EnhancedInput->BindAction(ClickAction, ETriggerEvent::Completed, this, &AChessPlayerController::OnClickCompleted);
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
    }
    // --- Конец отладочной информации ---

    if (SelectedPiece)
    {
        FVector MouseLocation, MouseDirection;
        if (DeprojectMousePositionToWorld(MouseLocation, MouseDirection))
        {
            FHitResult HitResult;
            FCollisionQueryParams QueryParams;
            QueryParams.AddIgnoredActor(SelectedPiece);

            if (GetWorld()->LineTraceSingleByChannel(HitResult, MouseLocation, MouseLocation + MouseDirection * 10000.f, ECC_WorldStatic))
            {
                FVector TargetLocation = HitResult.Location;
                TargetLocation.Z = OriginalPieceLocation.Z + 50.0f;
                SelectedPiece->SetActorLocation(TargetLocation);
            }
        }
    }
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
    if (!GameState || GameState->GetCurrentTurnColor() != PlayerColor)
    {
        return;
    }

    FHitResult HitResult;
    // Используем комплексную трассировку (true), так как у мешей фигур может не быть простой коллизии.
    // Это гарантирует, что клик будет зарегистрирован по видимой геометрии меша.
    GetHitResultUnderCursor(ECC_Visibility, true, HitResult);

    if (HitResult.bBlockingHit)
    {
        AChessPiece* HitPiece = Cast<AChessPiece>(HitResult.GetActor());
        AChessBoard* ChessBoard = Cast<AChessBoard>(UGameplayStatics::GetActorOfClass(this, AChessBoard::StaticClass()));

        if (HitPiece && ChessBoard && HitPiece->GetPieceColor() == PlayerColor)
        {
            SelectedPiece = HitPiece;
            OriginalPieceLocation = SelectedPiece->GetActorLocation();
            SelectedPiece->OnSelected(); // Подсветка самой фигуры

            // Подсветка допустимых ходов
            TArray<FIntPoint> ValidMoves = SelectedPiece->GetValidMoves(ChessBoard);
            for (const FIntPoint& Move : ValidMoves)
            {
                ChessBoard->HighlightSquare(Move, FLinearColor::Green);
            }
            // Подсветка текущей клетки
            ChessBoard->HighlightSquare(SelectedPiece->GetBoardPosition(), FLinearColor::Blue);
        }
    }
}

void AChessPlayerController::OnClickCompleted()
{
    AChessBoard* ChessBoard = Cast<AChessBoard>(UGameplayStatics::GetActorOfClass(this, AChessBoard::StaticClass()));
    if (ChessBoard)
    {
        // Убираем всю подсветку с доски в любом случае
        ChessBoard->ClearAllHighlights();
    }

    if (SelectedPiece)
    {
        SelectedPiece->OnDeselected();

        FHitResult HitResult;
        // Используем комплексную трассировку (true) для согласованности с OnClickStarted.
        GetHitResultUnderCursor(ECC_Visibility, true, HitResult);

        // Проверяем, был ли клик на доске или на другой фигуре
        if (ChessBoard && HitResult.bBlockingHit && (HitResult.GetActor() == ChessBoard || HitResult.GetActor()->IsA<AChessPiece>()))
        {
            FIntPoint TargetSquare = ChessBoard->WorldToGridPosition(HitResult.Location);
            
            // Вместо прямого вызова GameMode, отправляем RPC на сервер
            Server_AttemptMove(SelectedPiece, TargetSquare);
            // Клиент не должен сам возвращать фигуру. Сервер решит ее судьбу,
            // и правильная позиция придет через репликацию.
        }
        else
        {
            // Если клик был не на доске, возвращаем фигуру на место локально для мгновенной обратной связи.
            SelectedPiece->SetActorLocation(OriginalPieceLocation);
        }

        SelectedPiece = nullptr;
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
