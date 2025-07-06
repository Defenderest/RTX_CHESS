#include "PromotionMenuWidget.h"
#include "RotatingPieceWidget.h"

void UPromotionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Initialize all our display widgets with the correct meshes and materials.
	if (QueenDisplayWidget)
	{
		QueenDisplayWidget->InitializePiece(QueenMesh, PieceMaterial);
	}
	if (RookDisplayWidget)
	{
		RookDisplayWidget->InitializePiece(RookMesh, PieceMaterial);
	}
	if (BishopDisplayWidget)
	{
		BishopDisplayWidget->InitializePiece(BishopMesh, PieceMaterial);
	}
	if (KnightDisplayWidget)
	{
		KnightDisplayWidget->InitializePiece(KnightMesh, PieceMaterial);
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
