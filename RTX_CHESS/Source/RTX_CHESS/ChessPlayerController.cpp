#include "ChessPlayerController.h"
// #include "EnhancedInputSubsystems.h" // Для Enhanced Input
// #include "EnhancedInputComponent.h"   // Для Enhanced Input
// #include "ChessPiece.h"
// #include "ChessBoard.h"

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
    // if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    // {
    //     if (ChessMappingContext)
    //     {
    //         Subsystem->AddMappingContext(ChessMappingContext, 0);
    //     }
    // }
}

void AChessPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Привязка действий Enhanced Input
    // if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
    // {
    //     if (SelectAction)
    //     {
    //         EnhancedInput->BindAction(SelectAction, ETriggerEvent::Started, this, &AChessPlayerController::HandlePieceSelection);
    //     }
    //     if (MoveAction)
    //     {
    //         EnhancedInput->BindAction(MoveAction, ETriggerEvent::Started, this, &AChessPlayerController::HandlePieceMove);
    //     }
    // }
}

void AChessPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// void AChessPlayerController::HandlePieceSelection()
// {
//     // Логика выбора фигуры
// }

// void AChessPlayerController::HandlePieceMove()
// {
//     // Логика перемещения выбранной фигуры
// }
