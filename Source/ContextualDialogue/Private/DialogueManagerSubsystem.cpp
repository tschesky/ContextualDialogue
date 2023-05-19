// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueManagerSubsystem.h"

#include "ContextualDialogueFunctionLibrary.h"
#include "ContextualDialogueSettings.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Chaos/ChaosPerfTest.h"
#include "HAL/FileManagerGeneric.h"
#include "Misc/DefaultValueHelper.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY(DialogueManagerSubsystem);

const FString UDialogueManagerSubsystem::DEFAULT_DB_PATH = "Dialogue\\DB.json";

bool UDialogueManagerSubsystem::ResolveJsonPathAndLoadDatabase(FString DefaultFilePath)
{
	// Find the path to the DB json file in the Project Settings
	const UContextualDialogueSettings* DevSettings = GetDefault<UContextualDialogueSettings>();
	const FString ConfigPath = DevSettings->GetFullDialogueDBPath();

	UE_LOG(DialogueManagerSubsystem, Display, TEXT("[DIALOGUE] Config path from settings: %s!"), *ConfigPath)

	// If the path from config is empty or invalid, default to using the passed Default Path
	FString JsonPath = "";
	if (ConfigPath.IsEmpty() || !FPaths::FileExists(ConfigPath))
	{
		JsonPath = FPaths::Combine(FPaths::ProjectContentDir(), DefaultFilePath);

#if WITH_EDITOR
		UContextualDialogueFunctionLibrary::DisplayErrorPopup(
			"Hey there, partner. Just letting you know that the Dialogue Database path that you have selected "
			"in your project settings didn't really work out. I'll just be using the default DB.json for this play session. "
			"You should probably go get that checked.");
#endif
	}
	else
	{
		JsonPath = ConfigPath;
	}

	return LoadDialogueDatabaseFromJsonFile(JsonPath);
}

bool UDialogueManagerSubsystem::LoadDialogueDatabaseFromJsonFile(FString FullFilePath)
{
	// Clean any pre-existing dialogue
	DialogueDataBase.Empty();
	DialogueLookup.Empty();

	// Attempt to load the raw contents of the file into a string
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString JsonRaw;

	if (PlatformFile.FileExists(*FullFilePath))
	{
		FFileHelper::LoadFileToString(JsonRaw, *FullFilePath);
		UE_LOG(DialogueManagerSubsystem, Display, TEXT("[DIALOGUE] Located JSON file %s!"), *FullFilePath)
	}
	else
	{
		UE_LOG(
			DialogueManagerSubsystem,
			Warning,
			TEXT("[DIALOGUE] JSON file couldn't be found. Please make sure it's in the location given: %s"),
			*FullFilePath
		)

#if WITH_EDITOR
		UContextualDialogueFunctionLibrary::DisplayErrorPopup(
			"Hey there again, partner. Unfortunately, we couldn't locate neither  the file you requested "
			"nor the default DB.json file. Something's seriously messed up, dude. \n"
			"Quitting the game now...", true);
#endif

		return false;
	}

	// Read the JSON, using UE4 utility stuff
	const TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonRaw);
	const bool ReadSuccessful = FJsonSerializer::Deserialize(JsonReader, JsonParsed);

	if (!ReadSuccessful)
	{
		UE_LOG(DialogueManagerSubsystem, Display,
		       TEXT("[DIALOGUE] Something wend wrong and reading the JSON file resulted in a null reference"))

#if WITH_EDITOR
		UContextualDialogueFunctionLibrary::DisplayErrorPopup("Something wend wrong and reading the JSON file resulted in a null reference",
		                              true);
#endif

		return false;
	}

	// Convert the json to dialogue lines and add them to the DialogueDatabase.
	for (const auto& Element : JsonParsed->Values)
	{
		FString UniqueLineName = Element.Key;
		auto& LineJsonObject = Element.Value->AsObject();
		TSharedPtr<FDialogueLine> LineAsset(new FDialogueLine());
		LineAsset->UniqueName = UniqueLineName;
		LineAsset->LineToSay = LineJsonObject->GetStringField("Line");

		const TSharedPtr<FJsonObject>* Conditions;
		const TSharedPtr<FJsonObject>* Callbacks;
		const TSharedPtr<FJsonObject>* Parameters;
		const TSharedPtr<FJsonObject>* Filters;
		bool HasConditions = LineJsonObject->TryGetObjectField("Conditions", Conditions);
		bool HasCallbacks = LineJsonObject->TryGetObjectField("Callbacks", Callbacks);
		bool HasParameters = LineJsonObject->TryGetObjectField("Parameters", Parameters);
		bool HasFilters = LineJsonObject->TryGetObjectField("Filters", Filters);

		if (HasCallbacks)
		{
			for (auto& Callback : Callbacks->Get()->Values)
			{
				FDialogueCallback NewCallback;
				NewCallback.ObjectReference = Callback.Key;

				// Get callback value as a string (raw string or convert the json object to string again).
				FString RawCallback;
				if(!Callback.Value->TryGetString(RawCallback))
				{
					// TODO: make this more reliable (using struct or something else maybe).
					TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&RawCallback);
					TArray<TSharedPtr<FJsonValue>> Value = {Callback.Value};
					FJsonSerializer::Serialize(Value, Writer);
					RawCallback = RawCallback
						.Replace(TEXT("\r"), TEXT(""))
						.Replace(TEXT("\n"), TEXT(""))
						.Replace(TEXT("\t"), TEXT(""));
					RawCallback = RawCallback.RightChop(1).LeftChop(1); // Remove the brackets [] from the array previously created
				}
				
				const CHAR& ControlSequence = RawCallback.Len() > 0 ? RawCallback[0] : '!';
				switch (ControlSequence)
				{
				// TODO: Could probably index the enum with strings and have an implicit constructor from the char...?
				case '=':
					NewCallback.CallbackType = ASSIGN;
					break;
				case '+':
					NewCallback.CallbackType = ADD;
					break;
				case '-':
					NewCallback.CallbackType = SUBTRACT;
					break;
				case '{':
					NewCallback.CallbackType = EXECUTE;
					break;
				default:
					UE_LOG(DialogueManagerSubsystem, Display,
					       TEXT("[DIALOGUE] Unrecongnized callback control sequence: %hc"), ControlSequence)
					return false;
				}
				NewCallback.Parameter = NewCallback.CallbackType == EXECUTE ? RawCallback : RawCallback.RightChop(1);
				LineAsset->Callbacks.Add(NewCallback);
			}
		}

		if (HasConditions)
		{
			if (!ParseConditionsIntoArray(Conditions, LineAsset->Conditions))
				return false;
		}

		// Filters are essentially conditions, they just serve a different purpose
		if (HasFilters)
		{
			if (!ParseConditionsIntoArray(Filters, LineAsset->Filters))
				return false;
		}

		if (HasParameters)
		{
			TMap<FString, FString> Params;
			for (auto& Param : Parameters->Get()->Values)
			{
				Params.Emplace(Param.Key, Param.Value.Get()->AsString());
			}

			LineAsset->Parameters = Params;
		}

		// Remember the index at which we are adding and store it in the map for faster lookup later
		size_t IdxAddedAt = DialogueDataBase.Add(LineAsset);
		DialogueLookup.Add(LineAsset->UniqueName, LineAsset);

		// Do it after the line has been added so that we can store a pointer to it
		const TSharedPtr<FJsonObject>* ParsedCategories;
		bool HasCategories = LineJsonObject->TryGetObjectField("Categories", ParsedCategories);
		if (HasCategories)
		{
			for (auto& Filter : ParsedCategories->Get()->Values)
			{
				FString Category = Filter.Key;
				FString Value = Filter.Value->AsString();

				// Check if this category of filter already exists
				if (!Categories.Contains(Category))
					Categories.Add(Category, {});

				// Check if that type of filter in a given category exists
				if (!Categories.Find(Category)->Contains(Value))
					Categories.Find(Category)->Add(Value);

				Categories.Find(Category)->Find(Value)->Add(LineAsset);
			}
		}
	}

	return true;
}

