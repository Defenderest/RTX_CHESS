#include "ChessGameMode.h"
#include "ChessPlayerController.h"
#include "ChessGameState.h"
#include "ChessBoard.h"
#include "ChessPiece.h"
#include "PawnPiece.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "StockfishManager.h"
#include "ChessPlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

AChessGameMode::AChessGameMode()
{
    PrimaryActorTick.bCanEverTick = false;

    GameStateClass = AChessGameState::StaticClass();

    StockfishManager = CreateDefaultSubobject<UStockfishManager>(TEXT("StockfishManager"));
    CurrentGameMode = EGameModeType::PlayerVsPlayer;
    NumberOfPlayers = 0;
    BotSkillLevel = 10; // Глубина поиска для бота (1-15)
}

void AChessGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (GameStateClass)
    {
        UE_LOG(LogTemp, Log, TEXT("AChessGameMode::BeginPlay: Configured GameStateClass is: %s"), *GameStateClass->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessGameMode::BeginPlay: GameStateClass is NOT configured on this GameMode instance!"));
    }

    AChessGameState* TempGS = GetGameState<AChessGameState>();
    if (TempGS)
    {
        UE_LOG(LogTemp, Log, TEXT("AChessGameMode::BeginPlay: GetGameState<AChessGameState>() returned a valid GameState: %s"), *TempGS->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessGameMode::BeginPlay: GetGameState<AChessGameState>() returned NULL!"));
    }

    FindGameBoard();

    // Мы начинаем игру, только если в опциях запуска явно указан режим игры.
    // Это позволяет при первом запуске просто показать главное меню.
    FString IsBotGameValue = UGameplayStatics::ParseOption(this->OptionsString, TEXT("bIsBotGame"));
    if (!IsBotGameValue.IsEmpty())
    {
        if (IsBotGameValue.ToBool())
        {
            // Запускаем игру против бота
            StartBotGame();

            // Устанавливаем уровень сложности бота из параметров запуска
            if (StockfishManager)
            {
                FString SkillLevelValue = UGameplayStatics::ParseOption(this->OptionsString, TEXT("SkillLevel"));
                if (!SkillLevelValue.IsEmpty())
                {
                    BotSkillLevel = FCString::Atoi(*SkillLevelValue);
                    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Bot skill level set to %d from launch options."), BotSkillLevel);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("AChessGameMode: SkillLevel option not found for bot game. Using default skill level."));
                }
            }
        }
        else
        {
            // Запускаем игру "Игрок против Игрока"
            StartNewGame();
        }
    }
    // Если опция не найдена, ничего не делаем. GameState останется в EGamePhase::WaitingToStart,
    // и PlayerController покажет стартовое меню.
}

void AChessGameMode::StartBotGame()
{
    CurrentGameMode = EGameModeType::PlayerVsBot;
    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Starting new Player vs Bot game."));

    if (StockfishManager)
    {
        UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Initializing API-based bot."));
        StockfishManager->OnBestMoveReceived.AddDynamic(this, &AChessGameMode::HandleBotMoveReceived);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::StartBotGame: StockfishManager is NULL! Cannot start engine."));
    }
    
    SetupBoardAndGameState();

    // Оповещаем игрока о начале игры
    if (APlayerController* PC_Raw = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        if (AChessPlayerController* PC = Cast<AChessPlayerController>(PC_Raw))
        {
            PC->Client_GameStarted();
        }
    }
}

void AChessGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    AChessPlayerController* ChessController = Cast<AChessPlayerController>(NewPlayer);
    if (ChessController)
    {
        NumberOfPlayers++;
        if (CurrentGameMode == EGameModeType::PlayerVsBot)
        {
            // В игре с ботом человек всегда играет за белых
            ChessController->SetPlayerColor(EPieceColor::White);
            UE_LOG(LogTemp, Log, TEXT("AChessGameMode::PostLogin: Player %d joined a BOT game. Assigned color: White."), NumberOfPlayers);
        }
        else // PlayerVsPlayer
        {
            if (NumberOfPlayers == 1)
            {
                ChessController->SetPlayerColor(EPieceColor::White);
                UE_LOG(LogTemp, Log, TEXT("AChessGameMode::PostLogin: Player 1 joined. Assigned color: White."));
            }
            else if (NumberOfPlayers == 2)
            {
                ChessController->SetPlayerColor(EPieceColor::Black);
                UE_LOG(LogTemp, Log, TEXT("AChessGameMode::PostLogin: Player 2 joined. Assigned color: Black."));

                AChessGameState* CurrentGS = GetCurrentGameState();
                if (CurrentGS && CurrentGS->GetGamePhase() == EGamePhase::WaitingToStart)
                {
                    UE_LOG(LogTemp, Log, TEXT("AChessGameMode::PostLogin: Two players present. Starting PvP game."));
                    StartNewGame();
                }
            }
            else
            {
                // Логика для наблюдателей
                UE_LOG(LogTemp, Warning, TEXT("AChessGameMode::PostLogin: More than 2 players joined. Player %d is a spectator."), NumberOfPlayers);
            }
        }
    }
}

