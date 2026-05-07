#include "util.h"

static const option_t optionsbuffer[] = {
	// Create
	(option_t){"-state", optypedef(optypeREQUIRED | optypeSTR, NULLSTR(8), true), false, "The state the command should create."},
	(option_t){"-force", optypedef(optypeSWITCH, NULLSTR(8), false), false, "Should the command force the new state into creation, overwritten state if already created"},
	(option_t){"-file", optypedef(optypeFPATH, "-param", true), false, "Should update from file, works the same as the \"-update\" flag and ignores the \"-replace\" flag"},
	(option_t){"-param", optypedef(optypeMULTIPLE, "-file", true), false, "The parameter to create with the created state, In the format of \".<PARAM #0>.<PARAM #N>:\'[ENCODING]|\'[INIT VALUE], ...\".\nOmitting the <...> and including the [...].\nThere can be multiple definitions, wherein they are comma-seperated."},
	// Open
	(option_t){"-state", optypedef(optypeREQUIRED | optypeSTR, NULLSTR(8), true), false, "The state to open."},
	(option_t){"-param", optypedef(optypeMULTIPLE, NULLSTR(8), true), false, "The parameter to open, In the format of \".<PARAM #0>.<PARAM #N>, ...\".\nOmitting the <...> and including the [...].\nThere can be multiple definitions, wherein they are comma-seperated.\nThese are accessed via the \"-out\" label.\nA value of \"NULL\" opens the whole state/params as a buffer iunder the label"},
	(option_t){"-out", 	optypedef(optypeSTR, NULLSTR(8), true), false, "The label to grant to the opened state/params.\nA value of \"stdout\" prints the opened parameter to the screen."},
	(option_t){"-pure", optypedef(optypeSWITCH, NULLSTR(8), false), false, "Omit the value tree for the opened label, access it as a Buffer"},
	// Edit
	(option_t){"-state", optypedef(optypeREQUIRED | optypeSTR, NULLSTR(8), true), false, "The state/label to edit."},
	(option_t){"-param", optypedef(optypeREQUIRED | optypeMULTIPLE, NULLSTR(8), true), false, "The parameter to edit, In the format of \".<PARAM #0>.<PARAM #N>:\'[ENCODING]|\'[INIT VALUE], ...\".\nOmitting the <...> and including the [...].\nThere can be multiple definitions, wherein they are comma-seperated."},
	(option_t){"-file", optypedef(optypeREQUIRED | optypeFPATH, NULLSTR(8), true), false, "Use a file to populate params, In the format:\n\".<PARAM #0>.<PARAM #N>:[INDEX], ...\" works after -param initialisation.\nOmitting the <...> and including the [...].\nThere can be multiple definitions, wherein they are comma-seperated."},
	// Remove
	(option_t){"-state", optypedef(optypeREQUIRED | optypeSTR, NULLSTR(8), true), false, "The state/label to edit."},
	(option_t){"-param", optypedef(optypeREQUIRED | optypeMULTIPLE, NULLSTR(8), true), false, "The parameter to remove, In the format of \".<PARAM #0>.<PARAM #N>, ...\".\nOmitting the <...>.\nThere can be multiple definitions, wherein they are comma-seperated, A value of \"NULL\" removes the whole state/label."},
	(option_t){"-force", optypedef(optypeSWITCH, NULLSTR(8), false), false, "Force the state/label/parameter to be removed"},
	// Start
	(option_t){"-utility", optypedef(optypeREQUIRED | optypeSTR, NULLSTR(8), true), false, "The sub-utility to start."},
	(option_t){"-param", optypedef(optypeMULTIPLE, NULLSTR(8), true), false, "The params to pass to the utility."}
};

static const command_t commands[] = {
	{
		.cmd = "-c", .alias = "--create", .cmddesc = "Create a new state.",
		.func = cmdstandardFunc,
		.optionsbf = {
			.len = 4,
			.opbuffer = optionsbuffer
		}
	}, {
		.cmd = "-o", .alias = "--open", .cmddesc = "Open a state to under a Label.",
		.func = cmdstandardFunc,
		.optionsbf = {
			.len = 4,
			.opbuffer = optionsbuffer + 4
		}
	}, {
		.cmd = "-e", .alias = "--edit", .cmddesc = "Edit a state/label/param.",
		.func = cmdstandardFunc,
		.optionsbf = {
			.len = 3,
			.opbuffer = optionsbuffer + 8
		}
	}, {
		.cmd = "-r", .alias = "--remove", .cmddesc = "Remove a state/label/parameter.",
		.func = cmdstandardFunc,
		.optionsbf = {
			.len = 3,
			.opbuffer = optionsbuffer + 11
		}
	}, {
		.cmd = "-s", .alias = "--start", .cmddesc = "Start a utility.",
		.func = cmdstandardFunc,
		.optionsbf = {
			.len = 2,
			.opbuffer = optionsbuffer + 14
		}
	}
};
static const uint8_t cmdcc = 5;

