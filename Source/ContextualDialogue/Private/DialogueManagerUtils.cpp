#include "DialogueManagerUtils.h"

#include "FArrayUtils.h"
#include "Serialization/JsonTypes.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY(DialogueManagerUtils);

FString FDialogueCallback::ToString() const
{
	return FString::Printf(TEXT("{ Var: %s, Type: %hs, NewVal: %s }"), *ObjectReference, ECallbackType_str[CallbackType], *Parameter);
}

FString FDialogueCallback::CallbackParameterToString() const
{
	return ECallbackType_sign[CallbackType] + Parameter;
}

FString FDialogueCondition::ConditionValueAsString() const
{
	return EConditionType_sign[ConditionType] + ValueToCompare;
}

FString FDialogueCondition::ToString() const
{
	return FString::Printf(TEXT("{ Var: %s, Type: %hs, CompareTo: %s }"), *VariableToCheck, EConditionType_str[ConditionType], *ValueToCompare);
}

UContextualDialogueLine::UContextualDialogueLine()
{
}

bool UContextualDialogueLine::GetParameterValue(FString ParameterName, FString& ParameterValue)
{
	FString* Val = Parameters.Find(ParameterName);

	for(auto& Param : Parameters)
	{
		UE_LOG(DialogueManagerUtils, Display, TEXT("%s: %s"), *Param.Key, *Param.Value)
	}
	
	if(Val != nullptr)
	{
		ParameterValue = *Val;
		return true;
	}

	ParameterValue = "";
	return false;
}

FString UContextualDialogueLine::ToPrettyString()
{
	return "Dialogue line pretty string";
}

FString UContextualDialogueLine::ToJSONString()
{
	FString OutConditions = "[", OutCallbacks = "[", OutParameters = "[";

	for(FDialogueCallback Callback : Callbacks)
	{
		OutCallbacks.Append(Callback.ToString() + ", ");
	}
	OutCallbacks.Append("]");
	
	for(FDialogueCondition Condition : Conditions)
	{
		OutConditions.Append(Condition.ToString() + ", ");
	}
	OutConditions.Append("]");

	for(auto& param : Parameters)
	{
		OutParameters.Append("("+param.Key+": "+*param.Value+"), ");
	}
	OutParameters.Append("]");

	return FString::Printf(TEXT("{Name: %s, Conditions: %s, Callbacks: %s, Parameters: %s }"),
		*UniqueName, *OutConditions, *OutCallbacks, *OutParameters);
}

TSharedPtr<FJsonObject> UContextualDialogueLine::ToJsonObject()
{
	TSharedPtr<FJsonObject> LineJSON(new FJsonObject());
	
	const TSharedPtr<FJsonObject> ConditionsJSON(new FJsonObject());
	for (FDialogueCondition Condition : Conditions)
	{
		if(Condition.IsCritical)
		{
			const TSharedPtr<FJsonObject> ConditionJSON(new FJsonObject());
			ConditionJSON->SetStringField("val", Condition.ConditionValueAsString());
			ConditionJSON->SetStringField("critical", "True");
			
			ConditionsJSON->SetObjectField(Condition.VariableToCheck, ConditionJSON);
		}
		else
		{
			ConditionsJSON->SetStringField(Condition.VariableToCheck, Condition.ConditionValueAsString());
		}
	}
	LineJSON->SetObjectField("Conditions", ConditionsJSON);

	const TSharedPtr<FJsonObject> CallbacksJSON(new FJsonObject());
	for (FDialogueCallback Callback : Callbacks)
	{
		if(Callback.CallbackType == ECallbackType::EXECUTE)
		{
			TSharedPtr<FJsonObject> JsonParameters;
			const TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Callback.Parameter);
			const bool ReadSuccessful = FJsonSerializer::Deserialize(JsonReader, JsonParameters);

			if(ReadSuccessful)
				CallbacksJSON->SetObjectField(Callback.ObjectReference, JsonParameters);
		}
		else
		{
			CallbacksJSON->SetStringField(Callback.ObjectReference, Callback.CallbackParameterToString());
		}
	}
	LineJSON->SetObjectField("Callbacks", CallbacksJSON);

	const TSharedPtr<FJsonObject> ParametersJSON(new FJsonObject());
	for (TTuple<FString, FString> Parameter : Parameters)
	{
		ParametersJSON->SetStringField(Parameter.Key, Parameter.Value);
	}
	LineJSON->SetObjectField("Parameters", ParametersJSON);

	const TSharedPtr<FJsonObject> FiltersJSON(new FJsonObject());
	for (FDialogueCondition Condition : Filters)
	{
		FiltersJSON->SetStringField(Condition.VariableToCheck, Condition.ConditionValueAsString());
	}
	LineJSON->SetObjectField("Filters", FiltersJSON);
	
	return LineJSON;
}

