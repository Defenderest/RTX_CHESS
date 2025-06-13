#include "ChessGameMode.h"
#include "ChessPlayerController.h"
#include "ChessGameState.h"
#include "ChessBoard.h"
#include "ChessPiece.h"
#include "PawnPiece.h" // Добавлено для определения APawnPiece
#include "EngineUtils.h" // Для TActorIterator
#include "Engine/StaticMesh.h" // Для UStaticMesh
#include "UObject/ConstructorHelpers.h" // Для ConstructorHelpers::FObjectFinder
// #include "RookPiece.h" // Больше не нужен для NotifyMoveCompleted здесь
// #include "KingPiece.h" // Больше не нужен для NotifyMoveCompleted здесь

AChessGameMode::AChessGameMode()
{
    PrimaryActorTick.bCanEverTick = false; // Обычно GameMode не тикает каждый кадр

    // Установка классов по умолчанию
    // PlayerControllerClass = AChessPlayerController::StaticClass(); // Обычно устанавливается в Blueprints или DefaultGame.ini
    GameStateClass = AChessGameState::StaticClass(); // Устанавливаем класс состояния игры
    // DefaultPawnClass = nullptr; // Для шахмат игрок обычно не управляет пешкой напрямую

    // --- Загрузка стандартных мешей из C++ ---
    // ВАЖНО: Замените эти пути на актуальные пути к вашим ассетам статических мешей!
    TMap<EPieceType, FString> DefaultWhiteMeshAssetPaths;
    DefaultWhiteMeshAssetPaths.Add(EPieceType::Pawn,   TEXT("/Game/Defaults/Meshes/SM_Pawn_White.SM_Pawn_White"));
    DefaultWhiteMeshAssetPaths.Add(EPieceType::Rook,   TEXT("/Game/Defaults/Meshes/SM_Rook_White.SM_Rook_White"));
    DefaultWhiteMeshAssetPaths.Add(EPieceType::Knight, TEXT("/Game/Defaults/Meshes/SM_Knight_White.SM_Knight_White"));
    DefaultWhiteMeshAssetPaths.Add(EPieceType::Bishop, TEXT("/Game/Defaults/Meshes/SM_Bishop_White.SM_Bishop_White"));
    DefaultWhiteMeshAssetPaths.Add(EPieceType::Queen,  TEXT("/Game/Defaults/Meshes/SM_Queen_White.SM_Queen_White"));
    DefaultWhiteMeshAssetPaths.Add(EPieceType::King,   TEXT("/Game/Defaults/Meshes/SM_King_White.SM_King_White"));

    for (const auto& Pair : DefaultWhiteMeshAssetPaths)
    {
        ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(*Pair.Value);
        if (MeshFinder.Succeeded())
        {
            CppDefaultWhitePieceMeshes.Add(Pair.Key, MeshFinder.Object);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessGameMode Constructor: Failed to find C++ default WHITE %s static mesh at path: %s. Ensure this asset exists."), *StaticEnum<EPieceType>()->GetNameStringByValue(static_cast<int64>(Pair.Key)), *Pair.Value);
        }
    }

    TMap<EPieceType, FString> DefaultBlackMeshAssetPaths;
    DefaultBlackMeshAssetPaths.Add(EPieceType::Pawn,   TEXT("/Game/Defaults/Meshes/SM_Pawn_Black.SM_Pawn_Black"));
    DefaultBlackMeshAssetPaths.Add(EPieceType::Rook,   TEXT("/Game/Defaults/Meshes/SM_Rook_Black.SM_Rook_Black"));
    DefaultBlackMeshAssetPaths.Add(EPieceType::Knight, TEXT("/Game/Defaults/Meshes/SM_Knight_Black.SM_Knight_Black"));
    DefaultBlackMeshAssetPaths.Add(EPieceType::Bishop, TEXT("/Game/Defaults/Meshes/SM_Bishop_Black.SM_Bishop_Black"));
    DefaultBlackMeshAssetPaths.Add(EPieceType::Queen,  TEXT("/Game/Defaults/Meshes/SM_Queen_Black.SM_Queen_Black"));
    DefaultBlackMeshAssetPaths.Add(EPieceType::King,   TEXT("/Game/Defaults/Meshes/SM_King_Black.SM_King_Black"));

    for (const auto& Pair : DefaultBlackMeshAssetPaths)
    {
        ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(*Pair.Value);
        if (MeshFinder.Succeeded())
        {
            CppDefaultBlackPieceMeshes.Add(Pair.Key, MeshFinder.Object);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessGameMode Constructor: Failed to find C++ default BLACK %s static mesh at path: %s. Ensure this asset exists."), *StaticEnum<EPieceType>()->GetNameStringByValue(static_cast<int64>(Pair.Key)), *Pair.Value);
        }
    }
}

void AChessGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Дополнительное логирование для диагностики GameState
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

    FindGameBoard(); // Находим доску
    StartNewGame();  // Запускаем новую игру
}

AChessGameState* AChessGameMode::GetCurrentGameState() const
{
    return GetGameState<AChessGameState>();
}

void AChessGameMode::FindGameBoard()
{
    // Итерируемся по всем акторам типа AChessBoard и находим первый
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
    if (!GameBoard || !CurrentGS)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::StartNewGame: GameBoard or GameState is null. Cannot start new game."));
        return;
    }

    // Очищаем все существующие подсветки
    GameBoard->ClearAllHighlights();

    // Инициализируем доску (например, очищаем все клетки)
    GameBoard->InitializeBoard();

    // Спавним начальные фигуры
    SpawnInitialPieces();

    // Устанавливаем начальное состояние игры
    CurrentGS->SetCurrentTurnColor(EPieceColor::White); // Белые всегда начинают
    CurrentGS->SetGamePhase(EGamePhase::InProgress);

    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: New game started. White's turn."));
}

void AChessGameMode::EndTurn()
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (CurrentGS)
    {
        CurrentGS->Server_SwitchTurn();
        UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Turn ended. Now %s's turn."), (CurrentGS->GetCurrentTurnColor() == EPieceColor::White ? TEXT("White") : TEXT("Black")));
        CheckGameEndConditions();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::EndTurn: GameState is null. Cannot end turn."));
    }
}

void AChessGameMode::HandlePieceClicked(AChessPiece* ClickedPiece, AChessPlayerController* ByController)
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (!ClickedPiece || !GameBoard || !CurrentGS)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::HandlePieceClicked: Invalid input (ClickedPiece, GameBoard, or GameState is null)."));
        return;
    }

    // Если это не ход текущего игрока, или кликнутая фигура не принадлежит текущему игроку, игнорируем.
    if (ClickedPiece->GetPieceColor() != CurrentGS->GetCurrentTurnColor())
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessGameMode::HandlePieceClicked: Clicked piece does not belong to the current player."));
        return;
    }

    // Если фигура уже выбрана
    if (SelectedPiece)
    {
        // Если кликнута та же фигура, снимаем выделение
        if (SelectedPiece == ClickedPiece)
        {
            SelectedPiece->OnDeselected();
            GameBoard->ClearAllHighlights();
            SelectedPiece = nullptr;
            UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Deselected piece at (%d, %d)."), ClickedPiece->GetBoardPosition().X, ClickedPiece->GetBoardPosition().Y);
            return;
        }
        else // Выбрана другая фигура, снимаем выделение со старой
        {
            SelectedPiece->OnDeselected();
            GameBoard->ClearAllHighlights();
            SelectedPiece = nullptr; // Очищаем выбранную фигуру перед выбором новой
        }
    }

    // Выбираем новую фигуру
    SelectedPiece = ClickedPiece;
    SelectedPiece->OnSelected();
    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Selected %s %s at (%d, %d)."),
           (SelectedPiece->GetPieceColor() == EPieceColor::White ? TEXT("White") : TEXT("Black")),
           *UEnum::GetValueAsString(SelectedPiece->GetPieceType()),
           SelectedPiece->GetBoardPosition().X, SelectedPiece->GetBoardPosition().Y);

    // Подсвечиваем допустимые ходы
    TArray<FIntPoint> ValidMoves = SelectedPiece->GetValidMoves(GameBoard);
    for (const FIntPoint& Move : ValidMoves)
    {
        GameBoard->HighlightSquare(Move, FLinearColor::Green); // Подсвечиваем допустимые ходы зеленым
    }
    // Также подсвечиваем клетку выбранной фигуры
    GameBoard->HighlightSquare(SelectedPiece->GetBoardPosition(), FLinearColor::Blue);
}

