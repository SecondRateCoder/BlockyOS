
global PageTableLoad32
;	void PageTableLoad(PageDirEntry *)
PageTableLoad32:
	push ebp
	mov ebp, esp
	push eax
	mov eax, [ebp + 8]
	mov cr3, eax

	mov eax, cr0
	or eax, 0x80000001
	mov cr0, eax
	pop eax
	leave
	ret
