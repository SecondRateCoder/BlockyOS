// #include "convert.h"
// #include "exec.h"
// #include "pe.h"

// #define STB_IMAGE_IMPLEMENTATION
// #define STB_IMAGE_RESIZE_IMPLEMENTATION
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include "import/stb_image.h"
// #include "import/stb_image_resize2.h"
// #include "import/stb_image_write.h"

// #include <stdio.h>
// #include <stdlib.h>
// #include <stddef.h>
// #include <string.h>
// #include <sys/stat.h>

// typedef char SecName[16], SecFlag[4]; // 3 chars + null terminator
// typedef struct{char PeName[8];	SectionNameBe BeName;}PEBEPair;
// //	Transfer all Unformatted Sections, then Resource, 
// static const PEBEPair KnownSections[] = {
//     {".text",					".code"},
//     {".data",					".data"},
//     {".rdata",					".rodata"},
//     {".bss",					".bss"},
//     {PeResourceSection,			DefResourceSectionName},
//     {PeRelocDataSection,		DefRelocationSectionName},
//     {PeImportSection,			DefImportSectionName},
//     {PeExportSection,			DefExportSectionName},
//     {PeExceptionInfoSection,	DefExceptionSectionName},
//     {(char[8]){0},				(SectionNameBe){0}}
// };

// typedef struct{
// 	SecFlag Flag;
// 	uint8_t N;
// }SecFlagDesc;

// #define NUM_DEFAULTS (sizeof(Defaults) / sizeof(Defaults[0]))
// static const SecFlagDesc Defaults[] = {
// 	{"Dsc", 0}, 
// 	{"Exe", 1}, 
// 	{"Red", 2}, 
// 	{"Wrt", 3}
// };

// typedef struct{
// 	SecName Name;
// 	uint8_t *Data;
// 	size_t DataLen;
// 	bool Flags[16];
// }CmdSecDesc;

// typedef struct{
// 	char *InputFile, *OutputFile;
// 	bool DumpExecutable;
	
// 	PEBEPair *SSections;
// 	size_t SCount;

// 	CmdSecDesc *NSections;
// 	size_t NCount;
// }CLIConfig;

// // Helper to resolve string flag to bit index
// static int ResolveFlagIndex(const char *flagStr){
// 	for(size_t i = 0; i < NUM_DEFAULTS; i++){
// 		if(strncmp(flagStr, Defaults[i].Flag, 3) == 0){return Defaults[i].N;}
// 	}
// 	return -1; // Unknown flag
// }

// // Helper to parse space-separated flag string like "Exe Red Wrt"
// static void ParseFlags(const char *flagsStr, bool flagsOut[16]){
// 	memset(flagsOut, 0, sizeof(bool) * 16);
// 	char *copy = strdup(flagsStr);
// 	if(!copy){return;}
// 	char *saveptr = NULL, *token = strtok_r(copy, " \t\r\n", &saveptr);
// 	while(token){
// 		int idx = ResolveFlagIndex(token);
// 		if(idx >= 0 && idx < 16){flagsOut[idx] = true;
// 		}else{fprintf(stderr, "Warning: Unrecognized section flag '%s'\n", token);}
// 		token = strtok_r(NULL, " \t\r\n", &saveptr);
// 	}
// 	free(copy);
// }

// bool ParseCLIArgs(int argc, char **argv, CLIConfig *config){
// 	if(argc < 3){
// 		fprintf(stderr, 
// 			"usage: %s <input> [<output>] [-d (Dump the Executable)] "
// 			"[-S <Section Name in PE Executable> <Output Section Name>] "
// 			"\n[-S <Section Name in PE Executable> <Output Section Name>] "
// 			"[-S <Section Name in PE Executable> <Output Section Name>] "
// 			"\n[-N <Output Section Name> <Section Bytes> \"<FLAGS> <FLAGS> <FLAGS>\"] "
// 			"\n\n\"-N\" Flags:"
// 			"\n\tDsc: The Section is Non-Allocatable, "
// 			"\n\tExe: The Section is Executable, "
// 			"\n\tRed: The Section is Readable, "
// 			"\n\tWrt: The Section is Writable, "
// 			"...\n", 
// 			argv[0]);
// 		return EXIT_FAILURE;
// 		return false;
// 	}
// 	memset(config, 0, sizeof(CLIConfig));

