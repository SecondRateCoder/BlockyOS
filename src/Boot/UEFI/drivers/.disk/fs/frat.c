#include "frat.h"

BOOLEAN checkdisk(EFI_GUID GUID, EFI_GUID altGUID){
	DEBUGPRINT(L"\nVerifying Disk GPT");
	bool out = false;
	rawenv re = startup(GUID, altGUID, __FS_DEFAULTBLOCKSIZE);
	uint8_t *block = readblocks(re, GPT_LBA, sizeof(miniGPT));
	miniGPT *gpt = (miniGPT *)block;
	DEBUGDO{
		DEBUGPRINT(
			L"\nGPT Dump:"
			L"\n    Sig: \"%a\""
			L"\n    Rev: %u"
			L"\n    Header-Size: %u"
			L"\n    Header Checksum: %u"
			L"\n    localLBA: %llu"
			L"\n    altLBA: %llu"
			L"\n    firstUsable: %llu"
			L"\n    lastUsable: %llu"
			L"\n    Disk-GUID: ",
			gpt->sig, gpt->rev, gpt->hSize, gpt->hChecksum,
			gpt->localLBA, gpt->alternateLBA, gpt->fUsable, gpt->lUsable
		);
		prGUID(gpt->dGUID);
		DEBUGPRINT(
			L"\n    Partition-Table: %llu"
			L"\n    # of Partition-Entries: %u"
			L"\n    Partition-Entry Size: %u"
			L"\n    Partition-Table Checksum: %u",
			gpt->partEntryLoc, gpt->nPartEntries, gpt->partEntrySize,
			gpt->partArrayChecksum
		);
	}
	if(!__memcmp(gpt->sig, "EFI PART", 8)){out = TRUE;}else{out = FALSE;}
	__free(block);
	dispose(re);
	DEBUGPRINT(L"\nGPT Exists? %a", ((out == true)? "TRUE": "FALSE"));
	return out;
}

partdim loadpart(EFI_GUID GUID, EFI_GUID altGUID, GPTeNSTR name){
	char gptname[GPTeNAMELEN + 1];	ZeroMem(gptname, GPTeNAMELEN + 1);
	for(UINT8 cc = 0; cc < GPTeNAMELEN; ++cc){gptname[cc] = (name[cc] & 0xFF);}
		DEBUGPRINT(L"\n[%s:%u]  >>  Loading Partition:    %a", (L"" __FILE__), __LINE__, gptname);
	if(checkdisk(GUID, altGUID)){
		rawenv re = startup(GUID, altGUID, __FS_DEFAULTBLOCKSIZE);
		miniGPT *gpt = (miniGPT *)readblocks(re, GPT_LBA, sizeof(miniGPT));
		GPTentry *ge = (GPTentry *)readblocks(re, gpt->partEntryLoc, gpt->nPartEntries * sizeof(GPTentry));
		dispose(re);
		for(UINT32 i = 0; i < gpt->nPartEntries; i++){
			if(!__memcmp(name, ge[i].name, GPTeNAMESIZE)){
				partdim out = {.base = ge[i].sLBA, .high = ge[i].eLBA};
				__free(gpt);
				__free(ge);
				DEBUGPRINT(L"\n[%s:%u]  >>  Found Partition: %llu:%llu", (L"" __FILE__), __LINE__, out.base, out.high);
				return out;
			}
		}
		__free(gpt);
		__free(ge);
	}
	DEBUGPRINT(L"\n[%s:%u]  >>  Found No Partition named: %a", (L"" __FILE__), __LINE__, gptname);
	return (partdim){0, 0};
}

void formatpart(
	EFI_GUID GUID, EFI_GUID altGUID, 
	GPTeNSTR name, 
	UINT32 confBlockSize, UINT32 confLogSectors, 
	UINT32 verMAJOR, UINT32 verMINOR
){
	DEBUGDO{
		char gptname[GPTeNAMELEN + 1];	ZeroMem(gptname, GPTeNAMELEN + 1);
		for(UINT8 cc = 0; cc < GPTeNAMELEN; ++cc){gptname[cc] = (name[cc] & 0xFF);}
		DEBUGPRINT(L"\n[%s:%u]  >>  Formatting Partition:    %a", (L"" __FILE__), __LINE__, gptname);
	}
	partdim part = loadpart(GUID, altGUID, name);
	if(!part.high){return;}
	rawenv re = startup(GUID, altGUID, confBlockSize);
	DEBUGDO{
		DEBUGPRINT(
			L"\n[%s:%u]  >>  Format Target: %llu:%llu -> %llu LBAs"
			L"\n    Version-Code: [%u:%u]"
			L"\n    Configured Block-Size: %u"
			L"\n    Configured Cluster-Size: %u"
			L"\n    Configured # of Log-Sectors: %u"
			L"\n    Configured Log-Size: %u",
			((L"" __FILE__)), __LINE__, 
			part.base, part.high, part.high - part.base, 
			verMAJOR, verMINOR, confBlockSize, 
			CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize), 
			confLogSectors, confLogSectors * sizeof(fslogitem)
		);
	}

	// Root-Block
	void *block = __calloc(confLogSectors, confBlockSize);
	// ZeroMem(block, confBlockSize * confLogSectors);
	fsroot *fr = block;
	*fr = (fsroot){
		.confLogSectors = confLogSectors,
		.confBlockSize = confBlockSize,
		.confClusterSize = CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize),
		.verCode = MAKEVERSION(verMAJOR, verMINOR),
	};
	// Write FSROOT
	__memcpy(fr->signature, FRATSIG, sizeof(FRATSIG));
	writeblocks(re, block, part.base + FRATROOTOFFSET, sizeof(fsroot));

	// Log-Block
	__memset(block, 0, confBlockSize * confLogSectors);
	writeblocks(re, block, part.base + LOGBLOCKOFFSET, confBlockSize * confLogSectors);

	// Cluster-Map
	__free(block);
	block = __calloc(CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize), confBlockSize);
	// __memset(block, 0, CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize) * confBlockSize);
	writeblocks(re, block, part.base + CLUSTERMAPOFFSET(confLogSectors), CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize) * confBlockSize);
	__free(block);
	dispose(re);
	DEBUGPRINT(L"\n[%s:%u]  >>  Formatted Partition", (L"" __FILE__), __LINE__);
}

