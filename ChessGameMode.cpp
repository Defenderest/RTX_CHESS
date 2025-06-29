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

AChessGameMode::AChessGameMode()
{
    PrimaryActorTick.bCanEverTick = false;

    GameStateClass = AChessGameState::StaticClass();

    StockfishManager = CreateDefaultSubobject<UStockfishManager>(TEXT("StockfishManager"));
    CurrentGameMode = EGameModeType::PlayerVsPlayer;

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
    // Do not start game automatically
    // StartNewGame(); 
}

void AChessGameMode::StartBotGame()
{
    CurrentGameMode = EGameModeType::PlayerVsBot;
    if (StockfishManager)
    {
        StockfishManager->StartEngine();
    }
    StartNewGame();
}

void AChessGameMode::MakeBotMove()
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (CurrentGS && StockfishManager)
    {
        FString FEN = CurrentGS->GetFEN();
        FString BestMove = StockfishManager->GetBestMove(FEN);

        if (!BestMove.IsEmpty())
        {
            FIntPoint StartPos((BestMove[0] - 'a'), (BestMove[1] - '1'));
            FIntPoint EndPos((BestMove[2] - 'a'), (BestMove[3] - '1'));

            AChessPiece* PieceToMove = GameBoard->GetPieceAtGridPosition(StartPos);
            if (PieceToMove)
            {
                AttemptMove(PieceToMove, EndPos, nullptr);
            }
        }
    }
}

AChessGameState* AChessGameMode::GetCurrentGameState() const
{
    return GetGameState<AChessGameState>();
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
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (!GameBoard || !CurrentGS)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::StartNewGame: GameBoard or GameState is null. Cannot start new game."));
        return;
    }

    GameBoard->ClearAllHighlights();

    GameBoard->InitializeBoard();

    SpawnInitialPieces();

    CurrentGS->SetCurrentTurnColor(EPieceColor::White);
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

void AChessGameMode::HandlePieceClicked(AChessPiece* ClickedPiece, AChessPlayerController* ByController)
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (!ClickedPiece || !GameBoard || !CurrentGS)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::HandlePieceClicked: Invalid input (ClickedPiece, GameBoard, or GameState is null)."));
        return;
    }

    // Отладочный лог для проверки цвета фигуры и текущего хода
    const FString PieceColorStr = StaticEnum<EPieceColor>()->GetNameStringByValue(static_cast<int64>(ClickedPiece->GetPieceColor()));
    const FString TurnColorStr = StaticEnum<EPieceColor>()->GetNameStringByValue(static_cast<int64>(CurrentGS->GetCurrentTurnColor()));
    UE_LOG(LogTemp, Log, TEXT("HandlePieceClicked: Clicked on a %s piece. Current turn is %s."), *PieceColorStr, *TurnColorStr);

    if (ClickedPiece->GetPieceColor() != CurrentGS->GetCurrentTurnColor())
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessGameMode::HandlePieceClicked: Clicked piece does not belong to the current player."));
        return;
    }

    if (SelectedPiece)
    {
        if (SelectedPiece == ClickedPiece)
        {
            SelectedPiece->OnDeselected();
            GameBoard->ClearAllHighlights();
            SelectedPiece = nullptr;
            UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Deselected piece at (%d, %d)."), ClickedPiece->GetBoardPosition().X, ClickedPiece->GetBoardPosition().Y);
            return;
        }
        else
        {
            SelectedPiece->OnDeselected();
            GameBoard->ClearAllHighlights();
            SelectedPiece = nullptr;
        }
    }

    SelectedPiece = ClickedPiece;
    SelectedPiece->OnSelected();
    UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Selected %s %s at (%d, %d)."),
           (SelectedPiece->GetPieceColor() == EPieceColor::White ? TEXT("White") : TEXT("Black")),
           *UEnum::GetValueAsString(SelectedPiece->GetPieceType()),
           SelectedPiece->GetBoardPosition().X, SelectedPiece->GetBoardPosition().Y);

    TArray<FIntPoint> ValidMoves = SelectedPiece->GetValidMoves(GameBoard);
    for (const FIntPoint& Move : ValidMoves)
    {
        GameBoard->HighlightSquare(Move, FLinearColor::Green);
    }
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

    if (SelectedPiece)
    {
        if (AttemptMove(SelectedPiece, GridPosition, ByController))
        {
            UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Move successful to (%d, %d)."), GridPosition.X, GridPosition.Y);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AChessGameMode: Move failed to (%d, %d). Deselecting piece."), GridPosition.X, GridPosition.Y);
            SelectedPiece->OnDeselected();
            GameBoard->ClearAllHighlights();
            SelectedPiece = nullptr;
        }
    }
    else
    {
        GameBoard->ClearAllHighlights();
        UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Square clicked at (%d, %d) with no piece selected. Clearing highlights."), GridPosition.X, GridPosition.Y);
    }
}

