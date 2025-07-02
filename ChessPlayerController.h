#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ChessPiece.h" // Включаем для доступа к EPieceColor
#include "ChessPlayerController.generated.h"

// Forward declarations
class UInputMappingContext;
class UInputAction;
class AChessGameMode;
class UStartMenuWidget;
class AChessPiece;

UCLASS()
class RTX_CHESS_API AChessPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AChessPlayerController();

    void ShowStartMenu();

    UFUNCTION(BlueprintCallable, Category = "Chess Player Controller")
    void SetPlayerColor(EPieceColor NewColor);

    UFUNCTION(BlueprintPure, Category = "Chess Player Controller")
    EPieceColor GetPlayerColor() const;

    /** Gets the current chess game mode. */
    UFUNCTION(BlueprintPure, Category = "Chess Player Controller")
    AChessGameMode* GetChessGameMode() const;

    /** Sets the input mode to GameAndUI, suitable for gameplay. */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void SetInputModeForGame();

    /** Sets the input mode to UIOnly, suitable for menus. */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void SetInputModeForUI();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;

    void SetCamera();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chess Player Controller")
    EPieceColor PlayerColor;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UStartMenuWidget> StartMenuWidgetClass;

    // Enhanced Input. Назначьте эти ассеты в вашем Blueprint Player Controller.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputMappingContext* ChessMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* ClickAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* LookAction; // Для вращения камеры

    /** Handles the start of a click/drag. */
    void OnClickStarted();

    /** Handles camera look. */
    void HandleLook(const struct FInputActionValue& Value);

    /** [SERVER] Attempts to move a piece. Called from client, runs on server. */
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_AttemptMove(AChessPiece* PieceToMove, const FIntPoint& TargetGridPosition);

private:
    UPROPERTY()
    UStartMenuWidget* StartMenuWidgetInstance;

    UPROPERTY()
    AChessPiece* SelectedPiece;

    UPROPERTY()
    AChessBoard* ChessBoard;

    /** Вызывается при выборе своей фигуры. */
    void HandlePieceSelection(AChessPiece* PieceToSelect);

    /** Вызывается при клике на доску, когда фигура уже выбрана. */
    void HandleBoardClick(const FIntPoint& GridPosition);

    /** Снимает выделение с фигуры и убирает подсветку ходов. */
    void ClearSelectionAndHighlights();

    /** Флаг, чтобы отслеживать, установлен ли игровой режим ввода. */
    bool bIsInputModeSetForGame;
};