TSharedPtr<FJsonObject> UDialogueManagerSubsystem::DialogueDBToJsonObject()
{
	const TSharedPtr<FJsonObject> DialogueDbJSON(new FJsonObject());

	for(const TSharedPtr<FDialogueLine> Line : DialogueDataBase)
	{
		if(Line->UniqueName.IsEmpty())
			continue;
			
		DialogueDbJSON->SetObjectField(Line->UniqueName, Line->ToJsonObject());
	}

	return DialogueDbJSON;
}

void UDialogueManagerSubsystem::SaveDialogueProgress()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString FullSavePath = FPaths::Combine(FPaths::ProjectSavedDir(), SAVE_DIR);

	int SaveNo = GetNextSaveSlot(FullSavePath);
	
	FString NewSaveName = FString::Format(TEXT("{0}_{1}"), {FDateTime::Now().ToUnixTimestamp(), SaveNo});

	FullSavePath = FPaths::Combine(FullSavePath, NewSaveName);
	PlatformFile.CreateDirectory(*FullSavePath);
	
	SaveDialogueDbToJsonFile(FullSavePath, DB_SAVE_NAME);
	SaveWorldStateToJsonFile(FullSavePath, CONTEXT_SAVE_NAME);
}

int UDialogueManagerSubsystem::GetNextSaveSlot(const FString FullSavePath) const
{
	FString IgnoreOut;
	return GetNextSaveSlot(FullSavePath, IgnoreOut);
}

int UDialogueManagerSubsystem::GetNextSaveSlot(const FString FullSavePath, FString& OutLastPath) const
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	
	if(!FPaths::DirectoryExists(FullSavePath))
		PlatformFile.CreateDirectory(*FullSavePath);

	int SaveNo = -1;
	FString LastSaveSlot = "";
	
	// See if any save exists and if so - get the number to save under
	PlatformFile.IterateDirectory(*FullSavePath, [&SaveNo, &LastSaveSlot](const TCHAR* Path, bool IsDirectory)
	{
		if(!IsDirectory)
			return true;

		FString FullPath, Name, Ext;
		FPaths::Split(Path, FullPath, Name, Ext);

		TArray<FString> OutArray;
		Name.ParseIntoArray(OutArray, TEXT("_"), true);

		if(OutArray.Num() != 2)
			return false;

		const float NewSaveNo = FCString::Atof(*OutArray[1]);
		if(NewSaveNo > SaveNo)
		{
			SaveNo = NewSaveNo;
			LastSaveSlot = Path;
		}
		
		return true;
	});

	OutLastPath = LastSaveSlot;
	return ++SaveNo;
}

