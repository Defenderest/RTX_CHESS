#pragma once

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ChessPiece.h" // Включаем для доступа к EPieceColor
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "ChessPlayerController.generated.h"

// Forward declarations
class UInputMappingContext;
class UInputAction;
class AChessGameMode;
class UStartMenuWidget;
class AChessPiece;
class APawnPiece;
class UPromotionMenuWidget;

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

    /** Starts hosting a LAN session with a specific name. */
    void HostSession(const FString& SessionName);

    /** Finds and joins a LAN session with a specific name. */
    void FindAndJoinSession(const FString& SessionName);

    /** [CLIENT] Shows pawn promotion menu. Called from server. */
    UFUNCTION(Client, Reliable)
    void Client_ShowPromotionMenu(APawnPiece* PawnForPromotion);

    /** [SERVER] Called from client to finalize pawn promotion. */
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_CompletePawnPromotion(APawnPiece* PawnToPromote, EPieceType PromoteToType);

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;

    // --- Network Callbacks ---
    /** Callback for when session creation is complete. */
    void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    /** Callback for when session search is complete. */
    void OnFindSessionsComplete(bool bWasSuccessful);
    /** Callback for when joining a session is complete. */
    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

    void SetCamera();

    /** Handles player's choice from promotion menu. */
    UFUNCTION()
    void HandlePromotionSelection(EPieceType SelectedType);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chess Player Controller")
    EPieceColor PlayerColor;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UStartMenuWidget> StartMenuWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UPromotionMenuWidget> PromotionMenuWidgetClass;

    // Enhanced Input. Назначьте эти ассеты в вашем Blueprint Player Controller.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputMappingContext* ChessMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* ClickAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* LookAction; // Для вращения камеры

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* MoveCameraAction; // Для перемещения камеры

    /** Handles the start of a click/drag. */
    void OnClickStarted();

    /** Handles camera look. */
    void HandleLook(const struct FInputActionValue& Value);

    /** Handles camera movement. */
    void HandleCameraMove(const struct FInputActionValue& Value);

    /** [SERVER] Attempts to move a piece. Called from client, runs on server. */
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_AttemptMove(AChessPiece* PieceToMove, const FIntPoint& TargetGridPosition);

private:
    // --- Network Session Handling ---
    IOnlineSessionPtr SessionInterface;
    TSharedPtr<FOnlineSessionSearch> SessionSearch;

    FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
    FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
    FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;
    
    void FindSessions(const FString& SessionName);
    void JoinSession(const FOnlineSessionSearchResult& SearchResult);
    // --- End Network Session Handling ---

    UPROPERTY()
    UStartMenuWidget* StartMenuWidgetInstance;

    UPROPERTY()
    UPromotionMenuWidget* PromotionMenuWidgetInstance;
    
    /** The pawn that is waiting to be promoted. Set on client when promotion menu is shown. */
    UPROPERTY()
    APawnPiece* PawnAwaitingPromotion;

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

