#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

// Macros.
#define ELEMENT_COUNT 100000

#define DEFAULT_INCREMENT 5
#define DEFAULT_MULTIPLY 9
#define DEFAULT_SEED_MIN 0
#define DEFAULT_SEED_MAX 10
#define DEFAULT_SEED_STEP 1
#define DEFAULT_EXT_MIN 100
#define DEFAULT_EXT_MAX 1000000 
#define DEFAULT_EXT_STEP 100 
#define DEFAULT_INT_MAX 0

#define USE_EXT_AS_INT 0

#define ARG_PATH_NAME "path"
#define ARG_INCREMENT_NAME "inc"
#define ARG_MULTIPLY_NAME "mul" 
#define ARG_SEED_MIN_NAME "seed_min" 
#define ARG_SEED_MAX_NAME "seed_max" 
#define ARG_SEED_STEP_NAME "seed_step" 
#define ARG_EXT_MIN_NAME "ext_min" 
#define ARG_EXT_MAX_NAME "ext_max" 
#define ARG_EXT_STEP_NAME "ext_step" 
#define ARG_INT_MAX_NAME "int_max" 

#define ARG_SPECIAL_MAX_OPERATOR "max" 

#define VALUE_ASSIGNMENT_OPERATOR '='

#define IsLetterOrUnderscore(character) ((('A' <= character) && (character <= 'Z'))\
										|| (('a' <= character) && (character <= 'z')) || character == '_')


// Types.
typedef struct ProgramArgsStruct
{
	char* ExportPath;
	unsigned long long Increment;
	unsigned long long Multiply;
	unsigned long long SeedMin;
	unsigned long long SeedMax;
	unsigned long long SeedStep;
	unsigned long long ExtMin;
	unsigned long long ExtMax;
	unsigned long long ExtStep;
	unsigned long long IntMax;
} ProgramArgs;

typedef struct LCGStateStruct 
{
	unsigned long long LastValue;
	unsigned long long Increment;
	unsigned long long Multiply;
	unsigned long long IntMod;
} LCGState;

typedef struct LCGSequenceInfoStruct
{
	double AverageValue;
	size_t SequenceLength;
} LCGSequenceInfo;

typedef struct LCGSequenceCollectionInfoStruct
{
	unsigned long long ExtMod;
	double AverageValue;
	size_t SequenceLength;
} LCGSequenceCollectionInfo;


// Functions.
/* Memory. */
static void* AllocateMemory(size_t size)
{
	void* Pointer = malloc(size);

	if (!Pointer)
	{
		printf("Program failed to allocate %llu bytes of memory.", size);
		exit(EXIT_FAILURE);
	}

	return Pointer;
}

/* Number analysis. */
static size_t GetSequenceLength(int* numberArray, size_t arraySize)
{
	if (arraySize <= 1)
	{
		return arraySize;
	}

	size_t MaxIndex = arraySize - 1;
	int TargetElement = numberArray[arraySize - 1];

	for (long long i = MaxIndex - 1; i >= 0; i--)
	{
		if (TargetElement == numberArray[i])
		{
			return MaxIndex - i;
		}
	}

	return arraySize;
}

static double GetAverageValue(int* numberArray, size_t arraySize)
{
	double Value = 0.0;
	for (size_t i = 0; i < arraySize; i++)
	{
		Value += (double)numberArray[i];
	}

	return Value / (double)arraySize;
}

/* Number generation. */
static unsigned long long GenerateNumber(LCGState* state, unsigned long long extMod)
{
	state->LastValue = ((state->Multiply * state->LastValue) + state->Increment) % state->IntMod;
	return state->LastValue % extMod;
}

static void GetSequenceInfoForSeed(LCGSequenceInfo* info,
	unsigned long long increment,
	unsigned long long multiply,
	unsigned long long seed,
	unsigned long long extMod,
	unsigned long long intMod)
{
	LCGState GeneratorState = { seed, increment, multiply, intMod };
	int* NumberArray = (int*)AllocateMemory(sizeof(int) * ELEMENT_COUNT);

	for (size_t i = 0; i < ELEMENT_COUNT; i++)
	{
		NumberArray[i] = (int)GenerateNumber(&GeneratorState, extMod);
	}

	info->SequenceLength = GetSequenceLength(NumberArray, ELEMENT_COUNT);
	info->AverageValue = GetAverageValue(NumberArray, ELEMENT_COUNT);

	free(NumberArray);
}

