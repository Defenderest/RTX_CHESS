#include "UI/Widgets/GameOverWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Controllers/ChessPlayerController.h"

void UGameOverWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (BackToMenuButton)
    {
        BackToMenuButton->OnClicked.AddDynamic(this, &UGameOverWidget::OnBackToMenuClicked);
    }
}

void UGameOverWidget::SetResultText(const FText& Text)
{
    if (ResultText)
    {
        ResultText->SetText(Text);
    }
}

void UGameOverWidget::SetReasonText(const FText& Text)
{
    if (ReasonText)
    {
        ReasonText->SetText(Text);
    }
}

void UGameOverWidget::OnBackToMenuClicked()
{
    if (AChessPlayerController* PC = GetOwningPlayer<AChessPlayerController>())
    {
        PC->ReturnToMainMenu();
    }
}