bool main(uint32_t argl, char **argv){
	enable_ansi();
	printf(NANSIYELLOW(
		"Enum:\nRequired:%u\nArray:%u\nSwitch:%u\nFPATH:%u\nSTR:%u\nDIGIT:%u\n\n"),
		optypeREQUIRED, optypeMULTIPLE, optypeSWITCH, optypeFPATH, optypeSTR, optypeDIGIT, optypeREQUIRED
	);
	uint32_t argcc = 1;
	for(uint32_t cc = 0; cc < argl; ++cc){printf("%s ", argv[cc]);}
	while(argcc < argl){
		command_t *cmd = consume(argv + argcc, argl, &argcc);
		if(cmd){
			if(cmd->func){
				if(cmd->func(&(cmd->optionsbf))){continue;}
				else{exit(EXIT_FAILURE);}
			}
		}
		argcc++;
	}
	exit(EXIT_SUCCESS);
}

char *optype2str(optype_t t){
	char *out;
	uint32_t outlen = 0, outsuffixlen = 0;
 	if(FLAGCHECK(t.optype, optypeREQUIRED)){out = strdup("Required \0");	outlen = 9;}
	else{out = calloc(1, 1);}
	switch(PROXYFLAGUNSET(t.optype, optypeREQUIRED)){
		case optypeSTR: {
			outsuffixlen = 8;
			out = realloc(out, outlen + outsuffixlen);
			strncpy(out + outlen, "String.\0", outsuffixlen);
			break;
		}case optypeDIGIT: {
			outsuffixlen = 7;
			out = realloc(out, outlen + outsuffixlen);
			strncpy(out + outlen, "Digit.\0", outsuffixlen);
			break;
		}case optypeMULTIPLE: {
			outsuffixlen = 7;
			out = realloc(out, outlen + outsuffixlen);
			strncpy(out + outlen, "Array.\0", outsuffixlen);
			break;
		}case optypeSWITCH: {
			outsuffixlen = 8;
			out = realloc(out, outlen + outsuffixlen);
			strncpy(out + outlen, "Switch.\0", outsuffixlen);
			break;
		}case optypeFPATH: {
			outsuffixlen = 11;
			out = realloc(out, outlen + outsuffixlen);
			strncpy(out + outlen, "File Path.\0", outsuffixlen);
			break;
		}
	}
	return out;
}

command_t *consume(char **arg, uint32_t argl, uint32_t *argcc){
	uint32_t cc = 0;
	for(uint8_t cmdindex = 0; cmdindex < cmdcc; ++cmdindex){
 		if(!strcmp(commands[cmdindex].cmd, arg[cc]) || !strcmp(commands[cmdindex].alias, arg[cc])){
			// Duplicate Command and children
			command_t *cmd = __memdup((void *)(commands + cmdindex), sizeof(command_t));
			cmd->optionsbf.opbuffer = __memdup(((command_t *)(commands + cmdindex))->optionsbf.opbuffer, sizeof(option_t) * ((command_t *)(commands + cmdindex))->optionsbf.len);
			char *temp = NULL;
			cc++;
			// Populate options list
			for(; cc < argl; cc++){
				temp = arg[cc];
				if(!temp){continue;}
				if(memsearch(commands, sizeof(commands), temp, strlen(temp), offsetof(command_t, cmd)) || memsearch(commands, sizeof(commands), temp, strlen(temp), offsetof(command_t, alias))){goto finish;}
				for(uint8_t arg_cc = 0; arg_cc < cmd->optionsbf.len; ++arg_cc){
					option_t *opt = cmd->optionsbf.opbuffer + arg_cc;
					if(!strcmp(temp, opt->opt)){
						// Search for exclude enabled
						if(!strcmp(opt->optype.optexclude, NULLSTR(8))){opt->enabled = true;	// Always enable since it always has an exclude
						}else{
							for(uint32_t __cc = 0; __cc < cmd->optionsbf.len; ++__cc){
								option_t *opttemp = cmd->optionsbf.opbuffer + __cc;
								if(!strcmp(opttemp->opt, opt->optype.optexclude)){
									if(opttemp->enabled){
										printf(NANSIRED("ERROR! The option %s cannot be used in junction with the option %s"), opt->opt, !strcmp(opt->optype.optexclude, NULLSTR(8))? "(NULL)": opt->optype.optexclude);
										free(cmd->optionsbf.opbuffer);
										free(cmd);
										return NULL;
									}
								}
							}
							opt->enabled = true;
						}
						// Get value
						if((opt->optype.value = paramparse(opt->optype.optype, arg[cc + 1])) == NULL){
							printf(NANSIRED("ERROR: The option %s was not initialised properly\n\t<%s>"), opt->opt, opt->opdesc);
							free(cmd->optionsbf.opbuffer);
							free(cmd);
							return NULL;
						}
						cc += opt->optype.hasinput;
					}else if(!(strcmp(temp, "--help")) || !(strcmp(temp, "-h"))){
						// Print a command's help info
						printf(
							"\n\"%s\" [\"%s\"]: %s",
							cmd->cmd, cmd->alias, cmd->cmddesc);
						printf("Params:");
						cmdstandardFunc(&cmd->optionsbf);
						break;
					}
				}
			}
finish:
			*argcc += cc;
			return cmd;
		}
	}
	return NULL;
}

