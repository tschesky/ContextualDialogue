// Copyright 2018-2020 Francesco Camarlinghi. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ContextualDialogueSettings.generated.h"

/**
 * Contextual dialogue runtime settings.
 */
UCLASS(Config = Game, DefaultConfig)
class CONTEXTUALDIALOGUE_API UContextualDialogueSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, config, Category = "DialogueSubsystem", meta = (DisplayName = "Start the dialogue system?"))
	bool StartDialogueSubsystem = false;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, config, Category = "DialogueSubsystem", meta = (DisplayName = "Number of lines to debug print per query"))
	int numLinesInDebugQuery = 10;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, config, Category = "DialogueSubsystem", meta = (DisplayName = "Dialogue database location (JSON)", FilePathFilter="json"))
	FFilePath dialogueDbPath;
	
	/**
	 *  Returns the full dialogue DB path, handles the case of it being relative
	 *
	 *  @return Fully qualified dialogue DB path
	 */
	FString GetFullDialogueDBPath() const;

#if WITH_EDITOR
	/**
	 *  This function is automatically called by the editor any time a property edit has been committed. In our implementation
	 *  we use it to make a user-selected path relative
	 *
	 *  @param PropertyChangedEvent Edit event that triggered that call (contains the edited FProperty)
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	bool IsDBPathRelativeToProjectDir = true;

	/**
	 *	Utility function to convert an input FPaths into a fully-qualified (starting from C:/) path
	 *
	 *	@param Path			The path object to convert
	 *	@param BoolToCheck	The corresponding boolean to check; contains info about whether or not the path we're trying
	 *						to acquire is relative or not
	 *	@return The full path as an FString
	 */
	FString GetFullyQualifiedPath(const FFilePath* Path, const bool* BoolToCheck) const;

	/**
	 *	Utility function to convert an input FDirectoryPath into a fully-qualified (starting from C:/) folder path
	 *
	*	@param DirPath		The directory path object to convert
	 *	@param BoolToCheck	The corresponding boolean to check; contains info about whether or not the path we're trying
	 *						to acquire is relative or not
	 *	@return The full directory path as an FString
	 */
	FString GetFullyQualifiedDirPath(const FDirectoryPath* DirPath, const bool* BoolToCheck) const;
};
