#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ChessPlayerController.generated.h"

// Forward declarations
// class UInputMappingContext;
// class UInputAction;

UCLASS()
class RTX_CHESS_API AChessPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AChessPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Заготовки для функций обработки ввода
    // void HandlePieceSelection();
    // void HandlePieceMove();

// protected:
    // Enhanced Input
    // UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    // UInputMappingContext* ChessMappingContext;

    // UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    // UInputAction* SelectAction;

    // UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    // UInputAction* MoveAction;
};
