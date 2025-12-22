#include "Core/ChessGameState.h"
#include "Net/UnrealNetwork.h" // Для DOREPLIFETIME
#include "Board/ChessBoard.h"        // Для использования AChessBoard в логике проверки шаха/мата
#include "Pieces/PawnPiece.h"         // Добавлено для определения APawnPiece
#include "Kismet/GameplayStatics.h" // Для поиска AChessBoard
#include "Actors/PlayerPawn.h"
#include "Controllers/ChessPlayerController.h"

AChessGameState::AChessGameState()
{
    PrimaryActorTick.bCanEverTick = true; // GameState должен тикать для отсчета времени
    CurrentTurnColor = EPieceColor::White;
    CurrentGamePhase = EGamePhase::WaitingToStart;
    CurrentGameMode = EGameModeType::PlayerVsPlayer;
    EnPassantTargetSquare = FIntPoint(-1, -1); // Инициализация невалидным значением
    EnPassantPawnToCapture = nullptr;
    HalfmoveClock = 0;
    FullmoveNumber = 1;
    bCanWhiteCastleKingSide = true;
    bCanWhiteCastleQueenSide = true;
    bCanBlackCastleKingSide = true;
    bCanBlackCastleQueenSide = true;
    WhiteTimeSeconds = -1.f; // -1 означает бесконечность
    BlackTimeSeconds = -1.f;
    LobbyColorPreference = 1; // Random by default
}

void AChessGameState::OnRep_CurrentTurn()
{
    // Логика, выполняемая при изменении CurrentTurnColor на клиентах
    UE_LOG(LogTemp, Log, TEXT("AChessGameState: Current turn changed to %s (Client)"), (CurrentTurnColor == EPieceColor::White ? TEXT("White") : TEXT("Black")));
}

void AChessGameState::OnRep_LobbyStateChanged()
{
    // Вызывается на клиентах, когда состояние лобби меняется.
    // Сообщаем локальному контроллеру, чтобы он показал/скрыл UI.
    if (AChessPlayerController* PC = Cast<AChessPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
    {
        if (bIsInLobby)
        {
            PC->ShowLobbyUI();
        }
        else
        {
            PC->HideLobbyUI();
        }
    }
}

void AChessGameState::OnRep_GamePhase()
{
    // Логика, выполняемая при изменении CurrentGamePhase на клиентах
    UE_LOG(LogTemp, Log, TEXT("AChessGameState: Game phase changed to %s (Client)"), *UEnum::GetValueAsString(CurrentGamePhase));
}

void AChessGameState::OnRep_PlayerProfiles()
{
    // В будущем здесь можно будет обновить виджеты с информацией об игроках
}

void AChessGameState::OnRep_WhiteTimeSeconds()
{
    // В будущем здесь можно обновлять UI таймера
}

void AChessGameState::OnRep_BlackTimeSeconds()
{
    // В будущем здесь можно обновлять UI таймера
}

void AChessGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AChessGameState, CurrentTurnColor);
    DOREPLIFETIME(AChessGameState, CurrentGamePhase);
    DOREPLIFETIME(AChessGameState, WhitePlayerProfile);
    DOREPLIFETIME(AChessGameState, BlackPlayerProfile);
    DOREPLIFETIME(AChessGameState, WhiteTimeSeconds);
    DOREPLIFETIME(AChessGameState, BlackTimeSeconds);
    DOREPLIFETIME(AChessGameState, ActivePieces);
    DOREPLIFETIME(AChessGameState, EnPassantTargetSquare);
    DOREPLIFETIME(AChessGameState, EnPassantPawnToCapture);
    DOREPLIFETIME(AChessGameState, HalfmoveClock);
    DOREPLIFETIME(AChessGameState, FullmoveNumber);
    DOREPLIFETIME(AChessGameState, bCanWhiteCastleKingSide);
    DOREPLIFETIME(AChessGameState, bCanWhiteCastleQueenSide);
    DOREPLIFETIME(AChessGameState, bCanBlackCastleKingSide);
    DOREPLIFETIME(AChessGameState, bCanBlackCastleQueenSide);
    DOREPLIFETIME(AChessGameState, PawnToPromote);
    DOREPLIFETIME(AChessGameState, CurrentGameMode);
    DOREPLIFETIME(AChessGameState, bIsInLobby);
    DOREPLIFETIME(AChessGameState, LobbyTimeControl);
    DOREPLIFETIME(AChessGameState, LobbyColorPreference);
    DOREPLIFETIME(AChessGameState, MoveHistory);
}

