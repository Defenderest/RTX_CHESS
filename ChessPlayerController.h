#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ChessPlayerController.generated.h"

// Forward declarations
class UInputMappingContext;
class UInputAction;
class AChessGameMode; // Forward declare AChessGameMode

UCLASS()
class RTX_CHESS_API AChessPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AChessPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

    // Enhanced Input
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputMappingContext* ChessMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* SelectAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* LookUpAction; // Для вращения камеры вверх/вниз

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* LookRightAction; // Для вращения камеры влево/вправо

    // UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    // UInputAction* MoveAction; // Assuming MoveAction might be used later

    /** Handles the select action input. */
    void HandleSelectAction();

    /** Handles looking up/down. */
    void HandleLookUp(const struct FInputActionValue& Value);

    /** Handles looking left/right. */
    void HandleLookRight(const struct FInputActionValue& Value);

public:
    virtual void Tick(float DeltaTime) override;

    /** Gets the current chess game mode. */
    UFUNCTION(BlueprintPure, Category = "Chess Player Controller")
    AChessGameMode* GetChessGameMode() const;

    // Заготовки для функций обработки ввода
    // void HandlePieceSelection();
    // void HandlePieceMove();
};
