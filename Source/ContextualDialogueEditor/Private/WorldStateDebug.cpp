#include "WorldStateDebug.h"

#include "FDialogueEditorUtils.h"
#include "Kismet/GameplayStatics.h"

UWorldStateDebug::UWorldStateDebug()
{
	
}

void UWorldStateDebug::NativeConstruct()
{
	Super::NativeConstruct();

	// Register our function to be run whenever an Editor PIE session is started
	FEditorDelegates::PostPIEStarted.AddUFunction(this, FName("PIEStarted"));
}

void UWorldStateDebug::OnWorldStateUpdated(const TArray<FObjectValueMapping>& WorldState)
{
	m_CurrentWorldState = WorldState;

	RebuildDebugWidget();
}

void UWorldStateDebug::PIEStarted(bool bIsSimulating)
{
	if (const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(FContextualDialogueEditorUtils::MyGetWorld()))
	{
		if(UDialogueManagerSubsystem* MySubsystem = GameInstance->GetSubsystem<UDialogueManagerSubsystem>())
		{
			// Obtain our Dialogue Manager Subsystem and subscribe to its dialogue query delegate
			MySubsystem->OnWorldStateUpdated.AddDynamic(this, &UWorldStateDebug::OnWorldStateUpdated);

			TArray<FObjectValueMapping> OutArray;
			MySubsystem->GetWorldState().GenerateValueArray(OutArray);
			OnWorldStateUpdated(OutArray);
		}
	}
}

