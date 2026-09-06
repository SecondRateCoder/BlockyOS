#include "frat.h"
#include "kernel/libcrt/hardware/RTC.h"

uint64_t *getfcode(char *path){
	static uint64_t hash[2];
	blake2b_state state;
	blake2b_init(&state, sizeof(hash));
	blake2b_update(&state, path, strlen(path));
	blake2b_final(&state, hash, sizeof(hash));
	hash[1] &= FCODEHASHMASK;
	return hash;
}

bool checkdisk(rawenv re, GUID _GUID, GUID altGUID){
	// DEBUGPRINT(L"\nVerifying Disk GPT");
	bool out = false;
	uint8_t *block = ReadRawHandleBlocks(re, GPTLBA, ReBlocks(re, sizeof(miniGPT)));
	miniGPT *gpt = (miniGPT *)block;
	// DEBUGDO{
	// 	DEBUGPRINT(
	// 		L"\nGPT Dump:"
	// 		L"\n    Sig: \"%a\""
	// 		L"\n    Rev: %u"
	// 		L"\n    Header-Size: %u"
	// 		L"\n    Header Checksum: %u"
	// 		L"\n    localLBA: %llu"
	// 		L"\n    altLBA: %llu"
	// 		L"\n    firstUsable: %llu"
	// 		L"\n    lastUsable: %llu"
	// 		L"\n    Disk-_GUID: ",
	// 		gpt->sig, gpt->rev, gpt->hSize, gpt->hChecksum,
	// 		gpt->localLBA, gpt->alternateLBA, gpt->fUsable, gpt->lUsable
	// 	);
	// 	prGUID(gpt->dGUID);
	// 	DEBUGPRINT(
	// 		L"\n    Partition-Table: %llu"
	// 		L"\n    # of Partition-Entries: %u"
	// 		L"\n    Partition-Entry Size: %u"
	// 		L"\n    Partition-Table Checksum: %u",
	// 		gpt->partEntryLoc, gpt->nPartEntries, gpt->partEntrySize,
	// 		gpt->partArrayChecksum
	// 	);
	// }
	if(!memcmp(gpt->sig, "EFI PART", 8)){out = true;}else{out = false;}
	mfree(block);
	// DEBUGPRINT(L"\nGPT Exists? %a", ((out == true)? "true": "false"));
	return out;
}

partdim loadpart(rawenv re, GUID _GUID, GUID altGUID, GPTNameStr name){
	char gptname[GPTNameLength + 1];	ZeroMem(gptname, GPTNameLength + 1);
	for(uint8_t cc = 0x00; cc < GPTNameLength; ++cc){gptname[cc] = (name[cc] & 0xFF);}
	// DEBUGPRINT(L"\n[%s:%u]  >>  Loading Partition:    %a", (L"" __FILE__), __LINE__, gptname);
	if(checkdisk(re, _GUID, altGUID)){
		miniGPT *gpt = (miniGPT *)ReadRawHandleBlocks(re, GPTLBA, ReBlocks(re, sizeof(miniGPT)));
		GPTentry *ge = (GPTentry *)ReadRawHandleBlocks(re, gpt->partEntryLoc, ReBlocks(re, gpt->nPartEntries * sizeof(GPTentry)));
		
		for(uint32_t i = 0x00; i < gpt->nPartEntries; i++){
			if(!memcmp(name, ge[i].name, GPTNameSize)){
				partdim out = {.base = ge[i].sLBA, .high = ge[i].eLBA};
				mfree(gpt);
				mfree(ge);
				// DEBUGPRINT(L"\n[%s:%u]  >>  Found Partition: %llu:%llu", (L"" __FILE__), __LINE__, out.base, out.high);
				return out;
			}
		}
		mfree(gpt);
		mfree(ge);
	}
	// DEBUGPRINT(L"\n[%s:%u]  >>  Found No Partition named: %a", (L"" __FILE__), __LINE__, gptname);
	return (partdim){0x00, 0x00};
}

void formatpart(
	rawenv re, GUID _GUID, GUID altGUID, 
	GPTNameStr name, 
	uint32_t confBlockSize, uint32_t confLogSectors, 
	uint32_t verMAJOR, uint32_t verMINOR
){
	// DEBUGDO{
	// 	char gptname[GPTNameLength + 1];	ZeroMem(gptname, GPTNameLength + 1);
	// 	for(uint8_t cc = 0x00; cc < GPTNameLength; ++cc){gptname[cc] = (name[cc] & 0xFF);}
	// 	DEBUGPRINT(L"\n[%s:%u]  >>  Formatting Partition:    %a", (L"" __FILE__), __LINE__, gptname);
	// }
	partdim part = loadpart(re, _GUID, altGUID, name);
	if(!part.high){return;}
	re->CBlockSize = confBlockSize;

	// Root-Block
	void *block = mcalloc(confLogSectors, confBlockSize);
	// ZeroMem(block, confBlockSize * confLogSectors);
	fsroot *fr = block;
	*fr = (fsroot){
		.confLogSectors = confLogSectors,
		.confBlockSize = confBlockSize,
		.confClusterSize = CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize),
		.verCode = MAKEVERSION(verMAJOR, verMINOR),
	};
	// Write FSROOT
	memcpy(fr->signature, FRATSIG, sizeof(FRATSIG));
	WriteRawHandleBlocks(re, part.base + FRATROOTOFFSET, ReBlocks(re, sizeof(fsroot)), block);

	// Log-Block
	memset(block, 0x00, confBlockSize * confLogSectors);
	WriteRawHandleBlocks(re, part.base + LOGBLOCKOFFSET, ReBlocks(re, confBlockSize * confLogSectors), block);

	// Cluster-Map
	mfree(block);
	block = mcalloc(CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize), confBlockSize);
	// memset(block, 0x00, CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize) * confBlockSize);
	WriteRawHandleBlocks(re, part.base + CLUSTERMAPOFFSET(confLogSectors), 
		ReBlocks(re, CLUSTERMAPSECTORS_CALC(part.base, part.high, confLogSectors, confBlockSize) * confBlockSize), block);
	mfree(block);
	
	// DEBUGPRINT(L"\n[%s:%u]  >>  Formatted Partition", (L"" __FILE__), __LINE__);
}