void AChessGameMode::MakeBotMove()
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (CurrentGS && StockfishManager)
    {
        UE_LOG(LogTemp, Log, TEXT("ChessGameMode: Requesting bot move."));
        FString FEN = CurrentGS->GetFEN();
        // Запрашиваем ход, результат придет асинхронно в HandleBotMoveReceived
        StockfishManager->RequestBestMove(FEN, BotSkillLevel); 
    }
}

void AChessGameMode::HandleBotMoveReceived(const FString& BestMove)
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (!CurrentGS)
    {
        UE_LOG(LogTemp, Error, TEXT("ChessGameMode::HandleBotMoveReceived: GameState is null."));
        return;
    }

    // Убедимся, что все еще ход бота, прежде чем делать ход
    if (CurrentGS->GetCurrentTurnColor() != EPieceColor::Black)
    {
        UE_LOG(LogTemp, Warning, TEXT("ChessGameMode::HandleBotMoveReceived: Received bot move, but it's not Black's turn anymore. Ignoring move."));
        return;
    }

    if (!BestMove.IsEmpty())
    {
        UE_LOG(LogTemp, Log, TEXT("ChessGameMode: Bot suggests move: %s"), *BestMove);
        // Базовая проверка формата строки хода "e2e4"
        if (BestMove.Len() < 4)
        {
            UE_LOG(LogTemp, Error, TEXT("ChessGameMode: Bot's suggested move is too short: %s"), *BestMove);
            return;
        }

        FIntPoint StartPos((BestMove[0] - 'a'), (BestMove[1] - '1'));
        FIntPoint EndPos((BestMove[2] - 'a'), (BestMove[3] - '1'));

        AChessPiece* PieceToMove = CurrentGS->GetPieceAtGridPosition(StartPos);
        if (PieceToMove)
        {
            // Attempt the move. For a pawn promotion, this will return true and set the state to AwaitingPromotion.
            const bool bMoveSuccessful = AttemptMove(PieceToMove, EndPos, nullptr);

            // If the move was a promotion, we need to complete it immediately for the bot.
            if (bMoveSuccessful && CurrentGS->GetGamePhase() == EGamePhase::AwaitingPromotion)
            {
                APawnPiece* PawnToPromote = CurrentGS->GetPawnToPromote();
                // Ensure the pawn is the one we think it is.
                if (PawnToPromote && PawnToPromote == PieceToMove)
                {
                    EPieceType PromoteToType = EPieceType::Queen; // Default to Queen
                    if (BestMove.Len() == 5)
                    {
                        const TCHAR PromoteChar = FChar::ToLower(BestMove[4]);
                        switch (PromoteChar)
                        {
                            case 'q': PromoteToType = EPieceType::Queen; break;
                            case 'r': PromoteToType = EPieceType::Rook; break;
                            case 'b': PromoteToType = EPieceType::Bishop; break;
                            case 'n': PromoteToType = EPieceType::Knight; break;
                            default: UE_LOG(LogTemp, Warning, TEXT("Invalid promotion character in move '%s'. Defaulting to Queen."), *BestMove);
                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Pawn promotion detected, but move string '%s' does not specify piece. Defaulting to Queen."), *BestMove);
                    }

                    UE_LOG(LogTemp, Log, TEXT("ChessGameMode: Bot is promoting pawn to %s."), *UEnum::GetValueAsString(PromoteToType));
                    CompletePawnPromotion(PawnToPromote, PromoteToType);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("ChessGameMode: Mismatch in pawn promotion state. Bot move will likely fail."));
                }
            }
        }
        else
        {
            FString FEN = CurrentGS->GetFEN();
            UE_LOG(LogTemp, Error, TEXT("ChessGameMode: Bot's suggested move involves a non-existent piece at %s. FEN was: %s"), *StartPos.ToString(), *FEN);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ChessGameMode: Bot did not return a valid move."));
    }
}

