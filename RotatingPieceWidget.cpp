#include "RotatingPieceWidget.h"
#include "RotatingPieceCaptureActor.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"

void URotatingPieceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!IsValid(RenderTargetMaterialBase) || !IsValid(PieceImage))
	{
		return;
	}

	// Создаем Render Target, куда будет рисовать камера
	RenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("RotatingPieceRenderTarget"));
	RenderTarget->InitAutoDPI(256, 256);
	RenderTarget->UpdateResource();

	// Создаем динамический материал на основе базового
	PieceMaterialInstance = UMaterialInstanceDynamic::Create(RenderTargetMaterialBase, this);
	PieceMaterialInstance->SetTextureParameterValue(FName("RenderTargetTexture"), RenderTarget);

	// Устанавливаем материал для нашего виджета Image
	PieceImage->SetBrushFromMaterial(PieceMaterialInstance);

	// Спауним актора, который будет "фотографировать" фигуру
	// Размещаем его далеко, чтобы он не мешал основной сцене
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CaptureActor = GetWorld()->SpawnActor<ARotatingPieceCaptureActor>(FVector(0.f, 0.f, -5000.f), FRotator::ZeroRotator, SpawnParams);

	if (IsValid(CaptureActor))
	{
		CaptureActor->RotationSpeed = RotationSpeed;
		if (USceneCaptureComponent2D* CaptureComponent = CaptureActor->GetSceneCaptureComponent())
		{
			CaptureComponent->TextureTarget = RenderTarget;
		}
	}
}

void URotatingPieceWidget::InitializePiece(UStaticMesh* PieceMesh, UMaterialInterface* PieceMaterial)
{
	if (!IsValid(CaptureActor) || !IsValid(PieceMesh))
	{
		return;
	}

	CaptureActor->SetPieceMesh(PieceMesh, PieceMaterial);
}

void URotatingPieceWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	if (IsValid(CaptureActor))
	{
		CaptureActor->Destroy();
		CaptureActor = nullptr;
	}

	// RenderTarget и PieceMaterialInstance будут очищены сборщиком мусора
}
