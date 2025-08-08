#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ChessPiece.h" // Для EPieceColor, EPieceType
#include "Math/IntPoint.h" // Для FIntPoint
// #include "DatabaseManager.h"
#include "ChessGameMode.generated.h"

class UStockfishManager;

UENUM(BlueprintType)
enum class ETimeControlType : uint8
{
	Unlimited,
	Bullet_1_0,
	Blitz_3_2,
	Rapid_10_0
};

UENUM(BlueprintType)
enum class EGameModeType : uint8
{
    PlayerVsPlayer,
    PlayerVsBot
};

UENUM(BlueprintType)
enum class EPlayerColorPreference : uint8
{
    White   UMETA(DisplayName = "White"),
    Black   UMETA(DisplayName = "Black"),
    Random  UMETA(DisplayName = "Random")
};

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

// Forward declarations
class USoundBase;
class AChessPlayerController;
class AChessGameState;
class AChessBoard;
class UStaticMesh;
class APawnPiece;
// AChessPiece уже включен через ChessPiece.h

UCLASS()
class RTX_CHESS_API AChessGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AChessGameMode();

    // Вызывается в начале игры или при загрузке уровня
    virtual void BeginPlay() override;
    // GameModeBase не имеет Tick по умолчанию. Если нужен Tick, его нужно включить в конструкторе: PrimaryActorTick.bCanEverTick = true;
    virtual void Tick(float DeltaTime) override; 

    // Инициализирует и начинает новую шахматную партию
    UFUNCTION(BlueprintCallable, Category = "Chess Game Mode")
    void StartNewGame();

    UFUNCTION(BlueprintCallable, Category = "Chess Game Mode")
    void StartBotGame();

    /**
     * Задает выбор цвета игрока для следующей игры с ботом. Должна вызываться из UI перед StartBotGame.
     * @param ColorChoice Выбор игрока: Белый, Черный или Случайный.
     */
    UFUNCTION(BlueprintCallable, Category = "Chess Game Mode")
    void SetPlayerColorForBotGame(EPlayerColorPreference ColorChoice);

    /**
     * Задает выбор цвета игрока для следующей игры с ботом на основе числового значения (например, от слайдера).
     * @param ChoiceIndex 0 для Белых, 1 для Случайного, 2 для Черных.
     */
    UFUNCTION(BlueprintCallable, Category = "Chess Game Mode")
    void SetPlayerColorForBotGameFromInt(int32 ChoiceIndex);

    // Завершает текущий ход и передает управление следующему игроку
    UFUNCTION(BlueprintCallable, Category = "Chess Game Mode")
    void EndTurn();

    // Функции HandlePieceClicked и HandleSquareClicked удалены, так как логика выбора и перемещения
    // теперь полностью обрабатывается в AChessPlayerController с помощью drag-and-drop.

    // Пытается выполнить ход указанной фигурой на целевую клетку
    // PieceToMove - фигура, которую пытаются переместить
    // TargetGridPosition - целевая позиция на доске
    // RequestingController - контроллер игрока, запрашивающего ход
    // Возвращает true, если ход был успешным
    UFUNCTION(BlueprintCallable, Category = "Chess Game Mode")
    bool AttemptMove(AChessPiece* PieceToMove, const FIntPoint& TargetGridPosition, AChessPlayerController* RequestingController);

    // Вызывается контроллером для завершения превращения пешки
    void CompletePawnPromotion(APawnPiece* PawnToPromote, EPieceType PromoteToType);

    UFUNCTION(BlueprintPure, Category = "Chess Game Mode")
    UStockfishManager* GetStockfishManager() const;

    UFUNCTION(BlueprintPure, Category = "Chess Game Mode")
    EGameModeType GetCurrentGameModeType() const;

    UFUNCTION(BlueprintPure, Category = "Chess Game Mode")
    ETimeControlType GetCurrentTimeControl() const;

    float GetBurnEffectDuration() const;
    FVector GetBurnEffectScale() const;

