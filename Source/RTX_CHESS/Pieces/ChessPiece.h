#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/IntPoint.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "ChessPiece.generated.h"

// Forward declarations
class AChessBoard;
class AChessGameState;
class UStaticMeshComponent;
class USceneComponent;
class UStaticMesh;
class UMaterialInterface;
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

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;

    /** [CLIENT] Called when piece properties are replicated to set up material. */
    UFUNCTION()
    void OnRep_PieceProperties();

    /** [CLIENT] Called when board position is replicated to animate the move. */
    UFUNCTION()
    void OnRep_BoardPosition();

    /** Helper to set up the piece material based on color. */
    void SetupMaterial();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_PieceProperties, Category = "Chess Piece", meta = (AllowPrivateAccess = "true"))
    EPieceColor PieceColor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_PieceProperties, Category = "Chess Piece", meta = (AllowPrivateAccess = "true"))
    EPieceType TypeOfPiece;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_BoardPosition, Category = "Chess Piece", meta = (AllowPrivateAccess = "true"))
    FIntPoint BoardPosition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Chess Piece", meta = (AllowPrivateAccess = "true"))
    bool bHasMoved;

    UPROPERTY(Replicated)
    bool bIsCaptured;

    // --- Movement Animation ---
    UPROPERTY(EditDefaultsOnly, Category = "Chess Piece|Movement")
    float MoveSpeed = 2.0f; // Скорость движения фигуры (где 1.0f = 1 секунда на ход)

    UPROPERTY(EditDefaultsOnly, Category = "Chess Piece|Movement")
    float KnightArcHeight = 50.0f; // Высота прыжка коня в юнитах
    
    // --- Physics Properties ---
    UPROPERTY(EditDefaultsOnly, Category = "Chess Piece|Physics")
    float MassInKg = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Chess Piece|Physics", meta = (ToolTip = "Makes the piece tumble more realistically by lowering the center of mass."))
    FVector CenterOfMassOffset = FVector(0.f, 0.f, -10.f);

    UPROPERTY(EditDefaultsOnly, Category = "Chess Piece|Physics", meta = (ToolTip = "Damping for linear motion (sliding)."))
    float LinearDamping = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Chess Piece|Physics", meta = (ToolTip = "Damping for angular motion (spinning)."))
    float AngularDamping = 1.5f;
    // --- End Physics Properties ---

    FVector StartWorldLocation;
    FVector TargetWorldLocation;
    float MoveLerpAlpha;
    bool bIsMoving;
    
    // Вспомогательные переменные для механики сгорания захваченной фигуры
    float TimeSpentStationary;
    bool bIsBurning;
    // --- End Movement Animation ---

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USceneComponent> SceneRootComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> PieceMeshComponent;

    // Компонент для эффекта сгорания
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UParticleSystemComponent> BurnEffectComponent;

    // Эффект сгорания для захваченной фигуры (задается в Blueprint)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chess Piece|Effects", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UParticleSystem> BurnEffect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chess Piece|Materials", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UMaterialInterface> WhiteMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chess Piece|Materials", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UMaterialInterface> BlackMaterial;

    UPROPERTY(BlueprintReadOnly, Category = "Chess Piece|Materials", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UMaterialInstanceDynamic> DynamicMaterialInstance;

protected:
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_ApplyPhysicsAndImpulse(const FVector& ImpulseDirection, float ImpulseStrength, float TorqueStrength);
    
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_StartBurnEffect(float Duration, const FVector& Scale);

    void SelfDestruct();
    FTimerHandle SelfDestructTimerHandle;

public:
    virtual void Tick(float DeltaTime) override;

    // Вызывается из GameMode на сервере при взятии этой фигуры
    void HandleCapture(AChessPiece* CapturingPiece, const FVector& BoardCenter, float ImpulseStrength, float TorqueStrength);

    // Запускает плавную анимацию перемещения фигуры в указанную мировую позицию
    UFUNCTION(BlueprintCallable, Category = "Chess Piece")
    void AnimateMoveTo(const FVector& TargetLocation);

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

