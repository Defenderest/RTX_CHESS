#include "Core/ChessGameMode.h"
// #include "DatabaseManager.h"
#include "Actors/PlayerPawn.h"
#include "Controllers/ChessPlayerController.h"
#include "Core/ChessPlayerState.h"
#include "Core/ChessGameInstance.h"
#include "Core/ChessGameState.h"
#include "Board/ChessBoard.h"
#include "Pieces/ChessPiece.h"
#include "Pieces/PawnPiece.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Managers/StockfishManager.h"
#include "Controllers/ChessPlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Core/RatingEngine.h"

AChessGameMode::AChessGameMode()
{
    PrimaryActorTick.bCanEverTick = true;

    // DatabaseManager = CreateDefaultSubobject<UDatabaseManager>(TEXT("DatabaseManager"));
    // CurrentGameDBId = -1;

    GameStateClass = AChessGameState::StaticClass();
    PlayerStateClass = AChessPlayerState::StaticClass();
    DefaultPawnClass = APlayerPawn::StaticClass();

    StockfishManager = CreateDefaultSubobject<UStockfishManager>(TEXT("StockfishManager"));
    CurrentGameMode = EGameModeType::PlayerVsPlayer;
    NumberOfPlayers = 0;
    BotSkillLevel = 10; // Глубина поиска для бота (1-15)
    BotMoveDelay = 1.0f; // Задержка в 1 секунду по умолчанию
    bIsPromotionAfterCapture = false;
    PlayerColorChoiceForBotGame = EPlayerColorPreference::Random;
    BotColor = EPieceColor::Black; // По умолчанию бот играет за черных
    CurrentTimeControl = ETimeControlType::Unlimited;
    TimeIncrementSeconds = 0;

    // Значения подобраны для фигур с массой ~1кг.
    CaptureImpulseStrengthRange = FVector2D(12000.f, 18000.f);
    CaptureTorqueStrengthRange = FVector2D(8000000.f, 12000000.f);

    // Настройки эффекта сгорания
    BurnEffectDuration = 1.5f;
    BurnEffectScale = FVector(1.0f);
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

    // Устанавливаем контроль времени из параметров запуска
    const FString TimeControlValue = UGameplayStatics::ParseOption(this->OptionsString, TEXT("TimeControl"));
    if (!TimeControlValue.IsEmpty())
    {
        const int32 TimeControlIndex = FCString::Atoi(*TimeControlValue);
        if (ETimeControlType::Unlimited <= (ETimeControlType)TimeControlIndex && (ETimeControlType)TimeControlIndex <= ETimeControlType::Rapid_10_0)
        {
            CurrentTimeControl = (ETimeControlType)TimeControlIndex;
            UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Time control set to index %d from launch options."), TimeControlIndex);
        }
    }

    // Мы начинаем игру, только если в опциях запуска явно указан режим игры С БОТОМ.
    // Это позволяет при первом запуске просто показать главное меню. Сетевые игры (PvP)
    // запускаются автоматически из PostLogin, когда подключается второй игрок.
    const FString IsBotGameValue = UGameplayStatics::ParseOption(this->OptionsString, TEXT("bIsBotGame"));
    if (IsBotGameValue.ToBool())
    {
        // Устанавливаем цвет игрока из параметров запуска, если он был передан
        const FString ColorChoiceValue = UGameplayStatics::ParseOption(this->OptionsString, TEXT("ColorChoice"));
        if (!ColorChoiceValue.IsEmpty())
        {
            const int32 ChoiceIndex = FCString::Atoi(*ColorChoiceValue);
            SetPlayerColorForBotGameFromInt(ChoiceIndex);
            UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Player color preference set to index %d from launch options."), ChoiceIndex);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessGameMode: ColorChoice option not found for bot game. Using default random color."));
        }

        // Запускаем игру против бота
        StartBotGame();

        // Устанавливаем уровень сложности бота из параметров запуска
        if (StockfishManager)
        {
            const FString SkillLevelValue = UGameplayStatics::ParseOption(this->OptionsString, TEXT("SkillLevel"));
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
    // Если опция не найдена или это PvP, ничего не делаем. GameState останется в EGamePhase::WaitingToStart,
    // и PlayerController покажет стартовое меню. Игра начнется, когда подключится второй игрок.
    
    // Подключаемся к базе данных
    // if (DatabaseManager)
    // {
    //     DatabaseManager->Connect();
    // }
}

void AChessGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Проверка на окончание времени
    if (CurrentTimeControl != ETimeControlType::Unlimited)
    {
        AChessGameState* CurrentGS = GetCurrentGameState();
        if (CurrentGS && (CurrentGS->GetGamePhase() == EGamePhase::InProgress || CurrentGS->GetGamePhase() == EGamePhase::Check))
        {
            if (CurrentGS->WhiteTimeSeconds <= 0.f)
            {
                HandleGameOver(EGamePhase::BlackWins, FText::FromString(TEXT("Время вышло")));
            }
            else if (CurrentGS->BlackTimeSeconds <= 0.f)
            {
                HandleGameOver(EGamePhase::WhiteWins, FText::FromString(TEXT("Время вышло")));
            }
        }
    }
}

