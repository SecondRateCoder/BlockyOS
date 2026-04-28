#include "frat.h"

char *ppath;

bool checkdisk(char *path){
    // Check the GPT for a Disk.
    bool out = 0;
    ppath = strdup(path);
    rawenv *re = startup(path);
    configureblocksize(512);
    uint8_t *block = readblock(re, 1, 1);
    miniGPT *gpt = (miniGPT *)block;
    if(!memcmp(gpt->sig, "EFI PART", 8)){out = true;}else{out = false;}
    free(block);    dispose(re);
    return out;
}

GPTeNSTR *makeGPTeNSTR(char *str){
    GPTeNSTR *out = calloc(sizeof(GPTeNSTR), 1);
    for(size_t cc = 0; cc < __min(strlen(str), GPTeNAMELEN); ++cc){
        (*out)[cc] = str[cc];
    }
    return out;
}

LBA *loadpart(GPTeNSTR name){
    if(checkdisk(ppath)){
        rawenv *re = startup(ppath);
        configureblocksize(512);
        uint8_t *block = readblock(re, 1, 1);
        miniGPT *gpt = (miniGPT *)block;
        GPTentry *ge = (GPTentry *)(readblock(re, gpt->GPTarray, (gpt->partEntries * getblocksize()) / sizeof(GPTentry)));
        dispose(re);
        for(uint32_t i = 0; i < gpt->partEntries; i++){
            if(!memcmp(name, ge[i].name, GPTeNAMESIZE)){
                LBA *out = malloc(sizeof(LBA) * 2);
                out[0] = ge[i].sLBA;    out[1] = ge[i].eLBA;
                free(ge);
                return out;
            }
        }
    }
}

void formatpart(GPTeNSTR name){
    LBA *part = loadpart(name);
    void *block = malloc(getblocksize());
    *((fsroot *)block) = (fsroot){
        .confBlockSize = getblocksize(),
        .confClusterSize = (part[1] - part[0]) * 8,
        .verCode = {0, 1},
        .signature = FRATSIG,
    };
    // Reset all Info
    rawenv *re = startup(ppath);
    writeblock(re, block, part[0], 1);
    memset(block, 0, getblocksize());
    // Clear Log
    writeblock(re, block, part[0] + 1, 1);
    // Clear ClusterMap
    block = realloc(block, (part[1] - part[0]) * 8);
    memset(block, 0, (part[1] - part[0]) * 8);
    writeblock(re, block, part[0] + 2, 1);
    dispose(re);
    free(block);
}

LBA *queryparttablefs(miniGPT *gpt, rawenv *re){
    // Query all Partitions for the FileSystem.
    LBA parttable = gpt->GPTarray;
    GPTentry *ge = (GPTentry *)(readblock(re, parttable, (gpt->partEntries * getblocksize()) / sizeof(GPTentry)));
    for(uint32_t i = 0; i < gpt->partEntries; i++){
        if(queryfs(re, getblocksize() * ge[i].sLBA)){
            LBA *out = malloc(sizeof(LBA) * 2);
            out[0] = ge[i].sLBA;    out[1] = ge[i].eLBA;
            free(ge);
            return out;
        }
    }
    return NULL;
}

bool queryfs(rawenv *re, LBA base){
    // Check that a FileSystem exists at the bytebase.
    bool out = 0;
    uint8_t *block = readblock(re, base / getblocksize(), 3);
    fsroot *fr = (fsroot *)block;
    if(!memcmp(fr->signature, FRATSIG, 22)){out = true;}else{out = false;}
    free(block);
    return out;
}