AChessGameState* AChessGameMode::GetCurrentGameState() const
{
    return GetGameState<AChessGameState>();
}

UStockfishManager* AChessGameMode::GetStockfishManager() const
{
    return StockfishManager.Get();
}

EGameModeType AChessGameMode::GetCurrentGameModeType() const
{
    return CurrentGameMode;
}

void AChessGameMode::FindGameBoard()
{
    for (TActorIterator<AChessBoard> It(GetWorld()); It; ++It)
    {
        GameBoard = *It;
        if (GameBoard)
        {
            UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Found GameBoard: %s"), *GameBoard->GetName());
            break;
        }
    }

    if (!GameBoard)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode: GameBoard not found in the level!"));
    }
}

void AChessGameMode::StartNewGame()
{
    CurrentGameMode = EGameModeType::PlayerVsPlayer;
    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Starting new Player vs Player game."));
    SetupBoardAndGameState();

    // Оповещаем всех игроков о начале игры
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (AChessPlayerController* PC = Cast<AChessPlayerController>(It->Get()))
        {
            PC->Client_GameStarted();
        }
    }
}

void AChessGameMode::SetupBoardAndGameState()
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (!GameBoard || !CurrentGS)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::SetupBoardAndGameState: GameBoard or GameState is null. Cannot start new game."));
        return;
    }

    GameBoard->ClearAllHighlights();
    GameBoard->InitializeBoard();
    SpawnInitialPieces();

    CurrentGS->ResetGameStateForNewGame();

    CurrentGS->SetCurrentTurnColor(EPieceColor::White);
    CurrentGS->SetGamePhase(EGamePhase::InProgress);

    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Board and game state have been reset. White's turn."));
}

#include "ChessPlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

void AChessGameMode::EndTurn()
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (CurrentGS)
    {
        // Увеличиваем номер полного хода, если ход сделали черные
        if (CurrentGS->GetCurrentTurnColor() == EPieceColor::Black)
        {
            CurrentGS->IncrementFullmoveNumber();
        }

        CurrentGS->Server_SwitchTurn();
        UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Turn ended. Now %s's turn."), (CurrentGS->GetCurrentTurnColor() == EPieceColor::White ? TEXT("White") : TEXT("Black")));
        
        CheckGameEndConditions();

        if (CurrentGameMode == EGameModeType::PlayerVsBot && CurrentGS->GetCurrentTurnColor() == EPieceColor::Black)
        {
            MakeBotMove();
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::EndTurn: GameState is null. Cannot end turn."));
    }
}

// Функции HandlePieceClicked и HandleSquareClicked удалены, так как логика выбора и перемещения
// теперь полностью обрабатывается в AChessPlayerController.

void AChessGameMode::CompletePawnPromotion(APawnPiece* PawnToPromote, EPieceType PromoteToType)
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (!PawnToPromote || !CurrentGS || !GameBoard)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::CompletePawnPromotion: Invalid state for promotion."));
        return;
    }

    if (CurrentGS->GetGamePhase() != EGamePhase::AwaitingPromotion || CurrentGS->GetPawnToPromote() != PawnToPromote)
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessGameMode::CompletePawnPromotion: Received promotion request in wrong game phase or for wrong pawn."));
        return;
    }

    EPieceColor Color = PawnToPromote->GetPieceColor();
    FIntPoint GridPosition = PawnToPromote->GetBoardPosition();

    UE_LOG(LogTemp, Log, TEXT("Promoting pawn at (%d, %d) to %s"), GridPosition.X, GridPosition.Y, *UEnum::GetValueAsString(PromoteToType));

    // Удаляем старую пешку
    CurrentGS->RemovePieceFromState(PawnToPromote);
    PawnToPromote->Destroy();
    
    // Спавним новую фигуру
    AChessPiece* NewPiece = SpawnPieceAtPosition(PromoteToType, Color, GridPosition);
    if(NewPiece)
    {
        NewPiece->NotifyMoveCompleted(); // Новая фигура считается "сходившей"
    }

    // Сбрасываем состояние превращения
    CurrentGS->SetPawnToPromote(nullptr);

    // После превращения, игра может закончиться (мат новой фигурой)
    // поэтому сначала ставим InProgress, потом проверяем конец игры.
    CurrentGS->SetGamePhase(EGamePhase::InProgress);
    CheckGameEndConditions();

    // Если после превращения игра не закончилась, передаем ход
    if(CurrentGS->GetGamePhase() == EGamePhase::InProgress || CurrentGS->GetGamePhase() == EGamePhase::Check)
    {
        EndTurn();
    }
}