bool UContextualDialogueLine::PopulateFromJsonObject(FString NewUniqueName, const TSharedPtr<FJsonObject> LineJsonObject)
{
	this->UniqueName = NewUniqueName;

	const TSharedPtr<FJsonObject>* NewConditions;
	const TSharedPtr<FJsonObject>* NewCallbacks;
	const TSharedPtr<FJsonObject>* NewParameters;
	const TSharedPtr<FJsonObject>* NewFilters;
	const bool HasConditions = LineJsonObject->TryGetObjectField("Conditions", NewConditions);
	const bool HasCallbacks = LineJsonObject->TryGetObjectField("Callbacks", NewCallbacks);
	const bool HasParameters = LineJsonObject->TryGetObjectField("Parameters", NewParameters);
	const bool HasFilters = LineJsonObject->TryGetObjectField("Filters", NewFilters);

	if (HasCallbacks)
	{
		for (auto& Callback : NewCallbacks->Get()->Values)
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

			switch (const CHAR& ControlSequence = RawCallback.Len() > 0 ? RawCallback[0] : '!')
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
				UE_LOG(DialogueManagerUtils, Display,
				       TEXT("[DIALOGUE] Unrecongnized callback control sequence: %hc"), ControlSequence)
				return false;
			}
			NewCallback.Parameter = NewCallback.CallbackType == EXECUTE ? RawCallback : RawCallback.RightChop(1);
			this->Callbacks.Add(NewCallback);
		}
	}

	if (HasConditions)
	{
		if (!ParseConditionsIntoArray(NewConditions, this->Conditions))
			return false;
	}

	// Filters are essentially conditions, they just serve a different purpose
	if (HasFilters)
	{
		if (!ParseConditionsIntoArray(NewFilters, this->Filters))
			return false;
	}

	if (HasParameters)
	{
		TMap<FString, FString> Params;
		for (auto& Param : NewParameters->Get()->Values)
		{
			Params.Emplace(Param.Key, Param.Value.Get()->AsString());
		}

		this->Parameters = Params;
	}

	return true;
}

bool FMultipleLineScoring::CheckAndAddLine(UContextualDialogueLine* Line, FLineScore Score)
{
	// First element
	if(CurrLines == 0)
	{
		Lines.Add(Line);
		Scores.Add(Score);

		CurrMinScore = CurrMaxScore = &Scores.Last();
		CurrMinIdx = CurrMaxIdx = 0;

		CurrLines++;
		
		return true;
	}
	
	// If we're not at capacity, just add the line
	if(CurrLines < NoLines)
	{
		Lines.Add(Line);
		const int NewIdx = Scores.Add(Score);

		if(Score < *CurrMinScore)
		{
			CurrMinScore = &Scores.Last();
			CurrMinIdx = NewIdx;
		}
		else if (Score > *CurrMaxScore)
		{
			CurrMaxScore = &Scores.Last();
			CurrMaxIdx = NewIdx;
		}
		
		CurrLines++;
		return true;
	}

	// If we're better than the lowest score in array - swap new element with it
	if(Score > *CurrMinScore)
	{
		Lines[CurrMinIdx] = Line;
		Scores[CurrMinIdx] = Score;

		// Additionally, we could've beaten the best score in the array
		if(Score > *CurrMaxScore)
		{
			CurrMaxIdx = CurrMinIdx;
		}

		// Look for new minimum
		CurrMinScore = CurrMaxScore;
		for (int i = 0; i < Scores.Num(); i++)
		{
			if(Scores[i] < *CurrMinScore)
			{
				CurrMinScore = &Scores[i];
				CurrMinIdx = i;
			}
		}
	}
	
	return false;
}

void FMultipleLineScoring::GetNBestResults(const int NoOfElements, TArray<UContextualDialogueLine*>& OutLines)
{
	if (Lines.Num() == 0)
	{
		return;
	}
	
	// Sort scores and lines
	FArrayUtils::SortArray(Scores, [this](const int32 Index1, const int32 Index2) {
		Scores.SwapMemory(Index2, Index1);
		Lines.SwapMemory(Index2, Index1);
	});

	// TArray<TPair<FString, FString>> LinesAsStrings;
	// Algo::Transform(Lines, LinesAsStrings, [](TSharedPtr<FDialogueLine> Line){return Line->LineToSay; });
	const TArrayView<UContextualDialogueLine*> Tmp = MakeArrayView(Lines).Slice(FMath::Max(Lines.Num() - 1 - NoOfElements, 0),
																					FMath::Min(NoOfElements, Lines.Num()));

	for(UContextualDialogueLine* Line : Tmp)
	{
		// Only add lines with score other than 0
		if(Scores[Lines.Find(Line)].Score > 0.0f)
			OutLines.Add(Line);
	}
}

bool ParseConditionsIntoArray(const TSharedPtr<FJsonObject>* Conditions, TArray<FDialogueCondition>& OutArray)
{
	for(auto& Condition : Conditions->Get()->Values)
	{				
		FDialogueCondition NewCondition;
		NewCondition.VariableToCheck = Condition.Key;

		FString RawCondition;
				
		// We've hit a compound object
		if(Condition.Value->Type == EJson::Object)
		{
			// Try to get the nested object
			const TSharedPtr<FJsonObject>* CompoundCondition;
			if(Condition.Value->TryGetObject(CompoundCondition))
			{
				// Get the actual raw value and check if the condition is critical
				RawCondition = CompoundCondition->Get()->GetStringField("val");
				FString IsCritical = CompoundCondition->Get()->GetStringField("critical");
				NewCondition.IsCritical = IsCritical.ToLower() == "true";
			}
		}
		else
		{
			RawCondition = Condition.Value->AsString();
		}
				
		const CHAR& ControlSequence = RawCondition[0];
		switch(ControlSequence)
		{
			// TODO: Could probably index the enum with strings and have an implicit constructor from the char...?
			case '=':
				NewCondition.ConditionType = EQUAL;
			break;
		case '>':
			NewCondition.ConditionType = GT;
			break;
		case '<':
			NewCondition.ConditionType = LT;
			break;
		default:
			UE_LOG(DialogueManagerUtils, Display, TEXT("[DIALOGUE] Unrecongnized condition control sequence: %s"), RawCondition[0])
			return false;
		}
		NewCondition.ValueToCompare = RawCondition.RightChop(1);
		OutArray.Add(NewCondition);
	}

	return true;
}