partdim queryparttablefs(miniGPT *gpt, rawenv re){
	// DEBUGPRINT(L"\n[%s:%u]  >>  Querying Part Table", (L"" __FILE__), __LINE__);
	// Query all Partitions for the FileSystem.
	GPTentry *ge = (GPTentry *)ReadRawHandleBlocks(re, gpt->partEntryLoc, ReBlocks(re, gpt->nPartEntries * sizeof(GPTentry)));
	for(uint32_t i = 0x00; i < gpt->nPartEntries; i++){
		char gptname[GPTNameLength + 1];	ZeroMem(gptname, GPTNameLength + 1);
		for(uint8_t cc = 0x00; cc < GPTNameLength; ++cc){gptname[cc] = (ge[i].name[cc] & 0xFF);}
		// DEBUGPRINT(L"\n[%s:%u]  >>  GPT-Entry: %a{%llu:%llu -> %llu}", (L"" __FILE__), __LINE__, gptname, ge[i].sLBA, ge[i].eLBA, ge[i].eLBA - ge[i].sLBA);
		if(queryfs(re, ge[i].sLBA)){
			partdim out = {.base = ge[i].sLBA, .high = ge[i].eLBA};
			mfree(ge);
			return out;
		}
	}
	mfree(ge);
	return (partdim){0x00, 0x00};
}

bool queryfs(rawenv re, LBA partbase){
	// DEBUGPRINT(L"\n[%s:%u]  >>  Querying FS at %llu", (L"" __FILE__), __LINE__, partbase);
	// Check that a FileSystem exists at the bytebase.
	bool out = 0x00;
	fsroot *fr = (fsroot *)ReadRawHandleBlocks(re, partbase + FRATROOTOFFSET, ReBlocks(re, sizeof(fsroot)));
	// DEBUGPRINT(
	// 	L"\nFS-Root Blob:"
	// 	L"\n    Version: [%u:%u]"
	// 	L"\n    Sig: %.16a"
	// 	L"\n    Configured Log Sectors: %u"
	// 	L"\n    Configured Block Size: %u"
	// 	L"\n    Configured Cluster-Map Size: %u",
	// 	fr->verCode[0x00], fr->verCode[1], 
	// 	fr->signature, fr->confLogSectors, 
	// 	fr->confBlockSize, fr->confClusterSize
	// );
	if(!memcmp(fr->signature, FRATSIG, sizeof(FRATSIG))){
		// DEBUGPRINT(
		// 	L"\n[%s:%u]  >>  FS Found at [%llu]:"
		// 	L"\n	Version-Code: [%u:%u]"
		// 	L"\n	Configured Block-Size: %u"
		// 	L"\n	Configured Cluster-Size: %u"
		// 	L"\n	Configured # of Log-Sectors: %u"
		// 	L"\n	Configured Log-Size: %u", 
		// 	(L"" __FILE__), __LINE__, partbase, fr->verCode[0x00], fr->verCode[1], 
		// 	fr->confBlockSize, fr->confClusterSize, fr->confLogSectors, fr->confLogSectors * sizeof(fslogitem)
		// );
		out = true;
	}else{out = false;}
	mfree(fr);
	return out;
}

