/************************************************************************************
 *																					*
 * Copyright (C) 2020 Truong Bui.													*
 * Website:	https://github.com/truong-bui/AsyncLoadingScreen						*
 * Licensed under the MIT License. See 'LICENSE' file for full license information. *
 *																					*
 ************************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/StrongObjectPtr.h"
#include "AsyncLoadingScreenLibrary.generated.h"

class UTexture2D;

/** Runtime-only data used by the optional strategic-map loading layout. */
struct ASYNCLOADINGSCREEN_API FStrategicMapLoadingScreenData
{
	UTexture2D* Background = nullptr;
	FVector4 GameViewportRectPixels = FVector4(0.0, 0.0, 1.0, 1.0);
	bool bAnchorBackgroundToRight = false;
	FText PrimaryTitle;
	FText PrimaryBody;
	FText SecondaryTitle;
	FText SecondaryBody;
};

/**
 * Async Loading Screen Function Library
 */
UCLASS()
class ASYNCLOADINGSCREEN_API UAsyncLoadingScreenLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
private:
	static int32 DisplayBackgroundIndex;
	static int32 DisplayTipTextIndex;
	static int32 DisplayMovieIndex;	
	static bool  bShowLoadingScreen;
	static bool bUseStrategicMapLoadingScreen;
	static TStrongObjectPtr<UTexture2D> StrategicMapBackground;
	static FStrategicMapLoadingScreenData StrategicMapLoadingScreenData;
public:
	
	/**
	 * Set which background will be displayed on the loading screen by index. The "SetDisplayBackgroundManually" option in Background setting needs to be "true" to use this function.
	 * 
	 * @param BackgroundIndex Valid index of the Background in "Images" array in Background setting. If the index is not valid, then it will display random background instead.
	 **/
	UFUNCTION(BlueprintCallable, Category = "Async Loading Screen")
	static void SetDisplayBackgroundIndex(int32 BackgroundIndex);

	/**
	 * Set which text will be displayed on the loading screen by index. The "SetDisplayTipTextManually" option in Tip Widget setting needs to be "true" to use this function.
	 *
	 * @param TipTextIndex Valid index of the text in "TipText" array in Tip Widget setting. If the index is not valid, then it will display random text instead.
	 **/
	UFUNCTION(BlueprintCallable, Category = "Async Loading Screen")
	static void SetDisplayTipTextIndex(int32 TipTextIndex);

	/**
	 * Set which movie will be displayed on the loading screen by index. The "SetDisplayMovieIndexManually" option needs to be "true" to use this function.
	 *
	 * @param MovieIndex Valid index of the movie in "MoviePaths" array.
	 **/
	UFUNCTION(BlueprintCallable, Category = "Async Loading Screen")
	static void SetDisplayMovieIndex(int32 MovieIndex);


	/**
	 * Set enable/disable the loading screen for next levels
	 *
	 * @param bIsEnableLoadingScreen Should we enable the loading screen for next level?
	 **/
	UFUNCTION(BlueprintCallable, Category = "Async Loading Screen")
	static void SetEnableLoadingScreen(bool bIsEnableLoadingScreen);

	/**
	 * Configure the next loading screen as an aspect-correct strategic map.
	 *
	 * GameViewport* values are source-texture pixels. The highlighted frame is
	 * painted outside this rectangle so the gameplay image itself is never covered.
	 * Anchor-to-right crops the left side first (East Asia); false crops the right
	 * side first (Europe).
	 */
	UFUNCTION(BlueprintCallable, Category = "Async Loading Screen|Strategic Map")
	static void SetStrategicMapLoadingScreen(
		UTexture2D* Background,
		float GameViewportLeft,
		float GameViewportTop,
		float GameViewportWidth,
		float GameViewportHeight,
		bool bAnchorBackgroundToRight,
		FText PrimaryTitle,
		FText PrimaryBody,
		FText SecondaryTitle,
		FText SecondaryBody);

	/** Restore the layout selected in project settings for subsequent travel. */
	UFUNCTION(BlueprintCallable, Category = "Async Loading Screen|Strategic Map")
	static void ClearStrategicMapLoadingScreen();


	/**
	 * Get enable/disable the loading screen for next levels
	 *
	 **/
	UFUNCTION(BlueprintPure, Category = "Async Loading Screen")
	static inline bool GetIsEnableLoadingScreen() { return bShowLoadingScreen; }

	/**
	 * Stop the loading screen. To use this function, you must enable the "bAllowEngineTick" option.
	 * Call this function in BeginPlay event to stop the Loading Screen (works with Delay node).
	 *
	 **/
	UFUNCTION(BlueprintCallable, Category = "Async Loading Screen")
	static void StopLoadingScreen();

	static inline int32 GetDisplayBackgroundIndex() { return DisplayBackgroundIndex; }
	static inline int32 GetDisplayTipTextIndex() { return DisplayTipTextIndex; }
	static inline int32 GetDisplayMovieIndex() { return DisplayMovieIndex; }
	static inline bool IsStrategicMapLoadingScreenEnabled() { return bUseStrategicMapLoadingScreen; }
	static inline const FStrategicMapLoadingScreenData& GetStrategicMapLoadingScreenData() { return StrategicMapLoadingScreenData; }

};
