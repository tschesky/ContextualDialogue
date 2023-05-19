#pragma once
#include "IContextualDialogueModuleInterface.h"

class RunWorldStateDebugWidget : public IContextualDialogueListenerInterface, public TSharedFromThis<RunWorldStateDebugWidget>
{
public:
	virtual ~RunWorldStateDebugWidget() {}
	
	virtual void OnStartupModule() override;
	virtual void OnShutdownModule() override;

	void MakeMenuEntry(FMenuBuilder &MenuBuilder);

protected:
	TSharedPtr<FUICommandList> CommandList;
	void MapCommands();
	void ExecuteRunWorldStateDebugWidget();
};