void AChessGameMode::GetTimeControlSettings(ETimeControlType InControlType, int32& OutStartTime, int32& OutIncrement) const
{
    OutStartTime = -1; // -1 означает бесконечное время
    OutIncrement = 0;

    switch (InControlType)
    {
    case ETimeControlType::Bullet_1_0:
        OutStartTime = 60; // 1 минута
        OutIncrement = 0;
        break;
    case ETimeControlType::Blitz_3_2:
        OutStartTime = 180; // 3 минуты
        OutIncrement = 2;
        break;
    case ETimeControlType::Rapid_10_0:
        OutStartTime = 600; // 10 минут
        OutIncrement = 0;
        break;
    case ETimeControlType::Unlimited:
    default:
        // Значения по умолчанию уже установлены
        break;
    }
}

void AChessGameMode::StartBotGame()
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    CurrentGameMode = EGameModeType::PlayerVsBot;
    if (CurrentGS)
    {
        CurrentGS->SetCurrentGameMode(CurrentGameMode);
    }
    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Starting new Player vs Bot game."));

    // --- Определение цвета игрока и бота ---
    EPieceColor PlayerColor = EPieceColor::White; // Цвет игрока по умолчанию
    
    if (PlayerColorChoiceForBotGame == EPlayerColorPreference::Random)
    {
        PlayerColorChoiceForBotGame = (FMath::RandBool()) ? EPlayerColorPreference::White : EPlayerColorPreference::Black;
    }

    if (PlayerColorChoiceForBotGame == EPlayerColorPreference::White)
    {
        PlayerColor = EPieceColor::White;
        BotColor = EPieceColor::Black;
    }
    else // Black
    {
        PlayerColor = EPieceColor::Black;
        BotColor = EPieceColor::White;
    }
    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Player color set to %s, Bot color set to %s."), 
        (PlayerColor == EPieceColor::White ? TEXT("White") : TEXT("Black")), 
        (BotColor == EPieceColor::White ? TEXT("White") : TEXT("Black")));
    // --- Конец определения цвета ---

    if (StockfishManager)
    {
        UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Initializing API-based bot."));
        StockfishManager->InitializeLocalEngine(); // Explicitly start the engine process here
        StockfishManager->OnBestMoveReceived.AddDynamic(this, &AChessGameMode::HandleBotMoveReceived);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::StartBotGame: StockfishManager is NULL! Cannot start engine."));
    }

    if (CurrentGS)
    {
        FPlayerProfile BotProfile;
        BotProfile.PlayerName = TEXT("Stockfish");
        BotProfile.EloRating = 100 * BotSkillLevel; 
        BotProfile.Country = TEXT("AI");

        FPlayerProfile PlayerProfile;
        if (UChessGameInstance* GI = GetGameInstance<UChessGameInstance>())
        {
            PlayerProfile = GI->GetPlayerProfile();
        }

        if (PlayerColor == EPieceColor::White)
        {
            CurrentGS->WhitePlayerProfile = PlayerProfile;
            CurrentGS->BlackPlayerProfile = BotProfile;
        }
        else
        {
            CurrentGS->WhitePlayerProfile = BotProfile;
            CurrentGS->BlackPlayerProfile = PlayerProfile;
        }
        CurrentGS->OnRep_PlayerProfiles();
    }
    
    int32 StartTime, Increment;
    GetTimeControlSettings(CurrentTimeControl, StartTime, Increment);
    SetupBoardAndGameState(StartTime, Increment);

    // Оповещаем игрока о начале игры, спавним его пешку и УСТАНАВЛИВАЕМ ЦВЕТ
    if (APlayerController* PC_Raw = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        if (AChessPlayerController* PC = Cast<AChessPlayerController>(PC_Raw))
        {
            // Устанавливаем цвет игрока. Это свойство реплицируется на клиент.
            PC->SetPlayerColor(PlayerColor);

            // Добавляем задержку перед тем, как оповестить клиента о начале игры
            // и заспавнить его пешку. Это дает время для репликации цвета игрока
            // на клиент, предотвращая гонку состояний, когда логика камеры или UI
            // пытается определиться с перспективой до того, как станет известен
            // правильный цвет. Это устраняет "дрожание" камеры при игре за черных.
            FTimerHandle DeferredStartTimerHandle;
            GetWorldTimerManager().SetTimer(DeferredStartTimerHandle, [this, PC]()
            {
                if (PC)
                {
                    // Теперь, когда цвет должен был реплицироваться, оповещаем клиента.
                    PC->Client_GameStarted();

                    // Спавним пешку.
                    RestartPlayer(PC);
                    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Deferred game start and pawn spawn executed."));
                }
            }, 0.2f, false);
        }
    }

    // Если бот играет за белых, он должен сделать первый ход
    if (CurrentGS && CurrentGS->GetCurrentTurnColor() == BotColor)
    {
        GetWorldTimerManager().SetTimer(BotMoveTimerHandle, this, &AChessGameMode::MakeBotMove, BotMoveDelay, false);
    }
}