partdim queryparttablefs(miniGPT *gpt, rawenv re){
	DEBUGPRINT(L"\n[%s:%u]  >>  Querying Part Table", (L"" __FILE__), __LINE__);
	// Query all Partitions for the FileSystem.
	GPTentry *ge = (GPTentry *)readblocks(re, gpt->partEntryLoc, gpt->nPartEntries * sizeof(GPTentry));
	for(UINT32 i = 0; i < gpt->nPartEntries; i++){
		char gptname[GPTeNAMELEN + 1];	ZeroMem(gptname, GPTeNAMELEN + 1);
		for(UINT8 cc = 0; cc < GPTeNAMELEN; ++cc){gptname[cc] = (ge[i].name[cc] & 0xFF);}
		DEBUGPRINT(L"\n[%s:%u]  >>  GPT-Entry: %a{%llu:%llu -> %llu}", (L"" __FILE__), __LINE__, gptname, ge[i].sLBA, ge[i].eLBA, ge[i].eLBA - ge[i].sLBA);
		if(queryfs(re, ge[i].sLBA)){
			partdim out = {.base = ge[i].sLBA, .high = ge[i].eLBA};
			__free(ge);
			return out;
		}
	}
	__free(ge);
	return (partdim){0, 0};
}

BOOLEAN queryfs(rawenv re, LBA partbase){
	DEBUGPRINT(L"\n[%s:%u]  >>  Querying FS at %llu", (L"" __FILE__), __LINE__, partbase);
	// Check that a FileSystem exists at the bytebase.
	BOOLEAN out = 0;
	fsroot *fr = (fsroot *)readblocks(re, partbase + FRATROOTOFFSET, sizeof(fsroot));
	DEBUGPRINT(
		L"\nFS-Root Blob:"
		L"\n    Version: [%u:%u]"
		L"\n    Sig: %.16a"
		L"\n    Configured Log Sectors: %u"
		L"\n    Configured Block Size: %u"
		L"\n    Configured Cluster-Map Size: %u",
		fr->verCode[0], fr->verCode[1], 
		fr->signature, fr->confLogSectors, 
		fr->confBlockSize, fr->confClusterSize
	);
	if(!__memcmp(fr->signature, FRATSIG, sizeof(FRATSIG))){
		DEBUGPRINT(
			L"\n[%s:%u]  >>  FS Found at [%llu]:"
			L"\n	Version-Code: [%u:%u]"
			L"\n	Configured Block-Size: %u"
			L"\n	Configured Cluster-Size: %u"
			L"\n	Configured # of Log-Sectors: %u"
			L"\n	Configured Log-Size: %u", 
			(L"" __FILE__), __LINE__, partbase, fr->verCode[0], fr->verCode[1], 
			fr->confBlockSize, fr->confClusterSize, fr->confLogSectors, fr->confLogSectors * sizeof(fslogitem)
		);
		out = TRUE;
	}else{out = FALSE;}
	__free(fr);
	return out;
}

