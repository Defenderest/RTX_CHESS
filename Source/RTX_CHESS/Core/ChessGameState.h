#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ChessPiece.h"
#include "ChessGameMode.h"
#include "PlayerProfile.h"
#include "Math/IntPoint.h"
#include "Net/UnrealNetwork.h"
#include "ChessGameState.generated.h"

// Forward declarations
class AChessBoard;
class APawnPiece; // For storing a reference to the pawn for en passant capture

// EGamePhase was moved to ChessGameMode.h to resolve a circular dependency.

UCLASS()
class RTX_CHESS_API AChessGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AChessGameState();

	virtual void Tick(float DeltaSeconds) override;

    // Determines whose turn it is to move.
    UPROPERTY(ReplicatedUsing = OnRep_CurrentTurn, BlueprintReadOnly, Category = "Chess Game State")
    EPieceColor CurrentTurnColor;

    // Called when CurrentTurnColor changes to update on clients.
    UFUNCTION()
    void OnRep_CurrentTurn();

    // The current phase of the game.
    UPROPERTY(ReplicatedUsing = OnRep_GamePhase, BlueprintReadOnly, Category = "Chess Game State")
    EGamePhase CurrentGamePhase;

    // Called when CurrentGamePhase changes to update on clients.
    UFUNCTION()
    void OnRep_GamePhase();

    // --- Lobby Data ---
    UPROPERTY(ReplicatedUsing = OnRep_LobbyStateChanged)
    bool bIsInLobby;

    UPROPERTY(Replicated)
    ETimeControlType LobbyTimeControl;

    UFUNCTION()
    void OnRep_LobbyStateChanged();
    // --- End Lobby Data ---

    UPROPERTY(ReplicatedUsing = OnRep_PlayerProfiles, BlueprintReadOnly, Category = "Chess Game State")
    FPlayerProfile WhitePlayerProfile;

    UPROPERTY(ReplicatedUsing = OnRep_PlayerProfiles, BlueprintReadOnly, Category = "Chess Game State")
    FPlayerProfile BlackPlayerProfile;

    UFUNCTION()
    void OnRep_PlayerProfiles();

    UPROPERTY(ReplicatedUsing = OnRep_WhiteTimeSeconds, BlueprintReadOnly, Category = "Chess Game State")
    float WhiteTimeSeconds;

    UPROPERTY(ReplicatedUsing = OnRep_BlackTimeSeconds, BlueprintReadOnly, Category = "Chess Game State")
    float BlackTimeSeconds;

    UFUNCTION()
    void OnRep_WhiteTimeSeconds();

    UFUNCTION()
    void OnRep_BlackTimeSeconds();

    // The current game mode (PvP or PvE).
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    EGameModeType CurrentGameMode;

    // Array of all active pieces on the board.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    TArray<TObjectPtr<AChessPiece>> ActivePieces; // Using TObjectPtr for safety.

    // The square on which an en passant capture is possible. FIntPoint(-1,-1) if none.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    FIntPoint EnPassantTargetSquare;

    // The pawn that can be captured en passant. nullptr if none.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    TWeakObjectPtr<APawnPiece> EnPassantPawnToCapture;

    // Halfmove clock for the 50-move rule.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    int32 HalfmoveClock;

    // Fullmove counter.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    int32 FullmoveNumber;

    // Castling rights.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    bool bCanWhiteCastleKingSide;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    bool bCanWhiteCastleQueenSide;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    bool bCanBlackCastleKingSide;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chess Game State")
    bool bCanBlackCastleQueenSide;

    // Pawn awaiting promotion.
    UPROPERTY(Replicated)
    TObjectPtr<APawnPiece> PawnToPromote;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Returns the color of the player whose turn it is.
    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    EPieceColor GetCurrentTurnColor() const;

    // Returns the current fullmove number.
    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    int32 GetFullmoveNumber() const;

    // Switches the turn to the other player (must be called on the server from GameMode).
    void Server_SwitchTurn();

    // Plays the move animation for the specified player on all clients.
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayMoveAnimationForPlayer(EPieceColor PlayerColor);

    // Returns the current game phase.
    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    EGamePhase GetGamePhase() const;

    // Sets a new game phase (must be called on the server from GameMode).
    void SetGamePhase(EGamePhase NewPhase);

    // Adds a piece to the list of active pieces (usually on spawn).
    void AddPieceToState(AChessPiece* PieceToAdd);

    // Removes a piece from the list of active pieces (usually on capture).
    void RemovePieceFromState(AChessPiece* PieceToRemove);

    // Returns the piece at the specified grid position.
    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    AChessPiece* GetPieceAtGridPosition(const FIntPoint& GridPosition) const;

    // Checks if the specified player is in check.
    // Board is required to analyze opponent's possible moves.
    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    bool IsPlayerInCheck(EPieceColor PlayerColor, const AChessBoard* Board) const;

    // Checks if the specified player is in checkmate.
    // Board is required to analyze possible moves and game state.
    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    bool IsPlayerInCheckmate(EPieceColor PlayerColor, const AChessBoard* Board); // Removed const

    // Checks if the game is in a stalemate for the specified player.
    // Board is required to analyze possible moves.
    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    bool IsStalemate(EPieceColor PlayerColor, const AChessBoard* Board); // Removed const

    // Checks if a move is legal (i.e., does not leave the king in check).
    bool IsMoveLegal(AChessPiece* PieceToMove, const FIntPoint& TargetPosition, const AChessBoard* Board);

    // --- Lobby Management ---
    void SetIsInLobby(bool bNewState);
    void SetLobbyTimeControl(ETimeControlType NewTimeControl);

    // --- Pawn Promotion ---
    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    APawnPiece* GetPawnToPromote() const;
    void SetPawnToPromote(APawnPiece* Pawn);
    // --- End Pawn Promotion ---

public: // Changed from protected
    // --- Full Game State Management (called from GameMode) ---

    // Resets the game state to its initial position.
    void ResetGameStateForNewGame();

    // Updates castling rights based on a moved or captured piece.
    void UpdateCastlingRights(const AChessPiece* Piece, const FIntPoint& FromPosition);

    // Increments the fullmove counter.
    void IncrementFullmoveNumber();

    // Increments the halfmove clock.
    void IncrementHalfmoveClock();

    // Resets the halfmove clock.
    void ResetHalfmoveClock();
    
    // Internal method to change the current turn color, called by Server_SwitchTurn.
    void SetCurrentTurnColor(EPieceColor NewTurnColor);

    // --- En Passant Logic ---
    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    FIntPoint GetEnPassantTargetSquare() const;

    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    APawnPiece* GetEnPassantPawnToCapture() const;

    // Sets the data for a possible en passant capture (called by GameMode).
    // Must be called on the server.
    void SetEnPassantData(const FIntPoint& TargetSquare, APawnPiece* PawnToCapture);

    // Clears the en passant data (called by GameMode).
    // Must be called on the server.
    void ClearEnPassantData();

    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    FString GetFEN() const;

    UFUNCTION(BlueprintPure, Category = "Chess Game State")
    EGameModeType GetCurrentGameModeType() const;

    void SetCurrentGameMode(EGameModeType NewMode);
};
