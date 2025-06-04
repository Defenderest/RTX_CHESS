#include "ChessGameState.h"
// #include "Net/UnrealNetwork.h" // Для DOREPLIFETIME

AChessGameState::AChessGameState()
{
    // PrimaryActorTick.bCanEverTick = false; // Обычно GameState не тикает
    // CurrentTurnColor = EPieceColor::White;
}

// void AChessGameState::OnRep_CurrentTurn()
// {
//     // Логика, выполняемая при изменении CurrentTurnColor на клиентах
// }

// void AChessGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
// {
//     Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//     DOREPLIFETIME(AChessGameState, CurrentTurnColor);
//     DOREPLIFETIME(AChessGameState, AllPieces);
// }