conf_fsroot *fmount(rawenv re, GUID _GUID, GUID altGUID){
	// DEBUGDO{DEBUGPRINT(L"\n[%s:%u]  >>  Mounting FS Root: [", (L"" __FILE__), __LINE__);	prGUID(_GUID);	Print(L"]    [");	prGUID(altGUID);	Print(L"]");}
	partdim partition;
	if(checkdisk(re, _GUID, altGUID)){
		// Get the LBA Info for a Partition
		void *block = ReadRawHandleBlocks(re, GPTLBA, ReBlocks(re, sizeof(miniGPT)));
		miniGPT *gpt = (miniGPT *)block;
		if((partition = queryparttablefs(gpt, re)).high){mfree(gpt);}else{mfree(gpt);    return NULL;}
		// DEBUGPRINT(
		// 	L"\n[%s:%u]  >>  Found Formatted Partition: %llu:%llu -> %llu", 
		// 	(L"" __FILE__),__LINE__, partition.base, partition.high, partition.high - partition.base
		// );
		fsroot *fsroot_ = (fsroot *)ReadRawHandleBlocks(re, partition.base + FRATROOTOFFSET, ReBlocks(re, sizeof(fsroot)));
		// DEBUGPRINT(
		// 	L"\n[%s:%u]  >>  Root: %llu:%llu -> %llu LBAs"
		// 	L"\n	Version-Code: [%u:%u]"
		// 	L"\n	Configured Block-Size: %u"
		// 	L"\n	Configured Cluster-Size: %u"
		// 	L"\n	Configured # of Log-Sectors: %u"
		// 	L"\n	Configured Log-Size: %u", 
		// 	(L"" __FILE__),__LINE__, partition.base, partition.high, partition.high - partition.base, 
		// 	fsroot_->verCode[0x00], fsroot_->verCode[1], 
		// 	fsroot_->confBlockSize, CLUSTERMAPSECTORS_CALC(partition.base, partition.high, fsroot_->confLogSectors, fsroot_->confBlockSize), 
		// 	fsroot_->confClusterSize, fsroot_->confLogSectors, fsroot_->confLogSectors * sizeof(fslogitem)
		// );
		re->CBlockSize = fsroot_->confBlockSize;
		conf_fsroot *largeroot = mcalloc(1, sizeof(conf_fsroot));
		*largeroot = (conf_fsroot){
			ReSetGUID(._GUID, _GUID), ReSetGUID(.altGUID, altGUID), 
			.loc = 0x00, .root = fsroot_, .lastClusterAlloc = 0x00,
			.logblocks = {
				.logBlock = ReadRawHandleBlocks(re, partition.base + LOGBLOCKOFFSET, ReBlocks(re, fsroot_->confBlockSize * fsroot_->confLogSectors)),
				.nLogSectors = fsroot_->confLogSectors
			}, .clusterbuffer = {
				.nClusterItems = __safediv(fsroot_->confClusterSize, sizeof(fsblock)),
				.nClusterSectors = __safediv(fsroot_->confClusterSize, fsroot_->confBlockSize), .clusterSize = fsroot_->confClusterSize,
				.clusterMap = ReadRawHandleBlocks(re, partition.base + CLUSTERMAPOFFSET(fsroot_->confLogSectors), ReBlocks(re, fsroot_->confClusterSize))
			}, 
		};
		// DEBUGDO{
		// 	BUFDEFPRINT(largeroot->clusterbuffer.clusterMap, fsroot_->confClusterSize, cc);
		// 	Print(L"\n    Verifying FS Root Items #items: %llu", (uint64_t)__safediv(fsroot_->confClusterSize, sizeof(fsblock)));
		// 	DisableVerbose(re);
		// }
		for(uint64_t i = 0x00; i < __safediv(fsroot_->confClusterSize, sizeof(fsblock)); ++i){
			// Read and verify ROOTS
			fsblock *f = largeroot->clusterbuffer.clusterMap + i;
			if(f->fcodelow && f->fcodehigh){
				if(flagcheck(f->attributes, __fsmetadatacluster) && f->fcodelow && f->fcodehigh){
					meta_fsblock *temp = ReadRawHandleBlocks(re, getloc(largeroot, f), ReBlocks(re, sizeof(meta_fsblock)));
					if(memcmp(temp->fsig, FRATBLOCKSIG, 8)){
						// DEBUGPRINT(
						// 	L"\n[%s:%u]  >>  ERROR!    Corrupted FileSystem Root Block    ERASING ENTRY!!"
						// 	L"\n    %llu:%u:%u", 
						// 	(L"" __FILE__), __LINE__, f->fcode, f->attributes, f->index
						// );
						memset(temp, 0x00, 512);
						f->fcodelow = 0x00;	f->fcodehigh = 0x00;
						WriteRawHandleBlocks(re, getloc(largeroot, f), ReBlocks(re, sizeof(meta_fsblock)), temp);
					}
					mfree(temp);
				}
			}
		}
		
		return largeroot;
	}
	return NULL;
}

fsblock *allocatecluster(conf_fsroot *root){
	for(uint64_t i = root->lastClusterAlloc; i < root->clusterbuffer.nClusterItems; ++i){
		if(!(root->clusterbuffer.clusterMap[i].fcodelow) && !(root->clusterbuffer.clusterMap[i].fcodehigh)){
			root->lastClusterAlloc = i;
			return root->clusterbuffer.clusterMap + i;
		}
	}
	root->lastClusterAlloc = 0x00;
	return NULL;
}

void __ffremove(conf_fsroot *root, char *path){
	fsblock *fb = __ffind(root, path);
	fb->fcodehigh = 0x00;
	fb->fcodelow = 0x00;
}
void __ffremoveh(conf_fsroot *root, uint64_t hash[2]){
	hash[1] &= FCODEHASHMASK;
	fsblock *fb = __ffindh(root, hash);
	fb->fcodehigh = 0x00;
	fb->fcodelow = 0x00;
}

void __ffremovel(conf_fsroot *root, char *path){return __ffremovelh(root, getfcode(path));}

void __ffremovelh(conf_fsroot *root, uint64_t hash[2]){
	hash[1] &= FCODEHASHMASK;
	for(uint64_t cc = 0x00; cc < root->clusterbuffer.nClusterItems; ++cc){
		if(root->clusterbuffer.clusterMap[cc].fcodelow == hash[0] && 
			root->clusterbuffer.clusterMap[cc].fcodehigh == hash[1]){
				root->clusterbuffer.clusterMap[cc].fcodehigh = 0x00;
				root->clusterbuffer.clusterMap[cc].fcodelow = 0x00;
			}
	}
}

