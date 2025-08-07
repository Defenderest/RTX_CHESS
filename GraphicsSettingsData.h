#pragma once

#include "CoreMinimal.h"
#include "GraphicsSettingsData.generated.h"

/**
 * Структура для хранения настроек графики.
 * Значения по умолчанию соответствуют "High" или "Epic" (индекс 3 из диапазона 0-4 для качества).
 */
USTRUCT(BlueprintType)
struct FGraphicsSettingsData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	int32 AntiAliasingQuality = 3;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	int32 PostProcessingQuality = 3;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	int32 ShadowQuality = 3;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	int32 TextureQuality = 3;

	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	int32 EffectsQuality = 3;
	
	UPROPERTY(BlueprintReadWrite, Category = "Graphics Settings")
	bool bUseVSync = false;

	FGraphicsSettingsData() = default;
};
