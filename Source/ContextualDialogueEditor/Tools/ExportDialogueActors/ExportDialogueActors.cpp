// ReSharper disable CppMemberFunctionMayBeStatic
#include "ExportDialogueActors.h"

#include "ContextualDialogueEditor.h"
#include "DialogueManagerSubsystem.h"
#include "DialogueQueryDebug.h"
#include "FDialogueEditorUtils.h"
#include "LevelEditor.h"
#include "UnrealEdGlobals.h"
#include "Dialogs/Dialogs.h"
#include "Editor/UnrealEdEngine.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#define LOCTEXT_NAMESPACE "ExportDialogueActors"

class ExportDialogueActorsCommands : public TCommands<ExportDialogueActorsCommands>
         {
         public:
         
         	ExportDialogueActorsCommands()
         		: TCommands<ExportDialogueActorsCommands>(
         		TEXT("ExportDialogueActors"), // Context name for fast lookup
         		FText::FromString("Export Dialogue Actors"), // Context name for displaying
         		NAME_None,   // No parent context
         		FAppStyle::GetAppStyleSetName() // Icon Style Set
         		)
         	{
         	}
         
         	virtual void RegisterCommands() override
         	{
         		UI_COMMAND(ExportToFile, "Export current level to file", "Export information about dialogue Actors",
         		           EUserInterfaceActionType::Button, FInputChord());
         	}
         
         public:
         	TSharedPtr<FUICommandInfo> ExportToFile;
};

void ExportDialogueActors::MapCommands()
{
	const auto& Commands = ExportDialogueActorsCommands::Get();

	CommandList->MapAction(
		Commands.ExportToFile,
		FExecuteAction::CreateSP(this, &ExportDialogueActors::ExportDialogueActorsToFile),
		FCanExecuteAction());
}

void ExportDialogueActors::OnStartupModule()
{
	CommandList = MakeShareable(new FUICommandList);
	ExportDialogueActorsCommands::Register();
	MapCommands();
	FContextualDialogueEditorModule::Get().AddMenuExtension(
		FMenuExtensionDelegate::CreateRaw(this, &ExportDialogueActors::MakeMenuEntry),
		FName("Export"),
		CommandList);
	
	IContextualDialogueListenerInterface::OnStartupModule();
}

void ExportDialogueActors::OnShutdownModule()
{
	ExportDialogueActorsCommands::Unregister();
	IContextualDialogueListenerInterface::OnShutdownModule();
}

void ExportDialogueActors::MakeMenuEntry(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(ExportDialogueActorsCommands::Get().ExportToFile);
}

void ExportDialogueActors::ExportDialogueActorsToFile()
{	
	UE_LOG(LogClass, Log, TEXT("Clicked the Menu thing"));

	// Open up a modal for the user to select a save path
	FString PathToSaveIn;
	PromptUserForDirectory(
		PathToSaveIn,
		"Please select the path where the file should be exported",
		FPaths::ProjectDir());
	ExportPath = FPaths::Combine(PathToSaveIn, TEXT("WorldContext.json"));
	
	// Get the active viewport in Editor
	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	TSharedPtr<class IAssetViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport();

	// Setup play parameters for the PIE
	FRequestPlaySessionParams PIEParams;
	PIEParams.DestinationSlateViewport = ActiveLevelViewport;
	PIEParams.WorldType = EPlaySessionWorldType::PlayInEditor;

	// Add a delegate, since PIE takes a while to start
	PIEStartedDelegateHandle = FEditorDelegates::PostPIEStarted.AddRaw(this, &ExportDialogueActors::PIEStarted);
	GUnrealEd->RequestPlaySession(PIEParams);	
}

void ExportDialogueActors::PIEStarted(bool bIsSimulating)
{
	if (const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(FContextualDialogueEditorUtils::MyGetWorld()))
	{
		if(UDialogueManagerSubsystem* MySubsystem = GameInstance->GetSubsystem<UDialogueManagerSubsystem>())
		{
			// Get the world context as a JSOn object
			const TSharedPtr<FJsonObject> ContextAsJSON = MySubsystem->GetContextVariablesAsJSON();

			FString OutputString;
			const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
			FJsonSerializer::Serialize(ContextAsJSON.ToSharedRef(), Writer);

			// Save context to the selected file
			FFileHelper::SaveStringToFile(OutputString, *ExportPath);

			FEditorDelegates::PostPIEStarted.Remove(PIEStartedDelegateHandle);
			
			// Exit PIE
			GUnrealEd->RequestEndPlayMap();
		}
	}
}

#undef LOCTEXT_NAMESPACE