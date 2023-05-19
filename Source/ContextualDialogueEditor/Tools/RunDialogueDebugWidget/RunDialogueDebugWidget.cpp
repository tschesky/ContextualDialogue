#include "RunDialogueDebugWidget.h"

#include "EditorUtilityWidget.h"
#include "FDialogueEditorUtils.h"
#include "ContextualDialogueEditor.h"
#include "LevelEditor.h"

#define LOCTEXT_NAMESPACE "RunDialogueDebugQueryWidget"

class RunDialogueDebugWidgetCommands : public TCommands<RunDialogueDebugWidgetCommands>
{
public:
         
	RunDialogueDebugWidgetCommands()
		: TCommands<RunDialogueDebugWidgetCommands>(
		TEXT("RunDialogueDebugWidget"), // Context name for fast lookup
		FText::FromString("Run Dialogue Debug Widget "), // Context name for displaying
		NAME_None,   // No parent context
		FAppStyle::GetAppStyleSetName() // Icon Style Set
		)
	{
	}
         
	virtual void RegisterCommands() override
	{
		UI_COMMAND(RunDialogueWidget, "Run dialogue query widget", "Run editor utility widget for dialogue query debugging",
				   EUserInterfaceActionType::Button, FInputChord());
	}
         
public:
	TSharedPtr<FUICommandInfo> RunDialogueWidget;
};

void RunDialogueDebugWidget::MapCommands()
{
	const auto& Commands = RunDialogueDebugWidgetCommands::Get();

	CommandList->MapAction(
		Commands.RunDialogueWidget,
		FExecuteAction::CreateSP(this, &RunDialogueDebugWidget::RunDialogueQueryDebugWidget),
		FCanExecuteAction());
}

void RunDialogueDebugWidget::OnStartupModule()
{
	CommandList = MakeShareable(new FUICommandList);
	RunDialogueDebugWidgetCommands::Register();
	MapCommands();
	FContextualDialogueEditorModule::Get().AddMenuExtension(
		FMenuExtensionDelegate::CreateRaw(this, &RunDialogueDebugWidget::MakeMenuEntry),
		FName("Debug"),
		CommandList);
	
	IContextualDialogueListenerInterface::OnStartupModule();
}

void RunDialogueDebugWidget::OnShutdownModule()
{
	RunDialogueDebugWidgetCommands::Unregister();
	IContextualDialogueListenerInterface::OnShutdownModule();
}

void RunDialogueDebugWidget::MakeMenuEntry(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(RunDialogueDebugWidgetCommands::Get().RunDialogueWidget);
}

void RunDialogueDebugWidget::RunDialogueQueryDebugWidget()
{
	FContextualDialogueEditorUtils::RunEditorWidgetFromPath("/ContextualDialogue/Widgets/DialogueQueryDebug/W_DialogueQueryDebug.W_DialogueQueryDebug");
}

#undef LOCTEXT_NAMESPACE