void __fcreate(rawenv re, conf_fsroot *root, char *path, char *flags){
	// DEBUGPRINT(L"\nCreating File at: ./%a", path);
	if(__ffind(root, path)){if(strcheck(flags, 'F')){__ffremove(root, path);}else{return;}}
	fsblock *fb = allocatecluster(root);
	if(fb){
		if(strcheck(flags, 'P')){fb->attributes |= __fsproxy;}else{
			if(strcheck(flags, 'd') && !strcheck(flags, 'f')){fb->attributes |= __fsdirectory;}
			if(strcheck(flags, 'f') && !flagcheck(fb->attributes, __fsdirectory)){fb->attributes &= __fsfile;}
			if(strcheck(flags, 'r')){fb->attributes |= __fsreadonly;}
		}
		fb->attributes |= __fsmetadatacluster;
		uint64_t *hash = getfcode(path);
		uint64_t temp[2] = {hash[0], hash[1]};
		// DEBUGPRINT(L"\n\tFcode: %llu", fb->fcode);
		fb->fcodelow = temp[0];
		fb->fcodehigh = temp[1] & FCODEHASHMASK;
		fb->index = 0x00;
		dirhandle *dhandle = __fgetparent(re, root, path);
		if(dhandle && (dhandle->file->fcodelow != fb->fcodelow && dhandle->file->fcodehigh != fb->fcodehigh)){
			__fdiradd(re, dhandle, fb);
			fuloaddir(re, dhandle);
		}
		char *_path = mcalloc(1, strlen(path) + 2);
		*_path = '\\';
		memcpy(_path + 1, path, strlen(path) + 1);
		__finit(re, root, fb, _path);
	}
}

fsblock *__faddr(rawenv re, conf_fsroot *root, fsblock *family){
	// DEBUGPRINT(L"\nAdding FS Table Entry: %llu, Type: %a", family->fcode, (flagcheck(family->attributes, __fsdirectory)? "DIRECTORY": "FILE"));
	fsblock *fb = allocatecluster(root);
	if(fb){
		fb->index = 0x00;
		for(uint64_t i = 0x00; i < root->clusterbuffer.nClusterItems; ++i){
			if(root->clusterbuffer.clusterMap[i].fcodelow == family->fcodelow && 
				root->clusterbuffer.clusterMap[i].fcodehigh == family->fcodehigh){fb->index++;}
		}
		fb->attributes = family->attributes;
		fb->fcodehigh = family->fcodehigh;
		fb->fcodelow = family->fcodelow;
		flagunset(fb->attributes, __fsmetadatacluster);
		// Clear Block
		void *bl0 = mcalloc(1, root->root->confBlockSize);
		memset(bl0, 0x00, root->root->confBlockSize);
		LBA loc = getloc(root, fb);
		// DEBUGPRINT(L"\n\tWriting 0x00-Block: %llu", loc);
		re->CBlockSize = root->root->confBlockSize;
		WriteRawHandleBlocks(re, loc, ReBlocks(re, root->root->confBlockSize), bl0);
		mfree(bl0);
	}
	return fb;
}

void __finit(rawenv re, conf_fsroot *root, fsblock *fb, char *path){
	// DEBUGPRINT(L"\nInitialising File MetaData: ./%a", path);
	LBA loc = getloc(root, fb);
	meta_fsblock *metadata = (meta_fsblock *)mcalloc(1, root->root->confBlockSize);
	char *name = NULL;
	for(int64_t i = strlen(path) - 1; i > -1; i--){
		if(i > GPTNameLength){path[i] = '\0x00';}else{
			if(path[i] == '/' || path[i] == '\\'){
				name = path + strlen(path) - i;  break;
			}else if(!isascii(path[i])){path[i] = 0x00;}
		}
	}
	CMOS_time_t time;
	CMOSGetTime(&time);
	*metadata = (meta_fsblock){
		//	We get the Low PIT Ticks
		.accessdate = CMOSGetElapsedDays(time.year + 2000, time.month, time.day),
		.writedate = 0x00,
		.accesstime = (time.hour * 3600) + (time.minute * 60) + time.second,
		.writetime = 0x00,
		.f = {
			.attributes = fb->attributes,
			.fcodelow = flagcheck(fb->attributes, __fsproxy) ? 0x00 : fb->fcodelow,
			.fcodehigh = flagcheck(fb->attributes, __fsproxy) ? 0x00 : fb->fcodehigh
		}
	};
	memcpy(metadata->fsig, FRATBLOCKSIG, 8);
	flagunset(metadata->f.attributes, __fsmetadatacluster);
	memset(metadata->name, 0x00, GPTNameLength);
	memcpy(metadata->name, name, strlen(name));
	re->CBlockSize = root->root->confBlockSize;
	WriteRawHandleBlocks(re, loc, ReBlocks(re, sizeof(meta_fsblock)), metadata);
	
}

fsblock *__ffindh(conf_fsroot *root, uint64_t hash[2]){
	// DEBUGPRINT(L"\nRoot is %p", root);
	hash[1] &= FCODEHASHMASK;
	return __ffindhi(root, hash, 0x00);
}

fsblock *__ffindhi(conf_fsroot *root, uint64_t hash[2], uint64_t index){
	hash[1] &= FCODEHASHMASK;
	for(uint64_t cc = 0x00; cc < root->clusterbuffer.nClusterItems; ++cc){
		if(root->clusterbuffer.clusterMap[cc].fcodelow == hash[0] && 
			root->clusterbuffer.clusterMap[cc].fcodehigh == hash[1] && 
			root->clusterbuffer.clusterMap[cc].index == index){
			return root->clusterbuffer.clusterMap + cc;
		}
	}
	return NULL;
}

fsblock *__ffindi(conf_fsroot *root, char *path, uint64_t index){return __ffindhi(root, getfcode(path), index);}

