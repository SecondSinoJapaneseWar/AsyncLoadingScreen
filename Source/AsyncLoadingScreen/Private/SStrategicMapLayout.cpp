/************************************************************************************
 * Strategic-map loading layout for source-faithful, aspect-correct map transitions.
 ************************************************************************************/

#include "SStrategicMapLayout.h"

#include "AsyncLoadingScreenLibrary.h"
#include "Engine/Texture2D.h"
#include "Slate/DeferredCleanupSlateBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Widgets/Layout/SSafeZone.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "StrategicMapLoadingScreen"

namespace StrategicMapLoadingScreen
{
	struct FPlacedBackground
	{
		FVector2D DisplaySize = FVector2D::ZeroVector;
		FVector2D Origin = FVector2D::ZeroVector;
		FSlateRect GameRect = FSlateRect(0.0f, 0.0f, 0.0f, 0.0f);
	};

	FPlacedBackground PlaceBackground(
		const FVector2D& ViewSize,
		const FVector2D& SourceSize,
		const FVector4& SourceGameRect,
		const bool bAnchorToRight)
	{
		FPlacedBackground Result;
		if (ViewSize.X <= 0.0f || ViewSize.Y <= 0.0f || SourceSize.X <= 0.0f || SourceSize.Y <= 0.0f)
		{
			return Result;
		}

		const float GameWidthPixels = static_cast<float>(SourceGameRect.Z);
		const float GameHeightPixels = static_cast<float>(SourceGameRect.W);
		if (GameWidthPixels <= 0.0f || GameHeightPixels <= 0.0f)
		{
			return Result;
		}

		// The gameplay rectangle is the protected coordinate frame. A single uniform
		// scale keeps all of it visible; only the surrounding source buffer may crop.
		const float Scale = FMath::Min(
			ViewSize.X / GameWidthPixels,
			ViewSize.Y / GameHeightPixels);
		Result.DisplaySize = SourceSize * Scale;
		const float GameWidth = GameWidthPixels * Scale;
		const float GameHeight = GameHeightPixels * Scale;
		const FVector2D SourceGameCenter(
			static_cast<float>(SourceGameRect.X) + GameWidthPixels * 0.5f,
			static_cast<float>(SourceGameRect.Y) + GameHeightPixels * 0.5f);
		const float HalfGameWidth = GameWidth * 0.5f;
		const float HalfGameHeight = GameHeight * 0.5f;
		const float PreferredGameCenterX = bAnchorToRight
			? ViewSize.X - (SourceSize.X - SourceGameCenter.X) * Scale
			: SourceGameCenter.X * Scale;
		const float GameCenterX = FMath::Clamp(
			PreferredGameCenterX,
			HalfGameWidth,
			FMath::Max(HalfGameWidth, ViewSize.X - HalfGameWidth));
		const float GameCenterY = ViewSize.Y * 0.5f;
		Result.Origin = FVector2D(
			GameCenterX - SourceGameCenter.X * Scale,
			GameCenterY - SourceGameCenter.Y * Scale);
		Result.GameRect = FSlateRect(
			GameCenterX - HalfGameWidth,
			GameCenterY - HalfGameHeight,
			GameCenterX + HalfGameWidth,
			GameCenterY + HalfGameHeight);
		return Result;
	}

