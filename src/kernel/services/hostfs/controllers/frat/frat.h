#pragma once

#include "kernel/libcrt/def.h"
#include "kernel/libcrt/math/math.h"
#include "kernel/libcrt/memory/string.h"
#include "kernel/libcrt/memory/allocator/malloc.h"
#include "kernel/libcrt/BLAKE2/blake2.h"

#include "kernel/services/IO/service.h"

#define FRATPATHSEP										'\\'
#define FRATPATHnoSEP									'/'

#define FCODEHASHMASK									(UINT64_MAX & ~(UINT16_MAX << 48))
#define FRATSIG											"FRAT_FILESYSTEM"
#define FRATSIG_LEN										(sizeof(FRATSIG) - 1)
#define FRATSIGLEN										(FRATSIG_LEN)
#define FRATBLOCKSIG									"FRATROOT"
#define FRAT_PROGLIMIT									(2)
#define __FS_DEFAULTLOGSECTORS							(5)
#define __FS_DEFAULTBLOCKSIZE							(512)

#define FRATROOTOFFSET									(0)
#define LOGBLOCKOFFSET									(FRATROOTOFFSET + 1)
#define CLUSTERMAPOFFSET(nLogSectors)					(LOGBLOCKOFFSET + (nLogSectors) + 1)
#define DATAFIRSTOFFSET(nLogSectors, nClusterSectors)	(CLUSTERMAPOFFSET(nLogSectors) + (nClusterSectors) + 1)
#define DATAFIRST(root)									((root)->loc + DATAFIRSTOFFSET(root->logblocks.nLogSectors, root->clusterbuffer.nClusterSectors))

#define __CLUSTERMAPSECTORS_CALC(PARTFIRST, PARTLAST, nLogSectors) ( ((PARTLAST) - ((PARTFIRST) + (nLogSectors))) * (uint64_t)sizeof(fsblock) )
#define CLUSTERMAPSECTORS_CALC(PARTFIRST, PARTLAST, nLogSectors, confSectorSize) (__safediv(__CLUSTERMAPSECTORS_CALC(PARTFIRST, PARTLAST, nLogSectors) + ((confSectorSize) - 1), (confSectorSize)))

enumdef(uint8_t, fsattribute){
	__fsfile = 		0x0000,
	__fsdirectory = 0x0001,
	__fsreadonly = 	0x0002,
	__fsmetadatacluster = 0x0004,
	__fsproxy = 	0x0008
};

enumdef(uint16_t, logoperation){
	log__mov,
	log__delete,
	log__copy,
	log__alloc
};

typedef struct fslogitem{
	uint128_t sourcefcode;
	logoperation logop;
	uint64_t destfcode		: 48;
}__attribute__((packed)) fslogitem;

typedef struct fsblock{
	uint64_t fcodelow;
	uint64_t fcodehigh		: 48;
	uint64_t attributes		: 16;
	// uint32_t logalias;
	uint32_t index;
}__attribute__((packed)) fsblock;

/// @brief This is the expanded block of FileInfo
typedef struct meta_fsblock{
	char fsig[8];
	uint64_t headerversion;
	// uint8_t coremeta[32];
	fsblock f;
	uint64_t accesstime,
		   writetime,
		   accessdate,
		   writedate;
	char name[];
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
	GUID _GUID, altGUID;
	fsroot *root;
	LBA loc;
	uint32_t lastClusterAlloc;
	struct logblocks{
		uint32_t nLogSectors;
		fslogitem *logBlock;	// The Log Block
	}logblocks;
	struct clusterbuffer{
		uint64_t clusterSize,
		 	nClusterSectors,
			nClusterItems;
		fsblock *clusterMap;	// The Cluster-Map
	}clusterbuffer;
}conf_fsroot;

typedef struct fhandle{
	conf_fsroot *root;
	fsblock *file;
	uint64_t progress;
	char *path;
}fhandle;

