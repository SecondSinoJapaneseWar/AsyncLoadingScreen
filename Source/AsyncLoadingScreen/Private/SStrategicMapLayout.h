/************************************************************************************
 * Strategic-map loading layout for source-faithful, aspect-correct map transitions.
 ************************************************************************************/

#pragma once

#include "SLoadingScreenLayout.h"
#include "Widgets/Layout/SBox.h"

struct FALoadingScreenSettings;

class SStrategicMapLayout : public SLoadingScreenLayout
{
public:
	SLATE_BEGIN_ARGS(SStrategicMapLayout) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const FALoadingScreenSettings& Settings);

private:
	FOptionalSize GetLeftPanelWidth() const;
	FOptionalSize GetRightPanelWidth() const;
	EVisibility GetLeftPrimaryVisibility() const;
	EVisibility GetLeftSecondaryVisibility() const;
	EVisibility GetRightPrimaryVisibility() const;
	EVisibility GetRightSecondaryVisibility() const;
	bool IsPrimaryPanelOnLeft() const;
	FVector2D GetViewportSize() const;
	void GetOutsideWidths(float& OutLeftWidth, float& OutRightWidth) const;

	FVector2D BackgroundPixelSize = FVector2D(7680.0f, 4320.0f);
	FVector4 GameViewportRectPixels = FVector4(0.0, 0.0, 4096.0, 4096.0);
	bool bAnchorBackgroundToRight = false;
	float PanelMargin = 12.0f;
};
