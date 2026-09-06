#include "convert.h"
#include "exec.h"
#include "pe.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "import/stb_image.h"
#include "import/stb_image_resize2.h"
#include "import/stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>

char *logfile;

typedef char SecName[16], SecFlag[4]; // 3 chars + null terminator
typedef struct { char PeName[8]; SectionNameBe BeName; } PEBEPair;

typedef struct {
	char *Original;
	char *Overload;
} StringOverloadPair;

// Order-defining pair list
static const PEBEPair KnownSections[] = {
	{PeResourceSection,		DefResourceSectionName},
	{".text",				DefCodeSectionName},
	{".data",				DefDataSectionName},
	{".rdata",				DefReadonlyDataSectionName},
	{".bss",				DefUDefDataSectionName},
	{PeRelocDataSection,	DefRelocationSectionName},
	{PeImportSection,		DefImportSectionName},
	{PeExportSection,		DefExportSectionName},
	{PeExceptionInfoSection,DefExceptionSectionName},
	{(char[8]){0},			(SectionNameBe){0}}
};

typedef struct{
	SecFlag Flag;
	uint8_t N;
}SecFlagDesc;

#define NUM_DEFAULTS (sizeof(Defaults) / sizeof(Defaults[0]))
static const SecFlagDesc Defaults[] = {
	{"Dsc", 0}, 
	{"Exe", 1}, 
	{"Red", 2}, 
	{"Wrt", 3}
};

typedef struct{
	SecName Name;
	uint8_t *Data;
	size_t DataLen;
	bool Flags[16];
}CmdSecDesc;

typedef struct{
	char *InputFile, *OutputFile;
	bool DumpBEExecutable,  // Set by -d
		DumpPEExecutable;  // Set by -D
	
	PEBEPair *SSections;
	size_t SCount;

	CmdSecDesc *NSections;
	size_t NCount;

	StringOverloadPair *Overloads; // Added for -A
	size_t OverloadCount;
}CLIConfig;

// Helper to resolve string flag to bit index
static int ResolveFlagIndex(const char *flagStr){
	for(size_t i = 0; i < NUM_DEFAULTS; i++){
		if(strncmp(flagStr, Defaults[i].Flag, 3) == 0){return Defaults[i].N;}
	}
	return -1; // Unknown flag
}

// Helper to parse space-separated flag string
static void ParseFlags(const char *flagsStr, bool flagsOut[16]){
	memset(flagsOut, 0, sizeof(bool) * 16);
	char *copy = strdup(flagsStr);
	if(!copy){return;}
	char *saveptr = NULL, *token = strtok_r(copy, " \t\r\n", &saveptr);
	while(token){
		int idx = ResolveFlagIndex(token);
		if(idx >= 0 && idx < 16){flagsOut[idx] = true;}else
		{fprintf(stderr, "Warning: Unrecognized section flag '%s'\n", token);}
		token = strtok_r(NULL, " \t\r\n", &saveptr);
	}
	free(copy);
}