bool AChessGameMode::AttemptMove(AChessPiece* PieceToMove, const FIntPoint& TargetGridPosition, AChessPlayerController* RequestingController)
{
    UE_LOG(LogTemp, Log, TEXT("--- AttemptMove START ---"));

    AChessGameState* CurrentGS = GetCurrentGameState();
    if (!PieceToMove || !GameBoard || !CurrentGS)
    {
        UE_LOG(LogTemp, Error, TEXT("AttemptMove FAIL: Invalid input (PieceToMove, GameBoard, or GameState is null)."));
        return false;
    }

    // Проверка 1: Ход текущего цвета?
    if (PieceToMove->GetPieceColor() != CurrentGS->GetCurrentTurnColor())
    {
        UE_LOG(LogTemp, Warning, TEXT("AttemptMove FAIL: It's not %s's turn."), 
            (PieceToMove->GetPieceColor() == EPieceColor::White ? TEXT("White") : TEXT("Black")));
        return false;
    }
    UE_LOG(LogTemp, Log, TEXT("AttemptMove PASS: Turn check passed."));

    // Проверка 2: Является ли ход допустимым для этой фигуры?
    TArray<FIntPoint> ValidMoves = PieceToMove->GetValidMoves(CurrentGS, GameBoard);
    if (!ValidMoves.Contains(TargetGridPosition))
    {
        UE_LOG(LogTemp, Warning, TEXT("AttemptMove FAIL: Target position (%d, %d) is not a valid move for this piece."),
               TargetGridPosition.X, TargetGridPosition.Y);
        return false;
    }
    UE_LOG(LogTemp, Log, TEXT("AttemptMove PASS: Valid move check passed."));

    // Проверка 3: Не ставит ли этот ход своего короля под шах?
    FIntPoint OriginalPosition = PieceToMove->GetBoardPosition();
    AChessPiece* CapturedPiece = CurrentGS->GetPieceAtGridPosition(TargetGridPosition);

    // Временно "делаем" ход, чтобы проверить состояние доски после него
    CurrentGS->RemovePieceFromState(PieceToMove);
    if (CapturedPiece)
    {
        CurrentGS->RemovePieceFromState(CapturedPiece);
    }
    PieceToMove->SetBoardPosition(TargetGridPosition);
    CurrentGS->AddPieceToState(PieceToMove);

    bool bIsInCheckAfterMove = CurrentGS->IsPlayerInCheck(CurrentGS->GetCurrentTurnColor(), GameBoard);

    // Отменяем временный ход
    CurrentGS->RemovePieceFromState(PieceToMove);
    PieceToMove->SetBoardPosition(OriginalPosition);
    CurrentGS->AddPieceToState(PieceToMove);
    if (CapturedPiece)
    {
        CurrentGS->AddPieceToState(CapturedPiece);
    }

    if (bIsInCheckAfterMove)
    {
        UE_LOG(LogTemp, Warning, TEXT("AttemptMove FAIL: Move would put King in check."));
        return false;
    }
    UE_LOG(LogTemp, Log, TEXT("AttemptMove PASS: King safety check passed."));

    // --- Все проверки пройдены, выполняем ход ---
    UE_LOG(LogTemp, Log, TEXT("--- ALL CHECKS PASSED: EXECUTING MOVE ---"));

    // --- Специальная обработка рокировки ---
    if (PieceToMove->GetPieceType() == EPieceType::King && FMath::Abs(TargetGridPosition.X - OriginalPosition.X) == 2)
    {
        // Это ход рокировки. Нам нужно переместить и ладью.
        UE_LOG(LogTemp, Log, TEXT("Executing Move: Castling detected."));
        if (TargetGridPosition.X > OriginalPosition.X) // Рокировка в сторону короля (короткая)
        {
            const FIntPoint RookOriginalPos(GameBoard->GetBoardSize().X - 1, OriginalPosition.Y);
            const FIntPoint RookTargetPos(OriginalPosition.X + 1, OriginalPosition.Y);
            AChessPiece* RookToMove = CurrentGS->GetPieceAtGridPosition(RookOriginalPos);
            if (RookToMove && RookToMove->GetPieceType() == EPieceType::Rook)
            {
                GameBoard->ClearSquare(RookOriginalPos);
                GameBoard->SetPieceAtGridPosition(RookToMove, RookTargetPos);
                RookToMove->NotifyMoveCompleted();
            }
        }
        else // Рокировка в сторону ферзя (длинная)
        {
            const FIntPoint RookOriginalPos(0, OriginalPosition.Y);
            const FIntPoint RookTargetPos(OriginalPosition.X - 1, OriginalPosition.Y);
            AChessPiece* RookToMove = CurrentGS->GetPieceAtGridPosition(RookOriginalPos);
            if (RookToMove && RookToMove->GetPieceType() == EPieceType::Rook)
            {
                GameBoard->ClearSquare(RookOriginalPos);
                GameBoard->SetPieceAtGridPosition(RookToMove, RookTargetPos);
                RookToMove->NotifyMoveCompleted();
            }
        }
    }
    // --- Конец обработки рокировки ---

    const bool bIsPawnMove = PieceToMove->GetPieceType() == EPieceType::Pawn;
    const bool bIsCapture = CapturedPiece != nullptr;

    bool bIsPawnTwoStep = false;
    if (PieceToMove->GetPieceType() == EPieceType::Pawn && FMath::Abs(TargetGridPosition.Y - OriginalPosition.Y) == 2)
    {
        bIsPawnTwoStep = true;
    }

    if (!bIsPawnTwoStep)
    {
        CurrentGS->ClearEnPassantData();
    }

    if (CapturedPiece)
    {
        UE_LOG(LogTemp, Log, TEXT("Executing Move: Capturing %s at (%d, %d)."),
               *UEnum::GetValueAsString(CapturedPiece->GetPieceType()),
               TargetGridPosition.X, TargetGridPosition.Y);
        CurrentGS->RemovePieceFromState(CapturedPiece);
        GameBoard->ClearSquare(CapturedPiece->GetBoardPosition());
        CapturedPiece->OnCaptured();
        CapturedPiece->Destroy();
    }
    else if (PieceToMove->GetPieceType() == EPieceType::Pawn &&
             TargetGridPosition == CurrentGS->GetEnPassantTargetSquare() &&
             CurrentGS->GetEnPassantPawnToCapture() != nullptr)
    {
        APawnPiece* EnPassantCapturedPawn = CurrentGS->GetEnPassantPawnToCapture();
        if (EnPassantCapturedPawn)
        {
            UE_LOG(LogTemp, Log, TEXT("Executing Move: Capturing %s at (%d, %d) via En Passant."),
                   *UEnum::GetValueAsString(EnPassantCapturedPawn->GetPieceType()),
                   EnPassantCapturedPawn->GetBoardPosition().X, EnPassantCapturedPawn->GetBoardPosition().Y);
            CurrentGS->RemovePieceFromState(EnPassantCapturedPawn);
            GameBoard->ClearSquare(EnPassantCapturedPawn->GetBoardPosition());
            EnPassantCapturedPawn->OnCaptured();
            EnPassantCapturedPawn->Destroy();
        }
    }

    GameBoard->ClearSquare(OriginalPosition);
    GameBoard->SetPieceAtGridPosition(PieceToMove, TargetGridPosition);

    PieceToMove->NotifyMoveCompleted();

    // --- Проверка на превращение пешки ---
    APawnPiece* MovedPawn = Cast<APawnPiece>(PieceToMove);
    if (MovedPawn)
    {
        const int32 PromotionRank = (MovedPawn->GetPieceColor() == EPieceColor::White) ? GameBoard->GetBoardSize().Y - 1 : 0;
        if (TargetGridPosition.Y == PromotionRank)
        {
            CurrentGS->SetGamePhase(EGamePhase::AwaitingPromotion);
            CurrentGS->SetPawnToPromote(MovedPawn);
            if (RequestingController)
            {
                RequestingController->Client_ShowPromotionMenu(MovedPawn);
            }
            UE_LOG(LogTemp, Log, TEXT("Pawn promotion initiated for pawn at (%d, %d). Awaiting player choice."), TargetGridPosition.X, TargetGridPosition.Y);
            // НЕ вызываем EndTurn() здесь. Ждем выбора игрока.
            UE_LOG(LogTemp, Log, TEXT("--- AttemptMove END (Success - Promotion Pending) ---"));
            return true;
        }
    }
    // --- Конец проверки на превращение ---

    // --- Обновление состояния игры для FEN ---
    CurrentGS->UpdateCastlingRights(PieceToMove, OriginalPosition);
    if (bIsCapture)
    {
        // Если захвачена ладья, это также влияет на права рокировки
        CurrentGS->UpdateCastlingRights(CapturedPiece, CapturedPiece->GetBoardPosition());
    }
    
    if (bIsPawnMove || bIsCapture)
    {
        CurrentGS->ResetHalfmoveClock();
    }
    else
    {
        CurrentGS->IncrementHalfmoveClock();
    }
    // --- Конец обновления состояния игры ---

    if (bIsPawnTwoStep)
    {
        if (MovedPawn)
        {
            int32 Direction = (MovedPawn->GetPieceColor() == EPieceColor::White) ? 1 : -1;
            FIntPoint EnPassantSquare = FIntPoint(OriginalPosition.X, OriginalPosition.Y + Direction);
            CurrentGS->SetEnPassantData(EnPassantSquare, MovedPawn);
        }
    }

    GameBoard->ClearAllHighlights();

    EndTurn();

    UE_LOG(LogTemp, Log, TEXT("--- AttemptMove END (Success) ---"));
    return true;
}

