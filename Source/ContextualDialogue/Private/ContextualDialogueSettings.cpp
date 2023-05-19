// Copyright 2018-2020 Francesco Camarlinghi. All rights reserved.

#pragma once
#include "ContextualDialogueSettings.h"
#include "CoreMinimal.h"


FString UContextualDialogueSettings::GetFullDialogueDBPath() const
{
	return GetFullyQualifiedPath(&dialogueDbPath, &IsDBPathRelativeToProjectDir);
}

FString UContextualDialogueSettings::GetFullyQualifiedPath(const FFilePath* Path, const bool* BoolToCheck) const
{
	if(*BoolToCheck)
		return FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()), Path->FilePath);

	return Path->FilePath;
}

FString UContextualDialogueSettings::GetFullyQualifiedDirPath(const FDirectoryPath* DirPath, const bool* BoolToCheck) const
{
	if(*BoolToCheck)
		return FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()), DirPath->Path);

	return DirPath->Path;
}

#if WITH_EDITOR
void UContextualDialogueSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	bool* BoolToSet;
	FFilePath* PathToSet;

	// Setup stuff to change, depending on which FPath is being actually modified
	if (PropertyChangedEvent.MemberProperty->GetName() == "dialogueDbPath")
	{
		BoolToSet = &IsDBPathRelativeToProjectDir;
		PathToSet = &dialogueDbPath;
	} else
	{
		// In case it's none of the 3 ones we have, just return from the function - no actions to perform
		return;
	}

	// Get the full path and then make it relative to the project's root dir
	FString PathToModify = PathToSet->FilePath;
	if(FPaths::MakePathRelativeTo(PathToModify, *FPaths::ProjectDir()))
	{
		PathToSet->FilePath = PathToModify;
		*BoolToSet = true;
	}
	else
	{
		*BoolToSet = false;
	}
}
#endif