conf_fsroot *fmount(char *path){
    LBA *partition;
    if(checkdisk(path)){
        // Get the LBA Info for a Partition
        rawenv *re = startup(path);
        void *block = readblock(re, 1, 1);
        miniGPT *gpt = (miniGPT *)block;
        if((partition = queryparttablefs(gpt, re))){
            free(gpt);
        }else{free(gpt);    return NULL;}
        // Generate the fsroot
        block = readblock(re, partition[0], 1);
        fsroot *fsroot_ = (fsroot *)block;
        configureblocksize(fsroot_->confBlockSize);
        conf_fsroot *largeroot = malloc(sizeof(conf_fsblock));
        largeroot->loc = partition[0];
        largeroot->root = *fsroot_;
        largeroot->lastClusterAlloc = 0;
        largeroot->logLBA = partition[0] + 1;
        largeroot->clusterBuffer.clusterLBA = partition[0] + 2;
        largeroot->clusterBuffer.clusterSize = (partition[1] - (partition[0] + 2)) / getblocksize();
        largeroot->clusterBuffer.fs = readblock(re, partition[0] + 2, (partition[1] - partition[0]) / getblocksize());
        for(size_t i = 0; i < largeroot->clusterBuffer.clusterSize; ++i){
            // Read and verify ROOTS
            fsblock f = largeroot->clusterBuffer.fs[i];
            if(f.fcode != 0){
                conf_fsblock *temp = readblock(re, i + partition[0] + 2, 1);
                if(memcmp(temp->fsig, FRATBLOCKSIG, 8)){
                    printf("ERROR!\nCorrupted FileSystem Root Block");
                    memset(temp, 0, 512);
                    writeblock(re, temp, i + partition[0] + 2, 1);
                }
                free(temp);
            }
        }
        dispose(re);
        return largeroot;
    }
}

fsblock *allocatecluster(conf_fsroot *root){
    for(size_t i = root->lastClusterAlloc; i < root->clusterBuffer.clusterSize / getblocksize(); ++i){
        if(root->clusterBuffer.fs[i].fcode == 0){return root->clusterBuffer.fs + i;}
    }
    root->lastClusterAlloc = 0;
    return NULL;
}

void __fcreate(conf_fsroot *root, char *path, char *flags){
    fsblock *fb = allocatecluster(root);
    if(fb){
        if(strcheck(flags, 'd')){fb->attr |= __fsdirectory;}
        if(strcheck(flags, 'f') || !flagcheck(fb->attr, __fsfile)){fb->attr |= __fsfile;}
        if(strcheck(flags, 'r') || !flagcheck(fb->attr, __fsreadonly)){fb->attr |= __fsfile;}
        fb->attr |= __fsmetadatacluster;
        size_t hash;
        blake2b_state hashstate;
        blake2b_init(&hashstate, 8);
        blake2b_update(&hashstate, path, strlen(path));
        blake2b_final(&hashstate, &hash, 8);
        fb->fcode = hash;
        fb->logalias = 0;
        fb->index = 0;
        char *_path = malloc(strlen(path) + 1);
        *_path = '\\';
        memcpy(_path + 1, path, strlen(path));
        __finit(root, fb, _path);
    }
}

void __fadd(conf_fsroot *root, fsblock *family){
    fsblock *fb = allocatecluster(root);
    if(fb){
        fb->index = 1;
        for(size_t i = 0; i < root->clusterBuffer.clusterSize; ++i){
            if(root->clusterBuffer.fs[i].fcode == family->fcode){fb->index++;}
        }
        fb->attr = family->attr;
        flagunset(fb->attr, __fsmetadatacluster);
    }
}

void __finit(conf_fsroot *root, fsblock *fb, char *path){
    LBA loc = root->loc + 2 + root->clusterBuffer.clusterSize + (((size_t)fb - (size_t)root->clusterBuffer.fs) / sizeof(fsblock));
    void *block = malloc(getblocksize());
    conf_fsblock *metadata = (conf_fsblock *)block;
    time_t t = time(NULL);
    struct tm *truetime = localtime(&t);
    char *name;
    for(ssize_t i = strlen(path) - 1; i > -1; i--){
        if(path[i] == '/' || path[i] == '\\'){
            name = path - i;  break;
        }else if(!isascii(path[i])){path[i] = 0;}
    }
    *metadata = (conf_fsblock){
        .accessdate = truetime->tm_yday,
        .writedate = 0,
        .accesstime = (truetime->tm_hour * 3600) + (truetime->tm_min * 60) + truetime->tm_sec,
        .writetime = 0,
        .attributes = fb->attr,
        .fcode = fb->fcode,
        .fsig = FRATBLOCKSIG,
    };
    memcpy(metadata->name, name, GPTeNAMELEN);
    rawenv *re = startup(ppath);
    writeblock(re, block, loc, 1);
    dispose(re);
}

