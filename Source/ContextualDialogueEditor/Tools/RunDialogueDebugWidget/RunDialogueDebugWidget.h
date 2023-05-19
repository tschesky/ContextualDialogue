#pragma once
#include "IContextualDialogueModuleInterface.h"

class RunDialogueDebugWidget : public IContextualDialogueListenerInterface, public TSharedFromThis<RunDialogueDebugWidget>
{
public:
	virtual ~RunDialogueDebugWidget() {}

	virtual void OnStartupModule() override;
	virtual void OnShutdownModule() override;

	void MakeMenuEntry(FMenuBuilder &MenuBuilder);

protected:
	TSharedPtr<FUICommandList> CommandList;
	void MapCommands();
	void RunDialogueQueryDebugWidget();
};
