#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ChessPiece.h" // Для EPieceColor, EPieceType
#include "Math/IntPoint.h" // Для FIntPoint
#include "ChessGameMode.generated.h"

// Forward declarations
class AChessPlayerController;
class AChessGameState;
class AChessBoard;
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
    // virtual void Tick(float DeltaTime) override; 

    // Инициализирует и начинает новую шахматную партию
    UFUNCTION(BlueprintCallable, Category = "Chess Game Mode")
    void StartNewGame();

    // Завершает текущий ход и передает управление следующему игроку
    UFUNCTION(BlueprintCallable, Category = "Chess Game Mode")
    void EndTurn();

    // Обрабатывает клик игрока по фигуре
    // ByController - контроллер игрока, который кликнул
    UFUNCTION(BlueprintCallable, Category = "Chess Game Mode")
    void HandlePieceClicked(AChessPiece* ClickedPiece, AChessPlayerController* ByController);

    // Обрабатыates клик игрока по клетке доски (для перемещения выбранной фигуры или выбора клетки)
    // GridPosition - координаты клетки, по которой кликнули
    // ByController - контроллер игрока, который кликнул
    UFUNCTION(BlueprintCallable, Category = "Chess Game Mode")
    void HandleSquareClicked(const FIntPoint& GridPosition, AChessPlayerController* ByController);

    // Пытается выполнить ход указанной фигурой на целевую клетку
    // PieceToMove - фигура, которую пытаются переместить
    // TargetGridPosition - целевая позиция на доске
    // RequestingController - контроллер игрока, запрашивающего ход
    // Возвращает true, если ход был успешным
    UFUNCTION(BlueprintCallable, Category = "Chess Game Mode")
    bool AttemptMove(AChessPiece* PieceToMove, const FIntPoint& TargetGridPosition, AChessPlayerController* RequestingController);

protected:
    // Ссылка на объект шахматной доски на уровне. Должна быть установлена в BeginPlay.
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Chess Game Mode", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<AChessBoard> GameBoard;

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

private:
    // Вспомогательная функция для получения ChessGameState с проверкой типа
    AChessGameState* GetCurrentGameState() const;
};