bool AChessGameMode::AttemptMove(AChessPiece* PieceToMove, const FIntPoint& TargetGridPosition, AChessPlayerController* RequestingController)
{
    AChessGameState* CurrentGS = GetCurrentGameState();
    if (!PieceToMove || !GameBoard || !CurrentGS)
    {
        UE_LOG(LogTemp, Error, TEXT("AChessGameMode::AttemptMove: Invalid input (PieceToMove, GameBoard, or GameState is null)."));
        return false;
    }

    if (PieceToMove->GetPieceColor() != CurrentGS->GetCurrentTurnColor())
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessGameMode::AttemptMove: Piece does not belong to the current player's turn."));
        return false;
    }

    TArray<FIntPoint> ValidMoves = PieceToMove->GetValidMoves(GameBoard);

    if (!ValidMoves.Contains(TargetGridPosition))
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessGameMode::AttemptMove: Target position (%d, %d) is not a valid move for %s at (%d, %d)."),
               TargetGridPosition.X, TargetGridPosition.Y,
               *UEnum::GetValueAsString(PieceToMove->GetPieceType()),
               PieceToMove->GetBoardPosition().X, PieceToMove->GetBoardPosition().Y);
        return false;
    }

    FIntPoint OriginalPosition = PieceToMove->GetBoardPosition();
    AChessPiece* CapturedPiece = CurrentGS->GetPieceAtGridPosition(TargetGridPosition);

    CurrentGS->RemovePieceFromState(PieceToMove);
    if (CapturedPiece)
    {
        CurrentGS->RemovePieceFromState(CapturedPiece);
    }
    PieceToMove->SetBoardPosition(TargetGridPosition);
    CurrentGS->AddPieceToState(PieceToMove);

    bool bIsInCheckAfterMove = CurrentGS->IsPlayerInCheck(CurrentGS->GetCurrentTurnColor(), GameBoard);

    CurrentGS->RemovePieceFromState(PieceToMove);
    PieceToMove->SetBoardPosition(OriginalPosition);
    CurrentGS->AddPieceToState(PieceToMove);
    if (CapturedPiece)
    {
        CurrentGS->AddPieceToState(CapturedPiece);
    }

    if (bIsInCheckAfterMove)
    {
        UE_LOG(LogTemp, Warning, TEXT("AChessGameMode::AttemptMove: Move would put King in check. Invalid move."));
        return false;
    }

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

    if (CapturedPiece)
    {
        UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Capturing %s at (%d, %d)."),
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
            UE_LOG(LogTemp, Log, TEXT("AChessGameMode: Capturing %s at (%d, %d) via En Passant."),
                   *UEnum::GetValueAsString(EnPassantCapturedPawn->GetPieceType()),
                   EnPassantCapturedPawn->GetBoardPosition().X, EnPassantCapturedPawn->GetBoardPosition().Y);
            CurrentGS->RemovePieceFromState(EnPassantCapturedPawn);
            GameBoard->ClearSquare(EnPassantCapturedPawn->GetBoardPosition());
            EnPassantCapturedPawn->OnCaptured();
            EnPassantCapturedPawn->Destroy();
        }
    }

    GameBoard->ClearSquare(OriginalPosition);
    PieceToMove->SetBoardPosition(TargetGridPosition); 
    
    FVector NewWorldLocation = GameBoard->GridToWorldPosition(TargetGridPosition);
    PieceToMove->SetActorLocation(NewWorldLocation);
    
    GameBoard->SetPieceAtGridPosition(PieceToMove, TargetGridPosition);

    PieceToMove->NotifyMoveCompleted();

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

    GameBoard->ClearAllHighlights();
    SelectedPiece = nullptr;

    EndTurn();

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
        const TMap<EPieceType, TObjectPtr<UStaticMesh>>* InternalDefaultMeshesMap = (Color == EPieceColor::White) ? &CppDefaultWhitePieceMeshes : &CppDefaultBlackPieceMeshes;

        const FString ColorName = (Color == EPieceColor::White ? TEXT("White") : TEXT("Black"));
        const FString TypeName = StaticEnum<EPieceType>()->GetNameStringByValue(static_cast<int64>(Type));

        const TObjectPtr<UStaticMesh>* FoundBlueprintMeshEntry = BlueprintMeshesMap->Find(Type);

        if (FoundBlueprintMeshEntry && *FoundBlueprintMeshEntry)
        {
            MeshToSet = FoundBlueprintMeshEntry->Get();
        }
        else
        {
            if (FoundBlueprintMeshEntry)
            {
                 UE_LOG(LogTemp, Warning,
                    TEXT("AChessGameMode::SpawnPieceAtPosition: StaticMesh for %s %s is configured in GameMode Blueprint's TMap, but the entry is NULL. Attempting to use C++ default."),
                    *ColorName, *TypeName);
            }
            else
            {
                UE_LOG(LogTemp, Log, 
                    TEXT("AChessGameMode::SpawnPieceAtPosition: StaticMesh for %s %s is NOT configured in GameMode Blueprint's TMap. Attempting to use C++ default."),
                    *ColorName, *TypeName);
            }

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
