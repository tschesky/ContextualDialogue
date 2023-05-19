#pragma once
#include "EditorUtilityWidget.h"
#include "LevelEditor.h"
#include "WidgetBlueprint.h"

class FContextualDialogueEditorUtils
{
public:
	/**
	 *  Taken from : https://stackoverflow.com/questions/58582638/get-game-state-inside-editor-widget-in-unreal-engine-4
	 *  This is a custom implementation to GetWorld() which performs all the necessary checks and returns a World()
	 *  object regardless of whether we're in a PIE session or a GamePreview mode.
	 */
	static inline UWorld* MyGetWorld()
	{
		UWorld* Pie = nullptr;
		UWorld* GamePreview = nullptr;

		for (FWorldContext const& Context : GEngine->GetWorldContexts())
		{
			switch (Context.WorldType)
			{
			case EWorldType::PIE:
				Pie = Context.World();
				break;
			case EWorldType::GamePreview:
				GamePreview = Context.World();
				break;
			default:
				break;
			}
		}

		if (Pie)
		{
			return Pie;
		}
		else if (GamePreview)
		{
			return GamePreview;
		}

		return nullptr;
	}

	/**
	 *  Spawn a new editor tab (empty one) and place a Widget inside of it
	 *
	 *  @param SpawnTabArgs	Arguments to the tab creation
	 *  @param Blueprint	Blueprint class to be constructed
	 *  @return				Returns the created and populated tab 
	 */
	static inline TSharedRef<SDockTab> CreateUITab(const FSpawnTabArgs& SpawnTabArgs, UBlueprint* Blueprint)
	{
		UWorld* World = GEditor->GetEditorWorldContext().World();
		check(World);

		TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab);
		TSubclassOf<UUserWidget> WidgetClass = static_cast<TSubclassOf<UUserWidget>>(Blueprint->GeneratedClass);
		
		UEditorUtilityWidget* CreatedUMGWidget = Cast<UEditorUtilityWidget>(CreateWidget(World, WidgetClass));
		if (CreatedUMGWidget)
		{
			TSharedRef<SWidget> CreatedSlateWidget = CreatedUMGWidget->TakeWidget();
			SpawnedTab->SetContent(CreatedSlateWidget);
		}

		return SpawnedTab;
	}

	/**
	 *  Utility function to run an Editor Utility Widget with a click of a button - normally you have to locate the widget,
	 *  right click it and select "run".
	 *
	 *  @param Path	Path to run the widget from
	 */
	static inline void RunEditorWidgetFromPath(FString Path)
	{
		// Load the Widget object form given path
		UObject* Obj = LoadObject<UObject>(
			nullptr,
			*Path,
			nullptr,
			LOAD_None,
			nullptr
		);

		// Cast it to a Blueprint
		UBlueprint* Blueprint = Cast<UBlueprint, UObject>(Obj);

		// Make sure it actually is an EditorUtilityWidget
		if (Blueprint->GeneratedClass->IsChildOf(UEditorUtilityWidget::StaticClass()))
		{
			// Construct a default object
			const UEditorUtilityWidget* CDO = Cast<const UEditorUtilityWidget>(Blueprint->GeneratedClass->GetDefaultObject());
			if (CDO->ShouldAutoRunDefaultAction())
			{
				// If this is an instant-run blueprint, just execute it
				UEditorUtilityWidget* Instance = NewObject<UEditorUtilityWidget>(GetTransientPackage(), Blueprint->GeneratedClass);
				Instance->ExecuteDefaultAction();
			}
			else
			{
				// Otherwise register and display the sidget
				FName RegistrationName = FName(*(Blueprint->GetPathName() + TEXT("_ActiveTab")));
				FText DisplayName = FText::FromString(Blueprint->GetName());
				FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
				TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();

				if (!LevelEditorTabManager->HasTabSpawner(RegistrationName))
				{
					UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint, UBlueprint>(Blueprint);
					LevelEditorTabManager->RegisterTabSpawner(RegistrationName, FOnSpawnTab::CreateStatic(&CreateUITab, Blueprint))
						.SetDisplayName(DisplayName)
						.SetMenuType(ETabSpawnerMenuType::Hidden);
				}

				TSharedPtr<SDockTab> NewDockTab = LevelEditorTabManager->TryInvokeTab(RegistrationName);
			}
		}
	}
};
