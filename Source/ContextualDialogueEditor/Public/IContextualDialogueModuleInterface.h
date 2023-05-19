#pragma once

/**
 *  Simple interface to implement in our editor tools to reduce the amount of boilerplate code. Any new tool simply has to
 *  implement this interface and make sure to perform all of its startup/cleanup actions in the appropriate functions.
 *  Later we just need to make sure that the desired tool is registered with an EditorModule in the ModuleListeners[] array
 */
class IContextualDialogueListenerInterface
{
public:
	/**
	 *  Any actions to be performed on startup of the module should be placed here
	 */
	virtual void OnStartupModule() {};

	/**
	 *  Any actions to be performed on shutdown of the module should be placed here
	 */
	virtual void OnShutdownModule() {};
};

/**
 *  The interface to implement in out editor modules.
 */
class IContextualDialogueModuleInterface : public IModuleInterface
{
public:

	/**
	 *  The default implementation ensures that we run Startup() functions of all our listeners
	 */
	virtual void StartupModule() override
	{
		if (!IsRunningCommandlet())
		{
			AddModuleListeners();
			for (int32 i = 0; i < ModuleListeners.Num(); ++i)
			{
				ModuleListeners[i]->OnStartupModule();
			}
		}
	}

	/**
	 *  The default implementation ensures that we run Shutdown() functions of all our listeners
	 */
	virtual void ShutdownModule() override
	{
		for (int32 i = 0; i < ModuleListeners.Num(); ++i)
		{
			ModuleListeners[i]->OnShutdownModule();
		}
	}

	/**
	 * Adds a lister to the module
	 */
	virtual void AddModuleListeners() {};

protected:
	// Contains all the tools registered as listeners to this module
	TArray<TSharedRef<IContextualDialogueListenerInterface>> ModuleListeners;
};