void UDialogueManagerSubsystem::SaveDialogueDbToJsonFile(const FString& FolderPath, const FString& FileName)
{
	const TSharedPtr<FJsonObject> DialogueDB = DialogueDBToJsonObject();
	FString DialogueDBOutputString;
	const TSharedRef<TJsonWriter<>> DbWriter = TJsonWriterFactory<>::Create(&DialogueDBOutputString);
	
	FJsonSerializer::Serialize(DialogueDB.ToSharedRef(), DbWriter);
	
	FFileHelper::SaveStringToFile(DialogueDBOutputString, *FPaths::Combine(FolderPath, FileName));
}

void UDialogueManagerSubsystem::SaveWorldStateToJsonFile(const FString& FolderPath, const FString& FileName)
{
	const TSharedPtr<FJsonObject> WorldContext = GetContextVariablesAsJSON();
	FString WorldContextOutputString, DialogueDBOutputString;
	const TSharedRef<TJsonWriter<>> ContextWriter = TJsonWriterFactory<>::Create(&WorldContextOutputString);
	
	FJsonSerializer::Serialize(WorldContext.ToSharedRef(), ContextWriter);

	FFileHelper::SaveStringToFile(WorldContextOutputString, *FPaths::Combine(FolderPath, FileName));
}

bool UDialogueManagerSubsystem::LoadWorldContextFromLatestSaveJsonFile(TSharedPtr<FJsonObject>& OutParsed)
{
	const FString FullFilePath = FPaths::Combine(LastSaveSlotFullPath, CONTEXT_SAVE_NAME);
	
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString JsonRaw;

	if (PlatformFile.FileExists(*FullFilePath))
	{
		FFileHelper::LoadFileToString(JsonRaw, *FullFilePath);
		UE_LOG(DialogueManagerSubsystem, Display, TEXT("Loading context. Located JSON file %s!"), *FullFilePath)
	}
	else
	{
		UE_LOG(
			DialogueManagerSubsystem,
			Error,
			TEXT("Loading context. JSON file couldn't be found. Please make sure it's in the location given: %s"),
			*FullFilePath
		)

		return false;
	}
	
	const TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonRaw);
	const bool ReadSuccessful = FJsonSerializer::Deserialize(JsonReader, OutParsed);

	if (!ReadSuccessful)
	{
		UE_LOG(DialogueManagerSubsystem, Error,
			   TEXT("Loading context. Something wend wrong and reading the JSON file resulted in a null reference"))
		return false;
	}

	return true;
}

void UDialogueManagerSubsystem::UpdateWorldStateFromJSON(TSharedPtr<FJsonObject> ContextJSON)
{
	// First, delete objects that exist in the original mapping but not in the one we're trying to load
	/*TArray<FString> NewObjects;
	ContextJSON->Values.GetKeys(NewObjects);
	for(const TTuple<FString, FObjectValueMapping>& Elem : WorldState)
	{
		if(!NewObjects.Contains(Elem.Key))
		{
			UDialogueContextComponent* DSS =  Cast<UDialogueContextComponent>(Elem.Value.ContextRef);
			DSS_Components.Remove(DSS);
			WorldState.Remove(Elem.Key);
		}
	}*/

	// TODO: Assuming that no actors are created or deleted on runtime
	for(const TTuple<FString, TSharedPtr<FJsonValue, ESPMode::ThreadSafe>>& ContextMapping : ContextJSON->Values)
	{
		// No such object exists
		if(!WorldState.Contains(ContextMapping.Key))
		{
			// WorldState.Add(ContextMapping.Key);
			continue;
		}

		const TSharedPtr<FJsonObject>* Variables;
		ContextMapping.Value->TryGetObject(Variables);
		UDialogueContextComponent* DSS =  Cast<UDialogueContextComponent>(WorldState[ContextMapping.Key].ContextRef);
		
		for(const TTuple<FString, TSharedPtr<FJsonValue, ESPMode::ThreadSafe>>& Variable : Variables->Get()->Values)
		{
			const FString VariableName = Variable.Key;
			
			float OutNum;
			if(Variable.Value->TryGetNumber(OutNum))
			{
				WorldState[ContextMapping.Key].IntVals.Emplace(VariableName, OutNum);
				if(DSS)
					DSS->UpdateIntValue(VariableName, OutNum);
				continue;
			}

			FString OutStr;
			if(Variable.Value->TryGetString(OutStr))
			{
				WorldState[ContextMapping.Key].StrVals.Emplace(VariableName, OutStr);
				if(DSS)
					DSS->UpdateStrValue(VariableName, OutStr);
				continue;
			}
		}

		if(DSS)
			DSS->OnDialogueComponentLoaded.Broadcast();
	}
}

void UDialogueManagerSubsystem::ClearAllSaveSlots() const
{
	// Simply recursively delete all the save slots
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FString FullSavePath = FPaths::Combine(FPaths::ProjectSavedDir(), SAVE_DIR);

	if (PlatformFile.DirectoryExists(*FullSavePath))
	{
		FFileManagerGeneric::Get().DeleteDirectory(*FullSavePath, true, true);
	}

	// Recreate the save folder
	PlatformFile.CreateDirectory(*FullSavePath);
}


void UDialogueManagerSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);

	// Check if there is any saved progress
	const float SaveNo = GetNextSaveSlot(FPaths::Combine(FPaths::ProjectSavedDir(), SAVE_DIR), LastSaveSlotFullPath);
	IsSaveSlotAvailable = SaveNo != 0;

	/*if(UChurchInTheWildGameInstance* ChurchGI = Cast<UChurchInTheWildGameInstance>(UGameplayStatics::GetGameInstance(GetWorld())))
	{
		ChurchGI->OnPCGFinishedLoading.AddDynamic(this, &UDialogueManagerSubsystem::CheckAndLoadGame);
	}*/
	
	
	ResolveJsonPathAndLoadDatabase();
	PopulateWorldState();
}

void UDialogueManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();

	/* if(GetDefault<UChurchInTheWildDeveloperSettings>()->bUseAutoSaves)
		SaveDialogueProgress(); */
	
	DSS_Components.Empty();
	DialogueDataBase.Empty();
	Categories.Empty();
	WorldState.Empty();
}

bool UDialogueManagerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if(const UContextualDialogueSettings* DevSettings = GetDefault<UContextualDialogueSettings>())
		return DevSettings->StartDialogueSubsystem;

	return false;
}

void UDialogueManagerSubsystem::GetLinesForCurrentContext(
	const int NoLines,
	TArray<FQueryCategory> QueryCategories,
	TMap<FString, FString> RequiredParameters,
	TMap<FString, FString> ExcludedParameters,
	bool ProcessCallbacks,
	TArray<FDialogueLine>& OutLines,
	bool& RequestedNumOfLinesFound,
	int& ActualNumOfLinesFound)
{
	// Poll the world state
	UpdateWorldState();

	FMultipleLineScoring ScoringStruct(NoLines);

	// First filter out the database to only contain lines whose Filter conditions are met
	FDialogueDB FilteredDB = DialogueDataBase.FilterByPredicate(
		[&](const TSharedPtr<FDialogueLine> Line)
		{
			for (FDialogueCondition& Condition : Line->Filters)
			{
				// If at least one of filters is not fulfilled - don't add the line
				if (!IsConditionFulfilled(Condition))
					return false;
			}

			// If all of the conditions passed, then we're all good
			return true;
		});

	TArray<TSharedPtr<FDialogueLine>> LineCandidates;

	// Keep the lines that has all the Required Parameters and none of the exclude parameters
	for (auto Line : FilteredDB)
	{
		bool IsCandidate = true;

		// Required Parameters
		for (auto Parameter : RequiredParameters)
		{
			if (Line->Parameters.Contains(Parameter.Key))
			{
				if (Line->Parameters[Parameter.Key] != Parameter.Value && Parameter.Value != "*")
				{
					IsCandidate = false;
				}
			}
			else
			{
				IsCandidate = false;
			}
		}

		// Excluded Parameters
		for (auto Parameter : ExcludedParameters)
		{
			if (Line->Parameters.Contains(Parameter.Key))
			{
				if (Line->Parameters[Parameter.Key] == Parameter.Value || Parameter.Value == "*")
				{
					IsCandidate = false;
				}
			}
		}

		if (IsCandidate)
		{
			LineCandidates.Add(Line);
		}
	}

#if WITH_EDITOR
	TArray<FDialogueLine> DebugLines;
	TArray<FLineScore> DebugScores;
#endif

	if (QueryCategories.Num() > 0)
	{
		for (FQueryCategory Category : QueryCategories)
		{
			if (!Categories.Contains(Category.CategoryName) || !Categories.Find(Category.CategoryName)->Contains(
				Category.CategoryValue))
				continue;

			for (auto Line : *Categories.Find(Category.CategoryName)->Find(Category.CategoryValue))
			{
				if (!LineCandidates.Contains(Line)) continue; // TODO: optimize this
				FLineScore LineScore = GetLineScore(Line);
				ScoringStruct.CheckAndAddLine(Line, LineScore);

#if WITH_EDITOR
				DebugLines.Add(*Line);
				DebugScores.Add(LineScore);
#endif
			}
		}
	}

	else
	{
		for (TSharedPtr<FDialogueLine> Line : FilteredDB)
		{
			if (!LineCandidates.Contains(Line)) continue; // TODO: optimize this
			FLineScore LineScore = GetLineScore(Line);
			ScoringStruct.CheckAndAddLine(Line, LineScore);

#if WITH_EDITOR
			DebugLines.Add(*Line);
			DebugScores.Add(LineScore);
#endif
		}
	}

#if WITH_EDITOR
	OnDialogueQueryFinished.Broadcast(DebugScores, DebugLines);
#endif

	TArray<FDialogueLine> OutArray;
	ScoringStruct.GetNBestResults(NoLines, OutArray);

	if (ProcessCallbacks)
	{
		for (FDialogueLine SelectedLineOjb : OutArray)
		{
			UE_LOG(DialogueManagerSubsystem, Warning, TEXT("PROCESSING LINE MAPPINGS"))
			ProcessLineCallbacks(MakeShared<FDialogueLine>(SelectedLineOjb));
		}
	}

	OutLines = OutArray;
	RequestedNumOfLinesFound = NoLines == OutArray.Num();
	ActualNumOfLinesFound = OutArray.Num();
}

