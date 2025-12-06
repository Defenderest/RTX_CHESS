#include "Core/RatingEngine.h"
#include "PlayerProfile.h" // Включаем полное определение структуры

float RatingEngine::GetExpectedScore(int32 RatingA, int32 RatingB)
{
    // Формула для расчета ожидаемого результата в системе Эло
    return 1.0f / (1.0f + FMath::Pow(10.0f, (static_cast<float>(RatingB - RatingA) / 400.0f)));
}

void RatingEngine::CalculateNewRatings(FPlayerProfile& Winner, FPlayerProfile& Loser)
{
    const float ExpectedScoreWinner = GetExpectedScore(Winner.EloRating, Loser.EloRating);
    const float ExpectedScoreLoser = GetExpectedScore(Loser.EloRating, Winner.EloRating);

    const int32 OldWinnerRating = Winner.EloRating;
    const int32 OldLoserRating = Loser.EloRating;

    // S - это фактический результат: 1 за победу, 0.5 за ничью, 0 за поражение.
    Winner.EloRating = FMath::RoundToInt(OldWinnerRating + KFactor * (1.0f - ExpectedScoreWinner));
    Loser.EloRating = FMath::RoundToInt(OldLoserRating + KFactor * (0.0f - ExpectedScoreLoser));
}

void RatingEngine::CalculateNewRatingsDraw(FPlayerProfile& PlayerA, FPlayerProfile& PlayerB)
{
    const float ExpectedScoreA = GetExpectedScore(PlayerA.EloRating, PlayerB.EloRating);
    const float ExpectedScoreB = GetExpectedScore(PlayerB.EloRating, PlayerA.EloRating);

    const int32 OldRatingA = PlayerA.EloRating;
    const int32 OldRatingB = PlayerB.EloRating;

    // S - это фактический результат: 1 за победу, 0.5 за ничью, 0 за поражение.
    PlayerA.EloRating = FMath::RoundToInt(OldRatingA + KFactor * (0.5f - ExpectedScoreA));
    PlayerB.EloRating = FMath::RoundToInt(OldRatingB + KFactor * (0.5f - ExpectedScoreB));
}
