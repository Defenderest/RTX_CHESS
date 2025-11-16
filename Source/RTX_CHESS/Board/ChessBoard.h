#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/IntPoint.h"
#include "Components/StaticMeshComponent.h"
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

    // Board dimensions (e.g., 8x8 for standard chess)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chess Board Setup")
    FIntPoint BoardSize;

    // Size of a single board tile in Unreal world units (for a board with scale 1.0)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chess Board Setup")
    float TileSize;

    // Overall scale of the board. Applied to TileSize for correct positioning.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chess Board Setup")
    float BoardScale = 1.0f;

    // Z-axis offset for pieces relative to the tile surface.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Board Setup")
    float PieceZOffsetOnBoard;

public:
    virtual void Tick(float DeltaTime) override;

#if WITH_EDITORONLY_DATA
    // Toggles the drawing of a debug grid to visualize tile centers.
    UPROPERTY(EditAnywhere, Category = "Chess Board Setup|Debug")
    bool bDrawDebugGrid = true;
#endif

    // Initializes the board. This function now primarily prepares the board
    // and waits for the GameMode to spawn the pieces.
    UFUNCTION(BlueprintCallable, Category = "Chess Board")
    virtual void InitializeBoard();

    // Spawns the initial pieces on the board.
    // This function is now empty and kept for backward compatibility if called elsewhere.
    // The main logic has been moved to AChessGameMode.
    UFUNCTION(BlueprintCallable, Category = "Chess Board", meta=(DeprecatedFunction, DeprecationMessage="Use GameMode's SpawnInitialPieces instead"))
    virtual void SpawnPieces();

    // Returns the board dimensions (number of tiles in X and Y).
    UFUNCTION(BlueprintPure, Category = "Chess Board")
    FIntPoint GetBoardSize() const;

    // Checks if the specified grid coordinates (X, Y) are valid on the board.
    UFUNCTION(BlueprintPure, Category = "Chess Board")
    bool IsValidGridPosition(const FIntPoint& GridPosition) const;

    // Returns the piece at the specified grid position.
    UFUNCTION(BlueprintPure, Category = "Chess Board")
    AChessPiece* GetPieceAtGridPosition(const FIntPoint& GridPosition) const;

    // Sets a piece at the specified grid position.
    UFUNCTION(BlueprintCallable, Category = "Chess Board")
    void SetPieceAtGridPosition(AChessPiece* Piece, const FIntPoint& GridPosition);

    // Clears a tile of its piece.
    UFUNCTION(BlueprintCallable, Category = "Chess Board")
    void ClearSquare(const FIntPoint& GridPosition);
    
    // Converts grid coordinates to world coordinates (center of the tile).
    UFUNCTION(BlueprintPure, Category = "Chess Board")
    FVector GridToWorldPosition(const FIntPoint& GridPosition) const;

    // Converts world coordinates to grid coordinates.
    UFUNCTION(BlueprintPure, Category = "Chess Board")
    FIntPoint WorldToGridPosition(const FVector& WorldPosition) const;

    // Highlights the specified tile (e.g., to show possible moves).
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Chess Board Visuals")
    void HighlightSquare(const FIntPoint& GridPosition, FLinearColor HighlightColor);

    // Clears the highlight from all or a specific tile.
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Chess Board Visuals")
    void ClearHighlight(const FIntPoint& GridPosition);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Chess Board Visuals")
    void ClearAllHighlights();

    // Checks if the specified square is attacked by pieces of the given color.
    UFUNCTION(BlueprintPure, Category = "Chess Board")
    bool IsSquareAttackedBy(const FIntPoint& SquarePosition, EPieceColor AttackingColor) const;

protected:
    // Component for displaying the 3D board model.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> BoardMeshComponent;

#if WITH_EDITOR
    // Debug function to draw the grid.
    void DrawDebugGrid() const;
#endif
};
