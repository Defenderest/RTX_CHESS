#include "ChessGameState.h"
#include "Net/UnrealNetwork.h" // Для DOREPLIFETIME
#include "ChessBoard.h"        // Для использования AChessBoard в логике проверки шаха/мата
#include "PawnPiece.h"         // Добавлено для определения APawnPiece

AChessGameState::AChessGameState()
{
    PrimaryActorTick.bCanEverTick = false; // GameState обычно не тикает
    CurrentTurnColor = EPieceColor::White;
    CurrentGamePhase = EGamePhase::WaitingToStart;
    EnPassantTargetSquare = FIntPoint(-1, -1); // Инициализация невалидным значением
    EnPassantPawnToCapture = nullptr;
    HalfmoveClock = 0;
    FullmoveNumber = 1;
    bCanWhiteCastleKingSide = true;
    bCanWhiteCastleQueenSide = true;
    bCanBlackCastleKingSide = true;
    bCanBlackCastleQueenSide = true;
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
    DOREPLIFETIME(AChessGameState, EnPassantTargetSquare);
    DOREPLIFETIME(AChessGameState, EnPassantPawnToCapture);
    DOREPLIFETIME(AChessGameState, HalfmoveClock);
    DOREPLIFETIME(AChessGameState, FullmoveNumber);
    DOREPLIFETIME(AChessGameState, bCanWhiteCastleKingSide);
    DOREPLIFETIME(AChessGameState, bCanWhiteCastleQueenSide);
    DOREPLIFETIME(AChessGameState, bCanBlackCastleKingSide);
    DOREPLIFETIME(AChessGameState, bCanBlackCastleQueenSide);
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

                // Проверяем, останется ли игрок в шахе после этого ход��
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
                    return false; // Найден допустимый ход, кот��рый не приводит к шаху, значит это не пат
                }
            }
        }
    }

    return true; // Нет допустимых ходов, и игрок не в шахе, значит это пат
}

// --- Full Game State Management ---

void AChessGameState::ResetGameStateForNewGame()
{
    if (GetLocalRole() == ROLE_Authority)
    {
        HalfmoveClock = 0;
        FullmoveNumber = 1;
        bCanWhiteCastleKingSide = true;
        bCanWhiteCastleQueenSide = true;
        bCanBlackCastleKingSide = true;
        bCanBlackCastleQueenSide = true;
        ClearEnPassantData();
        // CurrentTurnColor и GamePhase устанавливаются в GameMode
    }
}

void AChessGameState::UpdateCastlingRights(const AChessPiece* Piece)
{
    if (GetLocalRole() != ROLE_Authority || !Piece)
    {
        return;
    }

    // Если король двинулся, он теряет оба права на рокировку
    if (Piece->GetPieceType() == EPieceType::King)
    {
        if (Piece->GetPieceColor() == EPieceColor::White)
        {
            if (bCanWhiteCastleKingSide) bCanWhiteCastleKingSide = false;
            if (bCanWhiteCastleQueenSide) bCanWhiteCastleQueenSide = false;
        }
        else
        {
            if (bCanBlackCastleKingSide) bCanBlackCastleKingSide = false;
            if (bCanBlackCastleQueenSide) bCanBlackCastleQueenSide = false;
        }
    }
    // Если двинулась (или была захвачена) ладья, теряется соответствующее право
    else if (Piece->GetPieceType() == EPieceType::Rook)
    {
        // Предполагается стандартная доска 8x8 для определения, какая это ладья
        const FIntPoint Pos = Piece->GetBoardPosition();
        if (Piece->GetPieceColor() == EPieceColor::White)
        {
            if (Pos == FIntPoint(7, 0) && bCanWhiteCastleKingSide) bCanWhiteCastleKingSide = false; // H1
            else if (Pos == FIntPoint(0, 0) && bCanWhiteCastleQueenSide) bCanWhiteCastleQueenSide = false; // A1
        }
        else
        {
            if (Pos == FIntPoint(7, 7) && bCanBlackCastleKingSide) bCanBlackCastleKingSide = false; // H8
            else if (Pos == FIntPoint(0, 7) && bCanBlackCastleQueenSide) bCanBlackCastleQueenSide = false; // A8
        }
    }
}

void AChessGameState::IncrementFullmoveNumber()
{
    if (GetLocalRole() == ROLE_Authority)
    {
        FullmoveNumber++;
    }
}

void AChessGameState::IncrementHalfmoveClock()
{
    if (GetLocalRole() == ROLE_Authority)
    {
        HalfmoveClock++;
    }
}

void AChessGameState::ResetHalfmoveClock()
{
    if (GetLocalRole() == ROLE_Authority)
    {
        HalfmoveClock = 0;
    }
}


void AChessGameState::SetCurrentTurnColor(EPieceColor NewTurnColor)
{
    if (GetLocalRole() == ROLE_Authority) // Убеждаемся, что это вызывается на сервере
    {
        if (CurrentTurnColor != NewTurnColor)
        {
            CurrentTurnColor = NewTurnColor;
            // OnRep_CurrentTurn(); // Вызываем OnRep_CurrentTurn вручную на сервере - OnRep вызывается автоматически при изменении реплицируемого свойства
        }
    }
}

