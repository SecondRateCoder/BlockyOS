#include "socket.h"


socket_ret __fhandle_sckwrite(socket_t * socket, void *data, UINTN posBYTES, UINTN nBYTES, UINTN nARGbytes, ...){
	unhandle *uh = (unhandle *)socket->persistent;
	if(!uh->dir){
		_fseek(uh->fhandle_, posBYTES);
		data = ReallocatePool(nBYTES, nBYTES + (nBYTES % uh->fhandle_->root->root->confBlockSize), data);
		ZeroMem(data + nBYTES, (nBYTES % uh->fhandle_->root->root->confBlockSize));
		nBYTES += (nBYTES % uh->fhandle_->root->root->confBlockSize);
		do{
			_fpush1(uh->fhandle_, data);
			if(posBYTES > nBYTES){break;
			}else{posBYTES += uh->fhandle_->root->root->confBlockSize;}
		}while(nBYTES);
		return socketret_noerr_empty;
	}
	return socketret__noimpl;
}
socket_ret __fhandle_sckread(socket_t * socket, UINTN posBYTES, UINTN readBYTES, UINTN nARGbytes, ...){
	unhandle *uh = (unhandle *)socket->persistent;
	if(!uh->dir){
		void *out = NULL;
		UINTN nBytes = 0;
		do{
			_fseek(uh->fhandle_, posBYTES);
			void *temp = _fread1(uh->fhandle_);
			if(temp){
				out = ReallocatePool(nBytes, nBytes + uh->fhandle_->root->root->confBlockSize, out);
				CopyMemC(out, temp, uh->fhandle_->root->root->confBlockSize);
				FreePool(temp);
				nBytes += uh->fhandle_->root->root->confBlockSize;
			}else{break;}
			if(readBYTES < uh->fhandle_->root->root->confBlockSize){break;
			}else{readBYTES -= uh->fhandle_->root->root->confBlockSize;}
		}while(readBYTES);
		return (socket_ret){
			.data = out,
			.nData = nBytes,
			.errout = __noerr
		};
	}
	return socketret__noimpl;
}
socket_ret __fhandle_sckclose(socket_t * socket, UINTN nARGbytes, ...){
	if(((unhandle *)socket->persistent)->dir){
		__fuloaddir(((unhandle *)socket->persistent)->dhandle_);
	}else{fsuloadh(((unhandle *)socket->persistent)->fhandle_);}
	FreePool(socket);
	return socketret_noerr_empty;
}
socket_ret __fhandle_sckOPENchild(socket_t * socket, UINTN nARGbytes, va_list args){// Handle-Handle
	if(nARGbytes > 8){
		char *childPATH = va_arg(args, char *);
		if(((unhandle *)socket->persistent)->dir){
			dirhandle *dh = ((unhandle *)socket->persistent)->dhandle_;
			char *path = __strdup(dh->path);
			char *loadargs = va_arg(args, char *);
			va_end(args);
			path = ReallocatePool(__strlen(path), __strlen(path) + __strlen(childPATH) + 1, path);
			CopyMemC(path + __strlen(path), childPATH, __strlen(childPATH) + 1);
			fsblock *fb = __ffind(((unhandle *)socket->persistent)->dhandle_->root, path);
			unhandle *uh = AllocatePool(sizeof(unhandle));
			*uh = (unhandle){
				.dir = flagcheck(fb->attr, __fsdirectory),
				.__notype = flagcheck(fb->attr, __fsdirectory)? 
					(void *)__floaddir(((unhandle *)socket->persistent)->dhandle_->root, path, loadargs): 
						(void *)fsloadh(((unhandle *)socket->persistent)->dhandle_->root, path, loadargs)
			};
			// Open Socket
			socket_t *socket = AllocatePool(sizeof(socket_t));
			*socket = (socket_t){
                .persistent = (void *)uh,
				.close = (socketCLOSE)__fhandle_sckclose,
				.open = (socketOPENchild)__fhandle_sckOPENchild,
                .read = (socketREAD)__fhandle_sckread,
                .write = (socketWRITE)__fhandle_sckwrite,
				.raw = {
					.read = (socketREADraw)__fhandle_sckread,
					.write = (socketWRITEraw)__fhandle_sckwrite
				},
			};
			return (socket_ret){
				.errout = __noerr,
				.data = socket,
				.nData = sizeof(socket_t)
			};
		}
	}
	return socketret__noimpl;
}
socket_ret __froot_sckread(socket_t * socket, UINTN posBYTES, UINTN readBYTES, UINTN nARGbytes, ...){return socketret__noimpl;}
socket_ret __froot_sckwrite(socket_t * socket, void *data, UINTN posBYTES, UINTN nBYTES, UINTN nARGbytes, ...){return socketret__noimpl;}
socket_ret __froot_sckclose(socket_t * socket, UINTN nARGbytes, ...){
	conf_fsroot *root = (conf_fsroot *)socket->persistent;
	FreePool(root->root);
	FreePool(root->clusterbuffer.clusterMap);
	FreePool(root);
	FreePool(socket);
	return (socket_ret){
		.data = NULL, .nData = 0,
		.errout = __noerr
	};
}
socket_ret __froot_sckOPENchild(socket_t * socket, UINTN nARGbytes, ...){
	fsblock *__ffind(conf_fsroot *root, char *path);
	if(nARGbytes >= 8){
		va_list args;
		va_start(args, nARGbytes);
		char *path = va_arg(args, char *);
		char *loadargs = va_arg(args, char *);
		va_end(args);
		fsblock *fb = __ffind((conf_fsroot *)socket->persistent, path);
		unhandle *uh = AllocatePool(sizeof(unhandle));
		*uh = (unhandle){
			.dir = flagcheck(fb->attr, __fsdirectory),
			.__notype = flagcheck(fb->attr, __fsdirectory)? 
				(void *)__floaddir((conf_fsroot *)socket->persistent, path, loadargs):
					(void *)fsloadh((conf_fsroot *)socket->persistent, path, loadargs)
		};
		// Open Socket
		socket_t *socket = AllocatePool(sizeof(socket_t));
	}
	return (socket_ret){__incompatible_arg, 0, NULL};
}
socket_ret *__froot_sckopen(UINT32 ignore, UINTN nARGbytes, va_list *args){
#ifdef __DEBUG__
	Print(L"    Opening FS Socket");
#endif
	socket_ret *sret = AllocatePool(sizeof(socket_ret));
	if(nARGbytes < sizeof(EFI_GUID)){
		*sret = (socket_ret){.errout = __incompatible_arg, .data = NULL, .nData = 0};
		return sret;
	}
	EFI_GUID guid = va_arg(*args, EFI_GUID), aGuid = va_arg(*args, EFI_GUID);
	conf_fsroot *root = fmount(guid, aGuid);
	if(root){
		socket_t *socket = AllocatePool(sizeof(socket_t));
		*socket = (socket_t){
			.persistent = (void *)root,
			.close = (socketCLOSE)__froot_sckclose,
			.open = (socketOPENchild)__froot_sckOPENchild,
			.read = (socketREAD)__froot_sckread,
			.write = (socketWRITE)__froot_sckwrite,
			.raw = {
				.read = (socketREADraw)__froot_sckread,
				.write = (socketWRITEraw)__froot_sckwrite
			}
		};
		*sret = (socket_ret){
			.errout = __noerr,
			.data = socket,
			.nData = sizeof(socket)
		};
		return sret;
	}
	*sret = (socket_ret){__incompatible_arg, 0, NULL};
	return sret;
}