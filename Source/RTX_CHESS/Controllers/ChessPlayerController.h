#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ChessPiece.h" 
#include "PlayerProfile.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Net/UnrealNetwork.h"
#include "UObject/SoftObjectPtr.h"
#include "ChessPlayerController.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCameraManagement, Log, All);

// Forward declarations
class UInputMappingContext;
class UInputAction;
class AChessGameMode;
class UStartMenuWidget;
class UPauseMenuWidget;
class AChessPiece;
class APawnPiece;
class UPromotionMenuWidget;
class UPlayerInfoWidget;
class UGameOverWidget;
class UAudioComponent;
class USoundBase;
class AMenuCameraActor;
class UNiagaraSystem;
class UUserWidget;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class ULobbyWidget;

UENUM()
enum class EChessSoundType : uint8
{
	Move,
	Capture,
	Castle,
	Check,
	Checkmate,
	GameStart
};

UCLASS()
class RTX_CHESS_API AChessPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AChessPlayerController();

    /** Toggles the pause menu. */
    void TogglePauseMenu();

    /** Toggles the graphics settings menu. */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ToggleGraphicsSettingsMenu();

    /** Toggles the player info widget. */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void TogglePlayerInfoWidget();


    /** Toggles the player profile widget. */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void ToggleProfileWidget();

    /** Returns the player to the main menu. */
    void ReturnToMainMenu();

    /** Shows the lobby UI. Called from GameState. */
    void ShowLobbyUI();

    /** Hides the lobby UI. */
    void HideLobbyUI();

    /** Leaves the lobby or destroys the session if host. */
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void LeaveLobby();

    /** Checks if this controller is the session host. */
    UFUNCTION(BlueprintPure, Category = "Lobby")
    bool IsHost() const;

    void ShowStartMenu();

    UFUNCTION(BlueprintCallable, Category = "Chess Player Controller")
    void SetPlayerColor(EPieceColor NewColor);

    UFUNCTION(BlueprintPure, Category = "Chess Player Controller")
    EPieceColor GetPlayerColor() const;

    /** Gets the current chess game mode. */
    UFUNCTION(BlueprintPure, Category = "Chess Player Controller")
    AChessGameMode* GetChessGameMode() const;

    /** [UI] Sets the player's color preference for a bot game. Wrapper around the GameMode function for easier Blueprint access. */
    UFUNCTION(BlueprintCallable, Category = "Chess Player Controller|UI")
    void SetPlayerColorChoiceForBotGame(int32 ChoiceIndex);

    /** Sets the input mode to GameAndUI, suitable for gameplay. */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void SetInputModeForGame();

    /** Sets the input mode to UIOnly, suitable for menus. */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void SetInputModeForUI();

    /** [CLIENT] Shows pawn promotion menu. Called from server. */
    UFUNCTION(Client, Reliable)
    void Client_ShowPromotionMenu(APawnPiece* PawnForPromotion);

    /** [CLIENT] Called from GameMode when the game officially starts. Hides menus, sets input mode. */
    UFUNCTION(Client, Reliable)
    void Client_GameStarted();

    /** [CLIENT] Plays a specific game sound. Called from server. */
    UFUNCTION(Client, Reliable)
    void Client_PlaySound(EChessSoundType SoundType);

    /** [CLIENT] Spawns the configured capture effect at a world location and handles hiding the captured piece. Called from server. */
    UFUNCTION(Client, Reliable)
    void Client_PlayCaptureEffect(AChessPiece* CapturedPiece, const FVector& Location, const FVector& Scale, const FVector& CellBoundingBox, float Lifetime, float Density);

    /** [CLIENT] Shows game over screen. Called from server. */
    UFUNCTION(Client, Reliable)
    void Client_ShowGameOverScreen(const FText& ResultText, const FText& ReasonText);

    /** [SERVER] Called from client (lobby widget) to start the game. */
    UFUNCTION(Server, Reliable)
    void Server_RequestStartGame();

    /** [SERVER] Called from client to finalize pawn promotion. */
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_CompletePawnPromotion(APawnPiece* PawnToPromote, EPieceType PromoteToType);

    /** [SERVER] Called from client to set the player's profile data on their PlayerState. */
    UFUNCTION(Server, Reliable)
    void Server_SetPlayerProfile(const FPlayerProfile& Profile);

