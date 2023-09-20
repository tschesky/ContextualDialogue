#pragma once

#include "DialogueManagerUtils.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(DialogueManagerUtils, Log, All);

struct FDialogueCondition;

/**
 *	Parses a passes FJsonObject into an Array of FDialogueCondition
 *
 *	@param[in]	Conditions	The FJsonObject to parse
 *	@param[out]	OutArray	The array to store conditions parsed from the string
 *
 *	@return Returns True if successfully parsed, False otherwise
 */
bool ParseConditionsIntoArray(const TSharedPtr<FJsonObject>* Conditions, TArray<FDialogueCondition>& OutArray);

/**
 *  Determines type of conditions we can encounter in the dialogue database. They're pretty obvious and simply
 *  cover all comparison operators.
 */
UENUM(BlueprintType)
enum EContextDialogueConditionType
{
	EQUAL,
	LT,
	GT,
	LET,
	GET
};

/**	Utility array for convenient enum-string conversion in ToString() functions */
static const char *EConditionType_str[] =
	{ "Equal", "Less than", "Greater than", "Less then/equal", "Greater then/equal" };

/** Utility array for saving conditions back into JSON */
static const char *EConditionType_sign[] =
	{ "=", "<", ">", "<=", ">=" };

/**
 * Defines types of callbacks encountered in the Dialogue DB - we can assign (strings and integers) as well as
 * subtract and add (integers only)
 */
enum ECallbackType
{
	ASSIGN,
	ADD,
	SUBTRACT,
	EXECUTE
};

/** Utility array for convenient enum-string conversion in ToString() functions */
static const char *ECallbackType_str[] =
	{ "Assign", "Add", "Subtract", "Execute" };

/** Utility array for saving callbacks back into JSON */
static const char *ECallbackType_sign[] =
	{ "=", "+", "-", "()" };

/**
 *  Contains a single dialogue callback within FDialogueLine. Callbacks keep information about which variable to modify
 *  and how exactly to influence it
 */
USTRUCT(BlueprintType)
struct FDialogueCallback
{
	GENERATED_BODY()
	
	FString ObjectReference;
	ECallbackType CallbackType;
	FString Parameter;

	/** Utility function for debug printing */
	FString ToString() const;

	/** Utility function for easier JSON saving of the database*/
	FString CallbackParameterToString() const;
};

inline bool operator==(const FDialogueCallback& lhs, const FDialogueCallback& rhs)
{
	return  lhs.ObjectReference == rhs.ObjectReference &&
			lhs.CallbackType == rhs.CallbackType &&
			lhs.Parameter == rhs.Parameter;
}

/**
 *  Contains a single dialogue condition within FDialogueLine. Conditions need to know which variable to check and
 *  what comparison type is to be executed
 */
USTRUCT(BlueprintType)
struct FDialogueCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString VariableToCheck;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EContextDialogueConditionType> ConditionType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ValueToCompare;

	/** If a condition is critical, it will set the entire score to 0 if unmet */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsCritical = false;
	
	/** Utility variable, used only in Editor, to highlight matched queries in the debug widget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsMatched = false;

	/** Get condition value as string*/
	FString ConditionValueAsString() const;
	
	FString ToString() const;
};

inline bool operator==(const FDialogueCondition& lhs, const FDialogueCondition& rhs)
{
	return  lhs.VariableToCheck == rhs.VariableToCheck &&
			lhs.ConditionType == rhs.ConditionType &&
			lhs.ValueToCompare == rhs.ValueToCompare &&
			lhs.IsCritical == rhs.IsCritical;
}

/**
 * Represents a single query category. Categories split the database into subsets for easier lookup.
 */
USTRUCT(BlueprintType)
struct FQueryCategory
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CategoryName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CategoryValue;
};

/**
 *	Encapsulates a whole Dialogue line from the database
 */
UCLASS(BlueprintType)
class UContextualDialogueLine : public UObject
{
	GENERATED_BODY()

public:
	UContextualDialogueLine();
	
	/** Unique line name, it's the key under which the line object is stored in JSON */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString UniqueName;

	/** List of all the conditions this line should meet to be selected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FDialogueCondition> Conditions;

	/** List of all the callbacks to be executed after this line has been picked */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FDialogueCallback> Callbacks;

	/** List of additional parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FString, FString> Parameters;

	/** List of all the conditions to be evaluated as "filters" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FDialogueCondition> Filters;

	/**
	 *	Get value of a parameter
	 *
	 *	@param[in]	ParameterName	Name of the parameter to look up
	 *	@param[out]	ParameterValue	Will contain the looked-up value if parameter exists
	 *
	 *	@return  True if parameter was found, false otherwise
	 */
	bool GetParameterValue(FString ParameterName, FString& ParameterValue);

	/** Get the string representation of this line */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual FString ToPrettyString();
	
	/** Get the string representation of this line */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual FString ToJSONString();

	/** Get this dialogue line as a json object */
	TSharedPtr<FJsonObject> ToJsonObject();

	/** Create this object from a json object */
	bool PopulateFromJsonObject(FString NewUniqueName, const TSharedPtr<FJsonObject> LineJsonObject);
};

UCLASS(BlueprintType)
class UTestLine : public UContextualDialogueLine
{
	GENERATED_BODY()
};

/**
 *  Custom equality operator for FString TMaps, needed for equality operator of the FDialogueLine
 */
inline bool operator==(const TMap<FString, FString>& lhs, const TMap<FString, FString>& rhs)
{
	for (auto& Elem : lhs)
	{
		if(!rhs.Contains(Elem.Key) || rhs[Elem.Key] != Elem.Value)
			return false;
	}
	return true;
}


