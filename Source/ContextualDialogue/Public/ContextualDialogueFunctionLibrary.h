// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ContextualDialogueFunctionLibrary.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(MiscUtils, Log, All);

/*
 *	A utility library containing misc functions that did not seem to fit the theme anywhere else
 */
UCLASS()
class CONTEXTUALDIALOGUE_API UContextualDialogueFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
#if WITH_EDITOR
	/**
	 *	In-editor utility function to display a warning message when the dialogue DB's path is wrongly set
	 *
	 *	@param MessageToDisplay	Message string to show in the popup
	 *	@param QuitPIE			Should the play session be ended after displaying this popup?
	 */
	static void DisplayErrorPopup(FString MessageToDisplay, bool QuitPIE = false);
#endif

};