void AChessGameState::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (GetLocalRole() == ROLE_Authority)
    {
        if (CurrentGamePhase == EGamePhase::InProgress || CurrentGamePhase == EGamePhase::Check)
        {
            if (CurrentTurnColor == EPieceColor::White && WhiteTimeSeconds > 0)
            {
                WhiteTimeSeconds -= DeltaSeconds;
            }
            else if (CurrentTurnColor == EPieceColor::Black && BlackTimeSeconds > 0)
            {
                BlackTimeSeconds -= DeltaSeconds;
            }
        }
    }
}

EPieceColor AChessGameState::GetCurrentTurnColor() const
{
    return CurrentTurnColor;
}

int32 AChessGameState::GetFullmoveNumber() const
{
    return FullmoveNumber;
}

void AChessGameState::Server_SwitchTurn()
{
    if (GetLocalRole() == ROLE_Authority) // Убеждаемся, что это вызывается на сервере
    {
        // Игрок, который только что сделал ход, это тот, чей ход был текущим.
        const EPieceColor PlayerWhoMoved = CurrentTurnColor;

        // Воспроизводим анимацию для этого игрока на всех клиентах.
        Multicast_PlayMoveAnimationForPlayer(PlayerWhoMoved);

        SetCurrentTurnColor((CurrentTurnColor == EPieceColor::White) ? EPieceColor::Black : EPieceColor::White);
    }
}

void AChessGameState::Multicast_PlayMoveAnimationForPlayer_Implementation(EPieceColor PlayerColor)
{
    // Эта функция выполняется на всех клиентах. Нам нужно найти правильного пеона и воспроизвести анимацию.
    // Мы итерируем по всем контроллерам игроков в мире.
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (AChessPlayerController* PC = Cast<AChessPlayerController>(It->Get()))
        {
            if (PC->GetPlayerColor() == PlayerColor)
            {
                if (APlayerPawn* Pawn = Cast<APlayerPawn>(PC->GetPawn()))
                {
                    Pawn->PlayMoveAnimation();
                    return; // Нашли игрока, дело сделано.
                }
            }
        }
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

    const FIntPoint KingPosition = King->GetBoardPosition();
    const EPieceColor OpponentColor = (PlayerColor == EPieceColor::White) ? EPieceColor::Black : EPieceColor::White;

    // Используем централизованную функцию AChessBoard для проверки атаки.
    // Это предотвращает дублирование логики и потенциальные рекурсии,
    // если GetValidMoves вызывает IsPlayerInCheck.
    return Board->IsSquareAttackedBy(KingPosition, OpponentColor);
}

bool AChessGameState::IsPlayerInCheckmate(EPieceColor PlayerColor, const AChessBoard* Board)
{
    if (!IsPlayerInCheck(PlayerColor, Board))
    {
        return false;
    }

    // Проверяем все фигуры игрока
    TArray<TObjectPtr<AChessPiece>> PiecesToProcess = ActivePieces;

    for (auto& PiecePtr : PiecesToProcess)
    {
        AChessPiece* PlayerPiece = PiecePtr.Get();
        if (PlayerPiece && PlayerPiece->GetPieceColor() == PlayerColor)
        {
            TArray<FIntPoint> PseudoLegalMoves = PlayerPiece->GetValidMoves(this, Board);
            
            for (const FIntPoint& TargetPosition : PseudoLegalMoves)
            {
                if (IsMoveLegal(PlayerPiece, TargetPosition, Board))
                {
                    // Найден хотя бы один легальный ход, значит это не мат
                    return false;
                }
            }
        }
    }

    return true; 
}

bool AChessGameState::IsStalemate(EPieceColor PlayerColor, const AChessBoard* Board)
{
    if (IsPlayerInCheck(PlayerColor, Board))
    {
        return false;
    }

    TArray<TObjectPtr<AChessPiece>> PiecesToProcess = ActivePieces;

    for (auto& PiecePtr : PiecesToProcess)
    {
        AChessPiece* PlayerPiece = PiecePtr.Get();
        if (PlayerPiece && PlayerPiece->GetPieceColor() == PlayerColor)
        {
            TArray<FIntPoint> PseudoLegalMoves = PlayerPiece->GetValidMoves(this, Board);
            
            for (const FIntPoint& TargetPosition : PseudoLegalMoves)
            {
                if (IsMoveLegal(PlayerPiece, TargetPosition, Board))
                {
                    return false;
                }
            }
        }
    }

    return true;
}