bool ParseCLIArgs(int argc, char **argv, CLIConfig *config){
	if(argc < 3){
		fprintf(stderr, 
			"usage: %s <input> [<output>] [-od (Dump BE)] [-id (Dump PE)] "
			"[-S <PE Section Name> <Output Section Name>] "
			"[-L <Log File Path] "
			"[-N <Output Section Name> <Section Bytes> \"<FLAGS>\"] "
			"[-A <ORIGINAL> <OVERLOAD>]\n", 
			argv[0]);
		return false;
	}
	memset(config, 0, sizeof(CLIConfig));
	int i = 1;
	config->InputFile = argv[i++];
	if(i < argc && argv[i][0] != '-'){config->OutputFile = argv[i++];}
	while(i < argc){
		if(!strcmp(argv[i], "-od")){
			config->DumpBEExecutable = true;
			i++;
		}else if(!strcmp(argv[i], "-L")){
			if(i + 1 >= argc){
				fprintf(stderr, "Error: -L requires <PE Section Name> and <Output Section Name>\n");
				return false;
			}
			logfile = strdup(argv[i + 1]);
			i += 2;
		}else if(!strcmp(argv[i], "-id")){
			config->DumpPEExecutable = true;
			i++;
		}else if(!strcmp(argv[i], "-S")){
			if(i + 2 >= argc){
				fprintf(stderr, "Error: -S requires <PE Section Name> and <Output Section Name>\n");
				return false;
			}
			config->SSections = realloc(config->SSections, sizeof(PEBEPair) * (config->SCount + 1));
			strncpy(config->SSections[config->SCount].PeName, argv[i + 1], sizeof(char[8]) - 1);
			strncpy(config->SSections[config->SCount].BeName, argv[i + 2], sizeof(SectionNameBe) - 1);
			config->SCount++;
			i += 3;
		}else if(!strcmp(argv[i], "-N")){
			if(i + 3 >= argc){
				fprintf(stderr, "Error: -N requires <Output Section Name>, <Section Bytes>, and \"<FLAGS>\"\n");
				return false;
			}
			config->NSections = realloc(config->NSections, sizeof(CmdSecDesc) * (config->NCount + 1));
			CmdSecDesc *newSec = &config->NSections[config->NCount];
			memset(newSec, 0, sizeof(CmdSecDesc));
			strncpy(newSec->Name, argv[i + 1], sizeof(SecName) - 1);
			newSec->DataLen = (size_t)strtoull(argv[i + 2], NULL, 0);
			ParseFlags(argv[i + 3], newSec->Flags);
			config->NCount++;
			i += 4;
		}else if(!strcmp(argv[i], "-A")){
			if(i + 2 >= argc){
				fprintf(stderr, "Error: -A requires <ORIGINAL> and <OVERLOAD>\n");
				return false;
			}
			config->Overloads = realloc(config->Overloads, sizeof(StringOverloadPair) * (config->OverloadCount + 1));
			config->Overloads[config->OverloadCount].Original = strdup(argv[i + 1]);
			config->Overloads[config->OverloadCount].Overload = strdup(argv[i + 2]);
			config->OverloadCount++;
			i += 3;
		}else{
			fprintf(stderr, "Error: Unrecognized option '%s'\n", argv[i]);
			return false;
		}
	}
	return true;
}
void FreeCLIConfig(CLIConfig *config){
	if(config->SSections){free(config->SSections);}
	if(config->NSections){free(config->NSections);}
	if(config->Overloads){
		for(size_t i = 0; i < config->OverloadCount; i++){
			if(config->Overloads[i].Original){free(config->Overloads[i].Original);}
			if(config->Overloads[i].Overload){free(config->Overloads[i].Overload);}
		}
		free(config->Overloads);
	}
}

