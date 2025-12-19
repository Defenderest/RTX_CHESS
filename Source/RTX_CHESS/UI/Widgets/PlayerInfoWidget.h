#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/PlayerProfile.h"
#include "PlayerInfoWidget.generated.h"

class UTextBlock;
class UImage;

/**
 * Виджет информации об игроках.
 * Отображает имена, аватарки (Steam) и пинг.
 */
UCLASS()
class RTX_CHESS_API UPlayerInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	// --- Элементы интерфейса (должны быть созданы в дизайнере с такими именами) ---
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WhitePlayerName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> WhitePlayerAvatar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BlackPlayerName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> BlackPlayerAvatar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PingInfoText;

	// --- Логика ---

	/** Обновляет всю информацию (имена, пинг, аватарки) */
	void UpdatePlayerInfo();

	/** Загружает аватар для конкретного игрока, если еще не загружен */
	void TryLoadAvatar(class APlayerState* PlayerState, UImage* TargetImage, bool& bIsLoaded);

private:
	bool bWhiteAvatarLoaded = false;
	bool bBlackAvatarLoaded = false;
	float UpdateTimer = 0.0f;
};