// 	int i = 1;
// 	config->InputFile = argv[i++];
// 	if(i < argc && argv[i][0] != '-'){
// 		config->OutputFile = argv[i++];
// 	}
// 	while(i < argc){
// 		if(strcmp(argv[i], "-d") == 0){
// 			config->DumpExecutable = true;
// 			i++;
// 		}else if(strcmp(argv[i], "-S") == 0){
// 			if(i + 2 >= argc){
// 				fprintf(stderr, "Error: -S requires <PE Section Name> and <Output Section Name>\n");
// 				return false;
// 			}
// 			config->SSections = realloc(config->SSections, sizeof(PEBEPair) * (config->SCount + 1));
// 			strncpy(config->SSections[config->SCount].PeName, argv[i + 1], sizeof(char[8]) - 1);
// 			strncpy(config->SSections[config->SCount].BeName, argv[i + 2], sizeof(SectionNameBe) - 1);
// 			config->SCount++;

// 			i += 3;
// 		}else if(strcmp(argv[i], "-N") == 0){
// 			if(i + 3 >= argc){
// 				fprintf(stderr, "Error: -N requires <Output Section Name>, <Section Bytes>, and \"<FLAGS>\"\n");
// 				return false;
// 			}
// 			config->NSections = realloc(config->NSections, sizeof(CmdSecDesc) * (config->NCount + 1));
// 			CmdSecDesc *newSec = &config->NSections[config->NCount];
// 			memset(newSec, 0, sizeof(CmdSecDesc));
// 			strncpy(newSec->Name, argv[i + 1], sizeof(SecName) - 1);
// 			newSec->DataLen = (size_t)strtoull(argv[i + 2], NULL, 0);
// 			ParseFlags(argv[i + 3], newSec->Flags);
// 			config->NCount++;
// 			i += 4;
// 		}else{fprintf(stderr, "Error: Unrecognized option '%s'\n", argv[i]);	return false;}
// 	}
// 	return true;
// }

// void FreeCLIConfig(CLIConfig *config){
// 	if(config->SSections){free(config->SSections);}
// 	if(config->NSections){free(config->NSections);}
// }

// // int main(int argc, char** argv){
// // 	CLIConfig In;
// // 	ParseCLIArgs(argc, argv, &In);
// // 	fclose(fopen(In.OutputFile, "wb"));

// // 	void *peheader = ReadPeExecutableHeader(In.InputFile);
// // 	ExpandedPeExecutable *epe = ExpandPeExecutableFormat(In.InputFile);
// // 	GenerateBeHeader(In.OutputFile, DefSystemSectionName, (ExecIcon){0}, "{}", 0);
// // 	for(register uint64_t cc = 0; cc < (epe->Fmt.Opt.HeaderType == Pe32? 
// // 		epe->Fmt.Opt.Pe32->mNumberOfRvaAndSizes: 
// // 		epe->Fmt.Opt.Pe32Plus->mNumberOfRvaAndSizes); ++cc
// // 	){
// // 		//  Temp[1] is the Target.
// // 		PEBEPair *Temp = NULL;
// // 		for(uint32_t cc_ = 0; cc_ < In.SCount; ++cc_){if(strcmp(epe->Fmt.SectionTable[cc].mName, In.SSections[cc_].PeName)){Temp = (In.SSections + cc_);	break;}}
// // 		if(!Temp){
// // 			Temp = calloc(1, sizeof(PEBEPair));
// // 			memcpy(Temp->PeName, epe->Fmt.SectionTable[cc].mName, sizeof(char[8]));
// // 		}

// // 		if(!strncmp(Temp->PeName, PeExportSection, sizeof(PeExportSection))){InitExportSection(In.OutputFile, epe, *Temp->BeName? Temp->BeName: DefExportSectionName);}else 
// // 		if(!strncmp(Temp->PeName, PeImportSection, sizeof(PeImportSection))){InitImportSection(In.OutputFile, epe, *Temp->BeName? Temp->BeName: DefImportSectionName);}else
// // 		if(!strncmp(Temp->PeName, PeExceptionInfoSection, sizeof(PeExceptionInfoSection))){InitExceptionSection(In.OutputFile, epe, *Temp->BeName? Temp->BeName: DefExceptionSectionName);}else
// // 		if(!strncmp(Temp->PeName, PeRelocDataSection, sizeof(PeRelocDataSection))){InitRelocationSection(In.OutputFile, epe, *Temp->BeName? Temp->BeName: DefRelocationSectionName);}else
// // 		if(!strncmp(Temp->PeName, PeResourceSection, sizeof(PeResourceSection))){
// // 			GenericLengthType Total = 0, TotalBytes = 0;
// // 			BeResourceConfigurator *RSRC = ParseResourceDirectoryTree(epe, &Total, &TotalBytes);
// // 			InitResourceSection(In.OutputFile, *Temp->BeName? Temp->BeName: DefRelocationSectionName, TotalBytes + (Total * 8), RSRC);
// // 			free(RSRC);
// // 		}else{
// // 			void *ptr = ReadSectionPe(In.InputFile, peheader, Temp->PeName);
// // 			AddSectionBe(In.OutputFile, Temp->BeName, ConvertPeSectionFlagsBe(epe->Fmt.SectionTable[cc].mCharacteristics), ptr, epe->Fmt.SectionTable[cc].mSizeOfRawData, 
// // 				epe->Fmt.SectionTable[cc].mVirtualAddress, epe->Fmt.SectionTable[cc].mVirtualSize, BeStandardAlign
// // 			);
// // 			free(ptr);
// // 		}
		