	class SStrategicMapBackdrop final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SStrategicMapBackdrop) {}
			SLATE_ARGUMENT(UTexture2D*, Background)
			SLATE_ARGUMENT(FVector2D, BackgroundPixelSize)
			SLATE_ARGUMENT(FVector4, GameViewportRectPixels)
			SLATE_ARGUMENT(bool, AnchorBackgroundToRight)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			BackgroundPixelSize = InArgs._BackgroundPixelSize;
			GameViewportRectPixels = InArgs._GameViewportRectPixels;
			bAnchorBackgroundToRight = InArgs._AnchorBackgroundToRight;
			if (IsValid(InArgs._Background))
			{
				BackgroundBrush = FDeferredCleanupSlateBrush::CreateBrush(InArgs._Background);
			}
		}

		virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override
		{
			return BackgroundPixelSize;
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			bool bParentEnabled) const override
		{
			const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
			const FVector2D ViewSize = AllottedGeometry.GetLocalSize();
			const FPlacedBackground Placement = PlaceBackground(
				ViewSize,
				BackgroundPixelSize,
				GameViewportRectPixels,
				bAnchorBackgroundToRight);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				WhiteBrush,
				ESlateDrawEffect::None,
				FLinearColor::Black);

			if (BackgroundBrush.IsValid())
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId + 1,
					AllottedGeometry.ToPaintGeometry(
						FVector2f(Placement.DisplaySize),
						FSlateLayoutTransform(FVector2f(Placement.Origin))),
					BackgroundBrush->GetSlateBrush(),
					ESlateDrawEffect::None,
					FLinearColor::White);
			}

			const float GameWidth = Placement.GameRect.Right - Placement.GameRect.Left;
			const float Thickness = FMath::Clamp(GameWidth * 0.0035f, 2.0f, 6.0f);
			const FLinearColor FrameColor(0.80f, 0.63f, 0.22f, 0.96f);

			auto DrawFramePart = [&](const float X, const float Y, const float Width, const float Height)
			{
				if (Width <= 0.0f || Height <= 0.0f)
				{
					return;
				}
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId + 2,
					AllottedGeometry.ToPaintGeometry(
						FVector2f(Width, Height),
						FSlateLayoutTransform(FVector2f(X, Y))),
					WhiteBrush,
					ESlateDrawEffect::None,
					FrameColor);
			};

			// Four strips sit strictly outside the 4096x4096 gameplay rectangle.
			DrawFramePart(
				Placement.GameRect.Left - Thickness,
				Placement.GameRect.Top - Thickness,
				GameWidth + Thickness * 2.0f,
				Thickness);
			DrawFramePart(
				Placement.GameRect.Left - Thickness,
				Placement.GameRect.Bottom,
				GameWidth + Thickness * 2.0f,
				Thickness);
			DrawFramePart(
				Placement.GameRect.Left - Thickness,
				Placement.GameRect.Top,
				Thickness,
				Placement.GameRect.Bottom - Placement.GameRect.Top);
			DrawFramePart(
				Placement.GameRect.Right,
				Placement.GameRect.Top,
				Thickness,
				Placement.GameRect.Bottom - Placement.GameRect.Top);

			return LayerId + 2;
		}

	private:
		TSharedPtr<FDeferredCleanupSlateBrush> BackgroundBrush;
		FVector2D BackgroundPixelSize = FVector2D(7680.0f, 4320.0f);
		FVector4 GameViewportRectPixels = FVector4(0.0, 0.0, 4096.0, 4096.0);
		bool bAnchorBackgroundToRight = false;
	};

	TSharedRef<SWidget> MakePrimaryPanel(
		const FStrategicMapLoadingScreenData& Data,
		const TAttribute<float>& DpiScale)
	{
		const FLinearColor Brass(0.80f, 0.63f, 0.22f, 1.0f);
		TSharedRef<SVerticalBox> Content = SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("BriefingKicker", "OPERATIONAL BRIEFING / 战役简报"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
				.ColorAndOpacity(Brass)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SNew(STextBlock)
				.Text(Data.PrimaryTitle)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 34))
				.ColorAndOpacity(FLinearColor(0.93f, 0.88f, 0.73f, 1.0f))
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SSeparator)
				.Thickness(1.0f)
				.ColorAndOpacity(FLinearColor(0.38f, 0.32f, 0.18f, 0.8f))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 14.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(Data.PrimaryBody)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 22))
				.ColorAndOpacity(FLinearColor(0.76f, 0.78f, 0.70f, 1.0f))
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SSpacer)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 14.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SCircularThrobber)
					.NumPieces(6)
					.Period(0.75f)
					.Radius(11.0f)
					.ColorAndOpacity(Brass)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				.Padding(12.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("LoadingBattlefield", "正在进入战场  /  LOADING THEATER"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
					.ColorAndOpacity(FLinearColor(0.88f, 0.81f, 0.58f, 1.0f))
				]
			];

		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor::Transparent)
			.Padding(18.0f)
			[
				SNew(SSafeZone)
				.IsTitleSafe(true)
				[
					SNew(SDPIScaler)
					.DPIScale(DpiScale)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							Content
						]
					]
				]
			];
	}

	TSharedRef<SWidget> MakeSecondaryPanel(
		const FStrategicMapLoadingScreenData& Data,
		const TAttribute<float>& DpiScale)
	{
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor::Transparent)
			.Padding(14.0f)
			[
				SNew(SSafeZone)
				.IsTitleSafe(true)
				[
					SNew(SDPIScaler)
					.DPIScale(DpiScale)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("TimelineKicker", "THEATER CLOCK / 战区时序"))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
							.ColorAndOpacity(FLinearColor(0.80f, 0.63f, 0.22f, 1.0f))
							.AutoWrapText(true)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 8.0f, 0.0f, 14.0f)
						[
							SNew(STextBlock)
							.Text(Data.SecondaryTitle)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 26))
							.ColorAndOpacity(FLinearColor(0.90f, 0.86f, 0.72f, 1.0f))
							.AutoWrapText(true)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(Data.SecondaryBody)
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
							.ColorAndOpacity(FLinearColor(0.67f, 0.73f, 0.69f, 1.0f))
							.AutoWrapText(true)
						]
						+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							SNew(SSpacer)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("DirectBounds", "DIRECT BOUNDS\n4096 × 4096"))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
							.ColorAndOpacity(FLinearColor(0.56f, 0.50f, 0.31f, 1.0f))
							.AutoWrapText(true)
						]
					]
				]
			];
	}
}

