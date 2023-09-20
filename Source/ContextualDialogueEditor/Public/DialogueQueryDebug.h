// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "DialogueManagerSubsystem.h"
#include "DialogueQueryDebug.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(DialogueQueryDebug, Log, All);

/**
 *  This is an Editor Utility Widget - a widget that can be called in the Editor and docked as a tab. This particular
 *  one displays the results of running Dialogue Queries - to be able to see exact scores of each dialogue line.
 */
UCLASS(BlueprintType)
class CONTEXTUALDIALOGUEEDITOR_API UDialogueQueryDebug : public UEditorUtilityWidget
{
	GENERATED_BODY()
	
public:
	UDialogueQueryDebug();
	virtual void NativeConstruct() override;

	/**
	 *  A function to be subscribed to the OnDialogueQueryFinished event dispatcher in the Dialogue Manager Subsystem.
	 *  Each time the query is finished, we need to get information about how the lines were scored in order to refresh
	 *  the information displayed in the widget.
	 *
	 *  @param Scores	The scores that have been awarded to dialogue lines by the DialogueManagerSubsystem
	 *  @param Lines	The array of all lines that have been returned from the query
	 */
	UFUNCTION()
	void OnDialogueQueryFinished( TArray<FLineScore> Scores, TArray<UContextualDialogueLine*> Lines);

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
	 *  Rebuild the widget whenever new query information is presented
	 */
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName = "RebuildWidget"))
	void RebuildDebugWidget();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue Debug Data")
	TArray<UContextualDialogueLine*> m_CurrentLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue Debug Data")
	TArray<FLineScore> m_CurrentScores;
};