bool UDialogueManagerSubsystem::GetLinesWithParametersForCurrentContext(
	TArray<FQueryCategory> QueryCategories,
	TMap<FString, FString> Parameters,
	TArray<FDialogueLine>& OutLines)
{
	// Get the lines of current context
	TArray<FDialogueLine> LinesInContext;
	bool AllLinesFound;
	int NLinesFound;
	TMap<FString, FString> p;
	TMap<FString, FString> ep;
	GetLinesForCurrentContext(
		9999,
		QueryCategories,
		p,
		ep,
		false,
		LinesInContext,
		AllLinesFound,
		NLinesFound);

	// Get only the one with the desired parameters.
	TArray<FDialogueLine> LineCandidates;
	for (auto Line : LinesInContext)
	{
		bool IsCandidate = true;
		for (auto Parameter : Parameters)
		{
			if (Line.Parameters.Contains(Parameter.Key))
			{
				if (Line.Parameters[Parameter.Key] != Parameter.Value)
				{
					IsCandidate = false;
				}
			}
			else
			{
				IsCandidate = false;
			}
		}
		if (IsCandidate)
		{
			LineCandidates.Add(Line);
		}
	}

	OutLines = LineCandidates;

	if (OutLines.Num() > 0)
	{
		return true;
	}
	return false;
}

void UDialogueManagerSubsystem::DialogueLineSelected(FDialogueLine Line)
{
	// Process callbacks of a given line
	ProcessSingleLineCallbacks(Line);

	// Check if a line should be deleted
	DeleteLineFromDataBase(Line);
}

bool UDialogueManagerSubsystem::DeleteLineFromDataBase(FDialogueLine Line)
{
	FString IsPersistent;
	if (Line.GetParameterValue("Persistent", IsPersistent) && IsPersistent.ToLower() == "true")
	{
		// The persistent parameter exists and is equal to true - we do not want to delete the line
		return false;
	}

	auto Ptr = DialogueLookup.Find(Line.UniqueName);
	const TSharedPtr<FDialogueLine> LinePtr = Ptr == nullptr ? nullptr : *Ptr;

	if (!LinePtr.IsValid())
		return false;

	// Remove the line from database AND lookup
	DialogueDataBase.Remove(LinePtr);
	DialogueLookup.Remove(Line.UniqueName);

	return true;
}

void UDialogueManagerSubsystem::ProcessSingleLineCallbacks(FDialogueLine Line)
{
	ProcessLineCallbacks(MakeShared<FDialogueLine>(Line));
}

// TODO: Probably template the whole shit with type of the variable ( ͡° ͜ʖ ͡°)
FLineScore UDialogueManagerSubsystem::GetLineScore(TSharedPtr<FDialogueLine> Line) const
{
	float TotalScore = 0;

	for (FDialogueCondition& Condition : Line->Conditions)
	{
		const bool IsFulfilled = IsConditionFulfilled(Condition);

		// If not fulfilled and critical - return the whole score as 0
		if (!IsFulfilled && Condition.IsCritical)
			return {0.0f, Line->Conditions.Num()};

		TotalScore += IsFulfilled ? 1 : 0;
	}

	return {TotalScore / Line->Conditions.Num(), Line->Conditions.Num()};
}

bool UDialogueManagerSubsystem::IsConditionFulfilled(FDialogueCondition& Condition) const
{
	// TODO: For now just assume correctness
	TArray<FString> Keys;
	Condition.VariableToCheck.ParseIntoArray(Keys, TEXT("."));
	if (IsStringANumber(Condition.ValueToCompare))
	{
		bool IsFulfilled = false;

		// Only check conditions if the objects actually exist
		if (WorldState.Contains(Keys[0]) && WorldState[Keys[0]].IntVals.Contains(Keys[1]))
		{
			const int VarValue = WorldState[Keys[0]].IntVals[Keys[1]];
			const int ValueToCompareAgainst = FCString::Atoi(*Condition.ValueToCompare);

			switch (Condition.ConditionType)
			{
			case EQUAL:
				IsFulfilled = VarValue == ValueToCompareAgainst;
				break;
			case GT:
				IsFulfilled = VarValue > ValueToCompareAgainst;
				break;
			case LT:
				IsFulfilled = VarValue < ValueToCompareAgainst;
				break;
			case GET:
				IsFulfilled = VarValue >= ValueToCompareAgainst;
				break;
			case LET:
				IsFulfilled = VarValue <= ValueToCompareAgainst;
				break;
			default:
				break;
			}
		}

		Condition.IsMatched = IsFulfilled;
		return IsFulfilled;
	}
	else
	{
		bool IsFulfilled = false;

		if (WorldState.Contains(Keys[0]))
		{
			FString VarValue;
			if (!WorldState[Keys[0]].StrVals.Contains(Keys[1]))
				VarValue = "";
			else
				VarValue = WorldState[Keys[0]].StrVals[Keys[1]];
			switch (Condition.ConditionType)
			{
			case EQUAL:
				IsFulfilled = VarValue == Condition.ValueToCompare;
				break;
			case GT:
			case LT:
			case GET:
			case LET:
			default:
				break;
			}
		}

		Condition.IsMatched = IsFulfilled;
		return IsFulfilled;
	}
}

