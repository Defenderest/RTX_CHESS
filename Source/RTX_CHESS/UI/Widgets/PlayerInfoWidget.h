#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/PlayerProfile.h"
#include "PlayerInfoWidget.generated.h"

/**
 * Базовый C++ класс для виджета информации об игроках.
 * Предоставляет функции для привязки (Binding) к текстовым полям в UMG для отображения данных об игре.
 *
 * --- Инструкция по настройке и использованию ---
 *
 * 1. **Создание Blueprint-виджета:**
 *    - В Unreal Editor создайте новый Blueprint-виджет. В качестве родительского класса выберите `PlayerInfoWidget`.
 *
 * 2. **Привязка функций к текстовым полям (Binding):**
 *    - Откройте ваш Blueprint-виджет в редакторе (Designer).
 *    - Выберите элемент `TextBlock`.
 *    - В панели "Details" найдите свойство "Text" (в разделе "Content").
 *    - Нажмите на кнопку "Bind" и выберите одну из доступных функций:
 *      - `GetWhitePlayerNameText`
 *      - `GetWhitePlayerRatingText`
 *      - `GetBlackPlayerNameText`
 *      - `GetBlackPlayerRatingText`
 *      - `GetPingText`
 *    - Повторите это для всех текстов, которые нужно отобразить.
 *
 * 3. **Отображение виджета:**
 *    - Создайте и добавьте виджет на экран как обычно (например, из `PlayerController`).
 *    - Данные будут обновляться автоматически, так как привязки (bindings) опрашиваются каждый кадр.
 */
UCLASS()
class RTX_CHESS_API UPlayerInfoWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** Возвращает имя белого игрока в виде текста для UMG. */
	UFUNCTION(BlueprintPure, Category = "Player Info Widget")
	FText GetWhitePlayerNameText() const;

	/** Возвращает рейтинг белого игрока в виде текста для UMG. */
	UFUNCTION(BlueprintPure, Category = "Player Info Widget")
	FText GetWhitePlayerRatingText() const;

	/** Возвращает имя черного игрока в виде текста для UMG. */
	UFUNCTION(BlueprintPure, Category = "Player Info Widget")
	FText GetBlackPlayerNameText() const;

	/** Возвращает рейтинг черного игрока в виде текста для UMG. */
	UFUNCTION(BlueprintPure, Category = "Player Info Widget")
	FText GetBlackPlayerRatingText() const;

	/** Возвращает пинг локального игрока в виде текста для UMG. */
	UFUNCTION(BlueprintPure, Category = "Player Info Widget")
	FText GetPingText() const;
};