protected:
    // UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Database")
    // TObjectPtr<UDatabaseManager> DatabaseManager;

    // Ссылка на объект шахматной доски на уровне. Должна быть установлена в BeginPlay.
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Chess Game Mode", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<AChessBoard> GameBoard;

    // Ссылка на выбранную фигуру удалена из GameMode.
    // Теперь AChessPlayerController отслеживает выбранную фигуру для операции drag-and-drop.

    UPROPERTY()
    TObjectPtr<UStockfishManager> StockfishManager;

    EGameModeType CurrentGameMode;

    /** Выбор цвета игрока для игры с ботом. Устанавливается через SetPlayerColorForBotGame. По умолчанию - Случайный. */
    EPlayerColorPreference PlayerColorChoiceForBotGame;

    ETimeControlType CurrentTimeControl;

    int32 BotSkillLevel;

    int32 TimeIncrementSeconds;

    /** Задержка в секундах перед ходом бота. Позволяет анимации хода игрока завершиться. */
    UPROPERTY(EditDefaultsOnly, Category = "Chess Setup|Bot")
    float BotMoveDelay;

    /** Диапазон силы импульса (Min, Max), применяемого к фигуре при ее взятии. Итоговое значение будет случайным в этом диапазоне. */
    UPROPERTY(EditDefaultsOnly, Category = "Chess Setup|Effects", meta = (DisplayName = "Capture Push Force Range"))
    FVector2D CaptureImpulseStrengthRange;

    /** Диапазон силы крутящего момента (Min, Max), применяемого к фигуре при ее взятии. Итоговое значение будет случайным в этом диапазоне. */
    UPROPERTY(EditDefaultsOnly, Category = "Chess Setup|Effects", meta = (DisplayName = "Capture Torque Strength Range"))
    FVector2D CaptureTorqueStrengthRange;

    /** Длительность эффекта сгорания в секундах. */
    UPROPERTY(EditDefaultsOnly, Category = "Chess Setup|Effects", meta = (ClampMin = "0.1", DisplayName = "Burn Effect Duration"))
    float BurnEffectDuration;

    /** Масштаб эффекта сгорания. Позволяет управлять размером эффекта. */
    UPROPERTY(EditDefaultsOnly, Category = "Chess Setup|Effects", meta = (DisplayName = "Burn Effect Scale"))
    FVector BurnEffectScale;

    /** Таймер для отложенного хода бота. */
    FTimerHandle BotMoveTimerHandle;

    /** Цвет фигур, за которые играет бот. Устанавливается в StartBotGame. */
    EPieceColor BotColor;

    void MakeBotMove();

    void GetTimeControlSettings(ETimeControlType InControlType, int32& OutStartTime, int32& OutIncrement) const;

    // Общая функция для настройки доски и состояния игры
    void SetupBoardAndGameState(int32 StartTime, int32 Increment);

    UFUNCTION()
    void HandleBotMoveReceived(const FString& BestMove);

    // Классы фигур для спавна. Устанавливаются в Blueprint или C++ конструкторе.
    UPROPERTY(EditDefaultsOnly, Category = "Chess Setup")
    TSubclassOf<AChessPiece> PawnClass;
    UPROPERTY(EditDefaultsOnly, Category = "Chess Setup")
    TSubclassOf<AChessPiece> RookClass;
    UPROPERTY(EditDefaultsOnly, Category = "Chess Setup")
    TSubclassOf<AChessPiece> KnightClass;
    UPROPERTY(EditDefaultsOnly, Category = "Chess Setup")
    TSubclassOf<AChessPiece> BishopClass;
    UPROPERTY(EditDefaultsOnly, Category = "Chess Setup")
    TSubclassOf<AChessPiece> QueenClass;
    UPROPERTY(EditDefaultsOnly, Category = "Chess Setup")
    TSubclassOf<AChessPiece> KingClass;

    // Меши для фигур, настраиваемые в Blueprint.
    // Если здесь не указан меш для какого-либо типа фигуры, будет использован стандартный меш из C++.
    UPROPERTY(EditDefaultsOnly, Category = "Chess Setup|Meshes", meta=(DisplayName="White Piece Meshes (Blueprint)"))
    TMap<EPieceType, TObjectPtr<UStaticMesh>> WhitePieceMeshes;

    UPROPERTY(EditDefaultsOnly, Category = "Chess Setup|Meshes", meta=(DisplayName="Black Piece Meshes (Blueprint)"))
    TMap<EPieceType, TObjectPtr<UStaticMesh>> BlackPieceMeshes;

    // Расставляет фигуры на доске в начальные позиции
    // Вызывается из StartNewGame
    virtual void SpawnInitialPieces();

    // Спавнит фигуру указанного типа и цвета в заданной позиции на доске
    // Type - тип создаваемой фигуры (Пешка, Ладья и т.д.)
    // Color - цвет создаваемой фигуры (Белый, Черный)
    // GridPosition - позиция на доске для спавна
    // Возвращает указатель на созданную фигуру или nullptr в случае ошибки
    virtual AChessPiece* SpawnPieceAtPosition(EPieceType Type, EPieceColor Color, const FIntPoint& GridPosition);

    // Проверяет условия окончания игры (мат, пат) и обновляет состояние игры (в ChessGameState)
    // Вызывается после каждого успешного хода
    virtual void CheckGameEndConditions();

    // Находит объект доски на сцене и сохраняет ссылку в GameBoard
    // Вызывается в BeginPlay
    virtual void FindGameBoard();

    /** Централизованно обрабатывает завершение игры: обновляет профили и показывает экран окончания игры. */
    void HandleGameOver(EGamePhase FinalPhase, const FText& Reason);

    // Вызывается, когда игрок успешно входит в игру
    virtual void PostLogin(APlayerController* NewPlayer) override;

    /** Выбирает стартовую точку для игрока в зависимости от его цвета (ищет PlayerStart с тегом "White" или "Black"). */
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
    // Счетчик подключенных игроков
    int32 NumberOfPlayers;

    /** ID текущей игры в базе данных. */
    // int64 CurrentGameDBId;

    // Вспомогательная функция для получения ChessGameState с проверкой типа
    AChessGameState* GetCurrentGameState() const;

    /** Сохраняет ход в базу данных. */
    // void RecordMove(const FString& MoveNotation);

    // Флаг для отслеживания случая, когда превращение пешки происходит в результате взятия.
    bool bIsPromotionAfterCapture;
};

