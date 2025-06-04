#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/IntPoint.h" // Для FIntPoint
#include "ChessBoard.generated.h"

// Forward declarations
class AChessPiece;

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

public:
    virtual void Tick(float DeltaTime) override;

    // Инициализирует доску, создает визуальное представление клеток (если необходимо)
    UFUNCTION(BlueprintCallable, Category = "Chess Board")
    virtual void InitializeBoard();

    // Возвращает размеры доски (количество клеток по X и Y)
    UFUNCTION(BlueprintPure, Category = "Chess Board")
    FIntPoint GetBoardSize() const;

    // Проверяет, являются ли указанные координаты (X, Y) допустимыми на доске
    UFUNCTION(BlueprintPure, Category = "Chess Board")
    bool IsValidGridPosition(const FIntPoint& GridPosition) const;

    // Возвращает фигуру, находящуюся в указанной клетке доски.
    // Может потребоваться взаимодействие с GameState или хранение ссылок на фигуры.
    // Для упрощения, эта функция может быть реализована в GameState, а Board будет ее вызывать.
    // Либо Board может хранить свою сетку ссылок на фигуры.
    UFUNCTION(BlueprintPure, Category = "Chess Board")
    AChessPiece* GetPieceAtGridPosition(const FIntPoint& GridPosition) const;

    // Устанавливает фигуру на указанную клетку доски.
    // Может включать обновление внутреннего представления доски.
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
    // Может хранить ссылки на фигуры на доске для быстрого доступа, если не используется GameState для этого.
    // TMap<FIntPoint, TWeakObjectPtr<AChessPiece>> PieceGrid; 
    // TWeakObjectPtr используется для избежания циклических ссылок, если фигуры также хранят ссылку на доску.
};
