#include "../src/kernel/ram/ram.h"

/// @brief An arbitrary pointer for weak function calls (Marked with "_weak" suffix).
size_t arbitrary_ptr;

void *malloc_weak(size_t size){return alloca_weak(size);}
void *alloca_weak(size_t size){
    if(space_validate(arbitrary_ptr, size) == true){
        addr_pointfree(arbitrary_ptr, size);
    }else{
        void *out = (void *)arbitrary_ptr;
        const memh_t hdr = (memh_t){
            .buffer_len = 20,
            .buffer = (char[]){0,0,0,0,0,0,0,0},
            .real_addr = out,
            .size = size
        };
        encode64(hdr.buffer+7, KERNEL_ID);
        arbitrary_ptr += size;
        hcontext_su(&hdr, -1);
        return out;
    }
}

void *realloca(void *ptr, size_t nsize){
    if(nsize < decode64(hcontext_attr_do(ptr, NULL, H_ATTRPEEK_SIZE, NULL, 0))){
        hcontext_attr_do(ptr, NULL, H_ATTRWRITE_SIZE, &nsize, 1);
    }else{
        if(space_validate(ptr, nsize) == false){
            //Can be used
            hcontext_attr_do(ptr, NULL, H_ATTRWRITE_SIZE, &nsize, 1);
        }else{
            void *temp = (void *)arbitrary_ptr;
            addr_pointfree(temp, nsize);
            if(space_validate(temp, nsize)){
                memdisplace(ptr, hcontext_attr_do(ptr, NULL, H_ATTRPEEK_SIZE, NULL, 0), ((size_t)temp)-((size_t)ptr));
                hcontext_attr_do(ptr, NULL, H_ATTRWRITE_SIZE, &nsize, 1);
                hcontext_attr_do(ptr, NULL, H_ATTRWRITE_RADDR, temp, 1);
            }
        }
    }
}

void free_weak(void *ptr){dealloca_weak(ptr);}
void dealloca_weak(void *ptr){
    //Remove the header metadata.
    hcontext_su(NULL, get_hindex(ptr, true));
    return;
}