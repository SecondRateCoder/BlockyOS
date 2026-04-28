#include "raw.h"
#include "ref/blake2.h"

#define FRATSIG "FRAT_FILESYSTEM USABLE"
#define FRATBLOCKSIG "FRATROOT"
#define PATHSEP '/'
#define PATHnoSEP '\\'

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
}miniGPT;

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
}GPTentry;

enumdef(fsattribute, uint8_t){
	__fsfile = 		0x0000,
	__fsdirectory = 0x0001,
	__fsreadonly = 	0x0002,
	__fsmetadatacluster = 0x0004
};

typedef struct fsblock{
	size_t fcode        : 42;
	size_t attr			: 16;
	uint32_t logalias;
	uint32_t index;
}fsblock;

/// @brief This is the expanded block of FileInfo
typedef struct conf_fsblock{
	char fsig[8];
	size_t headerversion;
	uint8_t coremeta[32];
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
}fsroot;

typedef struct conf_fsroot{
	fsroot root;
	size_t logLBA;
	LBA loc;
	uint32_t lastClusterAlloc;
	struct clusterBuffer{
		LBA clusterLBA;
		size_t clusterSize;
		fsblock *fs;
	}clusterBuffer;
}conf_fsroot;

typedef struct fshandle{
	conf_fsroot *root;
	fsblock *file;
	size_t progress;
	uint8_t progresslimit;
	struct handlecache{
		size_t progresstimestamp;
		// 1 means read, 0 means write.
		int8_t rw;
		void *block;
	}handlecache[8];
}fshandle;

typedef struct diritem{
	size_t fscode		: 42;
	size_t attributes	: 16;
	size_t index;
}diritem, *diribuffer;
typedef struct dirhandle{
    conf_fsroot *root;
	fsblock *current;
	char *path;
	diribuffer dirarray;
}dirhandle;

struct unhandle{
	bool dir;
	union{
		dirhandle *dhandle_;
		fhandle *fhandle_;
	};
}unhandle;

bool checkdisk(char *path);
LBA *queryparttablefs(miniGPT *gpt, rawenv *re);
bool queryfs(rawenv *re, LBA base);
conf_fsroot *fmount(char *path);
LBA *loadpart(GPTeNSTR name);
void formatpart(GPTeNSTR name);
GPTeNSTR *makeGPTeNSTR(char *str);

void __finit(conf_fsroot *root, fsblock *fb, char *path);


fshandle *fsloadh(conf_fsroot *root, char *path, char *args);
void _fpush1(fshandle *handle, void *buffer);
void *_fread1(fshandle *handle);