void UDialogueManagerSubsystem::AddDialogueComponentToWorldState(UDialogueContextComponent* ContextComponent)
{
	if (WorldState.Contains(ContextComponent->DSS_Name) || !IsValid(ContextComponent))
		return;

	FObjectValueMapping ContextMapping;
	ContextMapping.Name = ContextComponent->DSS_Name;
	ContextMapping.IntVals = ContextComponent->GetIntVars();
	ContextMapping.StrVals = ContextComponent->GetStrVars();
	ContextMapping.CallbackNames = ContextComponent->GetCallbackNames();
	ContextMapping.IsMappedToActor = true;
	ContextMapping.ContextRef = ContextComponent;

	WorldState.Add(ContextComponent->DSS_Name, ContextMapping);
}

void UDialogueManagerSubsystem::PopulateWorldState()
{
	// Add all the object components
	for (UDialogueContextComponent* ContextComponent : DSS_Components)
	{
		AddDialogueComponentToWorldState(ContextComponent);
	}

	if (WorldState.Contains("World"))
		return;

	// Add global world state
	FObjectValueMapping WorldMapping;
	WorldMapping.Name = "World";
	WorldMapping.IntVals = GetIntStateVariables();
	WorldMapping.StrVals = GetStrStateVariables();
	// No callbacks for world state.
	WorldMapping.IsMappedToActor = true;
	WorldMapping.ContextRef = this;

	WorldState.Add("World", WorldMapping);

	TArray<FObjectValueMapping> OutValues;
	WorldState.GenerateValueArray(OutValues);
	OnWorldStateUpdated.Broadcast(OutValues);
}

void UDialogueManagerSubsystem::UpdateWorldState()
{
	for (UDialogueContextComponent* ContextComponent : DSS_Components)
	{
		FObjectValueMapping ContextMapping;
		ContextMapping.Name = ContextComponent->DSS_Name;
		ContextMapping.IntVals = ContextComponent->GetIntVars();
		ContextMapping.StrVals = ContextComponent->GetStrVars();
		ContextMapping.CallbackNames = ContextComponent->GetCallbackNames();
		ContextMapping.IsMappedToActor = true;
		ContextMapping.ContextRef = ContextComponent;

		WorldState.Add(ContextComponent->DSS_Name, ContextMapping);
	}

	TArray<FObjectValueMapping> OutValues;
	WorldState.GenerateValueArray(OutValues);
	OnWorldStateUpdated.Broadcast(OutValues);
}

void UDialogueManagerSubsystem::SubscribeNewDSSComponent(UDialogueContextComponent* ContextComponent)
{
	DSS_Components.Add(ContextComponent);

	// TODO: Obviously stupid to call the whole function
	PopulateWorldState();
}

void UDialogueManagerSubsystem::UnsubscribeDSSComponent(UDialogueContextComponent* ContextComponent)
{
	DSS_Components.Remove(ContextComponent);

	// TODO: Obviously stupid to call the whole function
	PopulateWorldState();
}

void UDialogueManagerSubsystem::DebugPrintJsonObject(TSharedPtr<FJsonObject> JsonObject)
{
	if (!JsonObject.IsValid())
		return;

	for (const auto& Element : JsonObject->Values)
	{
		FString FieldName = Element.Key;
		const TSharedPtr<FJsonValue> FieldValue = Element.Value;
	}
}

void UDialogueManagerSubsystem::DebugPrintDialogueDataBase()
{
	UE_LOG(DialogueManagerSubsystem, Display, TEXT("[DIALOGUE] DIALOGUE DATABASE: "))
	UE_LOG(DialogueManagerSubsystem, Display, TEXT("{"))
	for (TSharedPtr<FDialogueLine> Line : DialogueDataBase)
	{
		UE_LOG(DialogueManagerSubsystem, Display, TEXT("\t %s"), *Line->ToString())
	}
	UE_LOG(DialogueManagerSubsystem, Display, TEXT("}"))
}

TSharedPtr<FJsonObject> UDialogueManagerSubsystem::GetContextVariablesAsJSON()
{
	TSharedPtr<FJsonObject> ReturnJSON(new FJsonObject());
	
	for (const TPair<FString, FObjectValueMapping>& Mapping : WorldState)
	{
		TSharedPtr<FJsonObject> MappingJSON(new FJsonObject());

		for (const TPair<FString, FString>& StrVar : Mapping.Value.StrVals)
		{
			MappingJSON->SetStringField(StrVar.Key, StrVar.Value);
		}

		for (const TPair<FString, int>& IntVal : Mapping.Value.IntVals)
		{
			MappingJSON->SetNumberField(IntVal.Key, IntVal.Value);
		}

		ReturnJSON->SetObjectField(Mapping.Key, MappingJSON);
	}

	return ReturnJSON;
}

FString UDialogueManagerSubsystem::GetDialogueLineParameterByName(FDialogueLine DialogueLine, FString ParameterName)
{
	if (DialogueLine.Parameters.Contains(ParameterName))
	{
		return DialogueLine.Parameters[ParameterName];
	}
	return "";
}

