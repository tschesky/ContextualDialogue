// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DialogueContextComponent.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DialogueManagerUtils.h"
#include "DialogueManagerSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(DialogueManagerSubsystem, Log, All);

typedef TArray<TSharedPtr<FDialogueLine>> FDialogueDB;
typedef TMap<FString, TSharedPtr<FDialogueLine>> FDialogueLookupTable;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueQueryFinished, TArray<FLineScore>, Scores, TArray<FDialogueLine>, Lines);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldStateUpdated, const TArray<FObjectValueMapping>&, WorldState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueAndWorldStateLoaded);

const FString SAVE_DIR = "DialogueSaveGames";
const FString DB_SAVE_NAME = "Dialogue.json";
const FString CONTEXT_SAVE_NAME = "WorldContext.json";

/**
 *	The subsystem implementing core logic of our dialogue system. It's a Game Instance Subsystem, so it is automatically
 *	created and shares the life time of a GameInstance. Upon startup, the database is loaded from a selected JSON file
 *	and can later be queried.
 */
UCLASS()
class CONTEXTUALDIALOGUE_API UDialogueManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	/** Broadcast whenever a requested query is finished running and some dialogue is returned */
	FOnDialogueQueryFinished OnDialogueQueryFinished;

	/** Broadcast whenever the world state changes */
	FOnWorldStateUpdated OnWorldStateUpdated;

	/** Broadcast whenever the world state changes */
	UPROPERTY(BlueprintAssignable)
	FOnDialogueAndWorldStateLoaded OnDialogueAndWorldStateLoaded;

	/**
	 *  Get multiple lines of dialogue given current world state
	 *
	 *  @param[in]	NoLines						How many lines should be returned
	 *  @param[in]	QueryCategories				Categories to pass to the query (lines without matching categories won't even be considered)
	 *  @param RequiredParameters[in]			Parameters (key-value) the lines must have. If you need that the lines contain only a parameter key (any value), set the parameter value to '*'.
	 *  @param ExcludedParameters[in]			Parameters (key-value) the lines must NOT have. If you need that the lines should not have a specific parameter key (any value), set the parameter value to '*'.
	 *  @param[in]	ProcessCallbacks			Should callbacks of selected lines be processed immediately upon selection?
	 *  @param[out]	OutLines					Holds lines returned by the query
	 *  @param[out] RequestedNumOfLinesFound	Returns true if the number of lines returned matches the requested NoLines
	 *  @param[out]	ActualNumOfLinesFound		The actual number of lines returned by this query
	 */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm="QueryCategories, RequiredParameters, ExcludedParameters"))
	void GetLinesForCurrentContext(
		const int NoLines,
		TArray<FQueryCategory> QueryCategories,
		TMap<FString, FString> RequiredParameters,
		TMap<FString, FString> ExcludedParameters,
		bool ProcessCallbacks,
		TArray<FDialogueLine>& OutLines,
		bool& RequestedNumOfLinesFound,
		int& ActualNumOfLinesFound);

	/**
	 *  Get multiple lines of dialogue given current world state and having the parameters given.
	 *  @param[in]	QueryCategories Categories to pass to the query (lines without matching categories won't even be considered)
	 *  @param[in] Parameters Parameters to look for as a map {"ParameterName": "ParameterValue"}
	 *  @param[out] OutLines Holds lines returned by the query
	 *  @return True if at least a line was found.
	 */
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm="Categories"))
	bool GetLinesWithParametersForCurrentContext(
	TArray<FQueryCategory> QueryCategories,
	TMap<FString, FString> Parameters,
	TArray<FDialogueLine>& OutLines);
	
	/**
	 *	This is a bit of a hack-y function to get around our "prompts" that we display. In normal circumstances, this
	 *	dialogue system should return one line that fits best within a given context. Since only one line is selected,
	 *	its callbacks etc would be processed immediately. We're straying from this straightforward implementation with out
	 *	PromptsToDisplay. Because of that, we query for multiple lines but the one which is actually "selected" is ultimately
	 *	determined by the player. Therefore, once a player selects a line, it should be "processed". This function artificially
	 *	simulates that behavior and includes everything that should happen when a queries line is "selected".
	 *
	 *	@param Line	Line that was selected for display.
	 */
	UFUNCTION(BlueprintCallable)
	void DialogueLineSelected(FDialogueLine Line);
	
	/**
	 *	Checks if a line should be deleted from the database after use. If it should - then it's removed.
	 *
	 *	@param Line	Line that should be checked for deletion
	 *	@return True if line has been deleted, false otherwise
	 */
	UFUNCTION(BlueprintCallable)
	bool DeleteLineFromDataBase(FDialogueLine Line);
	
	/**
	 *	Execute callbacks of a selected dialogue line
	 *
	 *	@param Line	Line whose callbacks should be processed/executed
	 */
	UFUNCTION(BlueprintCallable)
	void ProcessSingleLineCallbacks(FDialogueLine Line);

	/**
	 *	Utility function to set the value of "World.Speaker" variable. Simply calls SetVariable().
	 *
	 *	@param NewSpeaker New value to set
	 */
	UFUNCTION(BlueprintCallable)
	void SetCurrentSpeaker(const FString NewSpeaker);

	/**
	 *	Set a variable in the world state
	 *
	 *	@param	VarName		Name of the variable to set
	 *	@param	NewValue	Value that will be assigned to the variable
	 */
	UFUNCTION(BlueprintCallable)
    void SetWorldVariable(const FString VarName, const FString NewValue);

	/**
	 *	Get the value of a variable form the World State as a string
	 *
	 *	@param	Scope	Scope in which to look for the value
	 *	@param  VarName	Name of the variable to look up
	 *	@return Value of the requested variable as a string, empty
	 */
	UFUNCTION(BlueprintCallable)
	FString GetVariable(const FString& VarName, const FString& Scope = "World");

	/**
	 *	Return the current world state
	 *
	 * @return the world state representation as a map of objects tracked by the dialogue system
	 */
	UFUNCTION(BlueprintCallable)
	TMap<FString, FObjectValueMapping>& GetWorldState() { return WorldState; }

	/** Initialize the world state with starting values */
	UFUNCTION()
	void PopulateWorldState();

	/** Poll all dialogue components and update their values in the World State */
	UFUNCTION()
	void UpdateWorldState();

	/**
	 *	Return the value of an additional parameter (kept in the Parameters dict)
	 *
	 *	@param	DialogueLine	The dialogue line structure to look up the parameter in
	 *	@param	ParameterName	Name of the parameter to look up
	 *	@return value of the parameter as string
	 */
	UFUNCTION(BlueprintCallable)
	FString GetDialogueLineParameterByName(FDialogueLine DialogueLine, FString ParameterName);

	/**
	 *	Get a score for a single line, given a current World State
	 *
	 *	@param	Line	Line to score
	 *	@return Score achieved by the line, expressed as a FLineScore structure
	 */
	FLineScore GetLineScore(TSharedPtr<FDialogueLine> Line) const;


	bool IsConditionFulfilled(FDialogueCondition& Condition) const;
	
	/**
	 *	Subscribes a new dialogue component with the system
	 *
	 *	@param ContextComponent	A pointer to the component being subscribed
	 */
	void SubscribeNewDSSComponent(UDialogueContextComponent* ContextComponent);

	/**
	 *	Removes the selected dialogue component from the subsystem
	 *
	 *	@param ContextComponent	A pointer to the component to be removed
	 */
	void UnsubscribeDSSComponent(UDialogueContextComponent* ContextComponent);

	/**
	 *	Prints a json object's string representation to the console output
	 *
	 *	@param JsonObject	The object to be debug-printed
	 */
	static void DebugPrintJsonObject(TSharedPtr<FJsonObject> JsonObject);

	/** Print the entire dialogue database to the console output*/
	void DebugPrintDialogueDataBase();

	/**
	 *	Exports the contents of the World State as a JSON object. Used for exporting the world state to a file
	 *
	 *	@return The entire World State represented as a JSON object
	 */
	TSharedPtr<FJsonObject> GetContextVariablesAsJSON();

	/** Overrides from USubsystem */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	/**
	 *	Finds a JSON file location in settings and attempts to load it into memory. In case no valid file is selected from
	 *	the settings, the function defaults to the path passed in as argument
	 *
	 *	@param	DefaultFilePath	File location to default to, if the file path from Project Settings is invalid
	 *	@return True, if the database has been correctly read, False otherwise
	 */
	bool ResolveJsonPathAndLoadDatabase(FString DefaultFilePath = DEFAULT_DB_PATH);

	/**
	 *	A utility function - processes a given full path into an in-memory database
	 *
	 *	@param FullFilePath	Fully qualified file path to read the JSON from
	 *	@return True if read successful, false otherwise
	 */
	bool LoadDialogueDatabaseFromJsonFile(FString FullFilePath);

	/** Get the current number of lines in the dialogue database */
	inline int GetNoDialogueLines() const { return DialogueDataBase.Num(); }

	/** Get the contents of dialogue DB*/
	inline FDialogueDB GetDialogueDB() const { return DialogueDataBase; }

	/** Get the current contents of the dialogue database as a JSON object */
	TSharedPtr<FJsonObject> DialogueDBToJsonObject();

	/** Save the current state of the World variables as well as dialogue database to the project's Saved dir */
	void SaveDialogueProgress();

	/** Get the number of the next save slot */
	int GetNextSaveSlot(FString FullSavePath) const;
	int GetNextSaveSlot(FString FullSavePath, FString& OutLastPath) const;

	/** Set when loading game, contains info whether there are available saved games to load */
	UPROPERTY(BlueprintReadOnly)
	bool IsSaveSlotAvailable;

	/** If there are saved games available, this will contain full path to latest saved folder */
	UPROPERTY(BlueprintReadOnly)
	FString LastSaveSlotFullPath;
	
	/** Set in the main menu, if the player chooses to continue his previous save */
	UPROPERTY(BlueprintReadWrite)
	bool LoadGameRequested = false;

	/** Utility function that saves the contents of Dialogue Database into a file under the selected location */
	UFUNCTION(BlueprintCallable)
	void SaveDialogueDbToJsonFile(const FString& FolderPath, const FString& FileName);

	/** Utility function that saves the contents of World State into a file under the selected location */
	UFUNCTION(BlueprintCallable)
	void SaveWorldStateToJsonFile(const FString& FolderPath, const FString& FileName);

	/** Reads the world context and saves the resulting JSON into an out parameter */
	bool LoadWorldContextFromLatestSaveJsonFile(TSharedPtr<FJsonObject>&);

	/** Updates the world state from the newly read JSON */
	void UpdateWorldStateFromJSON(TSharedPtr<FJsonObject>);

	/** Clears out any pre-existing saves */
	UFUNCTION(BlueprintCallable)
	void ClearAllSaveSlots() const;
	