conf_fsroot *fmount(EFI_GUID GUID, EFI_GUID altGUID){
	DEBUGDO{DEBUGPRINT(L"\n[%s:%u]  >>  Mounting FS Root: [", (L"" __FILE__), __LINE__);	prGUID(GUID);	Print(L"]    [");	prGUID(altGUID);	Print(L"]");}
	partdim partition;
	if(checkdisk(GUID, altGUID)){
		// Get the LBA Info for a Partition
		rawenv re = startup(GUID, altGUID, __FS_DEFAULTBLOCKSIZE);
		void *block = readblocks(re, GPT_LBA, sizeof(miniGPT));
		miniGPT *gpt = (miniGPT *)block;
		if((partition = queryparttablefs(gpt, re)).high){
			__free(gpt);
		}else{__free(gpt);    return NULL;}
		DEBUGPRINT(
			L"\n[%s:%u]  >>  Found Formatted Partition: %llu:%llu -> %llu", 
			(L"" __FILE__),__LINE__, partition.base, partition.high, partition.high - partition.base
		);
		// Generate the fsroot
		fsroot *fsroot_ = (fsroot *)readblocks(re, partition.base + FRATROOTOFFSET, sizeof(fsroot));
		DEBUGPRINT(
			L"\n[%s:%u]  >>  Root: %llu:%llu -> %llu LBAs"
			L"\n	Version-Code: [%u:%u]"
			L"\n	Configured Block-Size: %u"
			L"\n	Configured Cluster-Size: %u"
			L"\n	Configured # of Log-Sectors: %u"
			L"\n	Configured Log-Size: %u", 
			(L"" __FILE__),__LINE__, partition.base, partition.high, partition.high - partition.base, 
			fsroot_->verCode[0], fsroot_->verCode[1], 
			fsroot_->confBlockSize, CLUSTERMAPSECTORS_CALC(partition.base, partition.high, fsroot_->confLogSectors, fsroot_->confBlockSize), 
			fsroot_->confClusterSize, fsroot_->confLogSectors, fsroot_->confLogSectors * sizeof(fslogitem)
		);
		setblocksize(re, fsroot_->confBlockSize);
		conf_fsroot *largeroot = __calloc(1, sizeof(conf_fsroot));
		*largeroot = (conf_fsroot){
			.loc = 0,
			.root = fsroot_,
			.lastClusterAlloc = 0,
			.logblocks = {
				.logBlock = readblocks(re, partition.base + LOGBLOCKOFFSET, fsroot_->confBlockSize * fsroot_->confLogSectors),
				.nLogSectors = fsroot_->confLogSectors
			},
			.clusterbuffer = {
				.nClusterSectors = __safediv(fsroot_->confClusterSize, fsroot_->confBlockSize),
				.clusterSize = fsroot_->confClusterSize,
				.nClusterItems = __safediv(fsroot_->confClusterSize, sizeof(fsblock)),
				.clusterMap = readblocks(re, partition.base + CLUSTERMAPOFFSET(fsroot_->confLogSectors), fsroot_->confClusterSize)
			},
			.GUID = GUID,
			.altGUID = altGUID
		};
		DEBUGDO{
			BUFDEFPRINT(largeroot->clusterbuffer.clusterMap, fsroot_->confClusterSize, cc);
			Print(L"\n    Verifying FS Root Items #items: %llu", (UINTN)__safediv(fsroot_->confClusterSize, sizeof(fsblock)));
			DisableVerbose(re);
		}
		for(UINTN i = 0; i < __safediv(fsroot_->confClusterSize, sizeof(fsblock)); ++i){
			// Read and verify ROOTS
			fsblock *f = largeroot->clusterbuffer.clusterMap + i;
			if(f->fcode != 0){
				if(flagcheck(f->attr, __fsmetadatacluster) && f->fcode != 0){
					meta_fsblock *temp = readblocks(re, getloc(largeroot, f), sizeof(meta_fsblock));
					if(__memcmp(temp->fsig, FRATBLOCKSIG, 8)){
						DEBUGPRINT(
							L"\n[%s:%u]  >>  ERROR!    Corrupted FileSystem Root Block    ERASING ENTRY!!"
							L"\n    %llu:%u:%u", 
							(L"" __FILE__), __LINE__, f->fcode, f->attr, f->index
						);
						__memset(temp, 0, 512);
						f->fcode = 0;
						writeblocks(re, temp, getloc(largeroot, f), sizeof(meta_fsblock));
					}
					__free(temp);
				}
			}
		}
		dispose(re);
		return largeroot;
	}
	return NULL;
}

fsblock *allocatecluster(conf_fsroot *root){
	for(UINTN i = root->lastClusterAlloc; i < root->clusterbuffer.nClusterItems; ++i){
		if(root->clusterbuffer.clusterMap[i].fcode == 0){
			root->lastClusterAlloc = i;
			return root->clusterbuffer.clusterMap + i;
		}
	}
	root->lastClusterAlloc = 0;
	return NULL;
}

void __ffremove(conf_fsroot *root, char *path){
	fsblock *fb = __ffind(root, path);
	fb->fcode = 0;
}
void __ffremoveh(conf_fsroot *root, UINTN hash){
	hash &= 0x3FFFFFFFFFF;
	fsblock *fb = __ffindh(root, hash);
	fb->fcode = 0;
}

void __ffremovel(conf_fsroot *root, char *path){return __ffremovelh(root, __getfcode(path));}

void __ffremovelh(conf_fsroot *root, UINTN hash){
	hash &= 0x3FFFFFFFFFF;
	for(UINTN cc = 0; cc < root->clusterbuffer.nClusterItems; ++cc){
		if(root->clusterbuffer.clusterMap[cc].fcode == hash){root->clusterbuffer.clusterMap[cc].fcode = 0;}
	}
}