void AChessGameMode::SetPlayerColorForBotGame(EPlayerColorPreference ColorChoice)
{
    PlayerColorChoiceForBotGame = ColorChoice;
    FString ChoiceStr;
    switch(ColorChoice)
    {
        case EPlayerColorPreference::White: ChoiceStr = "White"; break;
        case EPlayerColorPreference::Black: ChoiceStr = "Black"; break;
        case EPlayerColorPreference::Random: ChoiceStr = "Random"; break;
    }
    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Player color preference for next bot game set to %s."), *ChoiceStr);
}

void AChessGameMode::SetPlayerColorForBotGameFromInt(int32 ChoiceIndex)
{
    EPlayerColorPreference ColorChoice = EPlayerColorPreference::Random; // Значение по умолчанию
    switch (ChoiceIndex)
    {
    case 0:
        ColorChoice = EPlayerColorPreference::White;
        break;
    case 1:
        ColorChoice = EPlayerColorPreference::Random;
        break;
    case 2:
        ColorChoice = EPlayerColorPreference::Black;
        break;
    default:
        UE_LOG(LogTemp, Warning, TEXT("SetPlayerColorForBotGameFromInt: Неверный ChoiceIndex %d. Используется 'Случайный'."), ChoiceIndex);
        break;
    }
    // Используем уже существующую функцию, чтобы не дублировать логику
    SetPlayerColorForBotGame(ColorChoice);
}

void AChessGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    AChessPlayerController* ChessController = Cast<AChessPlayerController>(NewPlayer);
    if (ChessController)
    {
        NumberOfPlayers++;
        // Логика для ботов была перенесена в StartBotGame
        if (CurrentGameMode == EGameModeType::PlayerVsPlayer)
        {
            if (NumberOfPlayers == 1)
            {
                UE_LOG(LogTemp, Log, TEXT("AChessGameMode::PostLogin: Player 1 joined. Waiting for Player 2."));
            }
            else if (NumberOfPlayers == 2)
            {
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

AActor* AChessGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    AChessPlayerController* ChessController = Cast<AChessPlayerController>(Player);
    if (!ChessController)
    {
        // Fallback для контроллеров, не являющихся шахматными, или если приведение типа не удалось
        return Super::ChoosePlayerStart_Implementation(Player);
    }

    const FString ExpectedTag = (ChessController->GetPlayerColor() == EPieceColor::White) ? TEXT("White") : TEXT("Black");
    UE_LOG(LogTemp, Log, TEXT("Choosing player start for %s player."), *ExpectedTag);

    TArray<AActor*> PlayerStarts;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);

    for (AActor* Start : PlayerStarts)
    {
        if (Start->ActorHasTag(FName(*ExpectedTag)))
        {
            UE_LOG(LogTemp, Log, TEXT("Found matching PlayerStart: %s"), *GetNameSafe(Start));
            return Start;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("No PlayerStart with tag '%s' found. Falling back to default behavior."), *ExpectedTag);
    // Fallback на любую доступную стартовую точку, если точка с тегом не найдена
    return Super::ChoosePlayerStart_Implementation(Player);
}


void AChessGameMode::MakeBotMove()
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (CurrentGS && StockfishManager)
    {
        UE_LOG(LogTemp, Log, TEXT("ChessGameMode: Requesting bot move."));
        FString FEN = CurrentGS->GetFEN();

        // Определяем вариативность хода в зависимости от уровня сложности бота
        int32 MultiPV = 1; // По умолчанию - только лучший ход
        if (BotSkillLevel <= 4) // Низкий уровень сложности
        {
            MultiPV = 5; // Просим 5 лучших ходов для выбора
        }
        else if (BotSkillLevel <= 9) // Средний уровень
        {
            MultiPV = 3; // Просим 3 лучших хода
        }
        // Для высокого уровня (10+) MultiPV остается 1.

        UE_LOG(LogTemp, Log, TEXT("ChessGameMode: Requesting %d best moves (variations) for bot skill level %d."), MultiPV, BotSkillLevel);

        // Запрашиваем ход, результат придет асинхронно в HandleBotMoveReceived
        StockfishManager->RequestBestMove(FEN, BotSkillLevel, MultiPV);
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
    if (CurrentGS->GetCurrentTurnColor() != BotColor)
    {
        UE_LOG(LogTemp, Warning, TEXT("ChessGameMode::HandleBotMoveReceived: Received bot move, but it's not the Bot's turn anymore. Ignoring move."));
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

ETimeControlType AChessGameMode::GetCurrentTimeControl() const
{
    return CurrentTimeControl;
}

float AChessGameMode::GetBurnEffectDuration() const
{
    return BurnEffectDuration;
}

FVector AChessGameMode::GetBurnEffectScale() const
{
    return BurnEffectScale;
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
    AChessGameState* CurrentGS = GetCurrentGameState();
    CurrentGameMode = EGameModeType::PlayerVsPlayer;
    if (CurrentGS)
    {
        CurrentGS->SetCurrentGameMode(CurrentGameMode);
    }
    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Starting new Player vs Player game."));

    int32 StartTime, Increment;
    GetTimeControlSettings(CurrentTimeControl, StartTime, Increment);
    SetupBoardAndGameState(StartTime, Increment);

    // Назначаем цвета и оповещаем игроков о начале игры
    AChessPlayerController* WhitePlayer = nullptr;
    AChessPlayerController* BlackPlayer = nullptr;
    bool bWhitePlayerAssigned = false;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (AChessPlayerController* PC = Cast<AChessPlayerController>(It->Get()))
        {
             // For host, load profile from save game directly into its PlayerState
            if (PC->IsLocalController() && !GetWorld()->IsNetMode(NM_Client))
            {
                if (UChessGameInstance* GI = GetGameInstance<UChessGameInstance>())
                {
                    if (AChessPlayerState* PS = PC->GetPlayerState<AChessPlayerState>())
                    {
                        PS->SetPlayerProfile(GI->GetPlayerProfile());
                    }
                }
            }

            if (!bWhitePlayerAssigned)
            {
                WhitePlayer = PC;
                bWhitePlayerAssigned = true;
            }
            else
            {
                BlackPlayer = PC;
            }
        }
    }
    
    if (WhitePlayer)
    {
        WhitePlayer->SetPlayerColor(EPieceColor::White);
        UE_LOG(LogTemp, Log, TEXT("AChessGameMode::StartNewGame: Assigning White to PlayerController %s."), *WhitePlayer->GetName());
        if (AChessPlayerState* PS = WhitePlayer->GetPlayerState<AChessPlayerState>())
        {
            CurrentGS->WhitePlayerProfile = PS->GetPlayerProfile();
        }
        WhitePlayer->Client_GameStarted();
        RestartPlayer(WhitePlayer);
    }

    if (BlackPlayer)
    {
        BlackPlayer->SetPlayerColor(EPieceColor::Black);
        UE_LOG(LogTemp, Log, TEXT("AChessGameMode::StartNewGame: Assigning Black to PlayerController %s."), *BlackPlayer->GetName());
        if (AChessPlayerState* PS = BlackPlayer->GetPlayerState<AChessPlayerState>())
        {
            CurrentGS->BlackPlayerProfile = PS->GetPlayerProfile();
        }
        BlackPlayer->Client_GameStarted();
        RestartPlayer(BlackPlayer);
    }

    if (CurrentGS)
    {
        CurrentGS->OnRep_PlayerProfiles();

        // if (DatabaseManager)
        // {
        //     DatabaseManager->SaveNewGame(CurrentGS->WhitePlayerProfile.PlayerName, CurrentGS->BlackPlayerProfile.PlayerName, CurrentGameDBId);
        // }
    }
}

void AChessGameMode::SetupBoardAndGameState(int32 StartTime, int32 Increment)
{
    TimeIncrementSeconds = Increment;
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

    CurrentGS->WhiteTimeSeconds = StartTime;
    CurrentGS->BlackTimeSeconds = StartTime;

    CurrentGS->SetCurrentTurnColor(EPieceColor::White);
    CurrentGS->SetGamePhase(EGamePhase::InProgress);

    // Оповещаем всех игроков о начале игры, проигрывая звук
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (AChessPlayerController* PC = Cast<AChessPlayerController>(It->Get()))
        {
            PC->Client_PlaySound(EChessSoundType::GameStart);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Board and game state have been reset. White's turn."));
}

#include "Controllers/ChessPlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

// void AChessGameMode::RecordMove(const FString& MoveNotation)
// {
    // if (DatabaseManager && CurrentGameDBId != -1)
    // {
    //     AChessGameState* CurrentGS = GetCurrentGameState();
    //     if (CurrentGS)
    //     {
    //         FString FEN = CurrentGS->GetFEN();
    //         
    //         // Вычисляем номер полухода
    //         int32 MoveNumber = (CurrentGS->GetFullmoveNumber() - 1) * 2;
    //         // EndTurn уже переключил ход, поэтому логика обратная:
    //         // если сейчас ход черных, значит, только что сходили белые.
    //         if (CurrentGS->GetCurrentTurnColor() == EPieceColor::Black)
    //         {
    //             MoveNumber += 1; // Ход белых
    //         }
    //         else
    //         {
    //             MoveNumber += 2; // Ход черных
    //         }
    //         
    //         DatabaseManager->SaveMove(CurrentGameDBId, MoveNumber, MoveNotation, FEN);
    //     }
    // }
// }

void AChessGameMode::EndTurn()
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (CurrentGS)
    {
        // Добавляем инкремент, если он есть
        if (TimeIncrementSeconds > 0)
        {
            if (CurrentGS->GetCurrentTurnColor() == EPieceColor::White)
            {
                CurrentGS->WhiteTimeSeconds += TimeIncrementSeconds;
            }
            else
            {
                CurrentGS->BlackTimeSeconds += TimeIncrementSeconds;
            }
        }

        // Увеличиваем номер полного хода, если ход сделали черные
        if (CurrentGS->GetCurrentTurnColor() == EPieceColor::Black)
        {
            CurrentGS->IncrementFullmoveNumber();
        }

        CurrentGS->Server_SwitchTurn();
        UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Turn ended. Now %s's turn."), (CurrentGS->GetCurrentTurnColor() == EPieceColor::White ? TEXT("White") : TEXT("Black")));
        
        CheckGameEndConditions();

        if (CurrentGameMode == EGameModeType::PlayerVsBot && CurrentGS->GetCurrentTurnColor() == BotColor)
        {
            const EGamePhase CurrentPhase = CurrentGS->GetGamePhase();
            if (CurrentPhase == EGamePhase::InProgress || CurrentPhase == EGamePhase::Check)
            {
                // Используем таймер, чтобы создать задержку перед ходом бота.
                // Это дает время для завершения анимации хода игрока.
                GetWorldTimerManager().SetTimer(BotMoveTimerHandle, this, &AChessGameMode::MakeBotMove, BotMoveDelay, false);
            }
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

        // Логика задержки для новой фигуры удалена, так как Client_ShowDelayedPiece больше не используется.
        // Новая фигура просто появится на доске.
        if (bIsPromotionAfterCapture)
        {
            bIsPromotionAfterCapture = false; // Сбрасываем флаг
        }
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

    // --- Симуляция хода с учетом взятия на проходе и рокировки для проверки шаха ---
    APawnPiece* EnPassantPawnToCaptureSim = nullptr;
    if (PieceToMove->GetPieceType() == EPieceType::Pawn && 
        TargetGridPosition == CurrentGS->GetEnPassantTargetSquare())
    {
        EnPassantPawnToCaptureSim = CurrentGS->GetEnPassantPawnToCapture();
    }
    
    bool bIsCastlingSim = PieceToMove->GetPieceType() == EPieceType::King && FMath::Abs(TargetGridPosition.X - OriginalPosition.X) == 2;
    AChessPiece* RookToMoveSim = nullptr;
    FIntPoint RookOriginalPosSim, RookTargetPosSim;

    // Временно "делаем" ход, чтобы проверить состояние доски после него
    CurrentGS->RemovePieceFromState(PieceToMove);
    if (CapturedPiece) { CurrentGS->RemovePieceFromState(CapturedPiece); }
    if (EnPassantPawnToCaptureSim) { CurrentGS->RemovePieceFromState(EnPassantPawnToCaptureSim); }
    PieceToMove->SetBoardPosition(TargetGridPosition);
    CurrentGS->AddPieceToState(PieceToMove);

    if (bIsCastlingSim)
    {
        if (TargetGridPosition.X > OriginalPosition.X) // Рокировка в сторону короля
        {
            RookOriginalPosSim = FIntPoint(GameBoard->GetBoardSize().X - 1, OriginalPosition.Y);
            RookTargetPosSim = FIntPoint(OriginalPosition.X + 1, OriginalPosition.Y);
        }
        else // Рокировка в сторону ферзя
        {
            RookOriginalPosSim = FIntPoint(0, OriginalPosition.Y);
            RookTargetPosSim = FIntPoint(OriginalPosition.X - 1, OriginalPosition.Y);
        }
        RookToMoveSim = CurrentGS->GetPieceAtGridPosition(RookOriginalPosSim);
        if (RookToMoveSim)
        {
            CurrentGS->RemovePieceFromState(RookToMoveSim);
            RookToMoveSim->SetBoardPosition(RookTargetPosSim);
            CurrentGS->AddPieceToState(RookToMoveSim);
        }
    }

    const bool bIsInCheckAfterMove = CurrentGS->IsPlayerInCheck(CurrentGS->GetCurrentTurnColor(), GameBoard);

    // Отменяем временный ход
    CurrentGS->RemovePieceFromState(PieceToMove);
    PieceToMove->SetBoardPosition(OriginalPosition);
    CurrentGS->AddPieceToState(PieceToMove);
    if (CapturedPiece) { CurrentGS->AddPieceToState(CapturedPiece); }
    if (EnPassantPawnToCaptureSim) { CurrentGS->AddPieceToState(EnPassantPawnToCaptureSim); }
    if (bIsCastlingSim && RookToMoveSim)
    {
        CurrentGS->RemovePieceFromState(RookToMoveSim);
        RookToMoveSim->SetBoardPosition(RookOriginalPosSim);
        CurrentGS->AddPieceToState(RookToMoveSim);
    }
    // --- Конец симуляции ---

    if (bIsInCheckAfterMove)
    {
        UE_LOG(LogTemp, Warning, TEXT("AttemptMove FAIL: Move would put King in check."));
        return false;
    }
    UE_LOG(LogTemp, Log, TEXT("AttemptMove PASS: King safety check passed."));

    // --- Все проверки пройдены, выполняем ход ---
    UE_LOG(LogTemp, Log, TEXT("--- ALL CHECKS PASSED: EXECUTING MOVE ---"));

    // --- Специальная обработка рокировки ---
    bool bIsCastlingMove = false;
    if (PieceToMove->GetPieceType() == EPieceType::King && FMath::Abs(TargetGridPosition.X - OriginalPosition.X) == 2)
    {
        bIsCastlingMove = true;
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
    bool bIsEnPassantCapture = false;
    APawnPiece* EnPassantCapturedPawn = nullptr;

    if (!bIsCapture && bIsPawnMove &&
        TargetGridPosition == CurrentGS->GetEnPassantTargetSquare() &&
        CurrentGS->GetEnPassantPawnToCapture() != nullptr)
    {
        bIsEnPassantCapture = true;
        EnPassantCapturedPawn = CurrentGS->GetEnPassantPawnToCapture();
    }
    const bool bIsAnyCapture = bIsCapture || bIsEnPassantCapture;

    // Сбрасываем флаг перед проверкой
    bIsPromotionAfterCapture = false;

    // --- Обработка взятия (если оно есть) ---
    if (bIsAnyCapture)
    {
        AChessPiece* PieceToDestroy = bIsCapture ? CapturedPiece : EnPassantCapturedPawn;

        if (PieceToDestroy)
        {
            UE_LOG(LogTemp, Log, TEXT("Capture detected. Initiating physics push out."));

            // 1. Вызываем новую логику захвата на фигуре, которая обработает физику и самоуничтожение.
            // PieceToMove - это фигура, которая совершает ход и "толкает" другую.
            const FVector BoardCenter = GameBoard ? GameBoard->GetActorLocation() : FVector::ZeroVector;
            const float Impulse = FMath::RandRange(CaptureImpulseStrengthRange.X, CaptureImpulseStrengthRange.Y);
            const float Torque = FMath::RandRange(CaptureTorqueStrengthRange.X, CaptureTorqueStrengthRange.Y);
            PieceToDestroy->HandleCapture(PieceToMove, BoardCenter, Impulse, Torque);

            // 2. Логически удаляем фигуру из состояния игры и с доски.
            // Физический актор останется в мире на некоторое время, пока не сработает его таймер уничтожения.
            CurrentGS->RemovePieceFromState(PieceToDestroy);
            GameBoard->ClearSquare(PieceToDestroy->GetBoardPosition());
            PieceToDestroy->OnCaptured(); // Вызываем для обратной совместимости с BP и для логгирования
        }
    }
    
    // --- Логическое перемещение фигуры ---
    GameBoard->ClearSquare(OriginalPosition);
    GameBoard->SetPieceAtGridPosition(PieceToMove, TargetGridPosition);
    PieceToMove->NotifyMoveCompleted();

    // --- Проверка на превращение пешки ---
    APawnPiece* MovedPawn = Cast<APawnPiece>(PieceToMove);
    bool bIsPromotion = false;
    if (MovedPawn)
    {
        const int32 PromotionRank = (MovedPawn->GetPieceColor() == EPieceColor::White) ? GameBoard->GetBoardSize().Y - 1 : 0;
        if (TargetGridPosition.Y == PromotionRank)
        {
            bIsPromotion = true;
        }
    }

    // --- Обработка визуальных эффектов для взятия ---
    if (bIsAnyCapture && bIsPromotion)
    {
        // Это взятие с превращением. Устанавливаем флаг.
        bIsPromotionAfterCapture = true;
        // Визуальная логика задержки фигуры удалена. Анимация хода будет проигрываться как обычно.
    }

    if (bIsPromotion)
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
    // --- Конец проверки на превращение ---

    // --- Обновление состояния игры для FEN ---
    CurrentGS->UpdateCastlingRights(PieceToMove, OriginalPosition);
    if (bIsCapture)
    {
        // CapturedPiece здесь уже невалиден, так как он уничтожен.
        // Логика обновления прав на рокировку при взятии ладьи должна быть пересмотрена,
        // но для текущей задачи это не критично.
    }
    
    if (bIsPawnMove || bIsAnyCapture)
    {
        CurrentGS->ResetHalfmoveClock();
    }
    else
    {
        CurrentGS->IncrementHalfmoveClock();
    }

    // Определяем, был ли это ход пешкой на две клетки.
    const bool bIsPawnTwoStep = (bIsPawnMove && FMath::Abs(TargetGridPosition.Y - OriginalPosition.Y) == 2);
    
    // Если это не ход пешкой на 2 клетки, флаг взятия на проходе сбрасывается.
    // Если это ход на 2 клетки, флаг будет установлен ниже.
    if (!bIsPawnTwoStep)
    {
        CurrentGS->ClearEnPassantData();
    }

    if (bIsPawnTwoStep)
    {
        if (MovedPawn)
        {
            // Только устанавливаем флаг взятия на проходе, если вражеская пешка действительно может его совершить.
            // Некоторые движки (включая, возможно, используемую версию Stockfish) считают FEN неверным,
            // если указано поле для взятия на проходе, но нет пешки, которая может выполнить этот ход.
            bool bIsEnPassantPossible = false;
            const EPieceColor OpponentColor = (MovedPawn->GetPieceColor() == EPieceColor::White) ? EPieceColor::Black : EPieceColor::White;

            const FIntPoint AdjacentPositions[] = {
                FIntPoint(TargetGridPosition.X - 1, TargetGridPosition.Y),
                FIntPoint(TargetGridPosition.X + 1, TargetGridPosition.Y)
            };

            for (const FIntPoint& Pos : AdjacentPositions)
            {
                if (GameBoard->IsValidGridPosition(Pos))
                {
                    if (const AChessPiece* AdjacentPiece = CurrentGS->GetPieceAtGridPosition(Pos))
                    {
                        if (AdjacentPiece->GetPieceType() == EPieceType::Pawn && AdjacentPiece->GetPieceColor() == OpponentColor)
                        {
                            bIsEnPassantPossible = true;
                            break;
                        }
                    }
                }
            }

            if (bIsEnPassantPossible)
            {
                const int32 Direction = (MovedPawn->GetPieceColor() == EPieceColor::White) ? 1 : -1;
                const FIntPoint EnPassantSquare = FIntPoint(OriginalPosition.X, OriginalPosition.Y + Direction);
                CurrentGS->SetEnPassantData(EnPassantSquare, MovedPawn);
            }
        }
    }

    // --- Воспроизведение звука ---
    EChessSoundType SoundToPlay;
    if (bIsCastlingMove)
    {
        SoundToPlay = EChessSoundType::Castle;
    }
    else if (bIsCapture || bIsEnPassantCapture)
    {
        SoundToPlay = EChessSoundType::Capture;
    }
    else
    {
        SoundToPlay = EChessSoundType::Move;
    }

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (AChessPlayerController* PC = Cast<AChessPlayerController>(It->Get()))
        {
            PC->Client_PlaySound(SoundToPlay);
        }
    }
    // --- Конец воспроизведения звука ---

    GameBoard->ClearAllHighlights();

    // Создаем простую нотацию хода до вызова EndTurn
    const FString MoveNotation = FString::Printf(TEXT("%c%d%c%d"),
        'a' + OriginalPosition.X, OriginalPosition.Y + 1,
        'a' + TargetGridPosition.X, TargetGridPosition.Y + 1);

    EndTurn();

    // Записываем ход в БД после EndTurn, чтобы FEN был корректным
    // RecordMove(MoveNotation);

    UE_LOG(LogTemp, Log, TEXT("--- AttemptMove END (Success) ---"));
    return true;
}

void AChessGameMode::HandleGameOver(EGamePhase FinalPhase, const FText& Reason)
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (!CurrentGS)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleGameOver: GameState is null."));
        return;
    }

    // Предотвращаем повторный вызов
    const EGamePhase CurrentPhase = CurrentGS->GetGamePhase();
    if (CurrentPhase != EGamePhase::InProgress && CurrentPhase != EGamePhase::Check)
    {
        return; // Игра уже завершилась
    }

    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Game Over! Final Phase: %s. Reason: %s"), *UEnum::GetValueAsString(FinalPhase), *Reason.ToString());

    // 1. Устанавливаем финальную фазу
    CurrentGS->SetGamePhase(FinalPhase);

    // 2. Обновляем статистику в профилях
    FPlayerProfile WhiteProfile = CurrentGS->WhitePlayerProfile;
    FPlayerProfile BlackProfile = CurrentGS->BlackPlayerProfile;

    if (FinalPhase == EGamePhase::WhiteWins)
    {
        WhiteProfile.GamesWon++;
        BlackProfile.GamesLost++;
        // Обновляем рейтинг только для игр между людьми
        if (CurrentGameMode == EGameModeType::PlayerVsPlayer)
        {
            RatingEngine::CalculateNewRatings(WhiteProfile, BlackProfile);
        }
    }
    else if (FinalPhase == EGamePhase::BlackWins)
    {
        WhiteProfile.GamesLost++;
        BlackProfile.GamesWon++;
        // Обновляем рейтинг только для игр между людьми
        if (CurrentGameMode == EGameModeType::PlayerVsPlayer)
        {
            RatingEngine::CalculateNewRatings(BlackProfile, WhiteProfile);
        }
    }
    else if (FinalPhase == EGamePhase::Stalemate || FinalPhase == EGamePhase::Draw)
    {
        WhiteProfile.GamesDrawn++;
        BlackProfile.GamesDrawn++;
        // Обновляем рейтинг только для игр между людьми
        if (CurrentGameMode == EGameModeType::PlayerVsPlayer)
        {
            RatingEngine::CalculateNewRatingsDraw(WhiteProfile, BlackProfile);
        }
    }
    
    // Обновляем профили в GameState для репликации
    CurrentGS->WhitePlayerProfile = WhiteProfile;
    CurrentGS->BlackPlayerProfile = BlackProfile;
    CurrentGS->OnRep_PlayerProfiles(); // Ручной вызов для сервера

    // 3. Сохраняем профили для локальных игроков
    if (UChessGameInstance* GI = GetGameInstance<UChessGameInstance>())
    {
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            if (AChessPlayerController* PC = Cast<AChessPlayerController>(It->Get()))
            {
                if (PC->IsLocalController())
                {
                    if (PC->GetPlayerColor() == EPieceColor::White)
                    {
                        GI->UpdatePlayerProfile(WhiteProfile);
                    }
                    else if (PC->GetPlayerColor() == EPieceColor::Black)
                    {
                        GI->UpdatePlayerProfile(BlackProfile);
                    }
                    GI->SavePlayerProfile();
                }
            }
        }
    }

    // 4. Формируем текст для экрана
    FText ResultText;
    switch (FinalPhase)
    {
        case EGamePhase::WhiteWins:
            ResultText = FText::FromString(TEXT("Победа белых!"));
            break;
        case EGamePhase::BlackWins:
            ResultText = FText::FromString(TEXT("Победа черных!"));
            break;
        case EGamePhase::Stalemate:
        case EGamePhase::Draw:
            ResultText = FText::FromString(TEXT("Ничья"));
            break;
        default:
            ResultText = FText::FromString(TEXT("Игра окончена"));
            break;
    }

    // 5. Показываем экран окончания игры всем игрокам
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (AChessPlayerController* PC = Cast<AChessPlayerController>(It->Get()))
        {
            PC->Client_ShowGameOverScreen(ResultText, Reason);
        }
    }
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
    const bool bWasInCheck = CurrentGS->GetGamePhase() == EGamePhase::Check;

    if (CurrentGS->IsPlayerInCheck(CurrentTurnColor, GameBoard))
    {
        if (!bWasInCheck) // Воспроизводим звук, только если это новый шах
        {
            for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            {
                if (AChessPlayerController* PC = Cast<AChessPlayerController>(It->Get()))
                {
                    PC->Client_PlaySound(EChessSoundType::Check);
                }
            }
        }
        
        UE_LOG(LogTemp, Log, TEXT("AChessGameMode: %s is in check."), (CurrentTurnColor == EPieceColor::White ? TEXT("White") : TEXT("Black")));
        CurrentGS->SetGamePhase(EGamePhase::Check);

        if (CurrentGS->IsPlayerInCheckmate(CurrentTurnColor, GameBoard))
        {
            const EGamePhase FinalPhase = (OpponentColor == EPieceColor::White) ? EGamePhase::WhiteWins : EGamePhase::BlackWins;
            HandleGameOver(FinalPhase, FText::FromString(TEXT("Мат")));
            
            for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            {
                if (AChessPlayerController* PC = Cast<AChessPlayerController>(It->Get()))
                {
                    PC->Client_PlaySound(EChessSoundType::Checkmate);
                }
            }
        }
    }
    else
    {
        if (CurrentGS->IsStalemate(CurrentTurnColor, GameBoard))
        {
            HandleGameOver(EGamePhase::Stalemate, FText::FromString(TEXT("Пат")));
        }
        else
        {
            CurrentGS->SetGamePhase(EGamePhase::InProgress);
        }
    }
}
