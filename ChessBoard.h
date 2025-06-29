#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/IntPoint.h" // Для FIntPoint
#include "Components/StaticMeshComponent.h" // Для UStaticMeshComponent
#include "ChessBoard.generated.h"

// Forward declarations
class AChessPiece;
class UStaticMeshComponent;

UCLASS()
class RTX_CHESS_API AChessBoard : public AActor
{
    GENERATED_BODY()

public:
    AChessBoard();

protected:
    virtual void BeginPlay() override;

    // Размеры доски (например, 8x8 для стандартных шахмат)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chess Board Setup")
    FIntPoint BoardSize;

    // Размер одной клетки доски в мировых единицах Unreal Engine
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chess Board Setup")
    float TileSize;

    // Смещение по оси Z для фигур относительно поверхности клетки доски
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Board Setup")
    float PieceZOffsetOnBoard;

    // так как спавн теперь полностью контролируется GameMode.

public:
    virtual void Tick(float DeltaTime) override;

#if WITH_EDITORONLY_DATA
    // Включает/выключает отрисовку отладочной сетки для визуализации центров клеток
    UPROPERTY(EditAnywhere, Category = "Chess Board Setup|Debug")
    bool bDrawDebugGrid = true;
#endif

    // Инициализирует доску. Теперь эта функция в основном готовит доску
    // и ждет, пока GameMode расставит фигуры.
    UFUNCTION(BlueprintCallable, Category = "Chess Board")
    virtual void InitializeBoard();

    // Спавнит начальные фигуры на доске.
    // Эта функция теперь пуста и оставлена для обратной совместимости, если где-то вызывается.
    // Основная логика перенесена в AChessGameMode.
    UFUNCTION(BlueprintCallable, Category = "Chess Board", meta=(DeprecatedFunction, DeprecationMessage="Use GameMode's SpawnInitialPieces instead"))
    virtual void SpawnPieces();

    // Возвращает размеры доски (количество клеток по X и Y)
    UFUNCTION(BlueprintPure, Category = "Chess Board")
    FIntPoint GetBoardSize() const;

    // Проверяет, являются ли указанные координаты (X, Y) допустимыми на доске
    UFUNCTION(BlueprintPure, Category = "Chess Board")
    bool IsValidGridPosition(const FIntPoint& GridPosition) const;

    // Возвращает фигуру, находящуюся в указанной клетке доски.
    UFUNCTION(BlueprintPure, Category = "Chess Board")
    AChessPiece* GetPieceAtGridPosition(const FIntPoint& GridPosition) const;

    // Устанавливает фигуру на указанную клетку доски.
    UFUNCTION(BlueprintCallable, Category = "Chess Board")
    void SetPieceAtGridPosition(AChessPiece* Piece, const FIntPoint& GridPosition);

    // Очищает клетку доски от фигуры.
    UFUNCTION(BlueprintCallable, Category = "Chess Board")
    void ClearSquare(const FIntPoint& GridPosition);
    
    // Конвертирует координаты сетки доски в мировые координаты (центр клетки)
    UFUNCTION(BlueprintPure, Category = "Chess Board")
    FVector GridToWorldPosition(const FIntPoint& GridPosition) const;

    // Конвертирует мировые координаты в координаты сетки доски
    UFUNCTION(BlueprintPure, Category = "Chess Board")
    FIntPoint WorldToGridPosition(const FVector& WorldPosition) const;

    // Подсвечивает указанную клетку (например, для отображения возможных ходов)
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Chess Board Visuals")
    void HighlightSquare(const FIntPoint& GridPosition, FLinearColor HighlightColor);

    // Снимает подсветку со всех или с указанной клетки
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Chess Board Visuals")
    void ClearHighlight(const FIntPoint& GridPosition);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Chess Board Visuals")
    void ClearAllHighlights();

    // Проверяет, атакована ли указанная клетка фигурами заданного цвета
    UFUNCTION(BlueprintPure, Category = "Chess Board")
    bool IsSquareAttackedBy(const FIntPoint& SquarePosition, EPieceColor AttackingColor) const;

protected:
    // Компонент для отображения 3D модели доски
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> BoardMeshComponent;

#if WITH_EDITOR
    // Отладочная функция для отрисовки сетки
    void DrawDebugGrid() const;
#endif
};
