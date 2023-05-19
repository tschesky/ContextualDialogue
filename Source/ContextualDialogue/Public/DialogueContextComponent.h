// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DialogueContextComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDialogueComponentLoaded);
DECLARE_LOG_CATEGORY_EXTERN(DialogueContextComponent, Log, All);

/**
 *	This class implements the "Dialogue context component". Each actor with such component will be registered with the Dialogue System.
 *
 *	The class itself doesn't expose any functionality, the only logic executed is contained within the BeginPlay() function.
 *	When a play session is started - this component will poll all the variables for its owning actor and register them under
 *	that Actor's name (more specifically - DSS_Name) with the Dialogue System.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CONTEXTUALDIALOGUE_API UDialogueContextComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UDialogueContextComponent();
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	TMap<FString, int> GetIntVars() { return PopulateIntStateVariables(); }
	TMap<FString, FString> GetStrVars() const { return StrVars; }
	TArray<FString> GetCallbackNames() const { return CallbackNames; }
	FString GetDSSName() const { return DSS_Name; }

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DSS_Name")
	FString DSS_Name = "";
	
	/**
	 *	Update the value of a string variable in this component's owning actor
	 *
	 * @param VarName	Name of the variable to be updated
	 * @param NewVal	New value to be given to the variable
	 */
	void UpdateStrValue(FString VarName, FString NewVal);

	/**
	 *	Update the value of an integer variable in this component's owning actor
	 *
	 * @param VarName	Name of the variable to be updated
	 * @param NewVal	New value to be given to the variable
	 */
	void UpdateIntValue(FString VarName, int NewVal);

	/** Called when this context component has been updated from a loaded game */
	UPROPERTY(BlueprintAssignable)
	FDialogueComponentLoaded OnDialogueComponentLoaded;
	
	void ExecuteCallback(FString CallbackName, TMap<FString, FString> CallbackParameters);

protected:
	AActor* Owner;
	TMap<FString, int> IntVars;
	TMap<FString, FString> StrVars;
	TArray<FString> CallbackNames;

	/**
	 *	Get all the initial values of integer variables that should be registered with the Dialogue System
	 *
	 *	@return	The mapping of variable names to their integer values in this component's owning actor
	 */
	TMap<FString, int> PopulateIntStateVariables();

	/**
	 *	Get all the initial values of string variables that should be registered with the Dialogue System
	 *	
	 *  @return	The mapping of variable names to their string values in this component's owning actor
	 */
	TMap<FString, FString> PopulateStrStateVariables();

	/**
	 *	Get all the callback names that should be registered with the Dialogue System
	 *	
	 *  @return	The array of callback names in this component's owning actor
	 */
	TArray<FString> PopulateCallbackStateVariables();

};