protected:
	/** Location of the default database within the Content folder */
	static const FString DEFAULT_DB_PATH;

	/** Holds the DB in a parsed Json format */
	TSharedPtr<FJsonObject> JsonParsed;

	/** Holds the list of all the DSS components already subscribed*/
	TArray<UDialogueContextComponent*> DSS_Components;

	/** List of all the dialogue lines */
	FDialogueDB DialogueDataBase;

	/** Structure for faster lookup of dialogue lines. Normally, I would just store the Dialogue Database in a map itself
	 *  but some of the operations require a lot of iteration, for which TMaps are not the best. TMaps suck overall and even
	 *  though they are UE4's propriatery data structure, they don't work with half of the engine stuff (typedefs and
	 *  broadcast delegates to name some of them... ) */
	FDialogueLookupTable DialogueLookup;

	/** Maps dialogue lines to categories, for quick category lookup */
	TMap<FString, TMap<FString, TArray<TSharedPtr<FDialogueLine>>>> Categories;

	/** Contains the objects currently reflected in the Dialogue System's world state */
	TMap<FString, FObjectValueMapping> WorldState;
	
	/**
	 *	Register a dialogue context component with the subsystem and add its variables to the World State
	 *
	 *	@param ContextComponent	The context component to process
	 */
	void AddDialogueComponentToWorldState(UDialogueContextComponent* ContextComponent);
	
	// TODO: A copy-paste. Should move to some global function library, but RN can't be bothered to figure out how to dynamically switch
	// TODO: Between GetOwner() on components and *this on regular objects
	// TODO 2: Make an interface?
	/**
	 *	Returns any integer variables in this subsystem that should be registered with the dialogue system
	 *
	 *	@return All integer variables to register with the dialogue system, mapped name -> integer value
	 */
	TMap<FString, int> GetIntStateVariables();

	/**
	 *	Returns any string variables in this subsystem that should be registered with the dialogue system
	 *
	 *	@return All string variables to register with the dialogue system, mapped name -> string value
	 */
	TMap<FString, FString> GetStrStateVariables();

	/**
	 *	Update a string variable value on this Subsystem
	 *
	 *	@param VarName	Name of the variable to be updates
	 *	@param NewVal	New value to assign to the variable
	 */
	static void UpdateStrValue(FString VarName, FString NewVal);

	/**
	 *	Update an integer variable value on this Subsystem
	 *
	 *	@param VarName	Name of the variable to be updates
	 *	@param NewVal	New value to assign to the variable
	 */
	static void UpdateIntValue(FString VarName, int NewVal);

	/**
	 *	Utility function to check if a string can be parsed into a number (integer or float)
	 *
	 *	@param StrToCheck	String to parse
	 *	@return	True if the string parses into an int or a float, False otherwise
	 */
	static bool IsStringANumber(const FString& StrToCheck);

	/**
	 *	Execute callbacks of a selected dialogue line
	 *
	 *	@param Line	Line whose callbacks should be processed/executed
	 */
	void ProcessLineCallbacks(TSharedPtr<FDialogueLine> Line);

	/** After loading in the level - check if load was requested and load game if necessary */
	UFUNCTION(BlueprintCallable)
	void CheckAndLoadGame();
};