inline bool operator==(const UContextualDialogueLine& lhs, const UContextualDialogueLine& rhs)
{
	return  lhs.UniqueName == rhs.UniqueName &&
		   	lhs.Conditions == rhs.Conditions &&
		   	lhs.Callbacks == rhs.Callbacks &&
		   	lhs.Parameters == rhs.Parameters;
}

inline bool operator!=(const UContextualDialogueLine& lhs, const UContextualDialogueLine& rhs)
{
	return  !operator==(lhs, rhs);
}

/**
 *	Represents the mapping of a single UObject to its string and integers variables that are watched by and synchronized
 *	with the dialogue system.
 */
USTRUCT(BlueprintType)
struct FObjectValueMapping
{
	GENERATED_BODY()

	/** Name of the DSS object/actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;

	/** Should the variables kept in this mapping be updated on an Actor object during gameplay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsMappedToActor;

	/** Reference to the underlying object */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UObject* ContextRef;

	/** Map of all the string variables. Maps variable name -> string value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FString, FString> StrVals;

	/** Map of all the integer variables. Maps variable name -> integer value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FString, int> IntVals;

	/** Array of all the DSS callbacks. Callbacks always take a Map<FString, FString> as the only parameter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> CallbackNames;
};

/**
 *  Utility structure to represent a score achieved by a single line, given a certain world context
 */
USTRUCT(BlueprintType)
struct FLineScore
{
	GENERATED_BODY()

	/** A number of constructors, required by various data structures in which FLineScore will be kept */
	FLineScore() : FLineScore(0.0, 0) {}
	FLineScore(const FLineScore& Other): FLineScore(Other.Score, Other.NumQueries) {}
	FLineScore(const float S, const int N) : Score(S), NumQueries(N) {}

	/** Score achieved by a line */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Score;

	/** Total number of conditions that the line had */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int NumQueries;

	/**
	 *  Returns a string representation of the score. Used only for debugging purposes.
	 *
	 *  @return String representation of the structure
	 */
	FString ToString() const { return FString::Printf(TEXT("{ Score: %f, NumQueries: %i }"), Score, NumQueries); }

	/**
	 *	Compares two Line scores for equality. 2 lines are only equal if their scores AND total num of conditions are the same
	 *
	 *	@return	True, if the lines are equal, False otherwise
	 */
	bool Equals(const FLineScore& Other) const
	{
		return this->Score == Other.Score && this->NumQueries == Other.NumQueries;
	}

	/**
	 *	Hashes a FLineScore object into a unique-ish integer. Required to be able to use this struct as a TMap key
	 *
	 *	@return Generated unique hash
	 */
	uint32 GetTypeHash(const FLineScore& Thing);
};

/** Equality operator for the scores. They are only equal when both the score as well as total query number are equal */
inline bool operator==(const FLineScore& lhs, const FLineScore& rhs)
{
	return lhs.Score == rhs.Score && lhs.NumQueries == rhs.NumQueries;
}

/** Comparison operator. Default to comparing scores, if those are the same, compare NumQueries instead */
inline bool operator>(const FLineScore& lhs, const FLineScore& rhs)
{
	return lhs.Score == rhs.Score ? lhs.NumQueries > rhs.NumQueries : lhs.Score > rhs.Score;
}

/** Comparison operator. Default to comparing scores, if those are the same, compare NumQueries instead */
inline bool operator<(const FLineScore& lhs, const FLineScore& rhs)
{
	return lhs.Score == rhs.Score ? lhs.NumQueries < rhs.NumQueries : lhs.Score < rhs.Score;
}

/**
 *	A structure that keeps track of scores of multiple lines. The maximum amount of lines to hold is specified upon
 *	creating. It holds info about its current Min and Max scores. It is also capable of returning N best scores out
 *	of the currently held lines.
 */
USTRUCT()
struct FMultipleLineScoring
{
	GENERATED_BODY()

	FMultipleLineScoring() : FMultipleLineScoring(10) {};
	FMultipleLineScoring(int NoLines) : NoLines(NoLines), CurrLines(0), CurrMaxScore(nullptr), CurrMinScore(nullptr), CurrMaxIdx(0), CurrMinIdx(0) {};

	/** The maximum amount of lines to hold in this structure */
	int NoLines;

	/** Number of lines currently held*/
	int CurrLines;

	/** Pointers to LineScore structures that are the current minimum and maximum */
	FLineScore *CurrMaxScore, *CurrMinScore;

	/** Indices at which the current Minimum and Maximum are kept in the Lines TArray*/
	int CurrMaxIdx, CurrMinIdx;

	/** Currently kept lines and scores. The assumption is that Scores and Lines are kept in relative order */
	TArray<FLineScore> Scores;
	TArray<UContextualDialogueLine*> Lines;

	/**
	 *	Evaluate and add a line
	 *
	 *	@param Line		Dialogue line to be checked
	 *	@param Score	Score of the line
	 *
	 *	@return True if successfully added, False otherwise
	 */
	bool CheckAndAddLine(UContextualDialogueLine* Line, FLineScore Score);

	/**
	 *	Return N best lines from the current state.  The amount of lines returned can be actually different than requested N
	 *	e.g. in the case where less lines are kept in the structure than were requested
	 *
	 *	@param[in]	NoOfElements	How many elements to return
	 *	@param[out] OutLines		TArray to hold the returned N best lines
	 */
	void GetNBestResults(const int NoOfElements, TArray<UContextualDialogueLine*>& OutLines);
};