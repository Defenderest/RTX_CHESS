#include "UI/PromotionMenuWidget.h"

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
