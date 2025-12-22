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

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WhitePlayerPing;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BlackPlayerPing;

	/** Контейнер с кнопками выбора времени. Можно скрыть целиком. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UWidget> TimeControlContainer;

	/** Контейнер с кнопками старта и выбора цвета. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UWidget> StartGameContainer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UButton> Button_StartMatch;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UButton> Button_PickWhite;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UButton> Button_PickBlack;

	/** Рамки для візуального виділення вибору */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UBorder> WhiteSelectionBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UBorder> BlackSelectionBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UBorder> BulletBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UBorder> BlitzBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UBorder> RapidBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UBorder> UnlimitedBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UButton> Button_Bullet;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UButton> Button_Blitz;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UButton> Button_Rapid;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UButton> Button_Unlimited;

	// --- Кнопки для открытия профиля ---
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UButton> WhitePlayerButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UButton> BlackPlayerButton;

	UPROPERTY(EditAnywhere, Category = "Chess UI")
	TObjectPtr<class USoundBase> ClickSound;

	// --- Логика ---

	UFUNCTION()
	void OnBulletClicked();

	UFUNCTION()
	void OnBlitzClicked();

	UFUNCTION()
	void OnRapidClicked();

	UFUNCTION()
	void OnUnlimitedClicked();

	UFUNCTION()
	void OnStartMatchClicked();

	UFUNCTION()
	void OnPickWhiteClicked();

	UFUNCTION()
	void OnPickBlackClicked();

	UFUNCTION()
	void OnWhitePlayerClicked();

	UFUNCTION()
	void OnBlackPlayerClicked();

	/** Обновляет всю информацию (имена, пинг, аватарки) */
	void UpdatePlayerInfo();

	/** Оновлює візуальний стан вибору (рамки) */
	void UpdateSelectionVisuals();

	/** Загружает аватар для конкретного игрока, если еще не загружен */
	void TryLoadAvatar(class APlayerState* PlayerState, UImage* TargetImage, bool& bIsLoaded);

private:
	void OpenSteamProfile(const FUniqueNetIdRepl& PlayerNetId);

	bool bWhiteAvatarLoaded = false;
	bool bBlackAvatarLoaded = false;
	float UpdateTimer = 0.0f;

	FUniqueNetIdRepl WhitePlayerNetId;
	FUniqueNetIdRepl BlackPlayerNetId;
};
