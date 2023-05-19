#include "RunWorldStateDebugWidget.h"

#include "FDialogueEditorUtils.h"
#include "ContextualDialogueEditor.h"

#define LOCTEXT_NAMESPACE "RunWorldStateDebugWidget"

class RunWorldStateWidgetCommands : public TCommands<RunWorldStateWidgetCommands>
{
public:
         
	RunWorldStateWidgetCommands()
		: TCommands<RunWorldStateWidgetCommands>(
		TEXT("RunWorldStateWidget"), // Context name for fast lookup
		FText::FromString("Run World State Debug Widget"), // Context name for displaying
		NAME_None,   // No parent context
		FAppStyle::GetAppStyleSetName() // Icon Style Set
		)
	{
	}
         
	virtual void RegisterCommands() override
	{
		UI_COMMAND(RunWorldWidget, "Run world state debug widget", "Run editor utility widget for displaying the current world state",
				   EUserInterfaceActionType::Button, FInputChord());
	}
         
public:
	TSharedPtr<FUICommandInfo> RunWorldWidget;
};

void RunWorldStateDebugWidget::MapCommands()
{
	const auto& Commands = RunWorldStateWidgetCommands::Get();

	CommandList->MapAction(
		Commands.RunWorldWidget,
		FExecuteAction::CreateSP(this, &RunWorldStateDebugWidget::ExecuteRunWorldStateDebugWidget),
		FCanExecuteAction());
}

void RunWorldStateDebugWidget::OnStartupModule()
{
	CommandList = MakeShareable(new FUICommandList);
	RunWorldStateWidgetCommands::Register();
	MapCommands();
	FContextualDialogueEditorModule::Get().AddMenuExtension(
		FMenuExtensionDelegate::CreateRaw(this, &RunWorldStateDebugWidget::MakeMenuEntry),
		FName("Debug"),
		CommandList);
	
	IContextualDialogueListenerInterface::OnStartupModule();
}

void RunWorldStateDebugWidget::OnShutdownModule()
{
	RunWorldStateWidgetCommands::Unregister();
	IContextualDialogueListenerInterface::OnShutdownModule();
}

void RunWorldStateDebugWidget::MakeMenuEntry(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(RunWorldStateWidgetCommands::Get().RunWorldWidget);
}

void RunWorldStateDebugWidget::ExecuteRunWorldStateDebugWidget()
{
	FContextualDialogueEditorUtils::RunEditorWidgetFromPath("/ContextualDialogue/Widgets/WorldContextDebug/W_WorldStateDebug.W_WorldStateDebug");
}
#undef LOCTEXT_NAMESPACE