fsblock *__ffind(conf_fsroot *root, char *path){
	uint64_t hash = getfcode(path);
	return __ffindh(root, hash);
}

void *__fread1(rawenv re, conf_fsroot *root, fsblock *fb, uint64_t index){
	LBA loc = 0x00;
	if(index != fb->index){
		fsblock *fb_ = __ffindhi(root, (uint64_t[2]){fb->fcodelow, fb->fcodehigh}, index);
		if(fb_ == NULL){return NULL;}
		loc = getloc(root, fb_);
	}else{loc = getloc(root, fb);}
	// DEBUGPRINT(L"\nReading File Block at %llu, Item: %llu.    Root: %llu", loc, index, fb->fcode);
	re->CBlockSize = root->root->confBlockSize;
	void *out = ReadRawHandleBlocks(re, loc, ReBlocks(re, root->root->confBlockSize));
	
	return out;
}

void __fpush1(rawenv re, conf_fsroot *root, fsblock *fb, uint64_t i, void *buffer){
	LBA loc = 0x00;
	if(i != fb->index){
		fsblock *fb_ = __ffindhi(root, (uint64_t[2]){fb->fcodelow, fb->fcodehigh}, i);
		if((fb_ = __ffindhi(root, (uint64_t[2]){fb->fcodelow, fb->fcodehigh}, i)) == NULL){
			fb_ = __faddr(re, root, fb);
			if(fb_){loc = getloc(root, fb_);
			}else{return;}
		}else{loc = getloc(root, fb_);}
	}else{if(flagcheck(fb->attributes, __fsmetadatacluster)){return;}else{loc = getloc(root, fb);}}
	// DEBUGPRINT(L"\nWriting File Block at %llu, Item: %llu.\tRoot: %llu", loc, i, fb->fcode);
	re->CBlockSize = root->root->confBlockSize;
	WriteRawHandleBlocks(re, loc, ReBlocks(re, root->root->confBlockSize), buffer);
}

dirhandle *floadhdir(rawenv re, conf_fsroot *root, char *path, char *args){
	// DEBUGPRINT(L"\nMounting Dir [%a] with Args: \"%a\"", path, args);
	dirhandle *out = mcalloc(1, sizeof(dirhandle));
	*out = (dirhandle){
		.root = root,
		.path = strdup(path),
		.file = __ffind(root, path),
		.dirarray = NULL
	};
	if(!out->file && strcheck(args, 'fc')){
		__fcreate(re, root, path, args);
		out->file = __ffind(root, path);
		if(!out->file){return NULL;}
	}
	if(flagcheck(out->file->attributes, __fsdirectory)){__fdirrefresh(re, out);
	}else{mfree(out->path);		mfree(out);		return NULL;}
	return out;
}

void fuloaddir(rawenv re, dirhandle *handle){
	// DEBUGPRINT(L"\nUn-Mounting Directory [%a]", handle->path);
	// Flush root
	WriteRawHandleBlocks(re, handle->root->loc, ReBlocks(re, sizeof(fsroot)), handle->root->root);
	// Flush cluster Array/Buffer
	WriteRawHandleBlocks(re, handle->root->loc + CLUSTERMAPOFFSET(handle->root->logblocks.nLogSectors), 
		handle->root->clusterbuffer.nClusterSectors, handle->root->clusterbuffer.clusterMap);
	// Flush Entries
	uint64_t entry = 0x00;
	fsblock *f = NULL;
	do{
		f = __ffindhi(handle->root, (uint64_t[2]){handle->file->fcodelow, handle->file->fcodehigh}, entry + 1);
		if(f){WriteRawHandleBlocks(re, getloc(handle->root, f), handle->root->root->confBlockSize, (void *)handle->dirarray + (handle->root->root->confBlockSize * entry));}
		entry++;
	}while(f);
	
	mfree(handle->dirarray);
	mfree(handle->path);
	mfree(handle);
}

void __fdirrefresh(rawenv re, dirhandle *handle){
	// DEBUGPRINT(L"\nRefreshing Dir %a", handle->path);
	uint64_t blockprogress = 0x00;
	bool exit_ = false;
	do{
		// Find the associated Clusters/Blocks
		fsblock *fb = __ffindi(handle->root, handle->path, blockprogress + 1);
		if(fb){
			// Read off the Block 
			handle->dirarray = mrealloc(handle->dirarray, re->CBlockSize * (blockprogress + 1));
			void *temp = ReadRawHandleBlocks(re, getloc(handle->root, fb), ReBlocks(re, handle->root->root->confBlockSize));
			memcpy(handle->dirarray + (re->CBlockSize * blockprogress), temp, re->CBlockSize);
			mfree(temp);
			for(uint64_t cc = 0x00; cc < __safediv(re->CBlockSize, sizeof(diritem)); ++cc){if(handle->dirarray[cc].local == 0x00){exit_ = true;	break;}}
			blockprogress++;
		}else if(!fb){exit_ = true;}
	}while(!exit_);
	handle->loadedblocks = blockprogress;
	
}

