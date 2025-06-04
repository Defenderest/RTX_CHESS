#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChessPiece.generated.h"

// Перечисление для цвета фигуры
UENUM(BlueprintType)
enum class EPieceColor : uint8
{
    White UMETA(DisplayName = "White"),
    Black UMETA(DisplayName = "Black")
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
    King UMETA(DisplayName = "King")
};

UCLASS()
class RTX_CHESS_API AChessPiece : public AActor
{
    GENERATED_BODY()

public:
    AChessPiece();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Пример свойств фигуры (раскомментируйте и настройте при необходимости)
    // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Piece")
    // EPieceColor PieceColor;

    // UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Piece")
    // EPieceType TypeOfPiece; // Изменено имя, чтобы избежать конфликта с GetType()

    // Заготовки для логики перемещения
    // virtual bool CanMoveTo(int32 TargetX, int32 TargetY /*, AChessBoard* Board */);
    // virtual void MoveTo(int32 TargetX, int32 TargetY /*, AChessBoard* Board */);
};
