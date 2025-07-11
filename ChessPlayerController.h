#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ChessPiece.h" // Включаем для доступа к EPieceColor
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
class AChessPiece;
class APawnPiece;
class UPromotionMenuWidget;
class UAudioComponent;
class USoundBase;
class AMenuCameraActor;

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

    /** [SERVER] Called from client to finalize pawn promotion. */
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_CompletePawnPromotion(APawnPiece* PawnToPromote, EPieceType PromoteToType);

protected:
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

    /** Музыка для главного меню. */
    UPROPERTY(EditDefaultsOnly, Category = "UI|Sound")
    USoundBase* MenuMusic;

    /** Звуки игровых событий. Настраиваются в Blueprint контроллера. */
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

    /** Цвет для подсветки клеток, на которые можно сделать ход. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Highlight Colors")
    FLinearColor ValidMoveHighlightColor;

    /** Цвет для подсветки клетки с выделенной в данный момент фигурой. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Highlight Colors")
    FLinearColor SelectedPieceHighlightColor;

    /** Камера для главного меню. Может быть установлена в Blueprint для указания конкретной камеры на сцене. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chess Player Controller")
    TSoftObjectPtr<AMenuCameraActor> MenuCameraActor;

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

    /** [SERVER] Requests valid moves for a given piece from the server. */
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_RequestValidMoves(AChessPiece* ForPiece);

    /** [CLIENT] Receives the valid moves from the server and highlights the squares. */
    UFUNCTION(Client, Reliable)
    void Client_ReceiveValidMoves(const TArray<FIntPoint>& Moves);

private:
    UPROPERTY()
    UStartMenuWidget* StartMenuWidgetInstance;

    UPROPERTY()
    UAudioComponent* MenuMusicComponent;

    UPROPERTY()
    UPromotionMenuWidget* PromotionMenuWidgetInstance;
    
    /** The pawn that is waiting to be promoted. Set on client when promotion menu is shown. */
    UPROPERTY()
    APawnPiece* PawnAwaitingPromotion;

    UPROPERTY()
    AChessPiece* SelectedPiece;

    UPROPERTY()
    AChessBoard* ChessBoard;

    /** Хранит список валидных ходов для текущей выделенной фигуры. */
    TArray<FIntPoint> LastValidMoves;

    /** Sets up the UI and camera for active gameplay. Hides menus. */
    void SetupGameUI();

    /** Вызывается при выборе своей фигуры. */
    void HandlePieceSelection(AChessPiece* PieceToSelect);

    /** Вызывается при клике на доску, когда фигура уже выбрана. */
    void HandleBoardClick(const FIntPoint& GridPosition);

    /** Снимает выделение с фигуры и убирает подсветку ходов. */
    void ClearSelectionAndHighlights();

    /** Флаг, чтобы отслеживать, установлен ли игровой режим ввода. */
    bool bIsInputModeSetForGame;
};