static void GetAverageSequenceInfoForMod(LCGSequenceCollectionInfo* info, 
	unsigned long long increment,
	unsigned long long multiply,
	unsigned long long seedMin,
	unsigned long long seedMax,
	unsigned long long seedStep,
	unsigned long long extMod,
	unsigned long long intMod)
{
	info->AverageValue = 0.0;
	info->SequenceLength = 0;
	info->ExtMod = extMod;

	LCGSequenceInfo SingleSequenceInfo;

	for (unsigned long long Seed = seedMin; Seed <= seedMax; Seed += seedStep)
	{
		GetSequenceInfoForSeed(&SingleSequenceInfo, increment, multiply, Seed, extMod, intMod);
		info->AverageValue += SingleSequenceInfo.AverageValue;
		info->SequenceLength += SingleSequenceInfo.SequenceLength;
	}

	info->AverageValue /= 1.0 + (double)((seedMax - seedMin) / seedStep);
	info->SequenceLength /= 1 + ((seedMax - seedMin) / seedStep);
}


/* Exporting to file. */
static void ExportToFile(const char* path, ProgramArgs* args, LCGSequenceCollectionInfo* info, size_t infoEntryCount)
{
	remove(path);

	FILE* File = fopen(path, "w");
	if (!File)
	{
		printf("Failed to export data to file because the file couldn't be opened.\n");
		return;
	}

	fprintf(File, "Increment: %llu; Multiply: %llu; SeedMin: %llu; SeedMax: %llu; SeedStep: %llu; ExtMin: %llu; ExtMax: %llu; ExtStep: %llu; ",
		args->Increment, args->Multiply, args->SeedMin, args->SeedMax, args->SeedStep, args->ExtMin, args->ExtMax, args->ExtStep);

	if (args->IntMax == USE_EXT_AS_INT)
	{
		fprintf(File, "IntMax: Same as ExtMax");
	}
	else
	{
		fprintf(File, "IntMax: %llu", args->IntMax);
	}

	fprintf(File, "\nMod; Average Sequence Length; Average Value\n");

	for (size_t i = 0; i < infoEntryCount; i++)
	{
		fprintf(File, "%llu; %llu; %.2f\n", info[i].ExtMod, info[i].SequenceLength, info[i].AverageValue);
	}

	fclose(File);
	printf("Export finished.\n");
}


/* Args. */
static char* ParseWord(char* string, char* buffer, size_t bufferSize)
{
	size_t Index;
	for (Index = 0; (Index < bufferSize - 1) && IsLetterOrUnderscore(*string) && (*string != '\0'); Index++, string++)
	{
		buffer[Index] = *string;
	}

	buffer[Index] = '\0';
	return string;
}

static bool AssignArgumentValue(char* argumentName, char* value, ProgramArgs* programArgs)
{
	if (!strcmp(argumentName, ARG_PATH_NAME))
	{
		programArgs->ExportPath = value;
		// Verify here that the path works before proceeding with the calculations.
		FILE* File = fopen(value, "w");
		if (!File)
		{
			printf("Failed to open the file at path \"%s\"", value);
			return false;
		}
		fclose(File);

		return true;
	}

	unsigned long long Value;
	Value = !strcmp(value, ARG_SPECIAL_MAX_OPERATOR) ? 0xffffffffffffffff : strtoull(value, NULL, 10);

	if (!strcmp(argumentName, ARG_INCREMENT_NAME))
	{
		programArgs->Increment = Value;
	}
	else if (!strcmp(argumentName, ARG_MULTIPLY_NAME))
	{
		programArgs->Multiply = Value;
	}
	else if (!strcmp(argumentName, ARG_SEED_MIN_NAME))
	{
		programArgs->SeedMin = Value;
	}
	else if (!strcmp(argumentName, ARG_SEED_MAX_NAME))
	{
		programArgs->SeedMax = Value;
	}
	else if (!strcmp(argumentName, ARG_SEED_STEP_NAME))
	{
		if (Value == 0)
		{
			printf("Seed step may not be 0");
			return false;
		}
		programArgs->SeedStep = Value;
	}
	else if (!strcmp(argumentName, ARG_EXT_MIN_NAME))
	{
		if (Value == 0)
		{
			printf("Minimum external value may not be 0.");
			return false;
		}
		programArgs->ExtMin = Value;
	}
	else if (!strcmp(argumentName, ARG_EXT_MAX_NAME))
	{
		if (Value == 0)
		{
			printf("Maximum external value may not be 0.");
			return false;
		}
		programArgs->ExtMax = Value;
	}
	else if (!strcmp(argumentName, ARG_EXT_STEP_NAME))
	{
		if (Value == 0)
		{
			printf("External step value may not be 0.");
			return false;
		}
		programArgs->ExtStep = Value;
	}
	else if (!strcmp(argumentName, ARG_INT_MAX_NAME))
	{
		programArgs->IntMax = Value;
	}
	else
	{
		printf("Unknown argument: \"%s\"", argumentName);
		return false;
	}

	return true;
}

