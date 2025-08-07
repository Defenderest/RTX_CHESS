#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerProfile.h"
#include "PlayerInfoWidget.generated.h"

/**
 * Базовый C++ класс для виджета информации об игроках.
 * Содержит логику для сбора данных и предоставляет события для их отображения в Blueprint.
 */
UCLASS()
class RTX_CHESS_API UPlayerInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 *  Главная функция для обновления виджета. Вызывается из PlayerController.
	 *  Собирает все необходимые данные и вызывает Blueprint-события для обновления UI.
	 */
	UFUNCTION(BlueprintCallable, Category = "Player Info Widget")
	void UpdateDisplay();

protected:
	/**
	 *  Событие, вызываемое в Blueprint для обновления информации о белом игроке.
	 *  @param Profile - Профиль игрока (имя, рейтинг).
	 *  @param bIsBot - Является ли этот игрок ботом.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Player Info Widget", meta = (DisplayName = "On Update White Player"))
	void BP_OnUpdateWhitePlayer(const FPlayerProfile& Profile, bool bIsBot);

	/**
	 *  Событие, вызываемое в Blueprint для обновления информации о черном игроке.
	 *  @param Profile - Профиль игрока (имя, рейтинг).
	 *  @param bIsBot - Является ли этот игрок ботом.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Player Info Widget", meta = (DisplayName = "On Update Black Player"))
	void BP_OnUpdateBlackPlayer(const FPlayerProfile& Profile, bool bIsBot);

	/**
	 *  Событие, вызываемое в Blueprint для обновления пинга локального игрока.
	 *  @param Ping - Пинг в миллисекундах.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Player Info Widget", meta = (DisplayName = "On Update Ping"))
	void BP_OnUpdatePing(int32 Ping);
};
