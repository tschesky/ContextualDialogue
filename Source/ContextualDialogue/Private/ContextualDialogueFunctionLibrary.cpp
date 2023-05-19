// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ContextualDialogueFunctionLibrary.h"

#if WITH_EDITOR
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Misc/MessageDialog.h"

void UContextualDialogueFunctionLibrary::DisplayErrorPopup(FString MessageToDisplay, bool QuitPIE)
{
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(MessageToDisplay));
	
	if(QuitPIE)
		GUnrealEd->RequestEndPlayMap();
}
#endif