void *__fread1(conf_fsroot *root, fsblock *fb, size_t i){
    LBA loc = root->loc + 2 + root->clusterBuffer.clusterSize + (((size_t)fb - (size_t)root->clusterBuffer.fs) / sizeof(fsblock));
    if(i != fb->index){
        fsblock *fb_ = NULL;
        for(size_t cc = 0; cc <root->clusterBuffer.clusterSize; ++cc){
            if(i == root->clusterBuffer.fs[cc].index){fb = root->clusterBuffer.fs + cc;}
        }
        if(fb_ == NULL){return NULL;}
        loc = root->loc + 2 + root->clusterBuffer.clusterSize + (((size_t)fb_ - (size_t)root->clusterBuffer.fs) / sizeof(fsblock));
    }
    rawenv *re = startup(ppath);
    void *out = readblock(re, loc, 1);
    dispose(re);
    return out;
}

fsblock *__ffind(conf_fsroot *root, char *path){
    size_t hash;
    blake2b_state hashstate;
    blake2b_init(&hashstate, 8);
    blake2b_update(&hashstate, path, strlen(path));
    blake2b_final(&hashstate, &hash, 8);
    hash &= 0x3FFFFFFFFFF;
    for(size_t cc = 0; cc < root->clusterBuffer.clusterSize; ++cc){
        if(root->clusterBuffer.fs[cc].fcode == hash && root->clusterBuffer.fs[cc].index == 0){
            return root->clusterBuffer.fs + cc;
        }
    }
    return NULL;
}

void __fpush1(conf_fsroot *root, fsblock *fb, size_t i, void *buffer){
    LBA loc = root->loc + 2 + root->clusterBuffer.clusterSize + (((size_t)fb - (size_t)root->clusterBuffer.fs) / sizeof(fsblock));
    if(i != fb->index){
        fsblock *fb_ = NULL;
        for(size_t cc = 0; cc <root->clusterBuffer.clusterSize; ++cc){
            if(i == root->clusterBuffer.fs[cc].index){fb = root->clusterBuffer.fs + cc;}
        }
        if(fb_ == NULL){loc = 0;}
        loc = root->loc + 2 + root->clusterBuffer.clusterSize + (((size_t)fb_ - (size_t)root->clusterBuffer.fs) / sizeof(fsblock));
    }
    if(loc = 0){__fadd(root, fb);}
    rawenv *re = startup(ppath);
    writeblock(re, buffer, loc, 1);
    dispose(re);
}

fshandle *fsloadh(conf_fsroot *root, char *path, char *args){
    if(strcheck(args, 'c')){__fcreate(root, path, args);}
    fshandle *out = malloc(sizeof(fshandle));
    out->file = __ffind(root, path);
    out->root = root;
    out->progress = 0;
    for(uint8_t cc = 0; cc < (sizeof(out->handlecache) / sizeof(out->handlecache[0])); ++cc){
        out->handlecache[cc].block = NULL;
        out->handlecache[cc].progresstimestamp = 0;
        out->handlecache[cc].rw = 0;
    }
    return out;
}

void _fseek(fshandle *handle, size_t progress){handle->progress = progress;}

void _fpush1(fshandle *handle, void *buffer){
    for(uint8_t cc = 0; cc < (sizeof(handle->handlecache) / sizeof(handle->handlecache[0])); ++cc){
        if(abs(handle->progress - handle->handlecache[cc].progresstimestamp) > handle->progresslimit){
            handle->progress++;
            if(handle->handlecache[cc].block){free(handle->handlecache[cc].block);}
            handle->handlecache[cc].block = memdup(buffer, getblocksize());
            handle->handlecache[cc].progresstimestamp = handle->progress;
            handle->handlecache[cc].rw = 0;
            __fpush1(handle->root, handle->file, handle->progress, buffer);
            return;
        }
    }
}

void *_fread1(fshandle *handle){
    for(uint8_t cc = 0; cc < (sizeof(handle->handlecache) / sizeof(handle->handlecache[0])); ++cc){
        if(abs(handle->progress - handle->handlecache[cc].progresstimestamp) > handle->progresslimit){
            if(handle->handlecache[cc].block){free(handle->handlecache[cc].block);}
            void *buffer = __fread1(handle->root, handle->file, handle->progress);
            handle->handlecache[cc].block = memdup(buffer, getblocksize());
            handle->handlecache[cc].progresstimestamp = handle->progress;
            handle->handlecache[cc].rw = 0;
            handle->progress++;
        }
    }
}