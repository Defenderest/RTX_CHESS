#include "ChessPlayerController.h"
#include "EnhancedInputSubsystems.h" // Для Enhanced Input
#include "EnhancedInputComponent.h"   // Для Enhanced Input
#include "ChessPiece.h"               // Для AChessPiece
#include "ChessBoard.h"               // Для AChessBoard
#include "ChessGameMode.h"            // Для AChessGameMode

AChessPlayerController::AChessPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

void AChessPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Настройка Enhanced Input
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (ChessMappingContext)
        {
            Subsystem->AddMappingContext(ChessMappingContext, 0);
            UE_LOG(LogTemp, Log, TEXT("AChessPlayerController: ChessMappingContext added."));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AChessPlayerController: ChessMappingContext is not set!"));
        }
    }
}

void AChessPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Привязка действий Enhanced Input
    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
    {
        if (SelectAction)
        {
            EnhancedInput->BindAction(SelectAction, ETriggerEvent::Started, this, &AChessPlayerController::HandleSelectAction);
            UE_LOG(LogTemp, Log, TEXT("AChessPlayerController: SelectAction bound."));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AChessPlayerController: SelectAction is not set!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AChessPlayerController: EnhancedInputComponent not found!"));
    }
}

void AChessPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

AChessGameMode* AChessPlayerController::GetChessGameMode() const
{
    return Cast<AChessGameMode>(GetWorld()->GetAuthGameMode());
}

void AChessPlayerController::HandleSelectAction()
{
    FHitResult HitResult;
    if (GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
    {
        AChessGameMode* GameMode = GetChessGameMode();
        if (!GameMode)
        {
            UE_LOG(LogTemp, Error, TEXT("AChessPlayerController::HandleSelectAction: GameMode is null."));
            return;
        }

        AActor* ClickedActor = HitResult.GetActor();
        if (ClickedActor)
        {
            AChessPiece* ClickedPiece = Cast<AChessPiece>(ClickedActor);
            if (ClickedPiece)
            {
                // Кликнули по фигуре
                UE_LOG(LogTemp, Log, TEXT("AChessPlayerController: Clicked on piece: %s at (%d, %d)"),
                       *ClickedPiece->GetName(), ClickedPiece->GetBoardPosition().X, ClickedPiece->GetBoardPosition().Y);
                GameMode->HandlePieceClicked(ClickedPiece, this);
            }
            else
            {
                AChessBoard* ClickedBoard = Cast<AChessBoard>(ClickedActor);
                if (ClickedBoard)
                {
                    // Кликнули по доске, определяем клетку
                    FIntPoint GridPosition = ClickedBoard->WorldToGridPosition(HitResult.Location);
                    UE_LOG(LogTemp, Log, TEXT("AChessPlayerController: Clicked on board at grid position: (%d, %d)"), GridPosition.X, GridPosition.Y);
                    GameMode->HandleSquareClicked(GridPosition, this);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("AChessPlayerController: Clicked on unknown actor: %s"), *ClickedActor->GetName());
                }
            }
        }
    }
    else
    {
        // Кликнули в пустое место, снимаем выделение, если есть
        AChessGameMode* GameMode = GetChessGameMode();
        if (GameMode)
        {
            // Передаем невалидную позицию, чтобы GameMode снял выделение
            GameMode->HandleSquareClicked(FIntPoint(-1, -1), this);
            UE_LOG(LogTemp, Log, TEXT("AChessPlayerController: Clicked on empty space."));
        }
    }
}
