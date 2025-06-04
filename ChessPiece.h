#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/IntPoint.h" // Для использования FIntPoint
#include "ChessPiece.generated.h"

// Forward declarations
class AChessBoard;

// Перечисление для цвета фигуры
UENUM(BlueprintType)
enum class EPieceColor : uint8
{
    White UMETA(DisplayName = "White"),
    Black UMETA(DisplayName = "Black"),
    None UMETA(DisplayName = "None") // Для случаев, когда цвет не определен или не важен
};

// Перечисление для типа фигуры
UENUM(BlueprintType)
enum class EPieceType : uint8
{
    Pawn UMETA(DisplayName = "Pawn"),
    Rook UMETA(DisplayName = "Rook"),
    Knight UMETA(DisplayName = "Knight"),
    Bishop UMETA(DisplayName = "Bishop"),
    Queen UMETA(DisplayName = "Queen"),
    King UMETA(DisplayName = "King"),
    None UMETA(DisplayName = "None") // Для случаев, когда тип не определен
};

UCLASS()
class RTX_CHESS_API AChessPiece : public AActor
{
    GENERATED_BODY()

public:
    AChessPiece();

protected:
    virtual void BeginPlay() override;

    // Цвет фигуры
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chess Piece", meta = (AllowPrivateAccess = "true"))
    EPieceColor PieceColor;

    // Тип фигуры
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chess Piece", meta = (AllowPrivateAccess = "true"))
    EPieceType TypeOfPiece;

    // Текущая позиция фигуры на доске (в координатах сетки)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chess Piece", meta = (AllowPrivateAccess = "true"))
    FIntPoint BoardPosition;

    // Флаг, указывающий, совершила ли фигура свой первый ход (важно для пешек, ладей, короля)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chess Piece", meta = (AllowPrivateAccess = "true"))
    bool bHasMoved;

public:
    virtual void Tick(float DeltaTime) override;

    // Инициализирует фигуру заданным цветом, типом и позицией на доске
    UFUNCTION(BlueprintCallable, Category = "Chess Piece")
    virtual void InitializePiece(EPieceColor InColor, EPieceType InType, FIntPoint InBoardPosition);

    // Возвращает цвет фигуры
    UFUNCTION(BlueprintPure, Category = "Chess Piece")
    EPieceColor GetPieceColor() const;

    // Возвращает тип фигуры
    UFUNCTION(BlueprintPure, Category = "Chess Piece")
    EPieceType GetPieceType() const;

    // Возвращает текущую позицию фигуры на доске
    UFUNCTION(BlueprintPure, Category = "Chess Piece")
    FIntPoint GetBoardPosition() const;

    // Устанавливает новую позицию фигуры на доске
    UFUNCTION(BlueprintCallable, Category = "Chess Piece")
    virtual void SetBoardPosition(const FIntPoint& NewPosition);

    // Возвращает массив допустимых ходов для этой фигуры с учетом текущего состояния доски
    // Должен быть переопределен в дочерних классах для конкретных типов фигур
    UFUNCTION(BlueprintCallable, Category = "Chess Piece")
    virtual TArray<FIntPoint> GetValidMoves(const AChessBoard* Board) const;

    // Вызывается, когда фигура выбрана игроком
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Chess Piece")
    void OnSelected();
    virtual void OnSelected_Implementation();

    // Вызывается, когда с фигуры снимается выбор
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Chess Piece")
    void OnDeselected();
    virtual void OnDeselected_Implementation();

    // Вызывается, когда фигура захвачена
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Chess Piece")
    void OnCaptured();
    virtual void OnCaptured_Implementation();

    // Вызывается после того, как фигура совершила ход, чтобы обновить bHasMoved и другие состояния
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Chess Piece")
    void NotifyMoveCompleted();
    virtual void NotifyMoveCompleted_Implementation();
};
