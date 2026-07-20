/************************************************************************************
 *																					*
 * Copyright (C) 2020 Truong Bui.													*
 * Website:	https://github.com/truong-bui/AsyncLoadingScreen						*
 * Licensed under the MIT License. See 'LICENSE' file for full license information. *
 *																					*
 ************************************************************************************/


#include "AsyncLoadingScreenLibrary.h"
#include "MoviePlayer.h"
#include "Engine/Texture2D.h"

int32 UAsyncLoadingScreenLibrary::DisplayBackgroundIndex = -1;
int32 UAsyncLoadingScreenLibrary::DisplayTipTextIndex = -1;
int32 UAsyncLoadingScreenLibrary::DisplayMovieIndex = -1;
bool  UAsyncLoadingScreenLibrary::bShowLoadingScreen = true;
bool UAsyncLoadingScreenLibrary::bUseStrategicMapLoadingScreen = false;
TStrongObjectPtr<UTexture2D> UAsyncLoadingScreenLibrary::StrategicMapBackground;
FStrategicMapLoadingScreenData UAsyncLoadingScreenLibrary::StrategicMapLoadingScreenData;

void UAsyncLoadingScreenLibrary::SetDisplayBackgroundIndex(int32 BackgroundIndex)
{
	UAsyncLoadingScreenLibrary::DisplayBackgroundIndex = BackgroundIndex;

	// Winyunq's two strategic backgrounds use the same stable index contract as
	// the original manual-background API. Keeping this bridge on the established
	// exported function also lets older project modules opt into the new layout.
	const bool bEurope = BackgroundIndex == 1;
	const TCHAR* BackgroundPath = bEurope
		? TEXT("/Game/UI/LoadingScreen/BackgroundEurope.BackgroundEurope")
		: TEXT("/Game/UI/LoadingScreen/BackgroupEastAsia.BackgroupEastAsia");
	if (BackgroundIndex == 0 || BackgroundIndex == 1)
	{
		if (UTexture2D* Background = LoadObject<UTexture2D>(nullptr, BackgroundPath))
		{
			SetStrategicMapLoadingScreen(
				Background,
				bEurope ? 832.0f : 2702.0f,
				112.0f,
				4096.0f,
				4096.0f,
				!bEurope,
				FText::FromString(bEurope ? TEXT("欧洲战区 · PVE 1939") : TEXT("东亚战区 · PVE-R 1936")),
				FText::FromString(bEurope
					? TEXT("正在建立欧洲战场。游戏场景保持 4096×4096 原始比例；右侧扩展区显示行动简报与载入状态。\n\n目标地图：/Game/Map/Europe/64")
					: TEXT("正在建立东亚战场。游戏场景保持 4096×4096 原始比例；左侧扩展区显示行动简报与载入状态。\n\n目标地图：/Game/Map/EastAsia/64")),
				FText::FromString(bEurope ? TEXT("1939 · EUROPE") : TEXT("1936 · EAST ASIA")),
				FText::FromString(bEurope
					? TEXT("1939.01.01  战役开局\n1939.09.01  战争爆发\n\n法国城市将按威胁异步评估防线。")
					: TEXT("1936.01.01  战役开局\n1937.07.07  敌对生效\n\n中日初始中立，城市与席位沿用地标 Team Index。")));
			return;
		}
	}

	ClearStrategicMapLoadingScreen();
}

void UAsyncLoadingScreenLibrary::SetDisplayTipTextIndex(int32 TipTextIndex)
{
	UAsyncLoadingScreenLibrary::DisplayTipTextIndex = TipTextIndex;
}

void UAsyncLoadingScreenLibrary::SetDisplayMovieIndex(int32 MovieIndex)
{
	UAsyncLoadingScreenLibrary::DisplayMovieIndex = MovieIndex;	
}

void UAsyncLoadingScreenLibrary::SetEnableLoadingScreen(bool bIsEnableLoadingScreen)
{
	bShowLoadingScreen = bIsEnableLoadingScreen;
}

void UAsyncLoadingScreenLibrary::SetStrategicMapLoadingScreen(
	UTexture2D* Background,
	const float GameViewportLeft,
	const float GameViewportTop,
	const float GameViewportWidth,
	const float GameViewportHeight,
	const bool bAnchorBackgroundToRight,
	FText PrimaryTitle,
	FText PrimaryBody,
	FText SecondaryTitle,
	FText SecondaryBody)
{
	bUseStrategicMapLoadingScreen = IsValid(Background)
		&& GameViewportWidth > 0.0f
		&& GameViewportHeight > 0.0f;

	StrategicMapBackground.Reset(bUseStrategicMapLoadingScreen ? Background : nullptr);
	StrategicMapLoadingScreenData.Background = StrategicMapBackground.Get();
	StrategicMapLoadingScreenData.GameViewportRectPixels = FVector4(
		GameViewportLeft,
		GameViewportTop,
		GameViewportWidth,
		GameViewportHeight);
	StrategicMapLoadingScreenData.bAnchorBackgroundToRight = bAnchorBackgroundToRight;
	StrategicMapLoadingScreenData.PrimaryTitle = MoveTemp(PrimaryTitle);
	StrategicMapLoadingScreenData.PrimaryBody = MoveTemp(PrimaryBody);
	StrategicMapLoadingScreenData.SecondaryTitle = MoveTemp(SecondaryTitle);
	StrategicMapLoadingScreenData.SecondaryBody = MoveTemp(SecondaryBody);
}

void UAsyncLoadingScreenLibrary::ClearStrategicMapLoadingScreen()
{
	bUseStrategicMapLoadingScreen = false;
	StrategicMapBackground.Reset();
	StrategicMapLoadingScreenData = FStrategicMapLoadingScreenData();
}

void UAsyncLoadingScreenLibrary::StopLoadingScreen()
{
	GetMoviePlayer()->StopMovie();
}