void AChessGameMode::SpawnInitialPieces()
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (!CurrentGS || !GameBoard)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::SpawnInitialPieces: GameState or GameBoard is null. Cannot spawn pieces."));
        return;
    }

    // Очищаем все существующие фигуры перед спавном новых
    for (AChessPiece* Piece : CurrentGS->ActivePieces)
    {
        if (Piece)
        {
            Piece->Destroy();
        }
    }
    CurrentGS->ActivePieces.Empty();

    const FIntPoint BoardSize = GameBoard->GetBoardSize();
    const int32 LastRow = BoardSize.Y - 1;
    const int32 SecondToLastRow = BoardSize.Y - 2;

    // Расставляем пешки
    for (int32 i = 0; i < BoardSize.X; ++i)
    {
        SpawnPieceAtPosition(EPieceType::Pawn, EPieceColor::White, FIntPoint(i, 1));
        SpawnPieceAtPosition(EPieceType::Pawn, EPieceColor::Black, FIntPoint(i, SecondToLastRow));
    }

    // Расставляем основные фигуры симметрично
    // Ладьи
    SpawnPieceAtPosition(EPieceType::Rook, EPieceColor::White, FIntPoint(0, 0));
    SpawnPieceAtPosition(EPieceType::Rook, EPieceColor::White, FIntPoint(BoardSize.X - 1, 0));
    SpawnPieceAtPosition(EPieceType::Rook, EPieceColor::Black, FIntPoint(0, LastRow));
    SpawnPieceAtPosition(EPieceType::Rook, EPieceColor::Black, FIntPoint(BoardSize.X - 1, LastRow));

    // Кони
    SpawnPieceAtPosition(EPieceType::Knight, EPieceColor::White, FIntPoint(1, 0));
    SpawnPieceAtPosition(EPieceType::Knight, EPieceColor::White, FIntPoint(BoardSize.X - 2, 0));
    SpawnPieceAtPosition(EPieceType::Knight, EPieceColor::Black, FIntPoint(1, LastRow));
    SpawnPieceAtPosition(EPieceType::Knight, EPieceColor::Black, FIntPoint(BoardSize.X - 2, LastRow));

    // Слоны
    SpawnPieceAtPosition(EPieceType::Bishop, EPieceColor::White, FIntPoint(2, 0));
    SpawnPieceAtPosition(EPieceType::Bishop, EPieceColor::White, FIntPoint(BoardSize.X - 3, 0));
    SpawnPieceAtPosition(EPieceType::Bishop, EPieceColor::Black, FIntPoint(2, LastRow));
    SpawnPieceAtPosition(EPieceType::Bishop, EPieceColor::Black, FIntPoint(BoardSize.X - 3, LastRow));

    // Ферзь и Король
    // Для стандартной доски 8x8, король на E, ферзь на D.
    // Для досок другого размера, размещаем их по центру.
    const int32 KingPositionX = FMath::FloorToInt(BoardSize.X / 2.0f);
    const int32 QueenPositionX = KingPositionX - 1;

    SpawnPieceAtPosition(EPieceType::Queen, EPieceColor::White, FIntPoint(QueenPositionX, 0));
    SpawnPieceAtPosition(EPieceType::King, EPieceColor::White, FIntPoint(KingPositionX, 0));
    SpawnPieceAtPosition(EPieceType::Queen, EPieceColor::Black, FIntPoint(QueenPositionX, LastRow));
    SpawnPieceAtPosition(EPieceType::King, EPieceColor::Black, FIntPoint(KingPositionX, LastRow));

    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Initial pieces spawned dynamically for a %dx%d board."), BoardSize.X, BoardSize.Y);
}

AChessPiece* AChessGameMode::SpawnPieceAtPosition(EPieceType Type, EPieceColor Color, const FIntPoint& GridPosition)
{
    if (!GameBoard)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::SpawnPieceAtPosition: GameBoard is null. Cannot spawn piece."));
        return nullptr;
    }

    TSubclassOf<AChessPiece> PieceClassToSpawn = nullptr;
    switch (Type)
    {
        case EPieceType::Pawn:   PieceClassToSpawn = PawnClass; break;
        case EPieceType::Rook:   PieceClassToSpawn = RookClass; break;
        case EPieceType::Knight: PieceClassToSpawn = KnightClass; break;
        case EPieceType::Bishop: PieceClassToSpawn = BishopClass; break;
        case EPieceType::Queen:  PieceClassToSpawn = QueenClass; break;
        case EPieceType::King:   PieceClassToSpawn = KingClass; break;
        default:
            UE_LOG(LogTemp, Warning, TEXT("AChessGameMode::SpawnPieceAtPosition: Unknown piece type %d"), (uint8)Type);
            return nullptr;
    }

    if (!PieceClassToSpawn)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::SpawnPieceAtPosition: Piece class for type %d is not set in GameMode."), (uint8)Type);
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = GetInstigator();
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    FVector WorldLocation = GameBoard->GridToWorldPosition(GridPosition);
    const FRotator Rotation = (Color == EPieceColor::White) ? FRotator(0.f, 180.f, 0.f) : FRotator::ZeroRotator;

    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Attempting to spawn %s %s at Grid (%d, %d) -> World (X=%.2f, Y=%.2f, Z=%.2f)"),
        *StaticEnum<EPieceColor>()->GetNameStringByValue(static_cast<int64>(Color)),
        *StaticEnum<EPieceType>()->GetNameStringByValue(static_cast<int64>(Type)),
        GridPosition.X, GridPosition.Y,
        WorldLocation.X, WorldLocation.Y, WorldLocation.Z);

    // Спавним актора в (0,0,0), чтобы избежать смещений из-за коллизии при спавне,
    // а затем принудительно перемещаем его в нужную точку.
    AChessPiece* NewPiece = GetWorld()->SpawnActor<AChessPiece>(PieceClassToSpawn, FVector::ZeroVector, Rotation, SpawnParams);
    if (NewPiece)
    {
        NewPiece->InitializePiece(Color, Type, GridPosition);
        
        // Принудительно устанавливаем позицию ПОСЛЕ инициализации.
        NewPiece->SetActorLocation(WorldLocation, false, nullptr, ETeleportType::TeleportPhysics);

        // Финальный отладочный лог для проверки фактического положения после всех манипуляций.
        const FVector FinalActorLocation = NewPiece->GetActorLocation();
        UE_LOG(LogTemp, Warning, TEXT("AChessGameMode: %s %s spawned and placed at Grid (%d, %d). Final World Location: (X=%.2f, Y=%.2f, Z=%.2f)"),
            *StaticEnum<EPieceColor>()->GetNameStringByValue(static_cast<int64>(Color)),
            *StaticEnum<EPieceType>()->GetNameStringByValue(static_cast<int64>(Type)),
            GridPosition.X, GridPosition.Y,
            FinalActorLocation.X, FinalActorLocation.Y, FinalActorLocation.Z);

        UStaticMesh* MeshToSet = nullptr;
        const TMap<EPieceType, TObjectPtr<UStaticMesh>>* BlueprintMeshesMap = (Color == EPieceColor::White) ? &WhitePieceMeshes : &BlackPieceMeshes;

        const FString ColorName = (Color == EPieceColor::White ? TEXT("White") : TEXT("Black"));
        const FString TypeName = StaticEnum<EPieceType>()->GetNameStringByValue(static_cast<int64>(Type));

        const TObjectPtr<UStaticMesh>* FoundBlueprintMeshEntry = BlueprintMeshesMap->Find(Type);

        if (FoundBlueprintMeshEntry && *FoundBlueprintMeshEntry)
        {
            MeshToSet = (*FoundBlueprintMeshEntry).Get();
        }
        else
        {
            UE_LOG(LogTemp, Error, 
                TEXT("AChessGameMode::SpawnPieceAtPosition: CRITICAL - StaticMesh for %s %s is NOT configured in the GameMode Blueprint. Piece will have NO MESH. Please configure it in the 'Chess Setup|Meshes' section."),
                *ColorName, *TypeName);
        }

        if (MeshToSet)
        {
            NewPiece->SetPieceMesh(MeshToSet);
        }
        
        AChessGameState* CurrentGS = GetCurrentGameState();
        if (CurrentGS)
        {
            CurrentGS->AddPieceToState(NewPiece);
        }

        // Также регистрируем фигуру на самой доске, чтобы ее можно было выбрать кликом
        GameBoard->SetPieceAtGridPosition(NewPiece, GridPosition);
        
        UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Spawned %s %s at (%d, %d)"),
               (Color == EPieceColor::White ? TEXT("White") : TEXT("Black")),
               *UEnum::GetValueAsString(Type),
               GridPosition.X, GridPosition.Y);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::SpawnPieceAtPosition: Failed to spawn piece of type %d."), (uint8)Type);
    }
    return NewPiece;
}

