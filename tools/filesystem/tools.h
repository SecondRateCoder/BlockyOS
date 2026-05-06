#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <inttypes.h>
#include "ref/blake2.h"

#define PATHSEP '/'
#define PATHnoSEP '\\'
#define CMDiMAX 256

#define enumdef(name, type)     typedef type name;  enum

#define flagcheck(v, f) ((v & f) == f)
#define flagset(v, f)   (v |= f)
#define flagunset(v, f) (v &= ~f)

enumdef(strtokflags, uint32_t){
	strtok__ForceSameBorderingDelims = 0x1,
	strtok__ForceDifferentBorderingDelims,
	strtok__ForceStartingDelim = 0x80000000,
	strtok__ForceEndingDelim = 0x00002000
};

typedef struct strtok_t{
	/// @brief A duplicate of the Token.
	char *dup;
	/// @brief The recovered Token.
	char *tok;
	/// @brief The configured delims;
	char *delims;
	// The encountered delims
	char sdelim, edelim;
	uint32_t flags;
}strtok_t;

bool strcheck(char *s, char c);
ssize_t strchecki(char *s, char c);
void *memdup(void *mem, size_t s);
size_t __getfcode(char *s_);
char *readbuf(size_t s, char *prefix);

strtok_t *strtok_i(char *in, char *delims, uint32_t enables);
char *strtok_k(strtok_t *tstate);
void strtok_d(strtok_t *tstate);

int trng__(void *buffer, size_t len);

char *str_tolower(char *s);

bool __pattmatch(const char *pattern, const char *str);



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
	cmddescerrtype_outofmemory
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

typedef cmd_errout (* cmdinit__)(void **persistptr, char **argv, uint32_t argc);
typedef cmd_errout (* cmdfunc__)(size_t *buffer, void *persistent);

typedef struct cmddesc{
	const char cmd[8],
			   alias[8],
			   *desc;
	cmdinit__ init;
	cmdfunc__ func;
	uint8_t numflags;
	void *persistent;
	cmddesc_flagset flags;
}cmddesc;

cmd_errout *shellinit(char **argc, uint32_t argv);
cmd_errout shell(char *in);

#define MAXshellFHANDLES 8
#define cmd_erroutSUCCESS (cmd_errout){NULL, 0}
#define MKPERSIST_NOUSE "_mknouse"

// Create/mk
cmd_errout mkinit__(void **persistptr, char **argv, uint32_t argc);		cmd_errout mkop__(size_t *buffer, void *persistptr);
// Remove/rm
cmd_errout rmop__(size_t *buffer, void *persistptr);
// Write/<<
cmd_errout wrop__(size_t *buffer, void *persistptr);
// Read/>>
cmd_errout reop__(size_t *buffer, void *persistptr);
// Mount/mt
cmd_errout mtinit__(void **persistptr, char **argv, uint32_t argc);		cmd_errout mtop__(size_t *buffer, void *persistptr);
// UnMount/ut
cmd_errout utinit__(void **persistptr, char **argv, uint32_t argc);		cmd_errout utop__(size_t *buffer, void *persistptr);
// ListMount/lt
cmd_errout ltinit__(void **persistptr, char **argv, uint32_t argc);		cmd_errout ltop__(size_t *buffer, void *persistptr);
//Exit/ex
cmd_errout exitop__(size_t *buffer, void *persistptr);
//help/??
cmd_errout helpop__(size_t *buffer, void *persistptr);

static const cmddesc commands[] = {
	{
		"create", "mk", "Create a File/Directory at a Specified Path", 
		NULL, mkop__, 4, NULL,
		{
			{
				"p",
				"The Path to the Item. A value of \'.../\' routes to the Root.",
				cmddescargtype__string
			}, {
				"n", 
				"The Name of The File/Directory; Use: \"<NAME>\" when there are spaces but \"\" are not necessary when there aren't.",
				cmddescargtype__string
			}, {
				"sys",
				"The Item Type: \'d\': Directory, \'f\'(Default): File, \'s\': Symbolic Link.",
				cmddescargtype__char
			}, {
				"a",
				"An alias that the opened File will be given",
				cmddescargtype__string
			}
		}
	}, {
		"delete", "rm", "Remove a File/Directory at a Specified Path",
		NULL, rmop__, 2, NULL, 
		{
			{
				"p",
				"The Path to the File/Directory",
				cmddescargtype__string
			}, {
				"w",
				"A wildcard that should be used with the operation",
				cmddescargtype__char
			}
		}
	}, {
		"write", "<<", "Write Source Data to a File",
		NULL, wrop__, 4, NULL,
		{
			{
				"f",
				"The File/Symbolic-Link/Alias to write to",
				cmddescargtype__string
			}, {
				"a",
				"The args to r/w the File/Symbolic Link in. Can be: p(proxy: This takes \"d\" as a Literal File Path), r(Raw), a(append; Cannot Seek), w(Write: Starting at Pos 0(Default)). In the format of <ARG1>,<ARG2>...",
				cmddescargtype__string
			}, {
				"dss",
				"Format: [(If Proxy is Enabled) The amount of Data to write; A value of -1 euates to write all Data]:[The Destination File offset; in LBAs]:[(If Proxy is Enabled) The Source File Offset.]",
				cmddescargtype__qword
			}, {
				"d",
				"The Data to write; or the File Path",
				cmddescargtype__longdata
			}
		}
	}, {
		"read", ">>", "Read File Data to a Source",
		NULL, reop__, 4, NULL,
		{
			{
				"f",
				"The File/Symbolic-Link/Alias to read from",
				cmddescargtype__string
			}, {
				"o",
				"The output Source. Accepted Values: stdio(Console), alias(A Mounted File), ext(A source File)"
			}, {
				"dss",
				"Format: [(If Proxy is Enabled) The amount of Data to write; A value of -1 euates to write all Data]:[The Destination File offset; in LBAs]:[(If Proxy is Enabled) The Source File Offset.]",
				cmddescargtype__qword
			}, {
				"p",
				"The Path/Alias name",
				cmddescargtype__longdata
			}
		}
	}, {
		"mount", "mt", "Mount a File to an Alias. Outputs whether the Mount succeeded",
		mtinit__, mtop__, 2, NULL,
		{
			{
				"f",
				"The File Path to be mounted/opened",
				cmddescargtype__string
			}, {
				"a",
				"The created alias name",
				cmddescargtype__string
			}
		}
	}, {
		"unmount", "ut", "UnMount a File from an Alias. Outputs whether the Mount succeeded",
		utinit__, utop__, 2, NULL,
		{
			{
				"f",
				"The File Path to be mounted/opened",
				cmddescargtype__string
			}, {
				"a",
				"The created alias name",
				cmddescargtype__string
			}
		}
	}, {
		"lsmount", "lt", "List all Mounted Items",
		ltinit__, ltop__, 0, NULL, {}
	}, {
		"exit", "ex", "Exit The Program",
		NULL, exitop__, 0, NULL, {}
	}, {
		"help", "??", "Output Help Info",
		NULL, helpop__, 0, NULL, {}
	}
};

static const uint16_t numcommands = sizeof(commands) / sizeof(cmddesc);