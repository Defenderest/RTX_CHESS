#pragma once

#include "CoreMinimal.h"

// Чтобы избежать циклической зависимости, мы объявляем структуру FPlayerProfile здесь.
// Фактическое определение структуры будет включено в .cpp файл.
struct FPlayerProfile;

/**
 * Статический класс для вычисления изменений рейтинга Эло.
 */
class RTX_CHESS_API RatingEngine
{
public:
    /**
     * Рассчитывает новые рейтинги для двух игроков на основе исхода игры.
     * @param Winner Профиль победившего игрока (будет изменен).
     * @param Loser Профиль проигравшего игрока (будет изменен).
     */
    static void CalculateNewRatings(FPlayerProfile& Winner, FPlayerProfile& Loser);

    /**
     * Рассчитывает новые рейтинги в случае ничьей.
     * @param PlayerA Профиль первого игрока (будет изменен).
     * @param PlayerB Профиль второго игрока (будет изменен).
     */
    static void CalculateNewRatingsDraw(FPlayerProfile& PlayerA, FPlayerProfile& PlayerB);

private:
    // К-фактор определяет, насколько сильно меняется рейтинг.
    // Обычно используется K=32 для новичков, K=24 для игроков с рейтингом < 2400 и K=16 для топ-игроков.
    // Для простоты мы будем использовать единое значение.
    static constexpr float KFactor = 32.0f;

    /**
     * Рассчитывает ожидаемое количество очков для игрока A против игрока B.
     * @param RatingA Рейтинг игрока A.
     * @param RatingB Рейтинг игрока B.
     * @return Ожидаемое количество очков (значение от 0 до 1).
     */
    static float GetExpectedScore(int32 RatingA, int32 RatingB);
};
