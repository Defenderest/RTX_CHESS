#include "RotatingPieceWidget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void URotatingPieceWidget::InitializePiece(UTexture2D* PieceTexture)
{
	if (IsValid(PieceImage))
	{
		if (IsValid(PieceTexture))
		{
			PieceImage->SetBrushFromTexture(PieceTexture, false);
		}
		else
		{
			// Очищаем изображение, если текстура недействительна
			PieceImage->SetBrush(FSlateBrush());
		}
	}
}
