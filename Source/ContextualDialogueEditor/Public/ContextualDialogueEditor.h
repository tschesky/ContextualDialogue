#pragma once

#include "Engine.h"
#include "IContextualDialogueModuleInterface.h"

DECLARE_LOG_CATEGORY_EXTERN(ContextualDialogueEditorModule, Log, All);

class FContextualDialogueEditorModule: public IContextualDialogueModuleInterface
{
public:
	/**
	 * IContextualDialogueModuleInterface implementation
	 */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual void AddModuleListeners() override;
	
	/**
	 *  Get the loaded module
	 */
	static inline FContextualDialogueEditorModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FContextualDialogueEditorModule>("ContextualDialogueEditor");
	}

	/**
	 *  Get the availability of the module
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("ContextualDialogueEditor");
	}

	/**
	 *  Getter for the root item of our Editor menu 
	 */
	static TSharedRef<FWorkspaceItem> GetMenuRoot() { return MenuRoot; }

	/**
	 *	Utility function to add an item to our menu. It only forwards the call to the MenuExtender
	 *
	 *	@param ExtensionDelegate	Delegate function called to populate the menu item
	 *	@param ExtensionHook		Hook to the part of the menu that's being extended
	 *	@param CommandList			List of commands to handle actions taken by the menu
	 *	@param Position				Relative position when placed in the ExtensionHook
	 */
	void AddMenuExtension(
		const FMenuExtensionDelegate& ExtensionDelegate,
		FName ExtensionHook,
		const TSharedPtr<FUICommandList> &CommandList = nullptr,
		EExtensionHook::Position Position = EExtensionHook::Before ) const;

protected:
	TSharedPtr<FExtensibilityManager> LevelEditorMenuExtensibilityManager;
	TSharedPtr<FExtender> MenuExtender;
	static TSharedRef<FWorkspaceItem> MenuRoot;

	/**
	 *  Create the dialogue menu to display in our editor. It's passed to the AddMenuBarExtension() function and is called
	 *  by the editor automatically when constructing the menu bar.
	 *
	 *  @param MenuBuilder	Menu Builder returned by the editor
	 */
	void CreateDialogueDropdownMenu(FMenuBarBuilder& MenuBuilder) const;

	/**
	 *  Populate the dialogue menu with entries to execute various actions. It's passed as a callback when constructing
	 *  the dropdown menu and is called by the editor automatically when constructing our particular menu.
	 *
	 *  @param MenuBuilder	Menu Builder returned by the editor
	 */
	void AddDialogueMenuEntries(FMenuBuilder& MenuBuilder) const;
};