#include "RotatingPieceWidget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void URotatingPieceWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetPieceTexture(DefaultPieceTexture);
}

void URotatingPieceWidget::SetPieceTexture(UTexture2D* NewPieceTexture)
{
	if (IsValid(PieceImage))
	{
		if (IsValid(NewPieceTexture))
		{
			PieceImage->SetBrushFromTexture(NewPieceTexture, false);
			PieceImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			// Очищаем изображение, если текстура недействительна, и скрываем виджет
			PieceImage->SetBrush(FSlateBrush());
			PieceImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
