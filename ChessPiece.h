#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/IntPoint.h"
#include "Components/StaticMeshComponent.h"
#include "ChessPiece.generated.h"

// Forward declarations
class AChessBoard;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UDecalComponent;
class UParticleSystemComponent;
class UParticleSystem;

UENUM(BlueprintType)
enum class EPieceColor : uint8
{
    White UMETA(DisplayName = "White"),
    Black UMETA(DisplayName = "Black")
};

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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chess Piece", meta = (AllowPrivateAccess = "true"))
    EPieceColor PieceColor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chess Piece", meta = (AllowPrivateAccess = "true"))
    EPieceType TypeOfPiece;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chess Piece", meta = (AllowPrivateAccess = "true"))
    FIntPoint BoardPosition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chess Piece", meta = (AllowPrivateAccess = "true"))
    bool bHasMoved;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> PieceMeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UDecalComponent> SelectionDecalComponent;

    // Компонент для партиклов выделения
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UParticleSystemComponent> SelectionParticlesComponent;

    // Шаблон партиклов для выделения (задается в Blueprint)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chess Piece|Effects", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UParticleSystem> SelectionParticleSystem;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chess Piece|Materials", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UMaterialInterface> WhiteMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chess Piece|Materials", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UMaterialInterface> BlackMaterial;

    UPROPERTY(BlueprintReadOnly, Category = "Chess Piece|Materials", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UMaterialInstanceDynamic> DynamicMaterialInstance;

public:
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Chess Piece")
    virtual void InitializePiece(EPieceColor InColor, EPieceType InType, FIntPoint InBoardPosition);

    UFUNCTION(BlueprintPure, Category = "Chess Piece")
    EPieceColor GetPieceColor() const;

    UFUNCTION(BlueprintPure, Category = "Chess Piece")
    EPieceType GetPieceType() const;

    UFUNCTION(BlueprintPure, Category = "Chess Piece")
    FIntPoint GetBoardPosition() const;

    UFUNCTION(BlueprintCallable, Category = "Chess Piece")
    virtual void SetBoardPosition(const FIntPoint& NewPosition);

    UFUNCTION(BlueprintPure, Category = "Chess Piece")
    bool HasMoved() const;

    UFUNCTION(BlueprintCallable, Category = "Chess Piece")
    virtual TArray<FIntPoint> GetValidMoves(const class AChessGameState* GameState, const AChessBoard* Board) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Chess Piece")
    void OnSelected();
    virtual void OnSelected_Implementation();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Chess Piece")
    void OnDeselected();
    virtual void OnDeselected_Implementation();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Chess Piece")
    void OnCaptured();
    virtual void OnCaptured_Implementation();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Chess Piece")
    void NotifyMoveCompleted();
    virtual void NotifyMoveCompleted_Implementation();

    UFUNCTION(BlueprintCallable, Category = "Chess Piece")
    void SetPieceMesh(UStaticMesh* NewMesh);
};