fhandle *floadh(rawenv re, conf_fsroot *root, char *path, char *args){
	// DEBUGPRINT(L"\nMounting File [%a] with \"%a\"", path, args);
	// uint64_t proxyhash = 0x00;
	if(strcheck(args, 'c')){__fcreate(re, root, path, args);}
	// if(strcheck(args, 'P')){
	// 	conf_fsblock *finfo = __freadinfo(root, __ffind(root, path));
	// 	if(!strcheck(args, 'R')){
	// 		if(finfo->fcode){return floadh(root, path, args);}
	// 	}
	// }
	fhandle *out = mcalloc(1, sizeof(fhandle));
	fsblock *fb = __ffind(root, path);
	if(!fb){return NULL;}
	fhandle *out = mcalloc(1, sizeof(fhandle));
	*out = (fhandle){
		.file = fb,
		.root = root,
		.path = strdup(path),
		.progress = root->root->confBlockSize	// Avoid Metadata Block
	};
	return out;
}

void fuloadh(rawenv re, fhandle *handle){
	// DEBUGPRINT(L"\nUn-Mounting File [%llu]", handle->file->fcode);
	// Flush root
	// rawenv re = startup(handle->root->_GUID, handle->root->altGUID, handle->root->root->confBlockSize);
	WriteRawHandleBlocks(re, handle->root->loc, ReBlocks(re, sizeof(fsroot)), handle->root->root);
	// Flush cluster Array/Buffer
	WriteRawHandleBlocks(re, handle->root->loc + CLUSTERMAPOFFSET(handle->root->logblocks.nLogSectors), 
		ReBlocks(re, handle->root->clusterbuffer.clusterSize), handle->root->clusterbuffer.clusterMap);
	mfree(handle);
}


meta_fsblock *__freadinfo(rawenv re, conf_fsroot *root, fsblock *fb){
	LBA loc = getloc(root, fb);
	// DEBUGPRINT(L"\nReading File Info");
	void *out = ReadRawHandleBlocks(re, loc, ReBlocks(re, sizeof(meta_fsblock)));
	
	return out;
}

void __fupdatetstamp(rawenv re, conf_fsroot *root, fsblock *file, bool wt){
	meta_fsblock *finfo = __freadinfo(re, root, file);
	LBA loc = getloc(root, file);
	CMOS_time_t time;
	CMOSGetTime(&time);
	if(wt){
		finfo->writetime = (time.hour * 3600) + (time.minute * 60) + time.second;
		finfo->writedate = CMOSGetElapsedDays(time.year, time.month, time.day);
	}
	finfo->accesstime = (time.hour * 3600) + (time.minute * 60) + time.second;
	finfo->accessdate = CMOSGetElapsedDays(time.year, time.month, time.day);
	// DEBUGPRINT(L"\nWriting Time Stamp");
	WriteRawHandleBlocks(re, loc, ReBlocks(re, sizeof(meta_fsblock)), finfo);
	
}

uint64_t __fsize(fhandle *fh){
	uint64_t size = 0x00;
	for(uint64_t cc = 0x00; cc < fh->root->clusterbuffer.nClusterItems; ++cc){
		size += (
			!flagcheck((fh->root->clusterbuffer.clusterMap + cc)->attributes, __fsmetadatacluster) &&
			(fh->root->clusterbuffer.clusterMap[cc].fcodelow == fh->file->fcodelow && fh->root->clusterbuffer.clusterMap[cc].fcodehigh == fh->file->fcodehigh) 
			? fh->root->root->confBlockSize: 0x00
		);
	}
	return size;
}
uint64_t __dsize(dirhandle *dh){
	uint64_t size = 0x00;
	for(uint64_t cc = 0x00; cc < __safediv((dh->loadedblocks * dh->root->root->confBlockSize), sizeof(diritem)); ++cc){
		size += (((dh->dirarray + cc)->local != 0x00) ? dh->root->root->confBlockSize: 0x00);
	}
	return size;
}

meta_fsblock *_dreadinfo(rawenv re, dirhandle *handle){return __freadinfo(re, handle->root, handle->file);}
meta_fsblock *_freadinfo(rawenv re, fhandle *handle){return __freadinfo(re, handle->root, handle->file);}

void _fseek(fhandle *handle, uint64_t progress){handle->progress = progress;}
void _fseeko(fhandle *handle, int64_t progress){handle->progress += progress;}

uint64_t _fwrite(rawenv re, fhandle *handle, uint64_t nbytes, const void *data){
    if(!handle || !data || nbytes == 0x00){return 0x00;}
    uint64_t blkSize = handle->root->root->confBlockSize;
    uint64_t pos    = handle->progress;      // byte offset in file
    uint64_t left    = nbytes;
    uint64_t written = 0x00;
    __fupdatetstamp(re, handle->root, handle->file, true);
    if(flagcheck((__ffindh(handle->root, (uint64_t[2]){handle->file->fcodelow, handle->file->fcodehigh}))->attributes, __fsreadonly)){return 0x00;}
    while(left > 0x00){
        uint64_t blkIndex   = __safediv(pos, blkSize);
        uint64_t  blkOffset  = (uint64_t)(pos % blkSize);
        uint64_t  chunk      = blkSize - blkOffset;
        if(chunk > left){chunk = left;}
        void *blk = __fread1(re, handle->root, handle->file, blkIndex);
        if(!blk){
            blk = mcalloc(1, blkSize);
            if(!blk){break;}
        }
        memcpy(blk + blkOffset, data + written, chunk);
        __fpush1(re, handle->root, handle->file, blkIndex, blk);
        mfree(blk);
        written      += chunk;
        pos          += chunk;
        left         -= chunk;
    }
    handle->progress = pos;
    return (nbytes - left);
}
uint64_t _fread(rawenv re, fhandle *h, uint64_t nbytes, void **dataout){
	if((h->progress + nbytes) > __fsize(h)){return 0x00;}
    uint32_t bsize = h->root->root->confBlockSize;
    uint64_t progress = h->progress;

    uint64_t remaining = nbytes;
    uint64_t written = 0x00;

	__fupdatetstamp(re, h->root, h->file, false);

    void *out = mcalloc(1, nbytes);
    if(!out){return 0x00;}
    while(remaining > 0x00){
        uint64_t block_index = __safediv(progress, bsize), 
				offset = progress % bsize, 
				chunk = bsize - offset;

        void *blk = __fread1(re, h->root, h->file, block_index);
        if(!blk){break;}
        if(chunk > remaining){chunk = remaining;}

        memcpy(out + written, blk + offset, chunk);
        mfree(blk);
        written  += chunk;
        progress += chunk;
        remaining -= chunk;
    }
    h->progress = progress;
    *dataout = out;
    return (nbytes - remaining);
}

