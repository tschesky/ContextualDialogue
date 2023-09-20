// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueQueryDebug.h"

#include "FArrayUtils.h"
#include "FDialogueEditorUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Algo/Reverse.h"

#define DEBUG_MSG(x, ...) if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT(x), __VA_ARGS__));}
DEFINE_LOG_CATEGORY(DialogueQueryDebug)

UDialogueQueryDebug::UDialogueQueryDebug()
{
}

void UDialogueQueryDebug::NativeConstruct()
{
	Super::NativeConstruct();

	// Register our function to be run whenever an Editor PIE sessionis started
	FEditorDelegates::PostPIEStarted.AddUFunction(this, FName("PIEStarted"));
}

void UDialogueQueryDebug::OnDialogueQueryFinished(TArray<FLineScore> Scores, TArray<UContextualDialogueLine*> Lines)
{
	UE_LOG(DialogueQueryDebug, Warning, TEXT("WIDGET RECEIVED QUERY UPDATE. Num lines : %i ; Num scores: %i"), Lines.Num(), Scores.Num());

	// Parallel sort two arrays - in order to get the lines in ascending sort order
	FArrayUtils::SortArray(Scores, [&Scores, &Lines](const int32 Index1, const int32 Index2) {
		Scores.SwapMemory(Index2, Index1);
		Lines.SwapMemory(Index2, Index1);
	});

	// Revert the arrays to actually get them in descending orer
	Algo::Reverse(Lines);
	Algo::Reverse(Scores);
	
	m_CurrentLines = Lines;
	m_CurrentScores = Scores;

	// Rebuilt the widget with new information
	RebuildDebugWidget();
}

void UDialogueQueryDebug::PIEStarted(bool bIsSimulating)
{
	if (const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(FContextualDialogueEditorUtils::MyGetWorld()))
	{
		if(UDialogueManagerSubsystem* MySubsystem = GameInstance->GetSubsystem<UDialogueManagerSubsystem>())
		{
			UE_LOG(DialogueQueryDebug, Warning, TEXT("REGISTERING WITH DIALOGUE SUBSYSTEM"));

			// Obtain our Dialogue Manager Subsystem and subscribe to its dialogue query delegate
			MySubsystem->OnDialogueQueryFinished.AddDynamic(this, &UDialogueQueryDebug::OnDialogueQueryFinished);
		}
	}
}