static bool ReadCMDArguments(int argc, char** argv, ProgramArgs* programArgs)
{
	programArgs->ExportPath = NULL;
	programArgs->Increment = DEFAULT_INCREMENT;
	programArgs->Multiply = DEFAULT_MULTIPLY;
	programArgs->SeedMin = DEFAULT_SEED_MIN;
	programArgs->SeedMax = DEFAULT_SEED_MAX;
	programArgs->SeedStep = DEFAULT_SEED_STEP;
	programArgs->ExtMin = DEFAULT_EXT_MIN;
	programArgs->ExtMax = DEFAULT_EXT_MAX;
	programArgs->ExtStep = DEFAULT_EXT_STEP;
	programArgs->IntMax = DEFAULT_INT_MAX;

	for (int i = 1; i < argc; i++)
	{
		char* String = argv[i];
		char WordBuffer[32];
		String = ParseWord(String, WordBuffer, sizeof(WordBuffer));

		if (WordBuffer[0] == '\0')
		{
			printf("Expected argument name, got nothing.");
			return false;
		}

		if (*String != VALUE_ASSIGNMENT_OPERATOR)
		{
			printf("Expected '%c' (value assignment) after argument \"%s\"", VALUE_ASSIGNMENT_OPERATOR, WordBuffer);
			return false;
		}
		String++;

		if (!AssignArgumentValue(WordBuffer, String, programArgs))
		{
			return false;
		}
	}

	if (programArgs->ExportPath == NULL)
	{
		printf("Missing export path.");
		return false;
	}
	if (programArgs->ExtMax < programArgs->ExtMin)
	{
		printf("Invalid input, ExtMax < ExtMin");
		return false;
	}
	if (programArgs->SeedMax < programArgs->SeedMin)
	{
		printf("Invalid input, SeedMax < SeedMin");
		return false;
	}

	return true;
}


/* Main function. */
int main(int argc, char** argv)
{	
	ProgramArgs Args;
	if (!ReadCMDArguments(argc, argv, &Args))
	{
		return EXIT_FAILURE;
	}

	size_t ElementCount = (Args.ExtMax - Args.ExtMin) / Args.ExtStep;
	LCGSequenceCollectionInfo* SequenceCollectionInfoArray = (LCGSequenceCollectionInfo*)AllocateMemory(sizeof(LCGSequenceCollectionInfo) * ElementCount);

	for (unsigned long long ExtMod = Args.ExtMin, i = 0; i < ElementCount; ExtMod += Args.ExtStep, i++)
	{
		GetAverageSequenceInfoForMod(SequenceCollectionInfoArray + i, Args.Increment, Args.Multiply, Args.SeedMin, Args.SeedMax, Args.SeedStep,
			ExtMod, Args.IntMax == USE_EXT_AS_INT ? ExtMod : Args.IntMax);
		printf("Finished sequence for mod value %llu, (%.2f%% done)\n", ExtMod, (double)(i + 1) / (double)ElementCount * 100.0);
	}

	printf("Exporting data...\n");
	ExportToFile(Args.ExportPath, &Args, SequenceCollectionInfoArray, ElementCount);;

	return 0;
}