void AChessGameMode::HandleSquareClicked(const FIntPoint& GridPosition, AChessPlayerController* ByController)
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (!GameBoard || !CurrentGS)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::HandleSquareClicked: GameBoard or GameState is null."));
        return;
    }

    // Если фигура выбрана, пытаемся ее переместить
    if (SelectedPiece)
    {
        if (AttemptMove(SelectedPiece, GridPosition, ByController))
        {
            // Ход успешен, SelectedPiece уже очищен в AttemptMove
            UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Move successful to (%d, %d)."), GridPosition.X, GridPosition.Y);
        }
        else
        {
            // Ход не удался, снимаем выделение с фигуры и очищаем подсветку
            UE_LOG(LogTemp, Warning, TEXT("AChessGameMode: Move failed to (%d, %d). Deselecting piece."), GridPosition.X, GridPosition.Y);
            SelectedPiece->OnDeselected();
            GameBoard->ClearAllHighlights();
            SelectedPiece = nullptr;
        }
    }
    else
    {
        // Фигура не выбрана, просто очищаем подсветку, если есть
        GameBoard->ClearAllHighlights();
        UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Square clicked at (%d, %d) with no piece selected. Clearing highlights."), GridPosition.X, GridPosition.Y);
    }
}

bool AChessGameMode::AttemptMove(AChessPiece* PieceToMove, const FIntPoint& TargetGridPosition, AChessPlayerController* RequestingController)
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (!PieceToMove || !GameBoard || !CurrentGS || !RequestingController)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::AttemptMove: Invalid input (PieceToMove, GameBoard, GameState, or RequestingController is null)."));
        return false;
    }

    // 1. Проверяем, принадлежит ли фигура текущему игроку
    if (PieceToMove->GetPieceColor() != CurrentGS->GetCurrentTurnColor())
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessGameMode::AttemptMove: Piece does not belong to the current player's turn."));
        return false;
    }

    // 2. Получаем допустимые ходы для фигуры
    TArray<FIntPoint> ValidMoves = PieceToMove->GetValidMoves(GameBoard);

    // 3. Проверяем, является ли целевая позиция допустимым ходом
    if (!ValidMoves.Contains(TargetGridPosition))
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessGameMode::AttemptMove: Target position (%d, %d) is not a valid move for %s at (%d, %d)."),
               TargetGridPosition.X, TargetGridPosition.Y,
               *UEnum::GetValueAsString(PieceToMove->GetPieceType()),
               PieceToMove->GetBoardPosition().X, PieceToMove->GetBoardPosition().Y);
        return false;
    }

    // --- Симулируем ход для проверки на самошах ---
    FIntPoint OriginalPosition = PieceToMove->GetBoardPosition();
    AChessPiece* CapturedPiece = CurrentGS->GetPieceAtGridPosition(TargetGridPosition);

    // Временно перемещаем фигуру
    CurrentGS->RemovePieceFromState(PieceToMove); // Удаляем со старой позиции
    if (CapturedPiece)
    {
        CurrentGS->RemovePieceFromState(CapturedPiece); // Временно удаляем захваченную фигуру
    }
    PieceToMove->SetBoardPosition(TargetGridPosition); // Обновляем внутреннюю позицию фигуры
    CurrentGS->AddPieceToState(PieceToMove); // Добавляем на новую позицию

    bool bIsInCheckAfterMove = CurrentGS->IsPlayerInCheck(CurrentGS->GetCurrentTurnColor(), GameBoard);

    // --- Отменяем симулированный ход ---
    CurrentGS->RemovePieceFromState(PieceToMove); // Удаляем с временной новой позиции
    PieceToMove->SetBoardPosition(OriginalPosition); // Возвращаем внутреннюю позицию фигуры
    CurrentGS->AddPieceToState(PieceToMove); // Добавляем обратно на исходную позицию
    if (CapturedPiece)
    {
        CurrentGS->AddPieceToState(CapturedPiece); // Добавляем захваченную фигуру обратно
    }

    if (bIsInCheckAfterMove)
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessGameMode::AttemptMove: Move would put King in check. Invalid move."));
        return false;
    }

    // --- Выполняем фактический ход ---

    // Перед выполнением хода, очистим предыдущие данные о взятии на проходе,
    // если только этот ход сам не создает новую возможность для взятия на проходе.
    bool bIsPawnTwoStep = false;
    if (PieceToMove->GetPieceType() == EPieceType::Pawn && FMath::Abs(TargetGridPosition.Y - OriginalPosition.Y) == 2)
    {
        bIsPawnTwoStep = true;
    }

    if (!bIsPawnTwoStep)
    {
        CurrentGS->ClearEnPassantData();
    }

    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Executing move for %s from (%d, %d) to (%d, %d)."),
           *UEnum::GetValueAsString(PieceToMove->GetPieceType()),
           OriginalPosition.X, OriginalPosition.Y,
           TargetGridPosition.X, TargetGridPosition.Y);

    // Обработка взятия (обычного или на проходе)
    if (CapturedPiece) // Обычное взятие
    {
        UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Capturing %s at (%d, %d)."),
               *UEnum::GetValueAsString(CapturedPiece->GetPieceType()),
               TargetGridPosition.X, TargetGridPosition.Y);
        CurrentGS->RemovePieceFromState(CapturedPiece);
        GameBoard->ClearSquare(CapturedPiece->GetBoardPosition()); // Очищаем клетку захваченной фигуры на доске
        CapturedPiece->OnCaptured(); // Уведомляем захваченную фигуру
        CapturedPiece->Destroy(); // Уничтожаем актора захваченной фигуры
    }
    else if (PieceToMove->GetPieceType() == EPieceType::Pawn &&
             TargetGridPosition == CurrentGS->GetEnPassantTargetSquare() &&
             CurrentGS->GetEnPassantPawnToCapture() != nullptr) // Взятие на проходе
    {
        APawnPiece* EnPassantCapturedPawn = CurrentGS->GetEnPassantPawnToCapture();
        if (EnPassantCapturedPawn)
        {
            UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Capturing %s at (%d, %d) via En Passant."),
                   *UEnum::GetValueAsString(EnPassantCapturedPawn->GetPieceType()),
                   EnPassantCapturedPawn->GetBoardPosition().X, EnPassantCapturedPawn->GetBoardPosition().Y);
            CurrentGS->RemovePieceFromState(EnPassantCapturedPawn);
            GameBoard->ClearSquare(EnPassantCapturedPawn->GetBoardPosition()); // Очищаем клетку захваченной пешки
            EnPassantCapturedPawn->OnCaptured();
            EnPassantCapturedPawn->Destroy();
        }
    }

    // Обновляем позицию фигуры на доске и в состоянии игры
    GameBoard->ClearSquare(OriginalPosition); // Очищаем старую клетку на доске
    PieceToMove->SetBoardPosition(TargetGridPosition); 
    
    // Обновляем мировое положение актора фигуры
    FVector NewWorldLocation = GameBoard->GridToWorldPosition(TargetGridPosition);
    PieceToMove->SetActorLocation(NewWorldLocation);
    
    GameBoard->SetPieceAtGridPosition(PieceToMove, TargetGridPosition); // Обновляем ссылку на доске

    // Уведомляем фигуру о завершении хода (для флагов первого хода пешки/ладьи/короля)
    PieceToMove->NotifyMoveCompleted();

    // Если это был двойной ход пешки, устанавливаем данные для взятия на проходе
    if (bIsPawnTwoStep)
    {
        APawnPiece* MovedPawn = Cast<APawnPiece>(PieceToMove);
        if (MovedPawn)
        {
            int32 Direction = (MovedPawn->GetPieceColor() == EPieceColor::White) ? 1 : -1;
            FIntPoint EnPassantSquare = FIntPoint(OriginalPosition.X, OriginalPosition.Y + Direction);
            CurrentGS->SetEnPassantData(EnPassantSquare, MovedPawn);
            UE_LOG(LogTemp, Log, TEXT("AChessGameMode: En Passant opportunity created at (%d, %d) for pawn at (%d, %d)"),
                EnPassantSquare.X, EnPassantSquare.Y, MovedPawn->GetBoardPosition().X, MovedPawn->GetBoardPosition().Y);
        }
    }

    // Очищаем подсветку после успешного хода
    GameBoard->ClearAllHighlights();
    SelectedPiece = nullptr; // Снимаем выделение с фигуры

    EndTurn(); // Переключаем ход и проверяем условия окончания игры

    return true;
}

