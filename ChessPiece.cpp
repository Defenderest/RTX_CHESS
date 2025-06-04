#include "ChessPiece.h"
#include "ChessBoard.h" // Для использования AChessBoard в GetValidMoves

AChessPiece::AChessPiece()
{
    PrimaryActorTick.bCanEverTick = false; // Фигуры обычно не тикают каждый кадр
    PieceColor = EPieceColor::None;
    TypeOfPiece = EPieceType::None;
    BoardPosition = FIntPoint(-1, -1); // Невалидная начальная позиция
}

void AChessPiece::BeginPlay()
{
    Super::BeginPlay();
}

void AChessPiece::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AChessPiece::InitializePiece(EPieceColor InColor, EPieceType InType, FIntPoint InBoardPosition)
{
    PieceColor = InColor;
    TypeOfPiece = InType;
    BoardPosition = InBoardPosition;
    UE_LOG(LogTemp, Log, TEXT("AChessPiece: Initialized %s %s at (%d, %d)"),
           (PieceColor == EPieceColor::White ? TEXT("White") : TEXT("Black")),
           *UEnum::GetValueAsString(TypeOfPiece),
           BoardPosition.X, BoardPosition.Y);
}

EPieceColor AChessPiece::GetPieceColor() const
{
    return PieceColor;
}

EPieceType AChessPiece::GetPieceType() const
{
    return TypeOfPiece;
}

FIntPoint AChessPiece::GetBoardPosition() const
{
    return BoardPosition;
}

void AChessPiece::SetBoardPosition(const FIntPoint& NewPosition)
{
    BoardPosition = NewPosition;
    // Мировое положение актора будет обновлено GameMode или ChessBoard
    // UE_LOG(LogTemp, Log, TEXT("AChessPiece: Set %s %s to new board position (%d, %d)"),
    //        (PieceColor == EPieceColor::White ? TEXT("White") : TEXT("Black")),
    //        *UEnum::GetValueAsString(TypeOfPiece),
    //        BoardPosition.X, BoardPosition.Y);
}

TArray<FIntPoint> AChessPiece::GetValidMoves(const AChessBoard* Board) const
{
    // Базовая реализация: фигура не имеет допустимых ходов.
    // Эта функция должна быть переопределена в дочерних классах для каждого типа фигуры.
    UE_LOG(LogTemp, Warning, TEXT("AChessPiece::GetValidMoves: Base implementation called for %s %s. No valid moves returned."),
           (PieceColor == EPieceColor::White ? TEXT("White") : TEXT("Black")),
           *UEnum::GetValueAsString(TypeOfPiece));
    return TArray<FIntPoint>();
}

void AChessPiece::OnSelected_Implementation()
{
    // Логика по умолчанию для выбора фигуры (может быть переопределена в Blueprint)
    UE_LOG(LogTemp, Log, TEXT("AChessPiece: %s %s at (%d, %d) selected."),
           (PieceColor == EPieceColor::White ? TEXT("White") : TEXT("Black")),
           *UEnum::GetValueAsString(TypeOfPiece),
           BoardPosition.X, BoardPosition.Y);
}

void AChessPiece::OnDeselected_Implementation()
{
    // Логика по умолчанию для снятия выбора с фигуры (может быть переопределена в Blueprint)
    UE_LOG(LogTemp, Log, TEXT("AChessPiece: %s %s at (%d, %d) deselected."),
           (PieceColor == EPieceColor::White ? TEXT("White") : TEXT("Black")),
           *UEnum::GetValueAsString(TypeOfPiece),
           BoardPosition.X, BoardPosition.Y);
}

void AChessPiece::OnCaptured_Implementation()
{
    // Логика по умолчанию для захвата фигуры (может быть переопределена в Blueprint)
    UE_LOG(LogTemp, Log, TEXT("AChessPiece: %s %s at (%d, %d) captured."),
           (PieceColor == EPieceColor::White ? TEXT("White") : TEXT("Black")),
           *UEnum::GetValueAsString(TypeOfPiece),
           BoardPosition.X, BoardPosition.Y);
    // Здесь можно добавить логику для скрытия фигуры, воспроизведения анимации и т.д.
}