void *paramparse(optype optype, void *in){
	void *out = NULL;
	switch(PROXYFLAGUNSET(optype, optypeREQUIRED)){
		case optypeSTR: {
			out = strdup(in);
			break;
		} case optypeFPATH: {
			FILE *f = fopen(in, "r");
			if(f){
				fclose(f);
				out = strdup(in);
				break;
			}
			out = NULL;
			break;
		} case optypeDIGIT: {
			out = (void *)(strtoull(in, NULL, 10));
			break;
		} case optypeSWITCH: {out = (void *)true;		break;}
		case optypeMULTIPLE: {
			// Parse with standard syntax, seperated by ','
			optypemulbuffer *list = calloc(1, sizeof(optypemulbuffer));	out = (void *)list;
			list->optypemulbuffer = calloc(5, sizeof(optypemulbufferitem));
			char *dup = strdup(in);
			char *snippet = strtok(dup, ",");
			do{
				if(FACTORCHECK(list->len, 5)){list->optypemulbuffer = realloc(list->optypemulbuffer, (list->len + 5) * sizeof(optypemulbufferitem));}
				list->optypemulbuffer[list->len].optypemulbufferitem = strdup(snippet);	list->optypemulbuffer[list->len].len = strlen(snippet);
				list->len++;
			}while(snippet = strtok(NULL, ","));
			free(dup);
			break;
		}
	}
	return out;
}

bool process(command_t *cmd){if(cmd->func){return cmd->func(&(cmd->optionsbf));}else{return false;}}

bool cmdstandardFunc(opbuffer *buffer){
	uint8_t cc = 0;
	while(cc < buffer->len){
		char *type = optype2str(buffer->opbuffer[cc].optype);
		printf(
			"\n[%"PRIu32"] %.8s"
			"\nOption Type: %s"
			"\nOption Exclude: %s"
			"\nEnabled?: %s"
			"\nOperation Desc: %s",
			cc, buffer->opbuffer[cc].opt, type, 
			strcmp(buffer->opbuffer[cc].optype.optexclude, NULLSTR(8))? buffer->opbuffer[cc].optype.optexclude: "(NULL)",
			(buffer->opbuffer[cc].enabled? "TRUE": "FALSE"), buffer->opbuffer[cc].opdesc
		);
		free(type);
		switch(PROXYFLAGUNSET(buffer->opbuffer[cc].optype.optype, optypeREQUIRED)){
			case optypeDIGIT: {
				printf("\nValue: %zu\n", (size_t)(buffer->opbuffer[cc].optype.value));
				break;
			} case optypeFPATH: {
				printf("\nValue: %s\n", buffer->opbuffer[cc].optype.value? strdup(buffer->opbuffer[cc].optype.value): "(NULL)");
				break;
			} case optypeSTR: {
				printf("\nValue: %s\n", buffer->opbuffer[cc].optype.value);
				break;
			} case optypeSWITCH: {
				printf("\nValue: %s\n", buffer->opbuffer[cc].enabled? "SWITCH ENABLED": "SWITCH DISABLED");
				break;
			} case optypeMULTIPLE: {
				optypemulbuffer *bf = (optypemulbuffer *)(buffer->opbuffer[cc].optype.value);
				printf("\nValue: "); 
				if(bf){
					for(uint32_t _cc = 0; _cc < bf->len; ++_cc){
						printf(ANSIYELLOW("\n[%u]: {"), _cc);
						char *fmt = NULL;	uint8_t gran = 0;
						if(FACTORCHECK(bf->optypemulbuffer[_cc].len, 8)){fmt = ANSIBLUE("%" PRIu64 " ");	gran = 8;
						}else if(FACTORCHECK(bf->optypemulbuffer[_cc].len, 4)){fmt = ANSIBLUE("%" PRIu32 " ");	gran = 4;
						}else if(FACTORCHECK(bf->optypemulbuffer[_cc].len, 2)){fmt = ANSIBLUE("%" PRIu16 " ");	gran = 2;
						}else{fmt = ANSIBLUE("%" PRIu8 " ");	gran = 1;}
						for(uint32_t __cc = 0; __cc < bf->optypemulbuffer[_cc].len; __cc++){
							if(FACTORCHECK(__cc, BUFFERPRINT_MAXX)){printf("\n\t");}
							switch (gran){
								case 8: {printf(fmt, ((size_t *)bf->optypemulbuffer[_cc].optypemulbufferitem)[__cc]);}
								case 4: {printf(fmt, ((uint32_t *)bf->optypemulbuffer[_cc].optypemulbufferitem)[__cc]);}
								case 2: {printf(fmt, ((uint16_t *)bf->optypemulbuffer[_cc].optypemulbufferitem)[__cc]);}
								default: {printf(fmt, ((uint8_t *)bf->optypemulbuffer[_cc].optypemulbufferitem)[__cc]);}
							}
						}
						printf(ANSIYELLOW("\n}\n"));
					}
				}else{printf("(NULL)");}
			}
		}
		cc++;
	}
}