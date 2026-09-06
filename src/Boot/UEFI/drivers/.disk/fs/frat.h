#pragma once

#include "efi.h"
#include "efilib.h"
#include "Boot/UEFI/drivers/.disk/raw/raw.h"
#include "Boot/UEFI/tools/tools.h"

#define FCODEHASHMASK	(UINT64_MAX & ~(UINT16_MAX << 48))

#define FRATSIG "FRAT_FILESYSTEM"
#define FRATSIG_LEN (sizeof(FRATSIG) - 1)
#define FRATSIGLEN STR((FRATSIG_LEN))
#define FRATBLOCKSIG "FRATROOT"
#define FRAT_PROGLIMIT 2
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
#define __CLUSTERMAPSECTORS_CALC(PARTFIRST, PARTLAST, nLogSectors) ( ((PARTLAST) - ((PARTFIRST) + (nLogSectors))) * (UINT64)sizeof(fsblock) )
#define CLUSTERMAPSECTORS_CALC(PARTFIRST, PARTLAST, nLogSectors, confSectorSize) (__safediv(__CLUSTERMAPSECTORS_CALC(PARTFIRST, PARTLAST, nLogSectors) + ((confSectorSize) - 1), (confSectorSize)))

enumdef(UINT8, fsattribute){
	__fsfile = 		0x0000,
	__fsdirectory = 0x0001,
	__fsreadonly = 	0x0002,
	__fsmetadatacluster = 0x0004,
	__fsproxy = 	0x0008
};

enumdef(UINT16, logoperation){
	log__mov,
	log__delete,
	log__copy,
	log__alloc
};

typedef struct fslogitem{
	UINT64 sourcefcode		: 48;
	logoperation logop;
	UINT64 destfcode		: 48;
}__attribute__((packed)) fslogitem;

typedef struct fsblock{
	uint64_t fcodelow;
	uint64_t fcodehigh		: 48;
	uint64_t attributes		: 16;
	// UINT32 logalias;
	UINT32 index;
}__attribute__((packed)) fsblock;

/// @brief This is the expanded block of FileInfo
typedef struct meta_fsblock{
	char fsig[8];
	UINT64 headerversion;
	fsblock f;
	UINT64 accesstime,
			writetime,
			accessdate,
			writedate;
	char name[];
}meta_fsblock;

#define MAKEVERSION(MAJOR, MINOR)	{(UINT32)(MAJOR), (UINT32)(MINOR)}
typedef struct fsroot{
	char signature[sizeof(FRATSIG)];
	UINT32 confLogSectors,
			confBlockSize,
			confClusterSize;
	UINT32 verCode[2];
}__attribute__((packed)) fsroot;

typedef struct conf_fsroot{
	EFI_GUID GUID, altGUID;
	fsroot *root;
	LBA loc;
	UINT32 lastClusterAlloc;
	struct logblocks{
		UINT32 nLogSectors;
		fslogitem *logBlock;	// The Log Block
	}logblocks;
	struct clusterbuffer{
		UINT64 clusterSize,
		 	nClusterSectors,
			nClusterItems;
		fsblock *clusterMap;	// The Cluster-Map
	}clusterbuffer;
}conf_fsroot;

typedef struct fhandle{
	conf_fsroot *root;
	fsblock *file;
	UINT64 progress;
	char *path;
}fhandle;

typedef struct diritem{
	fsblock f;
	// A code, if it is 0, then this item is the end of the list
	INT64 local;
}__attribute__((packed)) diritem, *diribuffer;
typedef struct dirhandle{
    conf_fsroot *root;
	fsblock *file;
	char *path;
	UINT64 loadedblocks;
	diribuffer dirarray;
}dirhandle;

typedef struct unhandle{
	BOOLEAN dir;
	union{
		dirhandle *dhandle_;
		fhandle *fhandle_;
		void *__notype;
	};
}unhandle;

typedef struct dirrunner{
	dirhandle *dir;
	unhandle **handles;
	UINT64 *index;
	UINT64 depth;
	/*
		Uses the Keys:
			'*': This is a single-char Wild-Card. It represents all Chars,
			'\': This is a Terminator. It forces any Wild-Card to be parsed as a Char instead of a Wild-Card
			'***': This is a Block Wild-Card, It essentially means that ANY snippet can be within it as long as the preceding char and proceeding char match
	*/
	char *pattmatcher;
	// Iteration stack state
	UINT64 stack_size;
	dirhandle **dir_stack;
	UINT64 *idx_stack;
	UINT64 stack_depth;
}dirrunner;

LBA getloc(conf_fsroot *root, fsblock *fb);
UINT64 __fsize(fhandle *fh);
UINT64 __dsize(dirhandle *dh);

BOOLEAN checkdisk(EFI_GUID GUID, EFI_GUID altGUID);
partdim queryparttablefs(miniGPT *gpt, rawenv re);
BOOLEAN queryfs(rawenv re, LBA base);
conf_fsroot *fmount(EFI_GUID GUID, EFI_GUID altGUID);
partdim loadpart(EFI_GUID GUID, EFI_GUID altGUID, GPTeNSTR name);
void formatpart(
	EFI_GUID GUID, EFI_GUID altGUID, 
	GPTeNSTR name, 
	UINT32 confBlockSize, UINT32 confLogSectors, 
	UINT32 verMAJOR, UINT32 verMINOR
);
void __finit(conf_fsroot *root, fsblock *fb, char *path);


void fuloadh(fhandle *handle);
fhandle *floadh(conf_fsroot *root, char *path, char *args);
UINT64 _fwrite(fhandle *handle, UINT64 nbytes, const void *data);
UINT64 _fread(fhandle *h, UINT64 nbytes, void **dataout);
dirhandle *__fgetparent(conf_fsroot *root, char *path);
BOOLEAN __ftest(fhandle *h);
void fuloaddir(dirhandle *handle);
dirhandle *floadhdir(conf_fsroot *root, char *path, char *args);
meta_fsblock *_dreadinfo(dirhandle *handle);
meta_fsblock *_freadinfo(fhandle *handle);
fsblock *__faddr(conf_fsroot *root, fsblock *family);
void _fseek(fhandle *handle, UINT64 progress);
void _fseeko(fhandle *handle, INTN progress);

fsblock *__ffind(conf_fsroot *root, char *path);
fsblock *__ffindh(conf_fsroot *root, UINT64 hash[2]);
void __ffremove(conf_fsroot *root, char *path);
void __ffremoveh(conf_fsroot *root, UINT64 hash[2]);
void __ffremovel(conf_fsroot *root, char *path);
void __ffremovelh(conf_fsroot *root, UINT64 hash[2]);

void __fdiradd(dirhandle *dir, fsblock *fb);
void __fdirrefresh(dirhandle *handle);
void __fupdatetstamp(conf_fsroot *root, fsblock *file, BOOLEAN wt);
unhandle *__fdirlist(dirhandle *dir, UINT64 *index);
dirrunner *__dirr_init(dirhandle *handle, char *patternmatcher);
unhandle *__dirr(dirrunner *dr);
void __dirr_free(dirrunner *dr);

void __fprint_info(meta_fsblock *finfo);

LBA getloc(conf_fsroot *root, fsblock *fb);