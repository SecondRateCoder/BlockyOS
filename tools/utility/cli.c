#include "util.h"

#define bufdef(type, name, scale) typedef struct name{type *name; scale len}name
#define FLAGSET(n, f) (n |= f)
#define FLAGUNSET(n, f) (n &= ~f)
#define FLAGCHECK(n, f) ((n & f) == f)
typedef enum optype{
	optypeREQUIRED,
	optypeMULTIPLE,
	optypeSWITCH,
	optypeFPATH,
	optypeSTR,
	optypeDIGIT
}optype;

typedef struct optype_t{
	uint8_t optype;
	void *value;
	char optexclude[8];
}optype_t;

typedef struct option_t{
	char opt[8];
	optype_t optype;
	bool enabled;
	char *opdesc;
}option_t;

bufdef(option_t, opbuffer, uint32_t);

typedef bool (*cmdfunc)(opbuffer *buffer);

typedef struct command_t{
	char cmd[4];
	char *alias;
	cmdfunc func;
	opbuffer bf;
	char *cmddesc;
}command_t;

#define optypedef(type, exclude) {type, exclude}
#define cmddef(cmd, alias, opt, desc, proc) {(cmd), (alias), (opbuffer *)opt, desc}
command_t *commands[] = {
	cmddef("-c", "--create", {
			{"-state", optypedef(optypeREQUIRED | optypeSTR, NULL), false, "The state the command should create."},
			{"-force", optypedef(optypeSWITCH, null), false, "Should the command force the new state into creation, overwritten state if already created"},
			{"-file", optypedef(optypeFPATH, "-param"), false, "Should update from file, works the same as the \"-update\" flag and ignores the \"-replace\" flag"},
			{"-param", optypedef(optypeMULTIPLE, "-file"), false, "The parameter to create with the created state, In the format of \".<PARAM #0>.<PARAM #N>:\'[ENCODING]|\'[INIT VALUE], ...\".\nOmitting the <...> and including the [...].\nThere can be multiple definitions, wherein they are comma-seperated."}
		},
		"Create a new state."
	), cmddef("-o", "--open", {
			{"-state", optypedef(optypeREQUIRED | optypeSTR, NULL), false, "The state to open."},
			{"-param", optypedef(optypeMULTIPLE, NULL), false, "The parameter to open, In the format of \".<PARAM #0>.<PARAM #N>, ...\".\nOmitting the <...> and including the [...].\nThere can be multiple definitions, wherein they are comma-seperated.\nThese are accessed via the \"-out\" label.\nA value of \"NULL\" opens the whole state/params as a buffer iunder the label"},
			{"-out", optypedef(optypeSTR, NULL), false, "The label to grant to the opened state/params.\nA value of \"stdout\" prints the opened parameter to the screen."},
			{"-pure", optypedef(optypeSWITCH, NULL), false, "Omit the value tree for the opened label, access it as a Buffer"}
		},
		"Open a state to under a Label."
	), cmddef("-e", "--edit", {
			{"-state", optypedef(optypeREQUIRED | optypeSTR, NULL), false, "The state/label to edit."},
			{"-param", optypedef(optypeREQUIRED | optypeMULTIPLE, NULL), false, "The parameter to edit, In the format of \".<PARAM #0>.<PARAM #N>:\'[ENCODING]|\'[INIT VALUE], ...\".\nOmitting the <...> and including the [...].\nThere can be multiple definitions, wherein they are comma-seperated."},
			{"-file", optypedef(optypeREQUIRED | optypeFPATH), false, "Use a file to populate params, In the format:\n\".<PARAM #0>.<PARAM #N>:[INDEX], ...\" works after -param initialisation.\nOmitting the <...> and including the [...].\nThere can be multiple definitions, wherein they are comma-seperated."},
		},
		"Edit a state/label/param."
	), cmddef("-r", "--remove", {
			{"-state", optypedef(optypeREQUIRED | optypeSTR, NULL), false, "The state/label to edit."},
			{"-param", optypedef(optypeREQUIRED | optypeMULTIPLE, NULL), false, "The parameter to remove, In the format of \".<PARAM #0>.<PARAM #N>, ...\".\nOmitting the <...>.\nThere can be multiple definitions, wherein they are comma-seperated, A value of \"NULL\" removes the whole state/label."},
			{"-force", optypedef(optypeSWITCH, NULL), false, "Force the state/label/parameter to be removed"}
		},
		"Remove a state/label/parameter."
	), cmddef("-s", "--start", {
			{"-utility", optypedef(optypeREQUIRED | optypeSTR, NULL), false, "The sub-utility to start."},
			{"-param", optypedef(optypeMULTIPLE, NULL), false, "The params to pass to the utility."}
		},
		"Start a utility"
	)
};
uint8_t cmdcc = 5;

void *memdup(void *src, uint32_t len){
	void *out = malloc(len);
	if(out){
		memcpy(out, src, len);
		return out;
	}
	return NULL;
}

