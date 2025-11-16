#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ChessBlueprintFunctionLibrary.generated.h"

UCLASS()
class RTX_CHESS_API UChessBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Shows the main graphics settings widget.
	 * This is a globally accessible function.
	 * @param WorldContextObject The context to get the world from.
	 */
	UFUNCTION(BlueprintCallable, Category = "Chess | UI", meta = (WorldContext = "WorldContextObject"))
	static void ShowChessGraphicsSettings(const UObject* WorldContextObject);
};