typedef struct diritem{
	fsblock f;
	// A code, if it is 0, then this item is the end of the list
	int64_t local;
}__attribute__((packed)) diritem, *diribuffer;
typedef struct dirhandle{
    conf_fsroot *root;
	fsblock *file;
	char *path;
	uint64_t loadedblocks;
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
	uint64_t *index;
	uint64_t depth;
	/*
		Uses the Keys:
			'*': This is a single-char Wild-Card. It represents all Chars,
			'\': This is a Terminator. It forces any Wild-Card to be parsed as a Char instead of a Wild-Card
			'***': This is a Block Wild-Card, It essentially means that ANY snippet can be within it as long as the preceding char and proceeding char match
	*/
	char *pattmatcher;
	// Iteration stack state
	uint64_t stack_size;
	dirhandle **dir_stack;
	uint64_t *idx_stack;
	uint64_t stack_depth;
}dirrunner;

LBA getloc(conf_fsroot *root, fsblock *fb);
uint64_t __fsize(fhandle *fh);
uint64_t __dsize(dirhandle *dh);

bool checkdisk(rawenv re, GUID _GUID, GUID altGUID);
partdim queryparttablefs(miniGPT *gpt, rawenv re);
bool queryfs(rawenv re, LBA base);
conf_fsroot *fmount(rawenv re, GUID _GUID, GUID altGUID);
partdim loadpart(rawenv re, GUID _GUID, GUID altGUID, GPTNameStr name);
void formatpart(
	rawenv re, GUID _GUID, GUID altGUID, 
	GPTNameStr name, 
	uint32_t confBlockSize, uint32_t confLogSectors, 
	uint32_t verMAJOR, uint32_t verMINOR
);
void __finit(rawenv re, conf_fsroot *root, fsblock *fb, char *path);


void fuloadh(rawenv re, fhandle *handle);
fhandle *floadh(rawenv re, conf_fsroot *root, char *path, char *args);
uint64_t _fwrite(rawenv re, fhandle *handle, uint64_t nbytes, const void *data);
uint64_t _fread(rawenv re, fhandle *h, uint64_t nbytes, void **dataout);
dirhandle *__fgetparent(rawenv re, conf_fsroot *root, char *path);
bool __ftest(rawenv re, fhandle *h);
void fuloaddir(rawenv re, dirhandle *handle);
dirhandle *floadhdir(rawenv re, conf_fsroot *root, char *path, char *args);
meta_fsblock *_dreadinfo(rawenv re, dirhandle *handle);
meta_fsblock *_freadinfo(rawenv re, fhandle *handle);
fsblock *__faddr(rawenv re, conf_fsroot *root, fsblock *family);
void _fseek(fhandle *handle, uint64_t progress);
void _fseeko(fhandle *handle, int64_t progress);

fsblock *__ffind(conf_fsroot *root, char *path);
fsblock *__ffindh(conf_fsroot *root, uint64_t hash[2]);
void __ffremove(conf_fsroot *root, char *path);
void __ffremoveh(conf_fsroot *root, uint64_t hash[2]);
void __ffremovel(conf_fsroot *root, char *path);
void __ffremovelh(conf_fsroot *root, uint64_t hash[2]);

void __fdiradd(rawenv re, dirhandle *dir, fsblock *fb);
void __fdirrefresh(rawenv re, dirhandle *handle);
void __fupdatetstamp(rawenv re, conf_fsroot *root, fsblock *file, bool wt);
unhandle *__fdirlist(rawenv re, dirhandle *dir, uint64_t *index);
dirrunner *__dirr_init(dirhandle *handle, char *patternmatcher);
unhandle *__dirr(rawenv re, dirrunner *dr);
void __dirr_free(rawenv re, dirrunner *dr);

void __fprint_info(meta_fsblock *finfo);

LBA getloc(conf_fsroot *root, fsblock *fb);