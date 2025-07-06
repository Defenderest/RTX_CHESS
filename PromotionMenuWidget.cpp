#include "PromotionMenuWidget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UPromotionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Initialize all our display images with the correct textures.
	if (QueenDisplayImage && QueenTexture)
	{
		QueenDisplayImage->SetBrushFromTexture(QueenTexture);
	}
	if (RookDisplayImage && RookTexture)
	{
		RookDisplayImage->SetBrushFromTexture(RookTexture);
	}
	if (BishopDisplayImage && BishopTexture)
	{
		BishopDisplayImage->SetBrushFromTexture(BishopTexture);
	}
	if (KnightDisplayImage && KnightTexture)
	{
		KnightDisplayImage->SetBrushFromTexture(KnightTexture);
	}
}

void UPromotionMenuWidget::OnQueenSelected()
{
    OnPromotionPieceSelected.Broadcast(EPieceType::Queen);
    RemoveFromParent(); // Автоматически убираем виджет с экрана после выбора
}

void UPromotionMenuWidget::OnRookSelected()
{
    OnPromotionPieceSelected.Broadcast(EPieceType::Rook);
    RemoveFromParent();
}

void UPromotionMenuWidget::OnBishopSelected()
{
    OnPromotionPieceSelected.Broadcast(EPieceType::Bishop);
    RemoveFromParent();
}

void UPromotionMenuWidget::OnKnightSelected()
{
    OnPromotionPieceSelected.Broadcast(EPieceType::Knight);
    RemoveFromParent();
}