void UDialogueManagerSubsystem::SetCurrentSpeaker(const FString NewSpeaker)
{
	WorldState["World"].StrVals.Add("Speaker", NewSpeaker);

	TArray<FObjectValueMapping> OutValues;
	WorldState.GenerateValueArray(OutValues);
	OnWorldStateUpdated.Broadcast(OutValues);
}

void UDialogueManagerSubsystem::SetWorldVariable(const FString VarName, const FString NewValue)
{
	if (IsStringANumber(NewValue))
	{
		WorldState["World"].IntVals.Add(VarName, FCString::Atoi(*NewValue));
	}
	else
	{
		WorldState["World"].StrVals.Add(VarName, NewValue);
	}
	TArray<FObjectValueMapping> OutValues;
	WorldState.GenerateValueArray(OutValues);
	OnWorldStateUpdated.Broadcast(OutValues);
}

FString UDialogueManagerSubsystem::GetVariable(const FString& VarName, const FString& Scope)
{
	// If the scope doesn't exist, just return an empty string
	if (!WorldState.Contains(Scope))
		return "";

	// TODO: Just... Don't look at it (falls under the "refactor to unify variables types" category)
	FString* OutVarStr = WorldState[Scope].StrVals.Find(VarName);
	if (OutVarStr)
		return *OutVarStr;

	int* OutVarInt = WorldState[Scope].IntVals.Find(VarName);
	if (OutVarInt)
		return FString::FromInt(*OutVarInt);

	return "";
}

TMap<FString, int> UDialogueManagerSubsystem::GetIntStateVariables()
{
	TMap<FString, int> Result;

	// For now just int properties but hey
	for (TFieldIterator<FIntProperty> PropIt(this->GetClass()); PropIt; ++PropIt)
	{
		// Get property and get its current category
		const FProperty* Property = *PropIt;

		// Check if it's our Dialogue System State
		if (Property->GetName().Contains("DSS_"))
		{
			int32 Value = PropIt->GetSignedIntPropertyValue(Property->ContainerPtrToValuePtr<int32>(this));
			Result.Add(Property->GetName().Replace(TEXT("DSS_"), TEXT("")), Value);

			UE_LOG(DialogueManagerSubsystem, Display, TEXT("Int Field: %s, value: %s"), *Property->GetName(),
			       *FString::FromInt(Value))
		}
	}
	return Result;
}

TMap<FString, FString> UDialogueManagerSubsystem::GetStrStateVariables()
{
	TMap<FString, FString> Result;

	// For now just int properties but hey
	for (TFieldIterator<FStrProperty> PropIt(this->GetClass()); PropIt; ++PropIt)
	{
		// Get property and get its current category
		const FProperty* Property = *PropIt;

		// Check if it's our Dialogue System State
		if (Property->GetName().Contains("DSS_"))
		{
			FString Value = PropIt->GetPropertyValue(Property->ContainerPtrToValuePtr<FString>(this));
			Result.Add(Property->GetName().Replace(TEXT("DSS_"), TEXT("")), Value);

			UE_LOG(DialogueManagerSubsystem, Display, TEXT("String Field: %s, value: %s"), *Property->GetName(), *Value)
		}
	}
	return Result;
}

void UDialogueManagerSubsystem::UpdateStrValue(FString VarName, FString NewVal)
{
}

void UDialogueManagerSubsystem::UpdateIntValue(FString VarName, int NewVal)
{
}

bool UDialogueManagerSubsystem::IsStringANumber(const FString& StrToCheck)
{
	// TODO: what about "112.25.31" ?
	for (int i = 0; i < StrToCheck.Len(); i++)
	{
		FString letter = StrToCheck.Mid(i, 1);
		if (!letter.IsNumeric() && !letter.Contains("."))
		{
			return false;
		}
	}
	return true;
}