protected:
    /** Updates the information in the player info widget. */
    void UpdatePlayerInfo();
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void SetGameCamera();
    void SetMenuCamera();

    /** Determines whether to show the start menu or set up the game UI based on the current GameState. Called shortly after BeginPlay. */
    void DetermineInitialUI();

    /** Handles player's choice from promotion menu. */
    UFUNCTION()
    void HandlePromotionSelection(EPieceType SelectedType);

    /** [CLIENT] Called when PlayerColor is replicated. */
    UFUNCTION()
    void OnRep_PlayerColor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_PlayerColor, Category = "Chess Player Controller")
    EPieceColor PlayerColor;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UStartMenuWidget> StartMenuWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UPromotionMenuWidget> PromotionMenuWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UPauseMenuWidget> PauseMenuWidgetClass;

    /** 
     * Widget class for the graphics settings menu.
     * !!! IMPORTANT: This property MUST be set in your Player Controller's Blueprint (e.g., BP_ChessPlayerController).
     * If it is not set (None), the settings menu will not appear.
     */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> GraphicsSettingsWidgetClass;

    /** Widget class for displaying player information (score, ping, etc.). Assigned in Blueprint. */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UPlayerInfoWidget> PlayerInfoWidgetClass;

    /** Widget class for displaying the player profile. Assigned in Blueprint. */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> PlayerProfileWidgetClass;

    /** Widget class for the game over screen. Assigned in Blueprint. */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UGameOverWidget> GameOverWidgetClass;

    /** Widget class for the lobby. Assigned in Blueprint. */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<ULobbyWidget> LobbyWidgetClass;

    /** Music for the main menu. */
    UPROPERTY(EditDefaultsOnly, Category = "UI|Sound")
    USoundBase* MenuMusic;

    /** Game event sounds. Configured in the controller's Blueprint. */
    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Sound")
    USoundBase* GameStartSound;

    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Sound")
    USoundBase* MoveSound;
    
    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Sound")
    USoundBase* CaptureSound;

    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Sound")
    USoundBase* CastleSound;

    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Sound")
    USoundBase* CheckSound;

    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Sound")
    USoundBase* CheckmateSound;

    /** Niagara effect played on piece capture. Configured in the controller's Blueprint. */
    UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Effects")
    class UNiagaraSystem* CaptureEffect;

    /** Highlight color for valid move squares. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Highlight Colors")
    FLinearColor ValidMoveHighlightColor;

    /** Highlight color for the currently selected piece's square. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Highlight Colors")
    FLinearColor SelectedPieceHighlightColor;

    /** Mesh used to display a valid move. Configured in Blueprint. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Highlighting")
    UStaticMesh* ValidMoveIndicatorMesh;

    /** Scale for the valid move indicator mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Highlighting", meta = (EditCondition = "ValidMoveIndicatorMesh != nullptr"))
    FVector ValidMoveIndicatorScale = FVector(1.0f);

    /** Material for the valid move indicator mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Highlighting", meta = (EditCondition = "ValidMoveIndicatorMesh != nullptr"))
    UMaterialInterface* ValidMoveIndicatorMaterial;

    /** Camera for the main menu. Can be set in Blueprint to specify a particular camera in the scene. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Player Controller")
    TSoftObjectPtr<AMenuCameraActor> MenuCameraActor;

    // Enhanced Input. Assign these assets in your Blueprint Player Controller.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputMappingContext* ChessMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* ClickAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* LookAction; // For camera rotation

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* MoveCameraAction; // For camera movement

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* PauseAction; // For opening the pause menu

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* PlayerInfoAction; // For displaying player info

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* ToggleDebugAction; // For toggling debug info

    /** Handles the start of a click/drag. */
    void OnClickStarted();

    /** Handles camera look. */
    void HandleLook(const struct FInputActionValue& Value);

    /** Handles camera movement. */
    void HandleCameraMove(const struct FInputActionValue& Value);

    /** Toggles the visibility of debug information. */
    void ToggleDebugInfo();

    /** [SERVER] Attempts to move a piece. Called from client, runs on server. */
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_AttemptMove(AChessPiece* PieceToMove, const FIntPoint& TargetGridPosition);


private:
    UPROPERTY()
    UStartMenuWidget* StartMenuWidgetInstance;

    UPROPERTY()
    UPauseMenuWidget* PauseMenuWidgetInstance;

    UPROPERTY()
    UUserWidget* GraphicsSettingsWidgetInstance;

    UPROPERTY()
    UUserWidget* PlayerProfileWidgetInstance;

    UPROPERTY()
    UGameOverWidget* GameOverWidgetInstance;

    UPROPERTY()
    ULobbyWidget* LobbyWidgetInstance;

    UPROPERTY()
    class UPlayerInfoWidget* PlayerInfoWidgetInstance;

    UPROPERTY()
    UAudioComponent* MenuMusicComponent;

    UPROPERTY()
    UPromotionMenuWidget* PromotionMenuWidgetInstance;

    /** Timer for periodically updating player info (ping). */
    FTimerHandle PlayerInfoUpdateTimerHandle;
    
    /** The pawn that is waiting to be promoted. Set on client when promotion menu is shown. */
    UPROPERTY()
    APawnPiece* PawnAwaitingPromotion;

    UPROPERTY()
    AChessPiece* SelectedPiece;

    UPROPERTY()
    AChessBoard* ChessBoard;

    /** Stores the list of valid moves for the currently selected piece. */
    TArray<FIntPoint> LastValidMoves;

    /** Components used to visualize valid moves. */
    UPROPERTY()
    TArray<UStaticMeshComponent*> ValidMoveIndicatorComponents;

    /** Sets up the UI and camera for active gameplay. Hides menus. */
    void SetupGameUI();

    /** Called when selecting a friendly piece. */
    void HandlePieceSelection(AChessPiece* PieceToSelect);

    /** Called when clicking the board while a piece is selected. */
    void HandleBoardClick(const FIntPoint& GridPosition);

    /** Clears the piece selection and move highlights. */
    void ClearSelectionAndHighlights();

    /** Flag to track if the game input mode is set. */
    bool bIsInputModeSetForGame;

    /** [CLIENT-SIDE] Flag to track if the Client_GameStarted RPC has been received. Used to prevent a camera state race condition. */
    bool bHasGameStarted_Client;

    /** Flag controlling the display of debug information on screen. */
    bool bShowDebugInfo;

    /** Updates the input mode (UI/Game) based on whether any menu widget is visible. */
    void UpdateInputMode();
};