void __fcreate(conf_fsroot *root, char *path, char *flags){
	DEBUGPRINT(L"\nCreating File at: ./%a", path);
	if(__ffind(root, path)){if(strcheck(flags, 'F')){__ffremove(root, path);}else{return;}}
	fsblock *fb = allocatecluster(root);
	if(fb){
		if(strcheck(flags, 'P')){fb->attr |= __fsproxy;}else{
			if(strcheck(flags, 'd') && !strcheck(flags, 'f')){fb->attr |= __fsdirectory;}
			if(strcheck(flags, 'f') && !flagcheck(fb->attr, __fsdirectory)){fb->attr &= __fsfile;}
			if(strcheck(flags, 'r')){fb->attr |= __fsreadonly;}
		}
		fb->attr |= __fsmetadatacluster;
		fb->fcode = __getfcode(path);
		DEBUGPRINT(L"\n\tFcode: %llu", fb->fcode);
		fb->index = 0;
		dirhandle *dhandle = __fgetparent(root, path);
		if(dhandle && dhandle->file->fcode != fb->fcode){
			__fdiradd(dhandle, fb);
			fuloaddir(dhandle);
		}
		char *_path = __calloc(1, __strlen(path) + 2);
		*_path = '\\';
		__memcpy(_path + 1, path, __strlen(path) + 1);
		__finit(root, fb, _path);
	}
}

fsblock *__faddr(conf_fsroot *root, fsblock *family){
	DEBUGPRINT(L"\nAdding FS Table Entry: %llu, Type: %a", family->fcode, (flagcheck(family->attr, __fsdirectory)? "DIRECTORY": "FILE"));
	fsblock *fb = allocatecluster(root);
	if(fb){
		fb->index = 0;
		for(UINTN i = 0; i < root->clusterbuffer.nClusterItems; ++i){
			if(root->clusterbuffer.clusterMap[i].fcode == family->fcode){fb->index++;}
		}
		fb->attr = family->attr;
		fb->fcode = family->fcode;
		flagunset(fb->attr, __fsmetadatacluster);
		// Clear Block
		void *bl0 = __calloc(1, root->root->confBlockSize);
		__memset(bl0, 0, root->root->confBlockSize);
		LBA loc = getloc(root, fb);
		DEBUGPRINT(L"\n\tWriting 0-Block: %llu", loc);
		rawenv re = startup(root->GUID, root->altGUID, root->root->confBlockSize);
		writeblocks(re, bl0, loc, root->root->confBlockSize);
		__free(bl0);
		dispose(re);
	}
	return fb;
}

void __finit(conf_fsroot *root, fsblock *fb, char *path){
	DEBUGPRINT(L"\nInitialising File MetaData: ./%a", path);
	LBA loc = getloc(root, fb);
	meta_fsblock *metadata = (meta_fsblock *)__calloc(1, root->root->confBlockSize);
	EFI_TIME time;
	if(EFI_ERROR(gRT->GetTime(&time, NULL))){time = (EFI_TIME){0};}
	char *name;
	for(INT64 i = __strlen(path) - 1; i > -1; i--){
		if(i > GPTeNAMELEN){path[i] = '\0';}else{
			if(path[i] == '/' || path[i] == '\\'){
				name = path + __strlen(path) - i;  break;
			}else if(!isascii(path[i])){path[i] = 0;}
		}
	}
	*metadata = (meta_fsblock){
		.accessdate = time.Year,
		.writedate = 0,
		.accesstime = (time.Hour * 3600) + (time.Minute * 60) + time.Second,
		.writetime = 0,
		.attributes = fb->attr,
		.fcode = (flagcheck(fb->attr, __fsproxy)? 0: fb->fcode),
	};
	__memcpy(metadata->fsig, FRATBLOCKSIG, 8);
	flagunset(metadata->attributes, __fsmetadatacluster);
	__memset(metadata->name, 0, GPTeNAMELEN);
	__memcpy(metadata->name, name, __strlen(name));
	rawenv re = startup(root->GUID, root->altGUID, root->root->confBlockSize);
	writeblocks(re, metadata, loc, sizeof(meta_fsblock));
	dispose(re);
}

fsblock *__ffindh(conf_fsroot *root, UINTN hash){
	DEBUGPRINT(L"\nRoot is %p", root);
	hash &= 0x3FFFFFFFFFF;
	for(UINTN cc = 0; cc < root->clusterbuffer.nClusterItems; ++cc){
		if(root->clusterbuffer.clusterMap[cc].fcode == hash && root->clusterbuffer.clusterMap[cc].index == 0){
			return root->clusterbuffer.clusterMap + cc;
		}
	}
	return NULL;
}

fsblock *__ffindhi(conf_fsroot *root, UINTN hash, UINTN index){
	hash &= 0x3FFFFFFFFFF;
	for(UINTN cc = 0; cc < root->clusterbuffer.nClusterItems; ++cc){
		if(root->clusterbuffer.clusterMap[cc].fcode == hash && root->clusterbuffer.clusterMap[cc].index == index){
			return root->clusterbuffer.clusterMap + cc;
		}
	}
	return NULL;
}

fsblock *__ffindi(conf_fsroot *root, char *path, UINTN index){return __ffindhi(root, __getfcode(path), index);}

fsblock *__ffind(conf_fsroot *root, char *path){
	UINTN hash = __getfcode(path);
	return __ffindh(root, hash);
}