void UDialogueManagerSubsystem::ProcessLineCallbacks(TSharedPtr<FDialogueLine> Line)
{
	// Execute all the callbacks of the line
	for (FDialogueCallback Callback : Line->Callbacks)
	{
		TArray<FString> Keys;
		Callback.ObjectReference.ParseIntoArray(Keys, TEXT("."));
		FObjectValueMapping* ParentObject = WorldState.Find(Keys[0]);

		if (!ParentObject)
		{
			// If the object doesn't exist BUT its name is a dialogue line ID then add it to the world state
			if (DialogueLookup.Contains(Keys[0]) || Keys[0] == "this")
			{
				FObjectValueMapping ContextMapping;
				ContextMapping.Name = Keys[0] == "this" ? Line->UniqueName : Keys[0]; // Variable owner
				ContextMapping.IsMappedToActor = false;
				ContextMapping.ContextRef = nullptr;

				ParentObject = &WorldState.Add(ContextMapping.Name, ContextMapping);
			}
			else
			{
				UE_LOG(DialogueManagerSubsystem, Error,
				       TEXT(
					       "[DIALOGUE] Error while processing callbacks. Object: %s could not be found in the world mapping. \
					Please make sure the callback is formulated correctly. Full callback: %s"), *Keys[0],
				       *Callback.ToString())

#if WITH_EDITOR
				UContextualDialogueFunctionLibrary::DisplayErrorPopup(FString::Printf(TEXT(
					"[DIALOGUE] Error while processing callbacks. Object: %s could not be found in the world mapping. \
				Please make sure the callback is formulated correctly. Full callback: %s"), *Keys[0],
				                                              *Callback.ToString()));
#endif

				return;
			}
		}

		if (Callback.CallbackType == EXECUTE)
		{
			if (Keys.Num() < 1)
			{
				UE_LOG(DialogueManagerSubsystem, Error,
					   TEXT("[DIALOGUE] No reference found on callback '%s'. "), *Callback.ObjectReference);
				continue;
			}
			
			// Try to find callback in actor.
			if (!ParentObject->CallbackNames.Contains(Keys[1]))
			{
				UE_LOG(DialogueManagerSubsystem, Error,
					   TEXT("[DIALOGUE] No callbacks '%s' found on the object '%s'. "), *Keys[1],
					   *ParentObject->Name);
				continue;
			}
			
			// Read the JSON, using UE4 utility stuff
			TSharedPtr<FJsonObject> JsonParameters;
			const TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Callback.Parameter);
			const bool ReadSuccessful = FJsonSerializer::Deserialize(JsonReader, JsonParameters);

			// Try to read callback parameters.
			if (!ReadSuccessful)
			{
				// TODO: to make it successful, remove escaped character from the Callback.Parameter 
				UE_LOG(DialogueManagerSubsystem, Warning,
					   TEXT("[DIALOGUE] Something went wrong and reading the JSON file resulted in a null reference (in callbacks)"))
			
				#if WITH_EDITOR
				UContextualDialogueFunctionLibrary::DisplayErrorPopup("Something went wrong and reading the JSON file resulted in a null reference (in callbacks)",
											  false);
				#endif
				
				continue;
			}
			
			TMap<FString, FString> ParsedParameters;
			for (auto& Param : JsonParameters->Values)
			{
				FString ParamName = Param.Key;
				FString ParamValue;
				bool Success = Param.Value->TryGetString(ParamValue);
				if (!Success)
				{
					UE_LOG(DialogueManagerSubsystem, Warning,
					   TEXT("[DIALOGUE] Error reading callback parameter '%s', its value is not a correct string value."), *ParamName);
				}
				ParsedParameters.Add(ParamName, ParamValue);
			}
			Cast<UDialogueContextComponent>(ParentObject->ContextRef)->ExecuteCallback(Keys[1], ParsedParameters);
		}
		else
		{
			// TODO: Just like before, this is not the best and should be refactored into a templated container or something similar
			if (IsStringANumber(Callback.Parameter))
			{
				int NewVal;
				FDefaultValueHelper::ParseInt(Callback.Parameter, NewVal);
				const int OldVal = ParentObject->IntVals.Contains(Keys[1]) ? ParentObject->IntVals[Keys[1]] : 0;

				switch (Callback.CallbackType)
				{
				case ASSIGN:
					break;
				case ADD:
					NewVal += OldVal;
					break;
				case SUBTRACT:
					NewVal = OldVal - NewVal;
					break;
				default:
					return;
				}

				ParentObject->IntVals.Emplace(Keys[1], NewVal);

				if (ParentObject->IsMappedToActor)
				{
					// If the property is mapped to actor - update it in the actual actor
					if (ParentObject->ContextRef == this)
					{
						this->UpdateIntValue(Keys[1], NewVal);
					}
					else
					{
						static_cast<UDialogueContextComponent*>(ParentObject->ContextRef)->UpdateIntValue(Keys[1], NewVal);
					}
				}
			}
			else
			{
				FString NewVal = Callback.Parameter;
				const FString OldVal = ParentObject->StrVals.Contains(Keys[1]) ? ParentObject->StrVals[Keys[1]] : "";

				switch (Callback.CallbackType)
				{
				case ASSIGN:
					break;
				case ADD:
					NewVal += OldVal;
					break;
				case SUBTRACT:
					break;
				default:
					return;
				}
				ParentObject->StrVals.Emplace(Keys[1], NewVal);

				if (ParentObject->IsMappedToActor)
				{
					if (ParentObject->ContextRef == this)
					{
						this->UpdateStrValue(Keys[1], NewVal);
					}
					else
					{
						static_cast<UDialogueContextComponent*>(ParentObject->ContextRef)->UpdateStrValue(Keys[1], NewVal);
					}
				}
			}
		}
	}
}

void UDialogueManagerSubsystem::CheckAndLoadGame()
{
	if(LoadGameRequested)
	{
		// Re-load dialogue database
		LoadDialogueDatabaseFromJsonFile(FPaths::Combine(LastSaveSlotFullPath, DB_SAVE_NAME));

		// Load world context
		TSharedPtr<FJsonObject> WorldContextJSON;
		LoadWorldContextFromLatestSaveJsonFile(WorldContextJSON);
		UpdateWorldStateFromJSON(WorldContextJSON);

		TArray<FObjectValueMapping> OutValues;
		WorldState.GenerateValueArray(OutValues);
		OnWorldStateUpdated.Broadcast(OutValues);
		
		OnDialogueAndWorldStateLoaded.Broadcast();
	}
}
