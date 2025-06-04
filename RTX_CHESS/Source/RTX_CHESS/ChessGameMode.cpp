#include "ChessGameMode.h"
#include "ChessPlayerController.h" // Пример подключения
#include "ChessGameState.h"       // Пример подключения
#include "ChessBoard.h"           // Пример подключения
#include "ChessPiece.h"           // Для EPieceColor и др.

AChessGameMode::AChessGameMode()
{
    PrimaryActorTick.bCanEverTick = false; // Обычно GameMode не тикает каждый кадр

    // Установка классов по умолчанию
    // PlayerControllerClass = AChessPlayerController::StaticClass();
    // GameStateClass = AChessGameState::StaticClass();
    // DefaultPawnClass = nullptr; // Для шахмат игрок обычно не управляет пешкой напрямую
}

void AChessGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Логика начала игры
    // StartNewGame();
}

void AChessGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// void AChessGameMode::StartNewGame()
// {
//     // Логика инициализации новой игры: создание доски, расстановка фигур и т.д.
// }

// void AChessGameMode::EndTurn()
// {
//     // Логика завершения хода и передачи управления другому игроку
// }