bool memsearch(void *in, uint32_t inlen, void *cmp, uint32_t cmplen, uint832_t stride){
	for(uint32_t cc = stride; cc < inlen; cc+=(cmplen + stride)){
		if(cmplen + cc > inlen){return false;}
		if(!memcmp(in + cc, cmp, cmplen)){return true;}
	}
	return false;
}

bool main(char **argv, uint32_t argl){
	uint32_t argcc = 0;
	while(argcc < argl){
		command_t *cmd = consume(argv, &argcc);
		if(cmd.func){
			if(cmd.func(&(cmd.opbuffer)){continue;}
			else{exit 1;}
		}
	}
}

char *optype2str(optype_t t){
	char *out;
	if(FLAGCHECK(t, optypeREQUIRED)){out = strdup("Required ");}else{
	out = calloc(1);}
	switch(FLAGUNSET(t)){
		case optypeSTR: {
			out = realloc(out, strlen(out), strlen("String.\0"));
			strncpy(out + strlen("Required "), "String.\0", strlen("String.\0"));
		}case optypeDIGIT: {
			out = realloc(out, strlen(out), strlen("Digit.\0"));
			strncpy(out + strlen("Required "), "Digit.\0", strlen("Digit.\0"));
		}case optypeMULTIPLE: {
			out = realloc(out, strlen(out), strlen("Array.\0"));
			strncpy(out + strlen("Required "), "Array.\0", strlen("Array.\0"));
		}case optypeSWITCH: {
			out = realloc(out, strlen(out), strlen("Switch.\0"));
			strncpy(out + strlen("Required "), "Switch.\0", strlen("Switch.\0"));
		}case optypeFPATH: {
			out = realloc(out, strlen(out), strlen("File Path.\0"));
			strncpy(out + strlen("Required "), "File Path.\0", strlen("File Path.\0"));
		}
	}
	return out;
}

command_t *consume(char **arg, uint32_t *argl){
	for(uint32_t cc = 0; cc < *argl; ++cc){
		for(uint8_t cmdindex = 0; cmdindex < cmdcc; ++cmdindex){
			if(!strcmp(commands[cmdindex].cmd, arg[cc]) || !strcmp(commands[cmdindex].alias, arg[cc])){
				command_t *cmd = memdup(commands[cmdindex], sizeof(command_t));
				char *temp = NULL;
				// Populate options list
				for(;cc < *argl; cc++){
					temp = arg[cc];
					for(uint8_t argcc = 0; argcc < cmd.bf.len; ++argcc){
						if(!strcmp(temp, cmd.bf.opbuffer[argcc].opt)){
							// Search for exclude enabled
							if(!memsearch(cmd.bf.opbuffer, sizeof(option_t) * cmd.bf.len, cmd.bf.opbuffer[argcc].optype.optexclude, 8, offsetof(cmd.bf.opbuffer[argcc].optype.optexclude)){
								cmd.bf.opbuffer[argcc].enabled = true;
							}else{printf(ANSIRED("ERROR! The option %s cannot be used in junction with the option %s"), cmd.bf.opbuffer[argcc].optype.optexclude, cmd.bf.opbuffer[argcc].opt);
								return NULL;}
						}else(!strcmp(temp, "--help") || !strcmp(temp, "--help")){
							// Print a command's help info
							printf(
								"\n\"%s\" [\"%s\"]: %s",
								cmd.cmd, cdm.alias, cmd.cmddesc
							);
							printf("Params:");
							for(uint8_t cc = 0; cc < cmd.bf.len; ++cc){
								opbuffer item = cmd.bf.opbuffer[cc];
								printf(
									"\n\t\"%s\": %s"
									"\n\t%s"
									"\n\tCannot be used in combination with %s",
									item.opt, optype2str(item.optype.optype), item.opdesc, item.optype.optexclude
								);
							}
						}
					}
				}
				return cmd;
			}
		}
	}
	return NULL;
}

void *paramparse(uint8_t optype, void **in, uint8_t max){
	void *out = 0;
	uint32_t len = 0;
	for(uint8_t cc = 0; cc < max; ++cc){
		switch(FLAGUNSET(optype, optypeREQUIRED)){
			case optypeSTR: {
				out = strdup(in[cc]);
				return out;
			}
			case optypeMULTIPLE: {
				void *snippet = NULL;
				
				void **list = (void **)out;
				void **current = list + len;
				if((len % 5) == 0){list = realloc(list, (len + 5) * sizeof(void **));}
				else{
					uint32_t itemlen = 0;
					// Keep looping until index len has overflowed into the next entry
					while(in[cc][itemlen] != in[cc + 1][0]){itemlen++;}
					itemlen++;
					current[len] = memdup(in[cc], itemlen);
					len++;
				}
			}
			case optypeDIGIT: {
				out = malloc(sizeof(size_t));
				out[0] = strtol(in[cc]);
				return out;
			}
			case optypeFPATH: {
				FILE *f = fopen(in[cc], "r");
				if(f){
					fclose(f);
					out = strdup(in[cc]);
					return out;
				}
				return NULL;
			}
			default: {return out;}
		}
	}
	return out;
}

bool process(command_t *cmd){return cmd.func(&(cmd.bf));}
