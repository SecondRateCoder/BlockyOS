#pragma once

#include "ref/blake2.h"
#include "raw.h"

#define FRATSIG "FRAT_FILESYSTEM"
#define FRATSIG_LEN (sizeof(FRATSIG) - 1)
#define FRATSIGLEN STR((FRATSIG_LEN))
#define FRATBLOCKSIG "FRATROOT"
#define FRAT_PROGLIMIT 2
#define __FS_DEFAULTFORMATEDVERSION "1:0"
#define __FS_DEFAULTLOGSECTORS 5
#define __FS_DEFAULTBLOCKSIZE 512

#define FRATROOTOFFSET (0)
#define LOGBLOCKOFFSET (FRATROOTOFFSET + 1)
#define CLUSTERMAPOFFSET(nLogSectors) (LOGBLOCKOFFSET + (nLogSectors) + 1)
#define DATAFIRSTOFFSET(nLogSectors, nClusterSectors) (CLUSTERMAPOFFSET(nLogSectors) + (nClusterSectors) + 1)
#define DATAFIRST(root)	((root)->loc + DATAFIRSTOFFSET(root->logblocks.nLogSectors, root->clusterbuffer.nClusterSectors))

// #define __CLUSTERMAPSECTORS_CALC(PARTFIRST, PARTLAST, nLogSectors)	(((PARTLAST) - ((PARTFIRST) + (nLogSectors))) * sizeof(fsblock))
// #define CLUSTERMAPSECTORS_CALC(PARTFIRST, PARTLAST, nLogSectors, confSectorSize)	\
// __safediv((__CLUSTERMAPSECTORS_CALC(PARTFIRST, PARTLAST, nLogSectors) - 						\
// __safediv(__CLUSTERMAPSECTORS_CALC(PARTFIRST, PARTLAST, nLogSectors), (confSectorSize)) + ((__CLUSTERMAPSECTORS_CALC(PARTFIRST, PARTLAST, nLogSectors) % (confSectorSize)) != 0)	\
// ), confSectorSize)
#define __CLUSTERMAPSECTORS_CALC(PARTFIRST, PARTLAST, nLogSectors) ( ((PARTLAST) - ((PARTFIRST) + (nLogSectors))) * (size_t)sizeof(fsblock) )
#define CLUSTERMAPSECTORS_CALC(PARTFIRST, PARTLAST, nLogSectors, confSectorSize) (__safediv(__CLUSTERMAPSECTORS_CALC(PARTFIRST, PARTLAST, nLogSectors) + ((confSectorSize) - 1), (confSectorSize)))

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
	size_t index;
}__attribute__((packed)) fsblock;

/// @brief This is the expanded block of FileInfo
typedef struct meta_fsblock{
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
}meta_fsblock;

#define MAKEVERSION(MAJOR, MINOR)	{(uint32_t)(MAJOR), (uint32_t)(MINOR)}
typedef struct fsroot{
	char signature[sizeof(FRATSIG)];
	uint32_t confLogSectors,
			confBlockSize,
			confClusterSize;
	uint32_t verCode[2];
}__attribute__((packed)) fsroot;

typedef struct conf_fsroot{
	_GUID GUID, altGUID;
	char *path;
	fsroot *root;
	LBA loc;
	uint32_t lastClusterAlloc;
	struct logblocks{
		uint32_t nLogSectors;
		fslogitem *logBlock;	// The Log Block
	}logblocks;
	struct clusterbuffer{
		size_t clusterSize,
		 	nClusterSectors;
		fsblock *clusterMap;	// The Cluster-Map
	}clusterbuffer;
}conf_fsroot;

typedef struct fhandle{
	conf_fsroot *root;
	fsblock *file;
	size_t progress;
	char *path;
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
		void *__notype;
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
size_t __fsize(fhandle *fh);
size_t __dsize(dirhandle *dh);

bool checkdisk(char *path);
partdim queryparttablefs(miniGPT *gpt, rawenv re);
bool queryfs(rawenv re, LBA base);
conf_fsroot *fmount(char *path);
partdim loadpart(char *path, GPTeNSTR name);
void formatpart(
	char *path,
	GPTeNSTR name, 
	uint32_t confBlockSize, uint32_t confLogSectors, 
	uint32_t verMAJOR, uint32_t verMINOR
);
void __finit(conf_fsroot *root, fsblock *fb, char *path);


void fuloadh(fhandle *handle);
fhandle *floadh(conf_fsroot *root, char *path, char *args);
void __fcreate(conf_fsroot *root, char *path, char *flags);
size_t _fwrite(fhandle *handle, size_t nbytes, const void *data);
size_t _fread(fhandle *h, size_t nbytes, void **dataout);
dirhandle *__fgetparent(conf_fsroot *root, char *path);
bool __ftest(fhandle *h);
void fuloaddir(dirhandle *handle);
dirhandle *floadhdir(conf_fsroot *root, char *path, char *args);
meta_fsblock *_dreadinfo(dirhandle *handle);
meta_fsblock *_freadinfo(fhandle *handle);
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

void __fprint_info(meta_fsblock *finfo);

LBA getloc(conf_fsroot *root, fsblock *fb);

void fuloadroot(conf_fsroot *fr);