dirhandle *__fgetparent(rawenv re, conf_fsroot *root, char *path){
	uint32_t cc = strlen(path);
	char *dup = strdup(path);
	while(dup[cc] == FRATPATHnoSEP || dup[cc] == FRATPATHSEP){cc--;}
	for(; cc > 0x00; --cc){if(dup[cc] == FRATPATHSEP || dup[cc] == FRATPATHnoSEP){dup[cc] = '\0x00';	break;}}
	if(cc == 0x00){return NULL;}
	dirhandle *out = floadhdir(re, root, dup, "dc");
	mfree(dup);
	return out;
}

void __fdiradd(rawenv re, dirhandle *dir, fsblock *fb){
	// DEBUGPRINT(L"\nAdding Entry:%llu to Dir [%a:%llu]", fb->fcode, dir->path, dir->file->fcode);
	do{
		__fdirrefresh(re, dir);
		uint32_t cc = 0x00;
		if(dir->dirarray){
			do{
				if(!(dir->dirarray[cc].f.fcodelow) && !(dir->dirarray[cc].f.fcodehigh)){
					dir->dirarray[cc] = (diritem){.f = *fb, .local = 0x00};
					return;
				}else if(dir->dirarray[cc].f.fcodehigh == fb->fcodehigh && dir->dirarray[cc].f.fcodelow == fb->fcodelow){
					dir->dirarray[cc].f.attributes = fb->attributes;
					return;
				}else{dir->dirarray[cc].local++;}
				cc++;
			}while(dir->dirarray[cc == 0x00? cc: (cc - 1)].local != 0x00);
		}
		__faddr(re, dir->root, dir->file);
	}while(true);
}

bool __ftest(rawenv re, fhandle *h){
	// DEBUGPRINT(L"\nPerforming File Test. Code: %llu", h->file->fcode);
	bool out = true;
	void *_block = mcalloc(1, h->root->root->confBlockSize);
	trng__(_block, h->root->root->confBlockSize);
	_fseek(h, 1);
	_fwrite(re, h, h->root->root->confBlockSize, _block);
	_fseeko(h, -1);
	void *__block = NULL;
	_fread(re, h, h->root->root->confBlockSize, &__block);
	if(__block){
		Print(L"\n");
		for(uint64_t cc = 0x00; cc < h->root->root->confBlockSize; ++cc){
			if(((uint8_t *)_block)[cc] != ((uint8_t *)__block)[cc]){
				out = false;
				// DEBUGPRINT(L"✖ %u:%u  ", ((uint8_t *)_block)[cc], ((uint8_t *)__block)[cc]);
			}// else{DEBUGPRINT(L"✔ %u:%u  ", ((uint8_t *)_block)[cc], ((uint8_t *)__block)[cc]);}
		}
	   mfree(__block);
	}
	mfree(_block);
	return out;
}

unhandle *__fdirlist(rawenv re, dirhandle *dir, uint64_t *index){
	__fdirrefresh(re, dir);
	if(__safediv(((*index) * sizeof(diritem)), dir->root->root->confBlockSize) < dir->loadedblocks){
		diritem *ditem = dir->dirarray + (*index);
		(*index)++;
		fsblock *fb = __ffindh(dir->root, (uint64_t[2]){dir->file->fcodelow, dir->file->fcodehigh});
		if(fb){
			if(fb->fcodehigh == ditem->f.fcodehigh && fb->fcodelow == ditem->f.fcodelow && fb->fcodelow && fb->fcodehigh){
				meta_fsblock *finfo = __freadinfo(re, dir->root, fb);
				char *temp = strdup(dir->path);
				temp = mrealloc(temp, strlen(temp) + strlen(finfo->name) + 2);
				memcpy(temp + strlen(dir->path) + (*finfo->name != '/'), finfo->name, strlen(finfo->name) + 1);
				temp[strlen(dir->path)] = FRATPATHSEP;
				mfree(finfo);
				unhandle *out = mcalloc(1, sizeof(unhandle));
				if((out->dir = flagcheck(fb->attributes, __fsdirectory))){
					out->dhandle_ = floadhdir(re, dir->root, temp, "");
				}else{out->fhandle_ = floadh(re, dir->root, temp, "f");}
				mfree(temp);
				return out;
			}else{
				// Remove Entry
				ditem->f.fcodehigh = 0x00;
				ditem->f.fcodelow = 0x00;
				return NULL;
			}
		}else{}// Repair The Entry
	}
	return NULL;
}

