#include "ChessGameState.h"
#include "Net/UnrealNetwork.h" // Для DOREPLIFETIME
#include "ChessBoard.h"        // Для использования AChessBoard в логике проверки шаха/мата

AChessGameState::AChessGameState()
{
    PrimaryActorTick.bCanEverTick = false; // GameState обычно не тикает
    CurrentTurnColor = EPieceColor::White;
    CurrentGamePhase = EGamePhase::WaitingToStart;
}

void AChessGameState::OnRep_CurrentTurn()
{
    // Логика, выполняемая при изменении CurrentTurnColor на клиентах
    UE_LOG(LogTemp, Log, TEXT("AChessGameState: Current turn changed to %s (Client)"), (CurrentTurnColor == EPieceColor::White ? TEXT("White") : TEXT("Black")));
}

void AChessGameState::OnRep_GamePhase()
{
    // Логика, выполняемая при изменении CurrentGamePhase на клиентах
    UE_LOG(LogTemp, Log, TEXT("AChessGameState: Game phase changed to %s (Client)"), *UEnum::GetValueAsString(CurrentGamePhase));
}

void AChessGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AChessGameState, CurrentTurnColor);
    DOREPLIFETIME(AChessGameState, CurrentGamePhase);
    DOREPLIFETIME(AChessGameState, ActivePieces);
}

EPieceColor AChessGameState::GetCurrentTurnColor() const
{
    return CurrentTurnColor;
}

void AChessGameState::Server_SwitchTurn()
{
    if (GetLocalRole() == ROLE_Authority) // Убеждаемся, что это вызывается на сервере
    {
        SetCurrentTurnColor((CurrentTurnColor == EPieceColor::White) ? EPieceColor::Black : EPieceColor::White);
    }
}

EGamePhase AChessGameState::GetGamePhase() const
{
    return CurrentGamePhase;
}

void AChessGameState::SetGamePhase(EGamePhase NewPhase)
{
    if (GetLocalRole() == ROLE_Authority) // Убеждаемся, что это вызывается на сервере
    {
        if (CurrentGamePhase != NewPhase)
        {
            CurrentGamePhase = NewPhase;
            OnRep_GamePhase(); // Вызываем OnRep_GamePhase вручную на сервере
        }
    }
}

void AChessGameState::AddPieceToState(AChessPiece* PieceToAdd)
{
    if (PieceToAdd && !ActivePieces.Contains(PieceToAdd))
    {
        ActivePieces.Add(PieceToAdd);
    }
}

void AChessGameState::RemovePieceFromState(AChessPiece* PieceToRemove)
{
    if (PieceToRemove)
    {
        ActivePieces.Remove(PieceToRemove);
    }
}

AChessPiece* AChessGameState::GetPieceAtGridPosition(const FIntPoint& GridPosition) const
{
    for (AChessPiece* Piece : ActivePieces)
    {
        if (Piece && Piece->GetBoardPosition() == GridPosition)
        {
            return Piece;
        }
    }
    return nullptr;
}

bool AChessGameState::IsPlayerInCheck(EPieceColor PlayerColor, const AChessBoard* Board) const
{
    if (!Board)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameState::IsPlayerInCheck: Board is null."));
        return false;
    }

    // Находим короля игрока
    AChessPiece* King = nullptr;
    for (AChessPiece* Piece : ActivePieces)
    {
        if (Piece && Piece->GetPieceType() == EPieceType::King && Piece->GetPieceColor() == PlayerColor)
        {
            King = Piece;
            break;
        }
    }

    if (!King)
    {
        // Это может произойти, если король был захвачен (что означает конец игры)
        // Или если игра еще не началась и король не заспавнен.
        // В нормальной игре король всегда должен быть.
        UE_LOG(LogTemp, Warning, TEXT("AChessGameState::IsPlayerInCheck: King of %s not found."), (PlayerColor == EPieceColor::White ? TEXT("White") : TEXT("Black")));
        return false;
    }

    FIntPoint KingPosition = King->GetBoardPosition();
    EPieceColor OpponentColor = (PlayerColor == EPieceColor::White) ? EPieceColor::Black : EPieceColor::White;

    // Проверяем, может ли какая-либо фигура противника атаковать короля
    for (AChessPiece* OpponentPiece : ActivePieces)
    {
        if (OpponentPiece && OpponentPiece->GetPieceColor() == OpponentColor)
        {
            TArray<FIntPoint> OpponentValidMoves = OpponentPiece->GetValidMoves(Board);
            if (OpponentValidMoves.Contains(KingPosition))
            {
                return true; // Король находится под шахом
            }
        }
    }

    return false; // Король не под шахом
}

bool AChessGameState::IsPlayerInCheckmate(EPieceColor PlayerColor, const AChessBoard* Board) // Removed const
{
    if (!IsPlayerInCheck(PlayerColor, Board))
    {
        return false; // Не мат, если игрок не в шахе
    }

    // Проверяем все возможные ходы для всех фигур игрока
    for (AChessPiece* PlayerPiece : ActivePieces)
    {
        if (PlayerPiece && PlayerPiece->GetPieceColor() == PlayerColor)
        {
            TArray<FIntPoint> ValidMoves = PlayerPiece->GetValidMoves(Board);
            FIntPoint OriginalPosition = PlayerPiece->GetBoardPosition();

            for (const FIntPoint& TargetPosition : ValidMoves)
            {
                // Симулируем ход
                AChessPiece* CapturedPiece = GetPieceAtGridPosition(TargetPosition);

                // Временно удаляем фигуры из состояния
                RemovePieceFromState(PlayerPiece);
                if (CapturedPiece)
                {
                    RemovePieceFromState(CapturedPiece);
                }

                // Временно обновляем позицию фигуры
                PlayerPiece->SetBoardPosition(TargetPosition);
                AddPieceToState(PlayerPiece); // Добавляем на новую позицию

                // Проверяем, останется ли игрок в шахе после этого хода
                bool bStillInCheck = IsPlayerInCheck(PlayerColor, Board);

                // Отменяем симуляцию хода
                RemovePieceFromState(PlayerPiece);
                PlayerPiece->SetBoardPosition(OriginalPosition);
                AddPieceToState(PlayerPiece);
                if (CapturedPiece)
                {
                    AddPieceToState(CapturedPiece);
                }

                if (!bStillInCheck)
                {
                    return false; // Найден ход, который выводит из шаха, значит это не мат
                }
            }
        }
    }

    return true; // Нет ходов, чтобы выйти из шаха, значит это мат
}

bool AChessGameState::IsStalemate(EPieceColor PlayerColor, const AChessBoard* Board) // Removed const
{
    if (IsPlayerInCheck(PlayerColor, Board))
    {
        return false; // Не пат, если игрок в шахе (это может быть мат)
    }

    // Проверяем, есть ли у игрока хотя бы один допустимый ход, который не приводит к шаху
    for (AChessPiece* PlayerPiece : ActivePieces)
    {
        if (PlayerPiece && PlayerPiece->GetPieceColor() == PlayerColor)
        {
            TArray<FIntPoint> ValidMoves = PlayerPiece->GetValidMoves(Board);
            FIntPoint OriginalPosition = PlayerPiece->GetBoardPosition();

            for (const FIntPoint& TargetPosition : ValidMoves)
            {
                // Симулируем ход
                AChessPiece* CapturedPiece = GetPieceAtGridPosition(TargetPosition);

                RemovePieceFromState(PlayerPiece);
                if (CapturedPiece)
                {
                    RemovePieceFromState(CapturedPiece);
                }

                PlayerPiece->SetBoardPosition(TargetPosition);
                AddPieceToState(PlayerPiece);

                // Проверяем, будет ли игрок в шахе после этого хода
                bool bIsInCheckAfterMove = IsPlayerInCheck(PlayerColor, Board);

                // Отменяем симуляцию хода
                RemovePieceFromState(PlayerPiece);
                PlayerPiece->SetBoardPosition(OriginalPosition);
                AddPieceToState(PlayerPiece);
                if (CapturedPiece)
                {
                    AddPieceToState(CapturedPiece);
                }

                if (!bIsInCheckAfterMove)
                {
                    return false; // Найден допустимый ход, который не приводит к шаху, значит это не пат
                }
            }
        }
    }

    return true; // Нет допустимых ходов, и игрок не в шахе, значит это пат
}

void AChessGameState::SetCurrentTurnColor(EPieceColor NewTurnColor)
{
    if (GetLocalRole() == ROLE_Authority) // Убеждаемся, что это вызывается на сервере
    {
        if (CurrentTurnColor != NewTurnColor)
        {
            CurrentTurnColor = NewTurnColor;
            OnRep_CurrentTurn(); // Вызываем OnRep_CurrentTurn вручную на сервере
        }
    }
}