bool AChessGameState::IsMoveLegal(AChessPiece* PieceToMove, const FIntPoint& TargetPosition, const AChessBoard* Board)
{
    if (!PieceToMove || !Board)
    {
        return false;
    }

    const EPieceColor PlayerColor = PieceToMove->GetPieceColor();
    const FIntPoint OriginalPosition = PieceToMove->GetBoardPosition();

    // Определяем, какая фигура будет взята (если есть)
    AChessPiece* PieceToRemoveTemporarily = GetPieceAtGridPosition(TargetPosition);

    // Особый случай для взятия на проходе
    if (PieceToMove->GetPieceType() == EPieceType::Pawn && TargetPosition == EnPassantTargetSquare)
    {
        if (!PieceToRemoveTemporarily) // Взятие на проходе возможно только на пустую клетку
        {
            PieceToRemoveTemporarily = GetEnPassantPawnToCapture();
        }
    }

    // --- Симуляция хода ---
    RemovePieceFromState(PieceToMove);
    if (PieceToRemoveTemporarily)
    {
        RemovePieceFromState(PieceToRemoveTemporarily);
    }

    // Временно обновляем позицию фигуры
    PieceToMove->SetBoardPosition(TargetPosition);
    AddPieceToState(PieceToMove); // Добавляем на новую позицию

    // Проверяем, останется ли игрок в шахе после этого хода
    const bool bPlayerIsInCheckAfterMove = IsPlayerInCheck(PlayerColor, Board);

    // --- Отмена симуляции ---
    RemovePieceFromState(PieceToMove);
    PieceToMove->SetBoardPosition(OriginalPosition);
    AddPieceToState(PieceToMove);
    if (PieceToRemoveTemporarily)
    {
        AddPieceToState(PieceToRemoveTemporarily);
    }

    // Ход легален, если он НЕ оставляет короля в шахе.
    return !bPlayerIsInCheckAfterMove;
}

void AChessGameState::SetIsInLobby(bool bNewState)
{
    if (HasAuthority())
    {
        bIsInLobby = bNewState;
        // На сервере вызываем вручную, так как OnRep не срабатывает для хоста
        if (GetNetMode() != NM_Client)
        {
            OnRep_LobbyStateChanged();
        }
    }
}

void AChessGameState::SetLobbyTimeControl(ETimeControlType NewTimeControl)
{
    if (HasAuthority())
    {
        LobbyTimeControl = NewTimeControl;
    }
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
        PawnToPromote = nullptr;
        MoveHistory.Empty();
    }
}

void AChessGameState::UpdateCastlingRights(const AChessPiece* Piece, const FIntPoint& FromPosition)
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
        const FIntPoint Pos = FromPosition;
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

    // The FEN string must be generated from the authoritative game state, not the visual board.
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
            FEN += TEXT("/");
        }
    }

    // Active color
    FEN.Append(TEXT(" "));
    FEN.Append((CurrentTurnColor == EPieceColor::White) ? TEXT("w") : TEXT("b"));

    // Castling availability
    FEN.Append(TEXT(" "));
    FString CastlingRights;
    if (bCanWhiteCastleKingSide)  CastlingRights += TEXT("K");
    if (bCanWhiteCastleQueenSide) CastlingRights += TEXT("Q");
    if (bCanBlackCastleKingSide)  CastlingRights += TEXT("k");
    if (bCanBlackCastleQueenSide) CastlingRights += TEXT("q");
    if (CastlingRights.IsEmpty())
    {
        FEN.Append(TEXT("-"));
    }
    else
    {
        FEN.Append(CastlingRights);
    }

    // En Passant target square
    FEN.Append(TEXT(" "));
    if (EnPassantTargetSquare.X != -1 && EnPassantTargetSquare.Y != -1)
    {
        // Convert grid coordinates to algebraic notation (e.g., e3)
        FString EnPassantSquareString;
        EnPassantSquareString += TCHAR('a' + EnPassantTargetSquare.X);
        EnPassantSquareString += TCHAR('1' + EnPassantTargetSquare.Y);
        FEN.Append(EnPassantSquareString);
    }
    else
    {
        FEN.Append(TEXT("-"));
    }

    // Halfmove clock and fullmove number
    FEN.Append(TEXT(" "));
    FEN.Append(FString::FromInt(HalfmoveClock));
    FEN.Append(TEXT(" "));
    FEN.Append(FString::FromInt(FullmoveNumber));

    UE_LOG(LogTemp, Log, TEXT("AChessGameState::GetFEN: Generated FEN: %s"), *FEN);

    return FEN;
}

EGameModeType AChessGameState::GetCurrentGameModeType() const
{
    return CurrentGameMode;
}

void AChessGameState::SetCurrentGameMode(EGameModeType NewMode)
{
    if (GetLocalRole() == ROLE_Authority)
    {
        CurrentGameMode = NewMode;
    }
}

APawnPiece* AChessGameState::GetPawnToPromote() const
{
    return PawnToPromote;
}

void AChessGameState::SetPawnToPromote(APawnPiece* Pawn)
{
    if (GetLocalRole() == ROLE_Authority)
    {
        PawnToPromote = Pawn;
    }
}