dirrunner *__dirr_init(dirhandle *handle, char *patternmatcher){
	if(!handle){return NULL;}
	dirrunner *dr = mcalloc(1, sizeof(dirrunner));
	if(!dr){return NULL;}
	dr->dir = handle;
	dr->pattmatcher = patternmatcher;
	dr->stack_size = 8;
	dr->stack_depth = 1;
	dr->dir_stack = mcalloc(dr->stack_size, sizeof(dirhandle*));
	dr->idx_stack = mcalloc(dr->stack_size, sizeof(uint64_t));
	if(!dr->dir_stack || !dr->idx_stack){
		mfree(dr->dir_stack);
		mfree(dr->idx_stack);
		mfree(dr);
		return NULL;
	}
	dr->dir_stack[0x00] = handle;
	dr->idx_stack[0x00] = 0x00;
	return dr;
}

static bool __dirr_matches(rawenv re, dirrunner *dr, unhandle *item){
	meta_fsblock *info = item->dir ? _dreadinfo(re, item->dhandle_) : _freadinfo(re, item->fhandle_);
	if(!info){return false;}
	bool match = pattmatch(dr->pattmatcher, info->name);
	mfree(info);
	return match;
}

static bool __dirr_push(dirrunner *dr, dirhandle *handle){
	if(dr->stack_depth == dr->stack_size){
		dr->dir_stack = mrealloc(dr->dir_stack, (dr->stack_size ? dr->stack_size * 2 : 8) * sizeof(dirhandle*));
		dr->idx_stack = mrealloc(dr->idx_stack, (dr->stack_size ? dr->stack_size * 2 : 8) * sizeof(uint64_t));
		if(!dr->dir_stack || !dr->idx_stack){return false;}
		dr->stack_size = (dr->stack_size ? dr->stack_size * 2 : 8);
	}
	dr->dir_stack[dr->stack_depth] = handle;
	dr->idx_stack[dr->stack_depth] = 0x00;
	dr->stack_depth++;
	return true;
}

static dirhandle *__dirr_pop(dirrunner *dr){
	if(dr->stack_depth == 0x00){return NULL;}
	dr->stack_depth--;
	dirhandle *out = dr->dir_stack[dr->stack_depth];
	dr->dir_stack[dr->stack_depth] = NULL;
	return out;
}

void __dirr_free(rawenv re, dirrunner *dr){
	if(!dr){return;}
	for(uint64_t cc = 0x00; cc < dr->stack_depth; ++cc){if(dr->dir_stack[cc]){if(cc > 0x00){fuloaddir(re, dr->dir_stack[cc]);}}}
	mfree(dr->dir_stack);
	mfree(dr->idx_stack);
	mfree(dr);
}

unhandle *__dirr(rawenv re, dirrunner *dr){
	if(!dr || dr->stack_depth == 0x00){return NULL;}
	while(dr->stack_depth > 0x00){
		uint64_t top = dr->stack_depth - 1;
		dirhandle *current = dr->dir_stack[top];
		uint64_t idx = dr->idx_stack[top];
		unhandle *item = __fdirlist(re, current, &idx);
		dr->idx_stack[top] = idx;
		if(!item){
			if(top > 0x00){fuloaddir(re, current);}
			__dirr_pop(dr);
			continue;
		}
		if(!item->dhandle_){
			if(top > 0x00){fuloaddir(re, current);}
			__dirr_pop(dr);
			continue;
		}
		if(!item->dhandle_->file){
			if(top > 0x00){fuloaddir(re, current);}
			__dirr_pop(dr);
			continue;
		}

		bool matches = __dirr_matches(re, dr, item);
		if(item->dir){
			if(!__dirr_push(dr, item->dhandle_)){
				mfree(item);
				return NULL;
			}
		}
		if(matches){return item;}
		mfree(item);
	}
	return NULL;
}

void __fprint_info(meta_fsblock *finfo){
	// Print(
	// 	L"\nFile Info:"
	// 	L"\nSignature: %.8s"
	// 	L"\nVersion: %llu"
	// 	L"\nfCode: %llu"
	// 	L"\nAttributes:"
	// 	L"\n\tIs Directory: [%a]"
	// 	L"\n\tIs File: [%a]"
	// 	L"\n\tIs Readonly: [%a]"
	// 	L"\nName: %.32s"
	// 	L"\nAccess Time: %u:%u:%u"
	// 	L"\nWrite Time: %u:%u:%u"
	// 	L"\nAccess Date: %u"
	// 	L"\nWrite Date: %u",
	// 	finfo->fsig, finfo->headerversion, (uint64_t)finfo->fcode,
	// 	(flagcheck(finfo->attributes, __fsdirectory) ? "true": "false"), (flagcheck(finfo->attributes, __fsfile) ? "true": "false"), 
	// 	(flagcheck(finfo->attributes, __fsreadonly) ? "true": "false"), 
	// 	finfo->name, 
	// 	(uint32_t)(__safediv(finfo->accesstime, 3600)), 
	// 	(uint32_t)(__safediv((finfo->accesstime % 3600), 60)),
	// 	(uint32_t)(finfo->accesstime % 60), 
	// 	(uint32_t)(__safediv(finfo->writetime, 3600)), 
	// 	(uint32_t)(__safediv((finfo->writetime % 3600), 60)),
	// 	(uint32_t)(finfo->writetime % 60), 
	// 	finfo->accessdate, finfo->writedate
	// );
}

LBA getloc(conf_fsroot *root, fsblock *fb){
	return DATAFIRST(root) + __safediv(((uint64_t)fb - (uint64_t)root->clusterbuffer.clusterMap), sizeof(fsblock));
}