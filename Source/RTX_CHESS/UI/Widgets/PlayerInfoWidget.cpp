#include "PlayerInfoWidget.h"
#include "ChessGameState.h"
#include "ChessPlayerController.h"
#include "ChessPlayerState.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Internationalization/Text.h"

FText UPlayerInfoWidget::GetWhitePlayerNameText() const
{
	AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
	if (!GameState)
	{
		return FText::GetEmpty();
	}

	// Проверяем, не является ли белый игрок ботом
	if (GameState->GetCurrentGameModeType() == EGameModeType::PlayerVsBot)
	{
		AChessPlayerController* PC = GetOwningPlayer<AChessPlayerController>();
		if (PC && PC->GetPlayerColor() == EPieceColor::Black)
		{
			return FText::FromString(TEXT("Бот"));
		}
	}

	return FText::FromString(GameState->WhitePlayerProfile.PlayerName);
}

FText UPlayerInfoWidget::GetWhitePlayerRatingText() const
{
	AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
	if (!GameState)
	{
		return FText::GetEmpty();
	}
	return FText::FromString(FString::Printf(TEXT("(%d)"), GameState->WhitePlayerProfile.EloRating));
}

FText UPlayerInfoWidget::GetBlackPlayerNameText() const
{
	AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
	if (!GameState)
	{
		return FText::GetEmpty();
	}

	// Проверяем, не является ли черный игрок ботом
	if (GameState->GetCurrentGameModeType() == EGameModeType::PlayerVsBot)
	{
		AChessPlayerController* PC = GetOwningPlayer<AChessPlayerController>();
		if (PC && PC->GetPlayerColor() == EPieceColor::White)
		{
			return FText::FromString(TEXT("Бот"));
		}
	}

	return FText::FromString(GameState->BlackPlayerProfile.PlayerName);
}

FText UPlayerInfoWidget::GetBlackPlayerRatingText() const
{
	AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
	if (!GameState)
	{
		return FText::GetEmpty();
	}
	return FText::FromString(FString::Printf(TEXT("(%d)"), GameState->BlackPlayerProfile.EloRating));
}

FText UPlayerInfoWidget::GetPingText() const
{
	AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
	if (!GameState)
	{
		// Возвращаем пустой текст, если состояние игры недоступно
		return FText::GetEmpty();
	}

	int32 WhitePing = -1;
	int32 BlackPing = -1;

	// Итерируем по всем состояниям игроков, чтобы найти белого и черного
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (const AChessPlayerState* ChessPS = Cast<const AChessPlayerState>(PS))
		{
			// Ищем белого игрока по имени профиля
			if (!GameState->WhitePlayerProfile.PlayerName.IsEmpty() && ChessPS->GetPlayerProfile().PlayerName == GameState->WhitePlayerProfile.PlayerName)
			{
				WhitePing = FMath::RoundToInt(ChessPS->GetPingInMilliseconds());
			}
			// Ищем черного игрока по имени профиля
			if (!GameState->BlackPlayerProfile.PlayerName.IsEmpty() && ChessPS->GetPlayerProfile().PlayerName == GameState->BlackPlayerProfile.PlayerName)
			{
				BlackPing = FMath::RoundToInt(ChessPS->GetPingInMilliseconds());
			}
		}
	}
	
	// Формируем строки для пинга. Если игрок не найден (пинг -1), показываем прочерк.
	// Это корректно работает для ботов, т.к. у них нет PlayerState в массиве.
	const FString WhitePingLine = (WhitePing >= 0) ? FString::Printf(TEXT("Пинг (Белые): %d мс"), WhitePing) : TEXT("Пинг (Белые): -");
	const FString BlackPingLine = (BlackPing >= 0) ? FString::Printf(TEXT("Пинг (Черные): %d мс"), BlackPing) : TEXT("Пинг (Черные): -");

	return FText::FromString(FString::Printf(TEXT("%s\n%s"), *WhitePingLine, *BlackPingLine));
}
