/************************************************************************************
 *																					*
 * Copyright (C) 2020 Truong Bui.													*
 * Website:	https://github.com/truong-bui/AsyncLoadingScreen						*
 * Licensed under the MIT License. See 'LICENSE' file for full license information. *
 *																					*
 ************************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

struct FALoadingScreenSettings;
struct FWorldContext;
class UGameViewportClient;
class SWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogAsyncLoadingScreen, Log, All);

class FAsyncLoadingScreenModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */

	/**
	 * Called right after the module DLL has been loaded and the module object has been created
	 */
	virtual void StartupModule() override;
	
	/**
	 * Called before the module is unloaded, right before the module object is destroyed.
	 */
	virtual void ShutdownModule() override;

	/**
	 * Returns true if this module hosts gameplay code 
	 * 
	 * @return True for "gameplay modules", or false for engine code modules, plugins, etc.
	 */
	virtual bool IsGameModule() const override;

	/**
	 * Singleton-like access to this module's interface. This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FAsyncLoadingScreenModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FAsyncLoadingScreenModule>("AsyncLoadingScreen");
	}

	/**
	 * Checks to see if this module is loaded and ready. It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("AsyncLoadingScreen");
	}

	/**
	 * Is showing Startup Loading Screen?
	 */
	bool IsStartupLoadingScreen() { return bIsStartupLoadingScreen; }

	/** Explicitly starts a post-startup loading screen (required by seamless travel). */
	void StartLoadingScreen(UGameViewportClient* TargetViewport = nullptr);

	/** Stops either the runtime movie player or the editor viewport fallback. */
	void StopLoadingScreen();

private:
	/**
	 * Loading screen callback, it won't be called if we've already explicitly setup the loading screen
	 */
	void PreSetupLoadingScreen();

	/**
	 * Setup loading screen settings 
	 */
	void SetupLoadingScreen(const FALoadingScreenSettings& LoadingScreenSettings);

	/**
	 * Shuffle the movies list
	 */
	void ShuffleMovies(TArray<FString>& MoviesList);

#if WITH_EDITOR
	void HandleEditorPreLoadMap(
		const FWorldContext& WorldContext,
		const FString& MapName);
	void HandleEditorPostLoadMap(UWorld* LoadedWorld);
	void ShowEditorLoadingScreen(UGameViewportClient* TargetViewport);
	void HideEditorLoadingScreen();

	TMap<TWeakObjectPtr<UGameViewportClient>, TSharedPtr<SWidget>>
		EditorLoadingScreenWidgets;
#endif
private:

	bool bIsStartupLoadingScreen = false;
};
