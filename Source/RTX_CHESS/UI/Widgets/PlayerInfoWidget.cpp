#include "UI/Widgets/PlayerInfoWidget.h"
#include "Core/ChessGameState.h"
#include "Controllers/ChessPlayerController.h"
#include "Core/ChessPlayerState.h"
#include "Core/ChessGameInstance.h"
#include "GameFramework/PlayerState.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"

void UPlayerInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bWhiteAvatarLoaded = false;
	bBlackAvatarLoaded = false;

	if (WhitePlayerButton)
	{
		WhitePlayerButton->OnClicked.AddDynamic(this, &UPlayerInfoWidget::OnWhitePlayerClicked);
	}

	if (BlackPlayerButton)
	{
		BlackPlayerButton->OnClicked.AddDynamic(this, &UPlayerInfoWidget::OnBlackPlayerClicked);
	}

	if (Button_Bullet) Button_Bullet->OnClicked.AddDynamic(this, &UPlayerInfoWidget::OnBulletClicked);
	if (Button_Blitz) Button_Blitz->OnClicked.AddDynamic(this, &UPlayerInfoWidget::OnBlitzClicked);
	if (Button_Rapid) Button_Rapid->OnClicked.AddDynamic(this, &UPlayerInfoWidget::OnRapidClicked);
	if (Button_Unlimited) Button_Unlimited->OnClicked.AddDynamic(this, &UPlayerInfoWidget::OnUnlimitedClicked);

	if (Button_StartMatch) Button_StartMatch->OnClicked.AddDynamic(this, &UPlayerInfoWidget::OnStartMatchClicked);
	if (Button_PickWhite) Button_PickWhite->OnClicked.AddDynamic(this, &UPlayerInfoWidget::OnPickWhiteClicked);
	if (Button_PickBlack) Button_PickBlack->OnClicked.AddDynamic(this, &UPlayerInfoWidget::OnPickBlackClicked);

	UpdatePlayerInfo();
}

void UPlayerInfoWidget::UpdateSelectionVisuals()
{
	AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
	if (!GameState) return;

	// Яскраве золото для кращої видимості
	const FLinearColor SelectedColor(1.0f, 0.8f, 0.1f, 1.0f);
	const FLinearColor TransparentColor(0.0f, 0.0f, 0.0f, 0.0f);

	// 1. Оновлення рамок часу
	if (BulletBorder) BulletBorder->SetBrushColor(GameState->LobbyTimeControl == ETimeControlType::Bullet_1_0 ? SelectedColor : TransparentColor);
	if (BlitzBorder) BlitzBorder->SetBrushColor(GameState->LobbyTimeControl == ETimeControlType::Blitz_3_2 ? SelectedColor : TransparentColor);
	if (RapidBorder) RapidBorder->SetBrushColor(GameState->LobbyTimeControl == ETimeControlType::Rapid_10_0 ? SelectedColor : TransparentColor);
	if (UnlimitedBorder) UnlimitedBorder->SetBrushColor(GameState->LobbyTimeControl == ETimeControlType::Unlimited ? SelectedColor : TransparentColor);

	// 2. Оновлення рамок кольору
	// 0 = White, 1 = Random, 2 = Black
	if (WhiteSelectionBorder) WhiteSelectionBorder->SetBrushColor(GameState->LobbyColorPreference == 0 ? SelectedColor : TransparentColor);
	if (BlackSelectionBorder) BlackSelectionBorder->SetBrushColor(GameState->LobbyColorPreference == 2 ? SelectedColor : TransparentColor);
}

void UPlayerInfoWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Оновлюємо візуальний стан вибраних кнопок
	UpdateSelectionVisuals();

	// Обновляем инфу раз в 0.5 секунды, чтобы не грузить UI каждый кадр
	UpdateTimer += InDeltaTime;
	if (UpdateTimer >= 0.5f)
	{
		UpdatePlayerInfo();
		UpdateTimer = 0.0f;
	}

	// Логика скрытия кнопок выбора времени и старта игры
	AChessGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AChessGameState>() : nullptr;
	if (GameState)
	{
		AChessPlayerController* PC = GetOwningPlayer<AChessPlayerController>();
		bool bIsHost = PC && PC->IsHost();
		bool bInLobby = GameState->bIsInLobby;

		if (TimeControlContainer)
		{
			// Показываем кнопки времени только Хосту и только в Лобби
			if (bIsHost && bInLobby)
			{
				TimeControlContainer->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				TimeControlContainer->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		if (StartGameContainer)
		{
			// Показываем кнопки старта/цвета только Хосту и только в Лобби
			if (bIsHost && bInLobby)
			{
				StartGameContainer->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				StartGameContainer->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

void UPlayerInfoWidget::OnBulletClicked()
{
	if (ClickSound) UGameplayStatics::PlaySound2D(this, ClickSound);
	if (AChessPlayerController* PC = GetOwningPlayer<AChessPlayerController>())
	{
		PC->Server_SetLobbyTimeControl(ETimeControlType::Bullet_1_0);
	}
}

void UPlayerInfoWidget::OnBlitzClicked()
{
	if (ClickSound) UGameplayStatics::PlaySound2D(this, ClickSound);
	if (AChessPlayerController* PC = GetOwningPlayer<AChessPlayerController>())
	{
		PC->Server_SetLobbyTimeControl(ETimeControlType::Blitz_3_2);
	}
}

void UPlayerInfoWidget::OnRapidClicked()
{
	if (ClickSound) UGameplayStatics::PlaySound2D(this, ClickSound);
	if (AChessPlayerController* PC = GetOwningPlayer<AChessPlayerController>())
	{
		PC->Server_SetLobbyTimeControl(ETimeControlType::Rapid_10_0);
	}
}

void UPlayerInfoWidget::OnUnlimitedClicked()
{
	if (ClickSound) UGameplayStatics::PlaySound2D(this, ClickSound);
	if (AChessPlayerController* PC = GetOwningPlayer<AChessPlayerController>())
	{
		PC->Server_SetLobbyTimeControl(ETimeControlType::Unlimited);
	}
}

void UPlayerInfoWidget::OnStartMatchClicked()
{
	if (ClickSound) UGameplayStatics::PlaySound2D(this, ClickSound);
	if (AChessPlayerController* PC = GetOwningPlayer<AChessPlayerController>())
	{
		PC->Server_RequestStartGame();
	}
}

void UPlayerInfoWidget::OnPickWhiteClicked()
{
	if (ClickSound) UGameplayStatics::PlaySound2D(this, ClickSound);
	if (AChessPlayerController* PC = GetOwningPlayer<AChessPlayerController>())
	{
		PC->SetPlayerColorChoiceForBotGame(0); // 0 = White
	}
}

void UPlayerInfoWidget::OnPickBlackClicked()
{
	if (ClickSound) UGameplayStatics::PlaySound2D(this, ClickSound);
	if (AChessPlayerController* PC = GetOwningPlayer<AChessPlayerController>())
	{
		PC->SetPlayerColorChoiceForBotGame(2); // 2 = Black
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

	// Сбрасываем ID перед поиском
	WhitePlayerNetId = FUniqueNetIdRepl();
	BlackPlayerNetId = FUniqueNetIdRepl();

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
					WhitePlayerNetId = MyPS->GetUniqueId();
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
					BlackPlayerNetId = MyPS->GetUniqueId();
				}
			}
		}
	}
	else 
	{
		// Логика для PvP
		int32 FoundPlayerCount = 0;
		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (AChessPlayerState* ChessPS = Cast<AChessPlayerState>(PS))
			{
				bool bAssignedAsWhite = false;
				bool bAssignedAsBlack = false;

				// 1. Спробуємо знайти за офіційним профілем (якщо гра вже почалася)
				if (!GameState->WhitePlayerProfile.PlayerName.IsEmpty() && ChessPS->GetPlayerProfile().PlayerName == GameState->WhitePlayerProfile.PlayerName)
				{
					bAssignedAsWhite = true;
				}
				else if (!GameState->BlackPlayerProfile.PlayerName.IsEmpty() && ChessPS->GetPlayerProfile().PlayerName == GameState->BlackPlayerProfile.PlayerName)
				{
					bAssignedAsBlack = true;
				}
				// 2. Якщо ми в лобі і профілі ще порожні, просто беремо за чергою в списку
				else if (GameState->bIsInLobby)
				{
					if (FoundPlayerCount == 0) bAssignedAsWhite = true;
					else if (FoundPlayerCount == 1) bAssignedAsBlack = true;
				}

				if (bAssignedAsWhite)
				{
					WhiteNameStr = ChessPS->GetPlayerName();
					WhitePing = FMath::RoundToInt(ChessPS->GetPingInMilliseconds() * 4.0f); // Пинг в UE часто ділиться на 4
					WhitePS = PS;
					WhitePlayerNetId = PS->GetUniqueId();
				}
				else if (bAssignedAsBlack)
				{
					BlackNameStr = ChessPS->GetPlayerName();
					BlackPing = FMath::RoundToInt(ChessPS->GetPingInMilliseconds() * 4.0f);
					BlackPS = PS;
					BlackPlayerNetId = PS->GetUniqueId();
				}

				FoundPlayerCount++;
			}
		}
	}

	if (WhitePlayerName) WhitePlayerName->SetText(FText::FromString(WhiteNameStr));
	if (BlackPlayerName) BlackPlayerName->SetText(FText::FromString(BlackNameStr));

	// Обновляем отдельные текстовые блоки пинга
	if (WhitePlayerPing)
	{
		WhitePlayerPing->SetText(FText::FromString(WhitePing >= 0 ? FString::FromInt(WhitePing) : TEXT("-")));
	}

	if (BlackPlayerPing)
	{
		BlackPlayerPing->SetText(FText::FromString(BlackPing >= 0 ? FString::FromInt(BlackPing) : TEXT("-")));
	}

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

void UPlayerInfoWidget::OnWhitePlayerClicked()
{
	OpenSteamProfile(WhitePlayerNetId);
}

void UPlayerInfoWidget::OnBlackPlayerClicked()
{
	OpenSteamProfile(BlackPlayerNetId);
}

void UPlayerInfoWidget::OpenSteamProfile(const FUniqueNetIdRepl& PlayerNetId)
{
	if (!PlayerNetId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("OpenSteamProfile: PlayerNetId is not valid (possibly a bot)."));
		return;
	}

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineExternalUIPtr ExternalUI = OnlineSub->GetExternalUIInterface();
		IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();

		if (ExternalUI.IsValid() && Identity.IsValid())
		{
			TSharedPtr<const FUniqueNetId> LocalUserId = Identity->GetUniquePlayerId(0);
			if (LocalUserId.IsValid())
			{
				ExternalUI->ShowProfileUI(*LocalUserId, *PlayerNetId.GetUniqueNetId());
			}
		}
	}
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
