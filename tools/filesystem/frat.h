#pragma once

#include "raw.h"

#define FRATSIG "FRAT_FILESYSTEM USABLE"
#define FRATBLOCKSIG "FRATROOT"
#define FRAT_PROGLIMIT 2

typedef struct miniGPT{
	char sig[8];
	uint32_t rev;
	uint32_t hSize;
	uint32_t hChecksum;
	uint32_t r;
	LBA localLBA;
	LBA alternateLBA;
	LBA fUsable;
	LBA lUsable;
	size_t dGUID[2];
	LBA GPTarray;
	uint32_t partEntries;
	uint32_t partEntrySize;
}__attribute__((packed)) miniGPT;

typedef uint16_t cword_t;
#define GPTeNAMESIZE 72
#define GPTeNAMELEN (GPTeNAMESIZE / sizeof(cword_t))
typedef cword_t GPTeNSTR[GPTeNAMELEN];
typedef struct GPTentry{
	size_t GUID[2];
	size_t uGUID[2];
	LBA sLBA;
	LBA eLBA;
	size_t attr;
	GPTeNSTR name;
}__attribute__((packed)) GPTentry;

enumdef(fsattribute, uint8_t){
	__fsfile = 		0x0000,
	__fsdirectory = 0x0001,
	__fsreadonly = 	0x0002,
	__fsmetadatacluster = 0x0004,
	__fsproxy = 	0x0008
};

enumdef(logoperation, uint16_t){
	log__mov,
	log__delete,
	log__copy,
	log__alloc
};

typedef struct fslogitem{
	size_t sourcefcode		: 42;
	logoperation logop;
	size_t destfcode		: 42;
}__attribute__((packed)) fslogitem;

typedef struct fsblock{
	size_t fcode        : 42;
	size_t attr			: 16;
	// uint32_t logalias;
	uint32_t index;
}__attribute__((packed)) fsblock;

/// @brief This is the expanded block of FileInfo
typedef struct conf_fsblock{
	char fsig[8];
	size_t headerversion;
	// uint8_t coremeta[32];
	size_t fcode        : 42;
	size_t attributes	: 16;
	char name[GPTeNAMELEN];
	uint32_t accesstime,
		   writetime,
		   accessdate,
		   writedate;
}conf_fsblock;

typedef struct fsroot{
	uint32_t confBlockSize;
	uint32_t confClusterSize;
	size_t verCode[2];
	char signature[22];
}__attribute__((packed)) fsroot;

typedef struct conf_fsroot{
	fsroot *root;
	size_t logLBA;
	LBA loc;
	uint32_t lastClusterAlloc;
	struct clusterBuffer{
		LBA clusterLBA;
		size_t clusterSize;    // Number of entries (not sectors!)
		size_t clusterSectors; // Number of sectors for writeblock
		fsblock *fs;
	}clusterBuffer;
}conf_fsroot;

typedef struct fhandle{
	conf_fsroot *root;
	fsblock *file;
	size_t progress;
	uint8_t progresslimit;
	struct handlecache{
		ssize_t progresstimestamp;
		// 1 means read, 0 means write.
		int8_t rw;
		void *block;
	}handlecache[4];
}fhandle;

typedef struct diritem{
	size_t fcode		: 42;
	size_t attributes	: 16;
	// A code, if it is 0, then this item is the end of the list
	ssize_t local;
}__attribute__((packed)) diritem, *diribuffer;
typedef struct dirhandle{
    conf_fsroot *root;
	fsblock *file;
	char *path;
	size_t loadedblocks;
	diribuffer dirarray;
}dirhandle;

typedef struct unhandle{
	bool dir;
	union{
		dirhandle *dhandle_;
		fhandle *fhandle_;
	};
}unhandle;

typedef struct dirrunner{
	dirhandle *dir;
	unhandle **handles;
	size_t *index;
	size_t depth;
	/*
		Uses the Keys:
			'*': This is a single-char Wild-Card. It represents all Chars,
			'\': This is a Terminator. It forces any Wild-Card to be parsed as a Char instead of a Wild-Card
			'***': This is a Block Wild-Card, It essentially means that ANY snippet can be within it as long as the preceding char and proceeding char match
	*/
	char *pattmatcher;
	// Iteration stack state
	size_t stack_size;
	dirhandle **dir_stack;
	size_t *idx_stack;
	size_t stack_depth;
}dirrunner;

LBA getloc(conf_fsroot *root, fsblock *fb);

bool checkdisk(char *path);
LBA *queryparttablefs(miniGPT *gpt, rawenv *re);
bool queryfs(rawenv *re, LBA base);
conf_fsroot *fmount(char *path);
LBA *loadpart(GPTeNSTR name);
void formatpart(GPTeNSTR name);
GPTeNSTR *makeGPTeNSTR(char *str);

void __finit(conf_fsroot *root, fsblock *fb, char *path);


void fsuloadh(fhandle *handle);
fhandle *fsloadh(conf_fsroot *root, char *path, char *args);
void _fpush1(fhandle *handle, void *buffer);
void *_fread1(fhandle *handle);
dirhandle *__fgetparent(conf_fsroot *root, char *path);
bool __ftest(fhandle *h);
void __fuloaddir(dirhandle *handle);
dirhandle *__floaddir(conf_fsroot *root, char *path, char *args);
conf_fsblock *_dreadinfo(dirhandle *handle);
conf_fsblock *_freadinfo(fhandle *handle);
fsblock *__faddr(conf_fsroot *root, fsblock *family);
void _fseek(fhandle *handle, size_t progress);
void _fseeko(fhandle *handle, ssize_t progress);

fsblock *__ffind(conf_fsroot *root, char *path);
fsblock *__ffindh(conf_fsroot *root, size_t hash);
void __ffremove(conf_fsroot *root, char *path);
void __ffremoveh(conf_fsroot *root, size_t hash);
void __ffremovel(conf_fsroot *root, char *path);
void __ffremovelh(conf_fsroot *root, size_t hash);

void __fdiradd(dirhandle *dir, fsblock *fb);
void __fdirrefresh(dirhandle *handle);
void __fupdatetstamp(conf_fsroot *root, fsblock *file, bool wt);
unhandle *__fdirlist(dirhandle *dir, size_t *index);
dirrunner *__dirr_init(dirhandle *handle, char *patternmatcher);
unhandle *__dirr(dirrunner *dr);
void __dirr_free(dirrunner *dr);

void __fprint_info(conf_fsblock *finfo);