void SStrategicMapLayout::Construct(const FArguments& InArgs, const FALoadingScreenSettings& Settings)
{
	const FStrategicMapLoadingScreenData& Data = UAsyncLoadingScreenLibrary::GetStrategicMapLoadingScreenData();
	if (IsValid(Data.Background))
	{
		BackgroundPixelSize = FVector2D(Data.Background->GetSizeX(), Data.Background->GetSizeY());
	}
	GameViewportRectPixels = Data.GameViewportRectPixels;
	bAnchorBackgroundToRight = Data.bAnchorBackgroundToRight;

	TSharedRef<SOverlay> Root = SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(StrategicMapLoadingScreen::SStrategicMapBackdrop)
			.Background(Data.Background)
			.BackgroundPixelSize(BackgroundPixelSize)
			.GameViewportRectPixels(GameViewportRectPixels)
			.AnchorBackgroundToRight(bAnchorBackgroundToRight)
		];

	const TAttribute<float> DpiScale = TAttribute<float>::Create(
		TAttribute<float>::FGetter::CreateSP(this, &SStrategicMapLayout::GetDPIScale));

	// Four responsive slots let the primary briefing follow the actually larger
	// visible overflow side after cropping, rather than assuming the source side
	// remains larger at every aspect ratio.
	Root->AddSlot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Fill)
		.Padding(PanelMargin, 20.0f, PanelMargin, 20.0f)
		[
			SNew(SBox)
			.WidthOverride(this, &SStrategicMapLayout::GetLeftPanelWidth)
			.Visibility(this, &SStrategicMapLayout::GetLeftPrimaryVisibility)
			[
				StrategicMapLoadingScreen::MakePrimaryPanel(Data, DpiScale)
			]
		];

	Root->AddSlot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Fill)
		.Padding(PanelMargin, 20.0f, PanelMargin, 20.0f)
		[
			SNew(SBox)
			.WidthOverride(this, &SStrategicMapLayout::GetLeftPanelWidth)
			.Visibility(this, &SStrategicMapLayout::GetLeftSecondaryVisibility)
			[
				StrategicMapLoadingScreen::MakeSecondaryPanel(Data, DpiScale)
			]
		];

	Root->AddSlot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Fill)
		.Padding(PanelMargin, 20.0f, PanelMargin, 20.0f)
		[
			SNew(SBox)
			.WidthOverride(this, &SStrategicMapLayout::GetRightPanelWidth)
			.Visibility(this, &SStrategicMapLayout::GetRightPrimaryVisibility)
			[
				StrategicMapLoadingScreen::MakePrimaryPanel(Data, DpiScale)
			]
		];

	Root->AddSlot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Fill)
		.Padding(PanelMargin, 20.0f, PanelMargin, 20.0f)
		[
			SNew(SBox)
			.WidthOverride(this, &SStrategicMapLayout::GetRightPanelWidth)
			.Visibility(this, &SStrategicMapLayout::GetRightSecondaryVisibility)
			[
				StrategicMapLoadingScreen::MakeSecondaryPanel(Data, DpiScale)
			]
		];

	AddLoadingCompleteTextSlot(Root, Settings);
	ChildSlot[Root];
}

FVector2D SStrategicMapLayout::GetViewportSize() const
{
	const FVector2D Size = GetTickSpaceGeometry().GetLocalSize();
	return Size.IsNearlyZero() ? FVector2D(1920.0f, 1080.0f) : Size;
}

void SStrategicMapLayout::GetOutsideWidths(float& OutLeftWidth, float& OutRightWidth) const
{
	const FVector2D ViewSize = GetViewportSize();
	const StrategicMapLoadingScreen::FPlacedBackground Placement = StrategicMapLoadingScreen::PlaceBackground(
		ViewSize,
		BackgroundPixelSize,
		GameViewportRectPixels,
		bAnchorBackgroundToRight);
	OutLeftWidth = FMath::Clamp(Placement.GameRect.Left, 0.0f, ViewSize.X);
	OutRightWidth = FMath::Clamp(ViewSize.X - Placement.GameRect.Right, 0.0f, ViewSize.X);
}

FOptionalSize SStrategicMapLayout::GetLeftPanelWidth() const
{
	float LeftWidth = 0.0f;
	float RightWidth = 0.0f;
	GetOutsideWidths(LeftWidth, RightWidth);
	return FMath::Max(LeftWidth - PanelMargin * 2.0f, 0.0f);
}

FOptionalSize SStrategicMapLayout::GetRightPanelWidth() const
{
	float LeftWidth = 0.0f;
	float RightWidth = 0.0f;
	GetOutsideWidths(LeftWidth, RightWidth);
	return FMath::Max(RightWidth - PanelMargin * 2.0f, 0.0f);
}

bool SStrategicMapLayout::IsPrimaryPanelOnLeft() const
{
	float LeftWidth = 0.0f;
	float RightWidth = 0.0f;
	GetOutsideWidths(LeftWidth, RightWidth);
	return LeftWidth >= RightWidth;
}

EVisibility SStrategicMapLayout::GetLeftPrimaryVisibility() const
{
	return IsPrimaryPanelOnLeft() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SStrategicMapLayout::GetLeftSecondaryVisibility() const
{
	return IsPrimaryPanelOnLeft() ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SStrategicMapLayout::GetRightPrimaryVisibility() const
{
	return IsPrimaryPanelOnLeft() ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SStrategicMapLayout::GetRightSecondaryVisibility() const
{
	return IsPrimaryPanelOnLeft() ? EVisibility::Visible : EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE
