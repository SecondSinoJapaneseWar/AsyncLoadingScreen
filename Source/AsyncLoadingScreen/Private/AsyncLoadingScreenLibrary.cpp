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
#include "HAL/PlatformTime.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/ScopeLock.h"

int32 UAsyncLoadingScreenLibrary::DisplayBackgroundIndex = -1;
int32 UAsyncLoadingScreenLibrary::DisplayTipTextIndex = -1;
int32 UAsyncLoadingScreenLibrary::DisplayMovieIndex = -1;
bool  UAsyncLoadingScreenLibrary::bShowLoadingScreen = true;
bool UAsyncLoadingScreenLibrary::bUseStrategicMapLoadingScreen = false;
bool UAsyncLoadingScreenLibrary::bWaitForGameplayReady = false;
bool UAsyncLoadingScreenLibrary::bLoadingProgressTrackingActive = false;
TStrongObjectPtr<UTexture2D> UAsyncLoadingScreenLibrary::StrategicMapBackground;
FStrategicMapLoadingScreenData UAsyncLoadingScreenLibrary::StrategicMapLoadingScreenData;
FCriticalSection UAsyncLoadingScreenLibrary::LoadingProgressMutex;
FText UAsyncLoadingScreenLibrary::LoadingStageText;
FText UAsyncLoadingScreenLibrary::LoadingDetailText;
FString UAsyncLoadingScreenLibrary::LoadingHistoryKey;
double UAsyncLoadingScreenLibrary::LoadingStartedAtSeconds = 0.0;
float UAsyncLoadingScreenLibrary::LoadingExpectedDurationSeconds = 12.0f;
float UAsyncLoadingScreenLibrary::LoadingReportedProgress = 0.0f;

namespace
{
	const TCHAR* LoadingHistorySection = TEXT("AsyncLoadingScreen.LoadingHistory");
}

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
	bWaitForGameplayReady = bUseStrategicMapLoadingScreen;

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
	bWaitForGameplayReady = false;
	StrategicMapBackground.Reset();
	StrategicMapLoadingScreenData = FStrategicMapLoadingScreenData();
}

void UAsyncLoadingScreenLibrary::SetWaitForGameplayReady(const bool bShouldWait)
{
	bWaitForGameplayReady = bShouldWait;
}

void UAsyncLoadingScreenLibrary::BeginLoadingProgressTracking()
{
	const FString ProfileKey = StrategicMapLoadingScreenData.bAnchorBackgroundToRight
		? TEXT("EastAsia")
		: TEXT("Europe");
	float HistoricalSeconds = ProfileKey == TEXT("EastAsia") ? 14.0f : 11.0f;
	if (GConfig)
	{
		GConfig->GetFloat(
			LoadingHistorySection,
			*(ProfileKey + TEXT("AverageSeconds")),
			HistoricalSeconds,
			GGameUserSettingsIni);
	}

	FScopeLock Lock(&LoadingProgressMutex);
	bLoadingProgressTrackingActive = true;
	LoadingHistoryKey = ProfileKey;
	LoadingStartedAtSeconds = FPlatformTime::Seconds();
	LoadingExpectedDurationSeconds = FMath::Clamp(HistoricalSeconds, 2.0f, 120.0f);
	LoadingReportedProgress = 0.02f;
	LoadingStageText = FText::FromString(TEXT("正在载入战区地图"));
	LoadingDetailText = FText::FromString(FString::Printf(
		TEXT("依据历史记录预计约 %.1f 秒；正在解析地图包与依赖资源"),
		LoadingExpectedDurationSeconds));
}

void UAsyncLoadingScreenLibrary::UpdateLoadingProgress(
	FText Stage,
	FText Detail,
	const float Progress)
{
	FScopeLock Lock(&LoadingProgressMutex);
	if (!bLoadingProgressTrackingActive)
	{
		return;
	}
	LoadingStageText = MoveTemp(Stage);
	LoadingDetailText = MoveTemp(Detail);
	LoadingReportedProgress = FMath::Max(
		LoadingReportedProgress,
		FMath::Clamp(Progress, 0.0f, 0.99f));
}

float UAsyncLoadingScreenLibrary::GetEstimatedLoadingProgress()
{
	FScopeLock Lock(&LoadingProgressMutex);
	if (!bLoadingProgressTrackingActive)
	{
		return 0.0f;
	}
	const float ElapsedSeconds = static_cast<float>(
		FPlatformTime::Seconds() - LoadingStartedAtSeconds);
	// During the opaque map-package load UE has no trustworthy item count. Use
	// the learned duration only up to 70%; observable readiness stages own 70-100%.
	const float TimeEstimate = FMath::Min(
		0.70f,
		0.70f * ElapsedSeconds / FMath::Max(LoadingExpectedDurationSeconds, 0.1f));
	return FMath::Clamp(FMath::Max(LoadingReportedProgress, TimeEstimate), 0.0f, 1.0f);
}

FText UAsyncLoadingScreenLibrary::GetLoadingStageText()
{
	FScopeLock Lock(&LoadingProgressMutex);
	return LoadingStageText;
}

FText UAsyncLoadingScreenLibrary::GetLoadingDetailText()
{
	FScopeLock Lock(&LoadingProgressMutex);
	return LoadingDetailText;
}

void UAsyncLoadingScreenLibrary::StopLoadingScreen()
{
	FString CompletedHistoryKey;
	float CompletedSeconds = 0.0f;
	{
		FScopeLock Lock(&LoadingProgressMutex);
		if (bLoadingProgressTrackingActive)
		{
			CompletedHistoryKey = LoadingHistoryKey;
			CompletedSeconds = static_cast<float>(
				FPlatformTime::Seconds() - LoadingStartedAtSeconds);
			LoadingReportedProgress = 1.0f;
			LoadingStageText = FText::FromString(TEXT("战区已就绪"));
			LoadingDetailText = FText::FromString(TEXT("正在部署指挥界面"));
			bLoadingProgressTrackingActive = false;
		}
	}

	if (GConfig && !CompletedHistoryKey.IsEmpty() && CompletedSeconds > 0.0f)
	{
		const FString AverageKey = CompletedHistoryKey + TEXT("AverageSeconds");
		const FString SamplesKey = CompletedHistoryKey + TEXT("Samples");
		float PreviousAverage = CompletedSeconds;
		int32 PreviousSamples = 0;
		GConfig->GetFloat(
			LoadingHistorySection, *AverageKey, PreviousAverage, GGameUserSettingsIni);
		GConfig->GetInt(
			LoadingHistorySection, *SamplesKey, PreviousSamples, GGameUserSettingsIni);
		// Keep early samples responsive, then use an exponential moving average so
		// driver/content changes are learned without one outlier wrecking the bar.
		const float Blend = PreviousSamples < 4
			? 1.0f / static_cast<float>(PreviousSamples + 1)
			: 0.20f;
		const float NewAverage = FMath::Lerp(PreviousAverage, CompletedSeconds, Blend);
		GConfig->SetFloat(
			LoadingHistorySection, *AverageKey, NewAverage, GGameUserSettingsIni);
		GConfig->SetInt(
			LoadingHistorySection, *SamplesKey, PreviousSamples + 1, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
	GetMoviePlayer()->StopMovie();
}