// --- En Passant Logic ---

FIntPoint AChessGameState::GetEnPassantTargetSquare() const
{
    return EnPassantTargetSquare;
}

APawnPiece* AChessGameState::GetEnPassantPawnToCapture() const
{
    return EnPassantPawnToCapture.Get(); // TWeakObjectPtr::Get()
}

void AChessGameState::SetEnPassantData(const FIntPoint& TargetSquare, APawnPiece* PawnToCapture)
{
    if (GetLocalRole() == ROLE_Authority)
    {
        EnPassantTargetSquare = TargetSquare;
        EnPassantPawnToCapture = PawnToCapture;
        // Ручной вызов OnRep не ��ужен для TWeakObjectPtr или FIntPoint, если они реплицируются.
        // Однако, если клиенты должны немедленно отреагировать, можно рассмотреть OnRep функции.
        // Для FIntPoint OnRep может быть полезен, если есть визуализация EnPassantTargetSquare.
        // OnRep_EnPassantTargetSquare(); // Если бы такая функция была
        UE_LOG(LogTemp, Log, TEXT("AChessGameState: En Passant data SET - Target: (%d,%d), Pawn: %s"),
            TargetSquare.X, TargetSquare.Y, PawnToCapture ? *PawnToCapture->GetName() : TEXT("nullptr"));
    }
}

void AChessGameState::ClearEnPassantData()
{
    if (GetLocalRole() == ROLE_Authority)
    {
        if (EnPassantTargetSquare != FIntPoint(-1, -1) || EnPassantPawnToCapture.IsValid())
        {
            EnPassantTargetSquare = FIntPoint(-1, -1);
            EnPassantPawnToCapture = nullptr;
            UE_LOG(LogTemp, Log, TEXT("AChessGameState: En Passant data CLEARED."));
            // OnRep_EnPassantTargetSquare(); // Если бы такая функция была
        }
    }
}

FString AChessGameState::GetFEN() const
{
    FString FEN = "";
    const int32 BoardSize = 8; // FEN стандартно для доски 8x8

    for (int32 y = BoardSize - 1; y >= 0; --y)
    {
        int32 emptySquares = 0;
        for (int32 x = 0; x < BoardSize; ++x)
        {
            AChessPiece* piece = GetPieceAtGridPosition(FIntPoint(x, y));
            if (piece)
            {
                if (emptySquares > 0)
                {
                    FEN += FString::FromInt(emptySquares);
                    emptySquares = 0;
                }
                char pieceChar = ' ';
                switch (piece->GetPieceType())
                {
                case EPieceType::Pawn:   pieceChar = 'p'; break;
                case EPieceType::Rook:   pieceChar = 'r'; break;
                case EPieceType::Knight: pieceChar = 'n'; break;
                case EPieceType::Bishop: pieceChar = 'b'; break;
                case EPieceType::Queen:  pieceChar = 'q'; break;
                case EPieceType::King:   pieceChar = 'k'; break;
                }
                if (piece->GetPieceColor() == EPieceColor::White)
                {
                    pieceChar = FChar::ToUpper(pieceChar);
                }
                FEN.AppendChar(pieceChar);
            }
            else
            {
                emptySquares++;
            }
        }
        if (emptySquares > 0)
        {
            FEN += FString::FromInt(emptySquares);
        }
        if (y > 0)
        {
            FEN += "/";
        }
    }

    // Active color
    FEN += " ";
    FEN += (CurrentTurnColor == EPieceColor::White) ? "w" : "b";

    // Castling availability
    FString CastlingRights = "";
    if (bCanWhiteCastleKingSide) CastlingRights += "K";
    if (bCanWhiteCastleQueenSide) CastlingRights += "Q";
    if (bCanBlackCastleKingSide) CastlingRights += "k";
    if (bCanBlackCastleQueenSide) CastlingRights += "q";
    FEN += " " + (CastlingRights.IsEmpty() ? "-" : CastlingRights);

    // En Passant target square
    const FIntPoint EnPassantSquare = GetEnPassantTargetSquare();
    if (EnPassantSquare.X != -1 && EnPassantSquare.Y != -1)
    {
        FString EnPassantString;
        EnPassantString += TCHAR('a' + EnPassantSquare.X);
        EnPassantString += TCHAR('1' + EnPassantSquare.Y);
        FEN += " " + EnPassantString;
    }
    else
    {
        FEN += " -";
    }

    // Halfmove clock and fullmove number
    FEN += " " + FString::FromInt(HalfmoveClock);
    FEN += " " + FString::FromInt(FullmoveNumber);

    UE_LOG(LogTemp, Log, TEXT("AChessGameState::GetFEN: Generated FEN: %s"), *FEN);

    return FEN;
}
