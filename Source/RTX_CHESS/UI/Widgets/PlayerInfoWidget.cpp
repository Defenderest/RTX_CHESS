#include "UI/Widgets/PlayerInfoWidget.h"
#include "Core/ChessGameState.h"
#include "Controllers/ChessPlayerController.h"
#include "Core/ChessPlayerState.h"
#include "Core/ChessGameInstance.h"
#include "GameFramework/PlayerState.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"

void UPlayerInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bWhiteAvatarLoaded = false;
	bBlackAvatarLoaded = false;
	UpdatePlayerInfo(); // Первичное обновление
}

void UPlayerInfoWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Обновляем инфу раз в 0.5 секунды, чтобы не грузить UI каждый кадр
	UpdateTimer += InDeltaTime;
	if (UpdateTimer >= 0.5f)
	{
		UpdatePlayerInfo();
		UpdateTimer = 0.0f;
	}
}

void UPlayerInfoWidget::UpdatePlayerInfo()
{
	// Log update for debug
	// UE_LOG(LogTemp, Verbose, TEXT("UPlayerInfoWidget::UpdatePlayerInfo called"));

	AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
	if (!GameState) return;

	// --- 1. Обновляем Имена и Пинг ---
	
	FString WhiteNameStr = TEXT("Waiting...");
	FString BlackNameStr = TEXT("Waiting...");
	
	int32 WhitePing = -1;
	int32 BlackPing = -1;

	// Ищем реальных игроков в PlayerArray
	APlayerState* WhitePS = nullptr;
	APlayerState* BlackPS = nullptr;

	// Если игра с ботом
	if (GameState->GetCurrentGameModeType() == EGameModeType::PlayerVsBot)
	{
		AChessPlayerController* PC = GetOwningPlayer<AChessPlayerController>();
		if (PC)
		{
			// Получаем локального игрока (себя)
			APlayerState* MyPS = PC->PlayerState;
			
			if (PC->GetPlayerColor() == EPieceColor::White)
			{
				// Мы белые
				BlackNameStr = TEXT("Stockfish Bot");
				// Для бота аватарку не ищем (BlackPS = nullptr)
				
				// Для себя (Белые) берем свой PlayerState
				WhitePS = MyPS;
				if (MyPS)
				{
					WhiteNameStr = MyPS->GetPlayerName();
					WhitePing = FMath::RoundToInt(MyPS->GetPingInMilliseconds());
				}
			}
			else
			{
				// Мы черные
				WhiteNameStr = TEXT("Stockfish Bot");
				// Для бота аватарку не ищем (WhitePS = nullptr)

				// Для себя (Черные) берем свой PlayerState
				BlackPS = MyPS;
				if (MyPS)
				{
					BlackNameStr = MyPS->GetPlayerName();
					BlackPing = FMath::RoundToInt(MyPS->GetPingInMilliseconds());
				}
			}
		}
	}
	else 
	{
		// Логика для PvP (оставляем как есть, поиск по массиву)
		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (AChessPlayerState* ChessPS = Cast<AChessPlayerState>(PS))
			{
				// Вариант А: Сопоставление по имени профиля (так было в старом коде)
				if (!GameState->WhitePlayerProfile.PlayerName.IsEmpty() && ChessPS->GetPlayerProfile().PlayerName == GameState->WhitePlayerProfile.PlayerName)
				{
					WhiteNameStr = ChessPS->GetPlayerName(); // Берем реальное сетевое имя (Steam Nick)
					WhitePing = FMath::RoundToInt(ChessPS->GetPingInMilliseconds());
					WhitePS = PS;
				}
				else if (!GameState->BlackPlayerProfile.PlayerName.IsEmpty() && ChessPS->GetPlayerProfile().PlayerName == GameState->BlackPlayerProfile.PlayerName)
				{
					BlackNameStr = ChessPS->GetPlayerName();
					BlackPing = FMath::RoundToInt(ChessPS->GetPingInMilliseconds());
					BlackPS = PS;
				}
			}
		}
	}

	if (WhitePlayerName) WhitePlayerName->SetText(FText::FromString(WhiteNameStr));
	if (BlackPlayerName) BlackPlayerName->SetText(FText::FromString(BlackNameStr));

	if (PingInfoText)
	{
		FString PingStr = FString::Printf(TEXT("White: %s\nBlack: %s"), 
			(WhitePing >= 0 ? *FString::FromInt(WhitePing) : TEXT("-")),
			(BlackPing >= 0 ? *FString::FromInt(BlackPing) : TEXT("-"))
		);
		PingInfoText->SetText(FText::FromString(PingStr));
	}

	// --- 2. Грузим Аватарки ---
	TryLoadAvatar(WhitePS, WhitePlayerAvatar, bWhiteAvatarLoaded);
	TryLoadAvatar(BlackPS, BlackPlayerAvatar, bBlackAvatarLoaded);
}

void UPlayerInfoWidget::TryLoadAvatar(APlayerState* PlayerState, UImage* TargetImage, bool& bIsLoaded)
{
	if (!TargetImage || bIsLoaded || !PlayerState) return;

	if (UChessGameInstance* GameInstance = Cast<UChessGameInstance>(GetGameInstance()))
	{
		UTexture2D* Avatar = GameInstance->GetSteamAvatar(PlayerState);
		if (Avatar)
		{
			TargetImage->SetBrushFromTexture(Avatar);
			bIsLoaded = true; // Запоминаем, что загрузили, чтобы не долбить Steam API
			UE_LOG(LogTemp, Log, TEXT("Avatar loaded for player: %s"), *PlayerState->GetPlayerName());
		}
	}
}
