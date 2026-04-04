#include "DragVisualWidget.h"
#include "Components/Image.h"

void UDragVisualWidget::SetDragIcon(UTexture2D* IconTexture)
{
	if (IMG_DragIcon && IconTexture)
	{
		IMG_DragIcon->SetBrushFromTexture(IconTexture);
	}
}