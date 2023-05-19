#include "ContextualDialogueEditor.h"

#include "LevelEditor.h"
#include "ContextualDialogueEditor/Tools/ExportDialogueActors/ExportDialogueActors.h"
#include "ContextualDialogueEditor/Tools/RunDialogueDebugWidget/RunDialogueDebugWidget.h"
#include "ContextualDialogueEditor/Tools/RunWorldStateDebugWidget/RunWorldStateDebugWidget.h"

IMPLEMENT_GAME_MODULE(FContextualDialogueEditorModule, ContextualDialogueEditor);

DEFINE_LOG_CATEGORY(ContextualDialogueEditorModule)

#define LOCTEXT_NAMESPACE "ContextualDialogueEditor"

void FContextualDialogueEditorModule::StartupModule()
{
	// Only run in full editor, ignore commandlet calls
	if (!IsRunningCommandlet())
	{
		// Get the editor extensibility manager
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorMenuExtensibilityManager = LevelEditorModule.GetMenuExtensibilityManager();

		// Create a menu extender, using our functions to populate the new menu
		MenuExtender = MakeShareable(new FExtender);
		MenuExtender->AddMenuBarExtension(
			"Window",
			EExtensionHook::After,
			nullptr,
			FMenuBarExtensionDelegate::CreateRaw(this, &FContextualDialogueEditorModule::CreateDialogueDropdownMenu));

		// Add our extender to the manager to be automatically called when constructing the toolbar menu
		LevelEditorMenuExtensibilityManager->AddExtender(MenuExtender);
	}

	IContextualDialogueModuleInterface::StartupModule();
	
	UE_LOG(ContextualDialogueEditorModule, Warning, TEXT("FContextualDialogueEditorModule: Log Started"));
}

void FContextualDialogueEditorModule::ShutdownModule()
{
	UE_LOG(ContextualDialogueEditorModule, Warning, TEXT("FContextualDialogueEditorModule: Log Ended"));
}

void FContextualDialogueEditorModule::AddModuleListeners()
{
	// Register desired tools
	ModuleListeners.Add(MakeShareable(new ExportDialogueActors));
	ModuleListeners.Add(MakeShareable(new RunDialogueDebugWidget));
	ModuleListeners.Add(MakeShareable(new RunWorldStateDebugWidget));
}

void FContextualDialogueEditorModule::AddMenuExtension(const FMenuExtensionDelegate& ExtensionDelegate,
	FName ExtensionHook, const TSharedPtr<FUICommandList>& CommandList, EExtensionHook::Position Position) const
{
	// Forward the call to MenuExtender
	MenuExtender->AddMenuExtension(ExtensionHook, Position, CommandList, ExtensionDelegate);
}

void FContextualDialogueEditorModule::CreateDialogueDropdownMenu(FMenuBarBuilder& MenuBuilder) const
{
	// Create ur Dialogue Tools dropdown menu
	MenuBuilder.AddPullDownMenu(
		FText::FromString("Dialogue tools menu"),
		FText::FromString("Open the dialogue tools menu"),
		FNewMenuDelegate::CreateRaw(this, &FContextualDialogueEditorModule::AddDialogueMenuEntries),
		"Dialogue tools menu",
		FName(TEXT("DialogueToolsMenu"))
	);
}

void FContextualDialogueEditorModule::AddDialogueMenuEntries(FMenuBuilder& MenuBuilder) const
{
	// Add section for exporting dialogue
	MenuBuilder.BeginSection("ExportTools", FText::FromString("Export tools"));
	MenuBuilder.AddMenuSeparator(FName("Export"));
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("DialogueDebug", FText::FromString("Dialogue debug"));
	MenuBuilder.AddMenuSeparator(FName("Debug"));
	MenuBuilder.EndSection();
}

#undef LOCTEXT_NAMESPACE