void *__fread1(conf_fsroot *root, fsblock *fb, UINTN index){
	LBA loc = 0;
	if(index != fb->index){
		fsblock *fb_ = __ffindhi(root, fb->fcode, index);
		if(fb_ == NULL){return NULL;}
		loc = getloc(root, fb_);
	}else{loc = getloc(root, fb);}
	DEBUGPRINT(L"\nReading File Block at %llu, Item: %llu.    Root: %llu", loc, index, fb->fcode);
	rawenv re = startup(root->GUID, root->altGUID, root->root->confBlockSize);
	void *out = readblocks(re, loc, root->root->confBlockSize);
	dispose(re);
	return out;
}

void __fpush1(conf_fsroot *root, fsblock *fb, UINTN i, void *buffer){
	LBA loc = 0;
	if(i != fb->index){
		fsblock *fb_ = __ffindhi(root, fb->fcode, i);
		if((fb_ = __ffindhi(root, fb->fcode, i)) == NULL){
			fb_ = __faddr(root, fb);
			if(fb_){loc = getloc(root, fb_);
			}else{return;}
		}else{loc = getloc(root, fb_);}
	}else{
		if(flagcheck(fb->attr, __fsmetadatacluster)){
			// Reject the Write
			DEBUGPRINT(L"\nError: Attempted Write to Protected Metadata Cluster");
			return;
		}else{loc = getloc(root, fb);}
		
	}
	DEBUGPRINT(L"\nWriting File Block at %llu, Item: %llu.\tRoot: %llu", loc, i, fb->fcode);
	rawenv re = startup(root->GUID, root->altGUID, root->root->confBlockSize);
	writeblocks(re, buffer, loc, root->root->confBlockSize);
	dispose(re);
}

dirhandle *floadhdir(conf_fsroot *root, char *path, char *args){
	DEBUGPRINT(L"\nMounting Dir [%a] with Args: \"%a\"", path, args);
	dirhandle *out = __calloc(1, sizeof(dirhandle));
	*out = (dirhandle){
		.root = root,
		.path = __strdup(path),
		.file = __ffind(root, path),
		.dirarray = NULL
	};
	if(!out->file && strcheck(args, 'fc')){
		__fcreate(root, path, args);
		out->file = __ffind(root, path);
		if(!out->file){return NULL;}
	}
	if(flagcheck(out->file->attr, __fsdirectory)){__fdirrefresh(out);
	}else{
		__free(out->path);
		__free(out);
		return NULL;
	}
	return out;
}

void fuloaddir(dirhandle *handle){
	DEBUGPRINT(L"\nUn-Mounting Directory [%a]", handle->path);
	// Flush root
	rawenv re = startup(handle->root->GUID, handle->root->altGUID, handle->root->root->confBlockSize);
	writeblocks(re, handle->root->root, handle->root->loc, sizeof(fsroot));
	// Flush cluster Array/Buffer
	writeblocks(re, handle->root->clusterbuffer.clusterMap, handle->root->loc + CLUSTERMAPOFFSET(handle->root->logblocks.nLogSectors), handle->root->clusterbuffer.nClusterSectors);	
	// Flush Entries
	UINTN entry = 0;
	fsblock *f = NULL;
	do{
		f = __ffindhi(handle->root, handle->file->fcode, entry + 1);
		if(f){writeblocks(
			re, ((void *)handle->dirarray) + (handle->root->root->confBlockSize * entry), 
			getloc(handle->root, f), 
			handle->root->root->confBlockSize);
		}
		entry++;
	}while(f);
	dispose(re);
	__free(handle->dirarray);
	__free(handle->path);
	__free(handle);
}

void __fdirrefresh(dirhandle *handle){
	DEBUGPRINT(L"\nRefreshing Dir %a", handle->path);
	rawenv re = startup(handle->root->GUID, handle->root->altGUID, handle->root->root->confBlockSize);
	UINTN blockprogress = 0;
	BOOLEAN exit_ = FALSE;
	do{
		// Find the associated Clusters/Blocks
		fsblock *fb = __ffindi(handle->root, handle->path, blockprogress + 1);
		if(fb){
			// Read off the Block 
			handle->dirarray = __realloc(handle->dirarray, handle->loadedblocks * getblocksize(re), getblocksize(re) * (blockprogress + 1));
			void *temp = readblocks(re, getloc(handle->root, fb), handle->root->root->confBlockSize);
			__memcpy(handle->dirarray + (getblocksize(re) * blockprogress), temp, getblocksize(re));
			__free(temp);
			for(UINTN cc = 0; cc < __safediv(getblocksize(re), sizeof(diritem)); ++cc){if(handle->dirarray[cc].local == 0){exit_ = TRUE;	break;}}
			blockprogress++;
		}else if(!fb){exit_ = TRUE;}
	}while(!exit_);
	handle->loadedblocks = blockprogress;
	dispose(re);
}

