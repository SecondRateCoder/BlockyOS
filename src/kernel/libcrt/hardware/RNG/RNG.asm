bits 64
section .text
;	https://wiki.osdev.org/Random_Number_Generator
extern mt_rng

global GetTrueRandom64
GetTrueRandom64:
	;   set EAX to request function 7
	mov eax, 7
	;   set ECX to request subfunction 0
	mov ecx, 0
	cpuid
	;   the result we want is in EBX
	shr ebx, 18
	;   test for the flag of interest
	and ebx, 1
mov ecx, 100   ; number of retries
.retry:
	rdseed eax
	jc .done      ; carry flag is set on success
	loop .retry
.fail:
	;	We'll let that function return itself.
	jmp mt_rng
.done:
	ret