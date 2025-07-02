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
    ShowStartMenu();
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

#include "ChessGameState.h"

void AChessPlayerController::OnClickStarted()
{
    AChessGameState* GameState = GetWorld()->GetGameState<AChessGameState>();
    if (!GameState || GameState->GetCurrentTurnColor() != PlayerColor)
    {
        return;
    }

    FHitResult HitResult;
    GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

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

            SetInputMode(FInputModeGameAndUI());
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
        GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

        AChessGameMode* GameMode = GetChessGameMode();

        if (ChessBoard && GameMode && HitResult.bBlockingHit)
        {
            FIntPoint TargetSquare = ChessBoard->WorldToGridPosition(HitResult.Location);

            if (!GameMode->AttemptMove(SelectedPiece, TargetSquare, this))
            {
                // Если ход не удался, возвращаем фигуру на место
                SelectedPiece->SetActorLocation(OriginalPieceLocation);
            }
        }
        else
        {
            // Если клик был не на доске, возвращаем фигуру на место
            SelectedPiece->SetActorLocation(OriginalPieceLocation);
        }

        SelectedPiece = nullptr;

        if (StartMenuWidgetInstance && !StartMenuWidgetInstance->IsVisible())
        {
            SetInputMode(FInputModeGameOnly());
        }
    }
}