// // 	}
// // 	// InitImportSection(out, epe, data, , 0);
// // 	return EXIT_SUCCESS;
// // }
// int main(int argc, char** argv){
//     CLIConfig In;
//     if(!ParseCLIArgs(argc, argv, &In)){return EXIT_FAILURE;}
//     FILE *outFp = fopen(In.OutputFile, "wb");
//     if(outFp){fclose(outFp);}

//     void *peheader = ReadPeExecutableHeader(In.InputFile);
//     ExpandedPeExecutable *epe = ExpandPeExecutableFormat(In.InputFile);
//     if(!epe || !epe->Fmt.Header){
//         fprintf(stderr, "Error: Failed to parse PE executable header.\n");
//         return EXIT_FAILURE;
//     }
//     GenerateBeHeader(In.OutputFile, DefSystemSectionName, (ExecIcon){0}, "{}", 0);
	
//     for(uint16_t cc = 0; cc < epe->Fmt.Header->mNumberOfSections; ++cc){
//         PeImageSectionHeader *secHeader = &epe->Fmt.SectionTable[cc];
//         char peSecName[9] = {0}, targetBeName[16] = {0};
//         memcpy(peSecName, secHeader->mName, 8); // Ensure null termination
//         bool isOverrideFound = false;

//         //	Check CLI overrides (-S flags)
//         for(uint32_t cc_ = 0; cc_ < In.SCount; ++cc_){
//             if(strncmp(peSecName, In.SSections[cc_].PeName, 8) == 0){
//                 strncpy(targetBeName, In.SSections[cc_].BeName, sizeof(targetBeName) - 1);
//                 isOverrideFound = true;
//                 break;
//             }
//         }

//         //	Fall back to Predefined Known Sections List if not overridden
//         if(!isOverrideFound){
//             for(size_t k = 0; KnownSections[k].PeName[0] != 0; k++){
//                 if(strncmp(peSecName, KnownSections[k].PeName, 8) == 0){
//                     strncpy(targetBeName, KnownSections[k].BeName, sizeof(targetBeName) - 1);
//                     strncpy(targetBeName, peSecName, sizeof(targetBeName) - 1);
//                     break;
//                 }
//             }
//         }
//         //	Dispatch specialized conversion handlers by PE Section Type
//         if(strncmp(peSecName, PeExportSection, 8) == 0){InitExportSection(In.OutputFile, epe, targetBeName);}else
// 		if(strncmp(peSecName, PeImportSection, 8) == 0) {InitImportSection(In.OutputFile, epe, targetBeName);}else
// 		if(strncmp(peSecName, PeExceptionInfoSection, 8) == 0){InitExceptionSection(In.OutputFile, epe, targetBeName);}else
// 		if(strncmp(peSecName, PeRelocDataSection, 8) == 0){InitRelocationSection(In.OutputFile, epe, targetBeName);}else
// 		if(strncmp(peSecName, PeResourceSection, 8) == 0){
//             GenericLengthType Total = 0, TotalBytes = 0;
//             BeResourceConfigurator *RSRC = ParseResourceDirectoryTree(epe, &Total, &TotalBytes);
//             if(RSRC){
//                 InitResourceSection(In.OutputFile, targetBeName, TotalBytes + (Total * 8), RSRC);
//                 free(RSRC);
//             }
//         }else{
//             // Generic raw section copy
//             void *ptr = ReadSectionPe(In.InputFile, peheader, peSecName);
//             if(ptr){
//                 AddSectionBe(In.OutputFile, targetBeName, 
//                     ConvertPeSectionFlagsBe(secHeader->mCharacteristics), 
//                     ptr, secHeader->mSizeOfRawData, secHeader->mVirtualAddress, 
//                     secHeader->mVirtualSize, BeStandardAlign
//                 );
//                 free(ptr);
//             }
//         }
//     }
//     return EXIT_SUCCESS;
// }