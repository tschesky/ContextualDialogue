#pragma once

#include "IContextualDialogueModuleInterface.h"

class ExportDialogueActors : public IContextualDialogueListenerInterface, public TSharedFromThis<ExportDialogueActors>
{
public:
	virtual ~ExportDialogueActors() {}
	
	virtual void OnStartupModule() override;
	virtual void OnShutdownModule() override;

	void MakeMenuEntry(FMenuBuilder &MenuBuilder);
	void PIEStarted(bool bIsSimulating);

protected:
	TSharedPtr<FUICommandList> CommandList;
	void MapCommands();
	void ExportDialogueActorsToFile();
	FDelegateHandle PIEStartedDelegateHandle;

	FString ExportPath;
};