int main(int argc, char** argv){
	CLIConfig In;
  	if(!ParseCLIArgs(argc, argv, &In)){return EXIT_FAILURE;}
	fclose(fopen(In.OutputFile, "wb"));
	// if(logfile){fclose(fopen(logfile, "wb"));}
	
	void *peheader = ReadPeExecutableHeader(In.InputFile);
	ExpandedPeExecutable *epe = ExpandPeExecutableFormat(In.InputFile);
	if(In.DumpPEExecutable){DumpPe(epe);}
	if(!epe || !epe->Fmt.Header){
		fprintf(stderr, "Error: Failed to parse PE executable header.\n");
		FreeCLIConfig(&In);
		return EXIT_FAILURE;
	}

	//  Track which PE sections have been converted
	bool beGenerated = false, *processedPeSections = calloc(epe->Fmt.Header->mNumberOfSections, sizeof(bool));

	//  Process defined pairs in the order specified by KnownSections
	for(size_t k = 0; KnownSections[k].PeName[0] != 0; k++){
		const char *peName = KnownSections[k].PeName;
		char targetBeName[16] = {0};
		strncpy(targetBeName, KnownSections[k].BeName, sizeof(targetBeName) - 1);

		// Check if CLI overrides target BE name (-S)
		for(uint32_t c = 0; c < In.SCount; ++c){
			if(strncmp(peName, In.SSections[c].PeName, 8) == 0){
				strncpy(targetBeName, In.SSections[c].BeName, sizeof(targetBeName) - 1);
				break;
			}
		}
		// Find section header in PE binary
		PeImageSectionHeader *secHeader = FindSectionPe(epe->Raw, peName);
		if(secHeader){
			//  Mark PE section as processed
			size_t idx = secHeader - epe->Fmt.SectionTable;
			processedPeSections[idx] = true;
			//  Dispatch handling for existing PE section
			if(!strncmp(peName, PeExportSection, 8)){InitExportSection(In.OutputFile, epe, targetBeName);}else
			if(!strncmp(peName, PeImportSection, 8)){InitImportSection(In.OutputFile, epe, targetBeName);}else
			if(!strncmp(peName, PeExceptionInfoSection, 8)){InitExceptionSection(In.OutputFile, epe, targetBeName);}else
			if(!strncmp(peName, PeRelocDataSection, 8)){InitRelocationSection(In.OutputFile, epe, targetBeName);}else
			if(!strncmp(peName, PeResourceSection, 8)){
				GenerateBeHeader(In.OutputFile, targetBeName, 0, 
					epe->Fmt.Opt.Pe32->mMagic == Pe32? 
						epe->Fmt.Opt.Pe32->mAddressOfEntryPoint: 
						epe->Fmt.Opt.Pe32Plus->mAddressOfEntryPoint);
				GenericLengthType Total = 0, TotalBytes = 0;
				BeResourceConfigurator *RSRC = ParseResourceDirectoryTree(epe, &Total, &TotalBytes);
				if(RSRC){
					ExecIcon Icon = {0};
					const char *Paths[2] = {BeResourceIconPath "\\" BeResourceIconFile, BeResourceManifestPath "\\" BeResourceManifestFile};
					UpdateResourceTreeBe(RSRC, 2, (const char **)Paths, BRFTIcon, sizeof(ExecIcon), &Icon,  
						BRFTManifest, sizeof(JsonManifest), (JsonManifest){"{}"});
					InitResourceSection(In.OutputFile, targetBeName, TotalBytes + (Total * 8), RSRC);
					FreeBeResourceConfigurator(RSRC);
				}
			}else{
				void *ptr = ReadSectionPe(In.InputFile, peheader, peName);
				if(ptr){
					AddSectionBe(In.OutputFile, targetBeName, ConvertPeSectionFlagsBe(secHeader->mCharacteristics), 
						ptr, secHeader->mSizeOfRawData, secHeader->mVirtualAddress, secHeader->mVirtualSize, BeStandardAlign
					);
					free(ptr);
				}
			}
		}else{
			if(strncmp(peName, PeExportSection, 8) == 0){InitExportSection(In.OutputFile, epe, targetBeName);}else
			if(strncmp(peName, PeImportSection, 8) == 0){InitImportSection(In.OutputFile, epe, targetBeName);}else
			if(strncmp(peName, PeExceptionInfoSection, 8) == 0){InitExceptionSection(In.OutputFile, epe, targetBeName);}else
			if(strncmp(peName, PeRelocDataSection, 8) == 0){InitRelocationSection(In.OutputFile, epe, targetBeName);}
		}

	}
	//  Pass for any remaining unmapped PE sections in the input file
	for(uint16_t i = 0; i < epe->Fmt.Header->mNumberOfSections; ++i){
		if(!processedPeSections[i]){
			PeImageSectionHeader *secHeader = &epe->Fmt.SectionTable[i];
			char peSecName[9] = {0};
			memcpy(peSecName, secHeader->mName, 8);

			void *ptr = ReadSectionPe(In.InputFile, peheader, peSecName);
			if(ptr){
				AddSectionBe(In.OutputFile, peSecName, 
					ConvertPeSectionFlagsBe(secHeader->mCharacteristics), 
					ptr, secHeader->mSizeOfRawData, secHeader->mVirtualAddress, 
					secHeader->mVirtualSize, BeStandardAlign
				);
				free(ptr);
			}
		}
	}
	free(processedPeSections);
	if(In.DumpBEExecutable){DumpBe(In.OutputFile);}
	FreeCLIConfig(&In);
	return EXIT_SUCCESS;
} 