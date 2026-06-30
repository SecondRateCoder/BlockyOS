#pragma once

#include "tools.h"
#include "frat.h"

enumdef(cmddesc_argtype, uint8_t){
	// It's presence means it's true, false if absent
	cmddescargtype__switch = 1,
	// Only takes true and false
	cmddescargtype__bool,
	cmddescargtype__char,
	cmddescargtype__byte,
	cmddescargtype__word,
	cmddescargtype__dword,
	cmddescargtype__qword,
	// One word is Parsed; or a Quote if quote marks are used
	cmddescargtype__string,
	// All proceeding Info is treated as Data.
	cmddescargtype__longinteger,
	cmddescargtype__longdata = cmddescargtype__longinteger
};

enumdef(cmddesc_errtype, uint8_t){
	cmddescerrtype_eom = 0x1, 
    cmddescerrtype_undefined_setting
};

typedef struct cmddesc_flag{
	const char flag[3],
				*desc;
	cmddesc_argtype type;
}cmddesc_flag;

typedef cmddesc_flag cmddesc_flagset[5];

typedef struct cmd_errout{
	char *msg;
	cmddesc_errtype errcode;
}cmd_errout;

typedef cmd_errout (* cmdfunc__)(size_t *buffer, void **persistent);

typedef struct cmddesc{
	const char cmd[8],
			   alias[8],
			   *desc;
	cmdfunc__ func;
	void *persistent;
    struct flags{
        uint8_t nflags;
        cmddesc_flagset flags;
    }flags;
}cmddesc;

#define MAXshellFHANDLES 8
#define cmd_erroutSUCCESS (cmd_errout){NULL, 0}

enumdef(symtype_e, uint32_t){
	symtype_e__string,
	symtype_e__fhandle,
	symtype_e__dirhandle,
	symtype_e__unknowndata = ~(symtype_e__dirhandle | symtype_e__fhandle | symtype_e__string)
};
typedef struct __symref{char *alias;    void *data;     symtype_e type;		uint32_t nbytes;}__symref;
bufdef(symrefbuffer, __symref, uint32_t);

cmd_errout __shellparse(char *input);

extern volatile cmddesc commands[];
extern const size_t ncommands;