void AChessGameMode::CheckGameEndConditions()
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (!CurrentGS || !GameBoard)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::CheckGameEndConditions: GameState or GameBoard is null."));
        return;
    }

    EPieceColor CurrentTurnColor = CurrentGS->GetCurrentTurnColor();
    EPieceColor OpponentColor = (CurrentTurnColor == EPieceColor::White) ? EPieceColor::Black : EPieceColor::White;

    if (CurrentGS->IsPlayerInCheck(CurrentTurnColor, GameBoard))
    {
        UE_LOG(LogTemp, Log, TEXT("AChessGameMode: %s is in check."), (CurrentTurnColor == EPieceColor::White ? TEXT("White") : TEXT("Black")));
        CurrentGS->SetGamePhase(EGamePhase::Check);

        if (CurrentGS->IsPlayerInCheckmate(CurrentTurnColor, GameBoard))
        {
            UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Checkmate! %s wins!"), (OpponentColor == EPieceColor::White ? TEXT("White") : TEXT("Black")));
            CurrentGS->SetGamePhase((OpponentColor == EPieceColor::White) ? EGamePhase::WhiteWins : EGamePhase::BlackWins);
        }
    }
    else
    {
        if (CurrentGS->IsStalemate(CurrentTurnColor, GameBoard))
        {
            UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Stalemate! Game is a draw."));
            CurrentGS->SetGamePhase(EGamePhase::Stalemate);
        }
        else
        {
            CurrentGS->SetGamePhase(EGamePhase::InProgress);
        }
    }
}
