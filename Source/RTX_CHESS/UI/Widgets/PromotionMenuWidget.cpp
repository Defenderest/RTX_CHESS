#include "PromotionMenuWidget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/Border.h"
#include "Components/BackgroundBlur.h"

void UPromotionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BackgroundBlur)
	{
		BackgroundBlur->SetBlurStrength(BlurStrength);
	}

	if (BackgroundBorder)
	{
		BackgroundBorder->Background.DrawAs = ESlateBrushDrawType::Box;
	}

	// Initialize all our display images with the correct textures and size.
	if (QueenDisplayImage && QueenTexture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(QueenTexture);
		Brush.ImageSize = ImageSize;
		QueenDisplayImage->SetBrush(Brush);
	}
	if (RookDisplayImage && RookTexture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(RookTexture);
		Brush.ImageSize = ImageSize;
		RookDisplayImage->SetBrush(Brush);
	}
	if (BishopDisplayImage && BishopTexture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(BishopTexture);
		Brush.ImageSize = ImageSize;
		BishopDisplayImage->SetBrush(Brush);
	}
	if (KnightDisplayImage && KnightTexture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(KnightTexture);
		Brush.ImageSize = ImageSize;
		KnightDisplayImage->SetBrush(Brush);
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