fhandle *floadh(conf_fsroot *root, char *path, char *args){
	DEBUGPRINT(L"\nMounting File [%a] with \"%a\"", path, args);
	// UINTN proxyhash = 0;
	if(strcheck(args, 'c')){__fcreate(root, path, args);}
	// if(strcheck(args, 'P')){
	// 	conf_fsblock *finfo = __freadinfo(root, __ffind(root, path));
	// 	if(!strcheck(args, 'R')){
	// 		if(finfo->fcode){return floadh(root, path, args);}
	// 	}
	// }
	fhandle *out = __calloc(1, sizeof(fhandle));
	*out = (fhandle){
		.file = __ffind(root, path),
		.root = root,
		.path = __strdup(path),
		.progress = root->root->confBlockSize	// Avoid Metadata Block
	};
	return out;
}

void fuloadh(fhandle *handle){
	DEBUGPRINT(L"\nUn-Mounting File [%llu]", handle->file->fcode);
	// Flush root
	rawenv re = startup(handle->root->GUID, handle->root->altGUID, handle->root->root->confBlockSize);
	writeblocks(re, handle->root->root, handle->root->loc, sizeof(fsroot));
	// Flush cluster Array/Buffer
	writeblocks(re, handle->root->clusterbuffer.clusterMap, handle->root->loc + CLUSTERMAPOFFSET(handle->root->logblocks.nLogSectors), handle->root->clusterbuffer.clusterSize);
	__free(handle);
	dispose(re);
}


meta_fsblock *__freadinfo(conf_fsroot *root, fsblock *fb){
	LBA loc = getloc(root, fb);
	rawenv re = startup(root->GUID, root->altGUID, root->root->confBlockSize);
	DEBUGPRINT(L"\nReading File Info");
	void *out = readblocks(re, loc, sizeof(meta_fsblock));
	dispose(re);
	return out;
}

void __fupdatetstamp(conf_fsroot *root, fsblock *file, BOOLEAN wt){
	rawenv re = startup(root->GUID, root->altGUID, root->root->confBlockSize);
	meta_fsblock *finfo = __freadinfo(root, file);
	LBA loc = getloc(root, file);
	EFI_TIME time;
	if(!EFI_ERROR(uefi_call_wrapper(gRT->GetTime, 2, &time, NULL))){
		if(wt){
			finfo->writetime = (time.Hour * 3600) + (time.Minute * 60) + time.Second;
			finfo->writedate = time.Day;
		}
		finfo->accesstime = (time.Hour * 3600) + (time.Minute * 60) + time.Second;
		finfo->accessdate = time.Day;
		DEBUGPRINT(L"\nWriting Time Stamp");
		writeblocks(re, finfo, loc, sizeof(meta_fsblock));
	}
	dispose(re);
}

UINTN __fsize(fhandle *fh){
	UINTN size = 0;
	for(UINTN cc = 0; cc < fh->root->clusterbuffer.nClusterItems; ++cc){
		size += (
			!flagcheck((fh->root->clusterbuffer.clusterMap + cc)->attr, __fsmetadatacluster) &&
			((fh->root->clusterbuffer.clusterMap + cc)->fcode == fh->file->fcode) 
			? fh->root->root->confBlockSize: 0
		);
	}
	return size;
}
UINTN __dsize(dirhandle *dh){
	UINTN size = 0;
	for(UINTN cc = 0; cc < __safediv((dh->loadedblocks * dh->root->root->confBlockSize), sizeof(diritem)); ++cc){
		size += (((dh->dirarray + cc)->local != 0) ? dh->root->root->confBlockSize: 0);
	}
	return size;
}

meta_fsblock *_dreadinfo(dirhandle *handle){return __freadinfo(handle->root, handle->file);}
meta_fsblock *_freadinfo(fhandle *handle){return __freadinfo(handle->root, handle->file);}

void _fseek(fhandle *handle, UINTN progress){handle->progress = progress;}
void _fseeko(fhandle *handle, INTN progress){handle->progress += progress;}

UINTN _fwrite(fhandle *handle, UINTN nbytes, const void *data){
    if(!handle || !data || nbytes == 0){return 0;}
    UINTN blkSize = handle->root->root->confBlockSize;
    UINT64 pos    = handle->progress;      // byte offset in file
    UINTN left    = nbytes;
    UINTN written = 0;
    __fupdatetstamp(handle->root, handle->file, TRUE);
    if(flagcheck((__ffindh(handle->root, handle->file->fcode))->attr, __fsreadonly)){return 0;}
    while(left > 0){
        UINT64 blkIndex   = __safediv(pos, blkSize);
        UINTN  blkOffset  = (UINTN)(pos % blkSize);
        UINTN  chunk      = blkSize - blkOffset;
        if(chunk > left){chunk = left;}
        void *blk = __fread1(handle->root, handle->file, blkIndex);
        if(!blk){
            blk = __calloc(1, blkSize);
            if(!blk){break;}
        }
        __safecopy(blk + blkOffset, data + written, chunk);
        __fpush1(handle->root, handle->file, blkIndex, blk);
        __free(blk);
        written      += chunk;
        pos          += chunk;
        left         -= chunk;
    }
    handle->progress = pos;
    return (nbytes - left);
}
UINTN _fread(fhandle *h, UINTN nbytes, void **dataout){
	if((h->progress + nbytes) > __fsize(h)){return 0;}
    UINT32 bsize = h->root->root->confBlockSize;
    UINTN progress = h->progress;

    UINTN remaining = nbytes;
    UINTN written = 0;

	__fupdatetstamp(h->root, h->file, false);

    void *out = __calloc(1, nbytes);
    if(!out){return 0;}
    while(remaining > 0){
        UINTN block_index = __safediv(progress, bsize);
        UINTN offset      = progress % bsize;
        UINTN chunk = bsize - offset;

        void *blk = __fread1(h->root, h->file, block_index);
        if(!blk){break;}
        if(chunk > remaining){chunk = remaining;}

        __memcpy(out + written, blk + offset, chunk);
        __free(blk);
        written  += chunk;
        progress += chunk;
        remaining -= chunk;
    }
    h->progress = progress;
    *dataout = out;
    return (nbytes - remaining);
}

