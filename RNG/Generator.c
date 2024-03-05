#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

// Macros.
#define INCREMENT 5
#define MULTIPLY 9
#define SEED 3

#define SEED_STEP 1
#define MAX_SEED 10
#define MOD_STEP 100
#define MAX_MOD 1000000

#define ELEMENT_COUNT 100000

#define DEFAULT_INCREMENT 5
#define DEFAULT_MULTIPLY 9
#define DEFAULT_SEED_MIN 0
#define DEFAULT_SEED_MAX 10
#define DEFAULT_SEED_STEP 1
#define DEFAULT_EXT_MIN 100
#define DEFAULT_EXT_MAX 1000000 
#define DEFAULT_EXT_STEP 100 
#define DEFAULT_INT_MAX 0xffffffffffffffff 

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
	unsigned long long Mod;
} LCGState;

typedef struct LCGSequenceInfoStruct
{
	double AverageValue;
	size_t SequenceLength;
} LCGSequenceInfo;

typedef struct LCGSequenceCollectionInfoStruct
{
	unsigned long long Mod;
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
static unsigned long long GenerateNumber(LCGState* state)
{
	state->LastValue = ((state->Multiply * state->LastValue) + state->Increment) % state->Mod;
	return state->LastValue;
}

static void GetSequenceInfoForSeed(LCGSequenceInfo* info, unsigned long long seed, unsigned long long mod)
{
	LCGState GeneratorState = { seed, INCREMENT, MULTIPLY, mod };
	int* NumberArray = (int*)AllocateMemory(sizeof(int) * ELEMENT_COUNT);

	for (size_t i = 0; i < ELEMENT_COUNT; i++)
	{
		NumberArray[i] = (int)GenerateNumber(&GeneratorState);
	}

	info->SequenceLength = GetSequenceLength(NumberArray, ELEMENT_COUNT);
	info->AverageValue = GetAverageValue(NumberArray, ELEMENT_COUNT);

	free(NumberArray);
}

static void GetAverageSequenceInfoForMod(LCGSequenceCollectionInfo* info, unsigned long long mod)
{
	info->AverageValue = 0.0;
	info->SequenceLength = 0;
	info->Mod = mod;

	LCGSequenceInfo SingleSequenceInfo;

	for (unsigned long long Seed = 0; Seed < MAX_SEED; Seed++)
	{
		GetSequenceInfoForSeed(&SingleSequenceInfo, Seed, mod);
		info->AverageValue += SingleSequenceInfo.AverageValue;
		info->SequenceLength += SingleSequenceInfo.SequenceLength;
	}

	info->AverageValue /= (double)MAX_SEED;
	info->SequenceLength /= MAX_SEED;
}

static void GetSingleSequenceInfoForMod(LCGSequenceCollectionInfo* info, unsigned long long mod)
{
	LCGSequenceInfo SingleSequenceInfo;
	GetSequenceInfoForSeed(&SingleSequenceInfo, SEED, mod);

	info->Mod = mod;
	info->AverageValue = SingleSequenceInfo.AverageValue;
	info->SequenceLength = SingleSequenceInfo.SequenceLength;
}


/* Exporting to file. */
static void ExportToFile(const char* path, LCGSequenceCollectionInfo* info, size_t infoEntryCount)
{
	remove(path);

	FILE* File = fopen(path, "w");
	if (!File)
	{
		printf("Failed to export data to file because the file couldn't be opened.");
		return;
	}

	fprintf(File, "Mod; Average Sequence Length; Average Value\n");

	for (size_t i = 0; i < infoEntryCount; i++)
	{
		fprintf(File, "%llu; %llu; %.2f\n", info[i].Mod, info[i].SequenceLength, info[i].AverageValue);
	}

	fclose(File);
}


/* Args. */
static char* ParseWord(char* string, char* buffer, size_t bufferSize)
{
	size_t Index;
	for (Index = 0; (Index < bufferSize - 1) && !IsLetterOrUnderscore(*string) && (*string != '\0'); Index++, string++)
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

	unsigned long long Value = strtoull(value, NULL, 10);
	if (strcmp(value, ARG_INCREMENT_NAME))
	{
		programArgs->Increment = value;
	}
	else if (strcmp(value, ARG_MULTIPLY_NAME))
	{
		programArgs->Multiply = Value;
	}
	else if (strcmp(value, ARG_SEED_MIN_NAME))
	{
		programArgs->SeedMin = Value;
	}
	else if (strcmp(value, ARG_SEED_MAX_NAME))
	{
		programArgs->SeedMax = Value;
	}
	else if (strcmp(value, ARG_SEED_STEP_NAME))
	{
		if (Value == 0)
		{
			printf("Seed step may not be 0");
			return false;
		}
		programArgs->SeedStep = Value;
	}
	else if (strcmp(value, ARG_EXT_MIN_NAME))
	{
		if (Value == 0)
		{
			printf("Minimum external value may not be 0.");
			return false;
		}
		programArgs->ExtMin = Value;
	}
	else if (strcmp(value, ARG_EXT_MAX_NAME))
	{
		if (Value == 0)
		{
			printf("Maximum external value may not be 0.");
			return false;
		}
		programArgs->ExtMax = Value;
	}
	else if (strcmp(value, ARG_EXT_STEP_NAME))
	{
		if (Value == 0)
		{
			printf("External step value may not be 0.");
			return false;
		}
		programArgs->ExtStep = Value;
	}
	else if (strcmp(value, ARG_INT_MAX_NAME))
	{
		if (Value == 0)
		{
			printf("Maximum internal value may not be 0.");
			return false;
		}
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
}


/* Main function. */
int main(int argc, char** argv)
{	
	ProgramArgs Args;
	if (!ReadCMDArguments(argc, argv, &Args))
	{
		return EXIT_FAILURE;
	}

	if (argc < 2)
	{
		printf("Missing exported file path.");
		return -1;
	}

	size_t ElementCount = MAX_MOD / MOD_STEP;
	LCGSequenceCollectionInfo* SequenceCollectionInfoArray = (LCGSequenceCollectionInfo*)AllocateMemory(sizeof(LCGSequenceCollectionInfo) * ElementCount);

	for (unsigned long long Mod = MOD_STEP, i = 0; i < ElementCount; Mod += MOD_STEP, i++)
	{
		GetSingleSequenceInfoForMod(SequenceCollectionInfoArray + i, Mod);
		printf("Finished sequence for mod value %llu, (%.2f%% done)\n", Mod, (double)(i + 1) / (double)ElementCount * 100.0);
	}

	printf("Exporting data...\n");
	ExportToFile(argv[1], SequenceCollectionInfoArray, ElementCount);
	printf("Finished!\n");

	return 0;
}