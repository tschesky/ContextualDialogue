#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "DialogueManagerSubsystem.h"
#include "WorldStateDebug.generated.h"

/**
 *  This is an Editor Utility Widget - a widget that can be called in the Editor and docked as a tab. This particular
 *  one displays the current state of the world context
 */
UCLASS(BlueprintType)
class CONTEXTUALDIALOGUEEDITOR_API UWorldStateDebug : public UEditorUtilityWidget
{
	GENERATED_BODY()
	
public:
	UWorldStateDebug();
	virtual void NativeConstruct() override;

	/**
	 *  A function to be subscribed to the OnWorldStateUpdated event dispatcher in the Dialogue Manager Subsystem.
	 *  Each time the world state is updated/populated we need to rebuild the widget with all the new information
	 *
	 *  @param WorldState	The scores that have been awarded to dialogue lines by the DialogueManagerSubsystem
	 */
	UFUNCTION()
	void OnWorldStateUpdated(const TArray<FObjectValueMapping>& WorldState);

	/**
	 *  A function to be subscribed to the PIEStarted event dispatcher of the editor. We need to get a reference to the Dialogue
	 *  Manager Subsystem - and that subsystem is only crated after a play session has been initiated. Hence, we need to
	 *  subscribe to the PIE event. We don't care about a standalone game play session - as this widget is editor-only anyway.
	 *
	 *  @param bIsSimulating	Param required by this callback, the value will be passed from the editor
	 */
	UFUNCTION()
	void PIEStarted(bool bIsSimulating);

	/**
	 *  Rebuild the widget whenever new world state information is presented
	 */
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName = "RebuildWidget"))
	void RebuildDebugWidget();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue Debug Data")
	TArray<FObjectValueMapping> m_CurrentWorldState;
};