void AChessGameMode::SpawnInitialPieces()
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (!CurrentGS)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::SpawnInitialPieces: GameState is null. Cannot spawn pieces."));
        return;
    }

    // Белые фигуры
    SpawnPieceAtPosition(EPieceType::Rook, EPieceColor::White, FIntPoint(0, 0));
    SpawnPieceAtPosition(EPieceType::Knight, EPieceColor::White, FIntPoint(1, 0));
    SpawnPieceAtPosition(EPieceType::Bishop, EPieceColor::White, FIntPoint(2, 0));
    SpawnPieceAtPosition(EPieceType::Queen, EPieceColor::White, FIntPoint(3, 0));
    SpawnPieceAtPosition(EPieceType::King, EPieceColor::White, FIntPoint(4, 0));
    SpawnPieceAtPosition(EPieceType::Bishop, EPieceColor::White, FIntPoint(5, 0));
    SpawnPieceAtPosition(EPieceType::Knight, EPieceColor::White, FIntPoint(6, 0));
    SpawnPieceAtPosition(EPieceType::Rook, EPieceColor::White, FIntPoint(7, 0));
    for (int32 i = 0; i < 8; ++i)
    {
        SpawnPieceAtPosition(EPieceType::Pawn, EPieceColor::White, FIntPoint(i, 1));
    }

    // Черные фигуры
    SpawnPieceAtPosition(EPieceType::Rook, EPieceColor::Black, FIntPoint(0, 7));
    SpawnPieceAtPosition(EPieceType::Knight, EPieceColor::Black, FIntPoint(1, 7));
    SpawnPieceAtPosition(EPieceType::Bishop, EPieceColor::Black, FIntPoint(2, 7));
    SpawnPieceAtPosition(EPieceType::Queen, EPieceColor::Black, FIntPoint(3, 7));
    SpawnPieceAtPosition(EPieceType::King, EPieceColor::Black, FIntPoint(4, 7));
    SpawnPieceAtPosition(EPieceType::Bishop, EPieceColor::Black, FIntPoint(5, 7));
    SpawnPieceAtPosition(EPieceType::Knight, EPieceColor::Black, FIntPoint(6, 7));
    SpawnPieceAtPosition(EPieceType::Rook, EPieceColor::Black, FIntPoint(7, 7));
    for (int32 i = 0; i < 8; ++i)
    {
        SpawnPieceAtPosition(EPieceType::Pawn, EPieceColor::Black, FIntPoint(i, 6));
    }

    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Initial pieces spawned."));
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
    FRotator Rotation = FRotator::ZeroRotator; // Поворот по умолчанию

    AChessPiece* NewPiece = GetWorld()->SpawnActor<AChessPiece>(PieceClassToSpawn, WorldLocation, Rotation, SpawnParams);
    if (NewPiece)
    {
        NewPiece->InitializePiece(Color, Type, GridPosition);
        // NewPiece->SetBoardPosition(GridPosition); // Позиция уже установлена в InitializePiece и используется для WorldLocation при спавне

        // Устанавливаем меш для фигуры
        UStaticMesh* MeshToSet = nullptr;
        const TMap<EPieceType, TObjectPtr<UStaticMesh>>* BlueprintMeshesMap = (Color == EPieceColor::White) ? &WhitePieceMeshes : &BlackPieceMeshes;
        const TMap<EPieceType, TObjectPtr<UStaticMesh>>* InternalDefaultMeshesMap = (Color == EPieceColor::White) ? &CppDefaultWhitePieceMeshes : &CppDefaultBlackPieceMeshes;

        const FString ColorName = (Color == EPieceColor::White ? TEXT("White") : TEXT("Black"));
        const FString TypeName = StaticEnum<EPieceType>()->GetNameStringByValue(static_cast<int64>(Type));

        const TObjectPtr<UStaticMesh>* FoundBlueprintMeshEntry = BlueprintMeshesMap->Find(Type);

        if (FoundBlueprintMeshEntry && *FoundBlueprintMeshEntry) // Найден в Blueprint и указывает на валидный меш
        {
            MeshToSet = FoundBlueprintMeshEntry->Get();
            // UE_LOG(LogTemp, Log, TEXT("AChessGameMode::SpawnPieceAtPosition: Using Blueprint-configured StaticMesh for %s %s."), *ColorName, *TypeName);
        }
        else // Не найден в Blueprint, или найден, но указатель на меш нулевой
        {
            if (FoundBlueprintMeshEntry) // Ключ был в карте Blueprint, но TObjectPtr был null
            {
                 UE_LOG(LogTemp, Warning,
                    TEXT("AChessGameMode::SpawnPieceAtPosition: StaticMesh for %s %s is configured in GameMode Blueprint's TMap, but the entry is NULL. Attempting to use C++ default."),
                    *ColorName, *TypeName);
            }
            else // Ключ не был найден в карте Blueprint
            {
                UE_LOG(LogTemp, Log, 
                    TEXT("AChessGameMode::SpawnPieceAtPosition: StaticMesh for %s %s is NOT configured in GameMode Blueprint's TMap. Attempting to use C++ default."),
                    *ColorName, *TypeName);
            }

            // Пытаемся использовать стандартные меши из C++
            const TObjectPtr<UStaticMesh>* FoundInternalMeshEntry = InternalDefaultMeshesMap->Find(Type);
            if (FoundInternalMeshEntry && *FoundInternalMeshEntry)
            {
                MeshToSet = FoundInternalMeshEntry->Get();
                UE_LOG(LogTemp, Log, TEXT("AChessGameMode::SpawnPieceAtPosition: Using C++ default StaticMesh for %s %s."), *ColorName, *TypeName);
            }
            else
            {
    
                 UE_LOG(LogTemp, Error, 
                    TEXT("AChessGameMode::SpawnPieceAtPosition: CRITICAL - StaticMesh for %s %s NOT found in Blueprint TMap AND C++ defaults are missing or invalid. Piece will have NO MESH. Check Blueprint configuration and C++ default asset paths for type '%s'."),
                    *ColorName, *TypeName, *TypeName);
            }
        }

        if (MeshToSet)
        {
            NewPiece->SetPieceMesh(MeshToSet);
        }
        // else: Ошибка об отсутствии меша уже залогирована выше.
        
        AChessGameState* CurrentGS = GetCurrentGameState();
        if (CurrentGS)
        {
            CurrentGS->AddPieceToState(NewPiece);
        }
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

    // Проверяем на шах
    if (CurrentGS->IsPlayerInCheck(CurrentTurnColor, GameBoard))
    {
        UE_LOG(LogTemp, Log, TEXT("AChessGameMode: %s is in check."), (CurrentTurnColor == EPieceColor::White ? TEXT("White") : TEXT("Black")));
        CurrentGS->SetGamePhase(EGamePhase::Check);

        // Проверяем на мат (если текущий игрок в шахе и у него нет допустимых ходов)
        if (CurrentGS->IsPlayerInCheckmate(CurrentTurnColor, GameBoard))
        {
            UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Checkmate! %s wins!"), (OpponentColor == EPieceColor::White ? TEXT("White") : TEXT("Black")));
            CurrentGS->SetGamePhase((OpponentColor == EPieceColor::White) ? EGamePhase::WhiteWins : EGamePhase::BlackWins);
        }
    }
    else
    {
        // Если не в шахе, проверяем на пат
        if (CurrentGS->IsStalemate(CurrentTurnColor, GameBoard))
        {
            UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Stalemate! Game is a draw."));
            CurrentGS->SetGamePhase(EGamePhase::Stalemate);
        }
        else
        {
            // Если не в шахе и не пат, игра продолжается
            CurrentGS->SetGamePhase(EGamePhase::InProgress);
        }
    }
}