dirhandle *__fgetparent(conf_fsroot *root, char *path){
	UINT32 cc = __strlen(path);
	char *dup = __strdup(path);
	while(dup[cc] == PATHnoSEP || dup[cc] == PATHSEP){cc--;}
	for(; cc > 0; --cc){if(dup[cc] == PATHSEP || dup[cc] == PATHnoSEP){dup[cc] = '\0';	break;}}
	if(cc == 0){return NULL;}
	dirhandle *out = floadhdir(root, dup, "dc");
	__free(dup);
	return out;
}

void __fdiradd(dirhandle *dir, fsblock *fb){
	DEBUGPRINT(L"\nAdding Entry:%llu to Dir [%a:%llu]", fb->fcode, dir->path, dir->file->fcode);
	do{
		__fdirrefresh(dir);
		UINT32 cc = 0;
		if(dir->dirarray){
			do{
				if(dir->dirarray[cc].fcode == 0){
					dir->dirarray[cc].attributes = fb->attr;
					dir->dirarray[cc].fcode = fb->fcode;
					dir->dirarray[cc].local = 0;
					return;
				}else if(dir->dirarray[cc].fcode == fb->fcode){
					dir->dirarray[cc].attributes = fb->attr;
					return;
				}else{dir->dirarray[cc].local++;}
				cc++;
			}while(dir->dirarray[cc == 0? cc: (cc - 1)].local != 0);
		}
		__faddr(dir->root, dir->file);
	}while(TRUE);
}

BOOLEAN __ftest(fhandle *h){
	DEBUGPRINT(L"\nPerforming File Test. Code: %llu", h->file->fcode);
	BOOLEAN out = TRUE;
	void *_block = __calloc(1, h->root->root->confBlockSize);
	trng__(_block, h->root->root->confBlockSize);
	_fseek(h, 1);
	_fwrite(h, h->root->root->confBlockSize, _block);
	_fseeko(h, -1);
	void *__block = NULL;
	_fread(h, h->root->root->confBlockSize, &__block);
	if(__block){
		Print(L"\n");
		for(UINTN cc = 0; cc < h->root->root->confBlockSize; ++cc){
			if(((uint8_t *)_block)[cc] != ((uint8_t *)__block)[cc]){
				out = FALSE;
				DEBUGPRINT(L"✖ %u:%u  ", ((uint8_t *)_block)[cc], ((uint8_t *)__block)[cc]);
			}else{DEBUGPRINT(L"✔ %u:%u  ", ((uint8_t *)_block)[cc], ((uint8_t *)__block)[cc]);}
		}
	   __free(__block);
	}
	__free(_block);
	return out;
}

unhandle *__fdirlist(dirhandle *dir, UINTN *index){
	__fdirrefresh(dir);
	if(__safediv(((*index) * sizeof(diritem)), dir->root->root->confBlockSize) < dir->loadedblocks){
		diritem *ditem = dir->dirarray + (*index);
		(*index)++;
		fsblock *fb = __ffindh(dir->root, ditem->fcode);
		if(fb){
			if(fb->fcode == ditem->fcode && (fb->fcode != 0)){
				meta_fsblock *finfo = __freadinfo(dir->root, fb);
				char *temp = __strdup(dir->path);
				temp = __realloc(temp, __strlen(dir->path), __strlen(temp) + __strlen(finfo->name) + 2);
				__memcpy(temp + __strlen(dir->path) + (*finfo->name != '/'), finfo->name, __strlen(finfo->name) + 1);
				temp[__strlen(dir->path)] = PATHSEP;
				__free(finfo);
				unhandle *out = __calloc(1, sizeof(unhandle));
				if((out->dir = flagcheck(fb->attr, __fsdirectory))){
					out->dhandle_ = floadhdir(dir->root, temp, "");
				}else{out->fhandle_ = floadh(dir->root, temp, "f");}
				__free(temp);
				return out;
			}else{
				// Remove Entry
				ditem->fcode = 0;
				return NULL;
			}
		}else{}// Repair The Entry
	}
	return NULL;
}

dirrunner *__dirr_init(dirhandle *handle, char *patternmatcher){
	if(!handle){return NULL;}
	dirrunner *dr = __calloc(1, sizeof(dirrunner));
	if(!dr){return NULL;}
	dr->dir = handle;
	dr->pattmatcher = patternmatcher;
	dr->stack_size = 8;
	dr->stack_depth = 1;
	dr->dir_stack = __calloc(dr->stack_size, sizeof(dirhandle*));
	dr->idx_stack = __calloc(dr->stack_size, sizeof(UINTN));
	if(!dr->dir_stack || !dr->idx_stack){
		__free(dr->dir_stack);
		__free(dr->idx_stack);
		__free(dr);
		return NULL;
	}
	dr->dir_stack[0] = handle;
	dr->idx_stack[0] = 0;
	return dr;
}

static BOOLEAN __dirr_matches(dirrunner *dr, unhandle *item){
	meta_fsblock *info = item->dir ? _dreadinfo(item->dhandle_) : _freadinfo(item->fhandle_);
	if(!info){return FALSE;}
	BOOLEAN match = __pattmatch(dr->pattmatcher, info->name);
	__free(info);
	return match;
}

static BOOLEAN __dirr_push(dirrunner *dr, dirhandle *handle){
	if(dr->stack_depth == dr->stack_size){
		UINTN new_size = dr->stack_size ? dr->stack_size * 2 : 8;
		dr->dir_stack = __realloc(dr->dir_stack, dr->stack_size * sizeof(dirhandle *), new_size * sizeof(dirhandle*));
		dr->idx_stack = __realloc(dr->idx_stack, dr->stack_size * sizeof(dirhandle *), new_size * sizeof(UINTN));
		if(!dr->dir_stack || !dr->idx_stack) return FALSE;
		dr->stack_size = new_size;
	}
	dr->dir_stack[dr->stack_depth] = handle;
	dr->idx_stack[dr->stack_depth] = 0;
	dr->stack_depth++;
	return TRUE;
}

static dirhandle *__dirr_pop(dirrunner *dr){
	if(dr->stack_depth == 0) return NULL;
	dr->stack_depth--;
	dirhandle *out = dr->dir_stack[dr->stack_depth];
	dr->dir_stack[dr->stack_depth] = NULL;
	return out;
}

void __dirr_free(dirrunner *dr){
	if(!dr){return;}
	for(UINTN cc = 0; cc < dr->stack_depth; ++cc){
		if(dr->dir_stack[cc]){if(cc > 0){fuloaddir(dr->dir_stack[cc]);}}
	}
	__free(dr->dir_stack);
	__free(dr->idx_stack);
	__free(dr);
}

unhandle *__dirr(dirrunner *dr){
	if(!dr || dr->stack_depth == 0){return NULL;}
	while(dr->stack_depth > 0){
		UINTN top = dr->stack_depth - 1;
		dirhandle *current = dr->dir_stack[top];
		UINTN idx = dr->idx_stack[top];
		unhandle *item = __fdirlist(current, &idx);
		dr->idx_stack[top] = idx;

		if(!item){
			if(top > 0){fuloaddir(current);}
			__dirr_pop(dr);
			continue;
		}
		if(!item->dhandle_){
			if(top > 0){fuloaddir(current);}
			__dirr_pop(dr);
			continue;
		}
		if(!item->dhandle_->file){
			if(top > 0){fuloaddir(current);}
			__dirr_pop(dr);
			continue;
		}

		BOOLEAN matches = __dirr_matches(dr, item);
		if(item->dir){
			if(!__dirr_push(dr, item->dhandle_)){
				__free(item);
				return NULL;
			}
		}
		if(matches){return item;}
		__free(item);
	}

	return NULL;
}

void __fprint_info(meta_fsblock *finfo){
	Print(
		L"\nFile Info:"
		L"\nSignature: %.8s"
		L"\nVersion: %llu"
		L"\nfCode: %llu"
		L"\nAttributes:"
		L"\n\tIs Directory: [%a]"
		L"\n\tIs File: [%a]"
		L"\n\tIs Readonly: [%a]"
		L"\nName: %.32s"
		L"\nAccess Time: %u:%u:%u"
		L"\nWrite Time: %u:%u:%u"
		L"\nAccess Date: %u"
		L"\nWrite Date: %u",
		finfo->fsig, finfo->headerversion, (UINTN)finfo->fcode,
		(flagcheck(finfo->attributes, __fsdirectory) ? "TRUE": "FALSE"), (flagcheck(finfo->attributes, __fsfile) ? "TRUE": "FALSE"), 
		(flagcheck(finfo->attributes, __fsreadonly) ? "TRUE": "FALSE"), 
		finfo->name, 
		(UINT32)(__safediv(finfo->accesstime, 3600)), 
		(UINT32)(__safediv((finfo->accesstime % 3600), 60)),
		(UINT32)(finfo->accesstime % 60), 
		(UINT32)(__safediv(finfo->writetime, 3600)), 
		(UINT32)(__safediv((finfo->writetime % 3600), 60)),
		(UINT32)(finfo->writetime % 60), 
		finfo->accessdate, finfo->writedate
	);
}

LBA getloc(conf_fsroot *root, fsblock *fb){
	return DATAFIRST(root) + __safediv(((UINTN)fb - (UINTN)root->clusterbuffer.clusterMap), sizeof(fsblock));
}