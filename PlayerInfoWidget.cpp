#include "PlayerInfoWidget.h"
#include "ChessGameState.h"
#include "ChessPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

void UPlayerInfoWidget::UpdateDisplay()
{
	AChessPlayerController* PC = GetOwningPlayer<AChessPlayerController>();
	AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;

	if (!PC || !GameState)
	{
		// Если данные недоступны, просто выходим, чтобы избежать ошибок.
		return;
	}

	bool bWhiteIsBot = false;
	bool bBlackIsBot = false;

	// Определяем, кто бот, если это игра против ИИ
	if (GameState->GetCurrentGameModeType() == EGameModeType::PlayerVsBot)
	{
		if (PC->GetPlayerColor() == EPieceColor::White)
		{
			bBlackIsBot = true;
		}
		else
		{
			bWhiteIsBot = true;
		}
	}

	// Вызываем Blueprint-события с собранными данными
	BP_OnUpdateWhitePlayer(GameState->WhitePlayerProfile, bWhiteIsBot);
	BP_OnUpdateBlackPlayer(GameState->BlackPlayerProfile, bBlackIsBot);

	// Обновляем пинг для локального игрока
	if (APlayerState* PS = PC->GetPlayerState())
	{
		const int32 Ping = FMath::RoundToInt(PS->GetPingInMilliseconds());
		BP_OnUpdatePing(Ping);
	}
	else
	{
		// Если PlayerState недоступен, отправляем -1 как индикатор ошибки
		BP_OnUpdatePing(-1);
	}
}
