// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueContextComponent.h"
#include "DialogueManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(DialogueContextComponent);

UDialogueContextComponent::UDialogueContextComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UDialogueContextComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = this->GetOwner();
	
	// Initialize int and string variables with whatever values are present in the Owner
	IntVars = PopulateIntStateVariables();
	StrVars = PopulateStrStateVariables();
	CallbackNames = PopulateCallbackStateVariables();

	// In case this property was left empty by dum-dum designers
	if(DSS_Name.IsEmpty())
		DSS_Name = GetOwner()->GetName();

	// Subscribe to the manager subsystem
	const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this->GetWorld());
	UDialogueManagerSubsystem* MySubsystem = GameInstance->GetSubsystem<UDialogueManagerSubsystem>();

	if(!MySubsystem)
		return;
	
	MySubsystem->SubscribeNewDSSComponent(this);
}

void UDialogueContextComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UDialogueManagerSubsystem* MySubsystem = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<UDialogueManagerSubsystem>();

	if(!MySubsystem)
		return;
	
	MySubsystem->UnsubscribeDSSComponent(this);
}

void UDialogueContextComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

TMap<FString, int> UDialogueContextComponent::PopulateIntStateVariables()
{
	TMap<FString, int> Result;

	// Iterate over all of the FIntProperties present in the owning actor
	for (TFieldIterator<FIntProperty> PropIt(Owner->GetClass()); PropIt; ++PropIt)
	{
		// Get property from iterator
		const FProperty* Property = *PropIt;

		// Check if it's our Dialogue System State (we rely on the variable names being prefixed with "DSS_")
		if(Property->GetName().Contains("DSS_"))
		{
			// Get value of the property and add it to results. NOTE that the "DSS_" prefix is removed
			int32 Value = PropIt->GetSignedIntPropertyValue(Property->ContainerPtrToValuePtr<UObject>(Owner));
			Result.Add(Property->GetName().Replace(TEXT("DSS_"), TEXT("")), Value);
			
			UE_LOG(DialogueContextComponent, Display, TEXT("Int Field: %s, value: %s"), *Property->GetName(), *FString::FromInt(Value))
		}
	}
	return Result;
}

TMap<FString, FString> UDialogueContextComponent::PopulateStrStateVariables()
{
	TMap<FString, FString> Result;

	// Iterate over all of the FStrProperty present in the owning actor
	for (TFieldIterator<FStrProperty> PropIt(Owner->GetClass()); PropIt; ++PropIt)
	{
		// Get property from iterator
		const FProperty* Property = *PropIt;

		// Check if it's our Dialogue System State (we rely on the variable names being prefixed with "DSS_")
		if(Property->GetName().Contains("DSS_"))
		{
			// Get value of the property and add it to results. NOTE that the "DSS_" prefix is removed
			FString Value = PropIt->GetPropertyValue(Property->ContainerPtrToValuePtr<UObject>(GetOwner()));
			Result.Add(Property->GetName().Replace(TEXT("DSS_"), TEXT("")), Value);
			
			UE_LOG(DialogueContextComponent, Display, TEXT("String Field: %s, value: %s"), *Property->GetName(), *Value)
		}
	}
	return Result;
}

TArray<FString> UDialogueContextComponent::PopulateCallbackStateVariables()
{
	TArray<FString> Result;

	for (TFieldIterator<UFunction> FuncIt (Owner->GetClass(), EFieldIteratorFlags::IncludeSuper); FuncIt; ++FuncIt) {
		const UFunction* Function = *FuncIt;

		if (Function->GetName().Contains("DSS_"))
		{
			// Prevent errors
			if (Function->NumParms != 1)
			{
				UE_LOG(DialogueContextComponent, Error,
				       TEXT(
					       "Function: %s takes more than one parameter. DSS callbacks should always take a Map<FString, FString> as the only parameter."
				       ), *Function->GetName())
			}
			
			Result.Add(Function->GetName().Replace(TEXT("DSS_"), TEXT("")));
		}
	}
	return Result;
}

void UDialogueContextComponent::UpdateStrValue(const FString VarName, const FString NewVal)
{
	UE_LOG(
		DialogueContextComponent,
		Display,
		TEXT("[DIALOGUE] DialogueContextComponent. Will update variable: %s with new value: %s"), *VarName, *NewVal
	)

	// Update the variable in the mapping
	StrVars.Emplace(VarName, NewVal);
	
	// Iterate over all of the FStrProperty present in the owning actor
	for (TFieldIterator<FStrProperty> PropIt(GetOwner()->GetClass()); PropIt; ++PropIt)
	{
		// Until we find a variable whose name matches the one passed to the function (+ "DSS_" prefix)
		if(PropIt->GetName().Equals("DSS_" + VarName))
		{
			// Set the property to a new value
			PropIt->SetPropertyValue_InContainer(this->GetOwner(), NewVal);
		}
	}
}

void UDialogueContextComponent::UpdateIntValue(const FString VarName, const int NewVal)
{
	UE_LOG(
		DialogueContextComponent,
		Display,
		TEXT("[DIALOGUE] DialogueContextComponent. Will update variable: %s with new value: %s"), *VarName, *FString::FromInt(NewVal)
	)

	// Update the variable in the mapping
	IntVars.Emplace(VarName, NewVal);
	
	// Iterate over all of the FIntProperty present in the owning actor
	for (TFieldIterator<FIntProperty> PropIt(GetOwner()->GetClass()); PropIt; ++PropIt)
	{
		// Until we find a variable whose name matches the one passed to the function (+ "DSS_" prefix)
		if(PropIt->GetName().Equals("DSS_" + VarName))
		{
			// Set the property to a new value
			PropIt->SetPropertyValue_InContainer(this->GetOwner(), NewVal);
		}
	}
}

void UDialogueContextComponent::ExecuteCallback(FString CallbackName, TMap<FString, FString> CallbackParameters)
{
	for (TFieldIterator<UFunction> FuncIt (Owner->GetClass(), EFieldIteratorFlags::IncludeSuper); FuncIt; ++FuncIt) {
		UFunction* Function = *FuncIt;

		if(Function->GetName().Equals("DSS_" + CallbackName))
		{
			struct FLocalParameters
			{
				TMap<FString, FString> Params;
			};

			FLocalParameters Parameters;
			Parameters.Params = CallbackParameters;
			
			Owner->ProcessEvent(Function, &Parameters);
			return;
		}
	}
}

