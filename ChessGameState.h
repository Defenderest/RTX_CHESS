#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ChessPiece.h" // Для EPieceColor и AChessPiece
#include "ChessGameMode.h" // Для EGameModeType
#include "Math/IntPoint.h" // Для FIntPoint
#include "Net/UnrealNetwork.h" // Для репликации
#include "ChessGameState.generated.h"

// Forward declarations
class AChessBoard;
class APawnPiece; // Для хранения ссылки на пешку для взятия на проходе

// Перечисление для текущей фазы игры
UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    WaitingToStart UMETA(DisplayName = "Waiting To Start"),
    InProgress UMETA(DisplayName = "In Progress"),
    AwaitingPromotion UMETA(DisplayName = "Awaiting Promotion"),
    Check UMETA(DisplayName = "Check"),
    WhiteWins UMETA(DisplayName = "White Wins"),
    BlackWins UMETA(DisplayName = "Black Wins"),
    Stalemate UMETA(DisplayName = "Stalemate"),
    Draw UMETA(DisplayName = "Draw")
};

UCLASS()
class RTX_CHESS_API AChessGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AChessGameState();

    // Определяет, чья очередь ходить
    UPROPERTY(ReplicatedUsing = OnRep_CurrentTurn, BlueprintReadOnly, Category = "Chess Game State")
    EPieceColor CurrentTurnColor;

    // Вызывается при изменении CurrentTurnColor для обновления на клиентах
    UFUNCTION()
    void OnRep_CurrentTurn();

    // Текущая фаза игры
    UPROPERTY(ReplicatedUsing = OnRep_GamePhase, BlueprintReadOnly, Category = "Chess Game State")
    EGamePhase CurrentGamePhase;

    // Вызывается при изменении CurrentGamePhase для обновления на клиентах
    UFUNCTION()
    void OnRep_GamePhase();

    // Текущий режим игры (PvP или PvE)
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    EGameModeType CurrentGameMode;

    // Массив всех активных фигур на доске
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    TArray<TObjectPtr<AChessPiece>> ActivePieces; // Используем TObjectPtr для безопасности

    // Клетка, на которую можно совершить взятие на проходе. FIntPoint(-1,-1) если нет такой возможности.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State") // VisibleInstanceOnly убрано, т.к. BlueprintReadOnly уже подразумевает видимость
    FIntPoint EnPassantTargetSquare;

    // Пешка, которая может быть взята на проходе. nullptr если нет такой возможности.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State") // VisibleInstanceOnly убрано
    TWeakObjectPtr<APawnPiece> EnPassantPawnToCapture;

    // Счетчик полуходов для правила 50 ходов
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    int32 HalfmoveClock;

    // Счетчик полных ходов
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    int32 FullmoveNumber;

    // Права на рокировку
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    bool bCanWhiteCastleKingSide;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    bool bCanWhiteCastleQueenSide;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    bool bCanBlackCastleKingSide;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    bool bCanBlackCastleQueenSide;

    // Пешка, ожидающая превращения
    UPROPERTY(Replicated)
    TObjectPtr<APawnPiece> PawnToPromote;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Возвращает цвет игрока, чей сейчас ход
    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    EPieceColor GetCurrentTurnColor() const;

    // Переключает ход на другого игрока (должен вызываться на сервере из GameMode)
    void Server_SwitchTurn();

    // Возвращает текущую фазу игры
    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    EGamePhase GetGamePhase() const;

    // Устанавливает новую фазу игры (должен вызываться на сервере из GameMode)
    void SetGamePhase(EGamePhase NewPhase);

    // Добавляет фигуру в список активных фигур (обычно при спавне)
    void AddPieceToState(AChessPiece* PieceToAdd);

    // Удаляет фигуру из списка активных фигур (обычно при захвате)
    void RemovePieceFromState(AChessPiece* PieceToRemove);

    // Возвращает фигуру, находящуюся в указанной клетке доски
    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    AChessPiece* GetPieceAtGridPosition(const FIntPoint& GridPosition) const;

    // Проверяет, находится ли указанный игрок под шахом
    // Board необходим для анализа возможных ходов фигур противника
    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    bool IsPlayerInCheck(EPieceColor PlayerColor, const AChessBoard* Board) const;

    // Проверяет, поставлен ли указанному игроку мат
    // Board необходим для анализа возможных ходов и состояния игры
    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    bool IsPlayerInCheckmate(EPieceColor PlayerColor, const AChessBoard* Board); // Removed const

    // Проверяет, находится ли игра в состоянии пата для указанного игрока
    // Board необходим для анализа возможных ходов
    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    bool IsStalemate(EPieceColor PlayerColor, const AChessBoard* Board); // Removed const

    // --- Pawn Promotion ---
    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    APawnPiece* GetPawnToPromote() const;
    void SetPawnToPromote(APawnPiece* Pawn);
    // --- End Pawn Promotion ---

public: // Changed from protected
    // --- Full Game State Management (called from GameMode) ---

    // Сбрасывает состояние игры в начальное положение.
    void ResetGameStateForNewGame();

    // Обновляет права на рокировку на основе перемещенной или захваченной фигуры.
    void UpdateCastlingRights(const AChessPiece* Piece, const FIntPoint& FromPosition);

    // Увеличивает счетчик полных ходов.
    void IncrementFullmoveNumber();

    // Увеличивает счетчик полуходов.
    void IncrementHalfmoveClock();

    // Сбрасывает счетчик полуходов.
    void ResetHalfmoveClock();
    
    // Внутренний метод для изменения цвета текущего хода, вызывается Server_SwitchTurn
    void SetCurrentTurnColor(EPieceColor NewTurnColor);

    // --- En Passant Logic ---
    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    FIntPoint GetEnPassantTargetSquare() const;

    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    APawnPiece* GetEnPassantPawnToCapture() const;

    // Устанавливает данные для возможного взятия на проходе (вызывается GameMode)
    // Должен вызываться на сервере
    void SetEnPassantData(const FIntPoint& TargetSquare, APawnPiece* PawnToCapture);

    // Очищает данные о взятии на проходе (вызывается GameMode)
    // Должен вызываться на сервере
    void ClearEnPassantData();

    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    FString GetFEN() const;

    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    EGameModeType GetCurrentGameModeType() const;

    void SetCurrentGameMode(EGameModeType NewMode);
};
