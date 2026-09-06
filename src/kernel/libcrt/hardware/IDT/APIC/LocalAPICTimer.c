#include "LocalAPICTimer.h"

uint32_t __noinline GetTimerVector(bool w, uint32_t vector){
	static uint32_t v = 0;
	if(w){v = vector;}
	return v;
}

bool AddTimerEvent(uint32_t Vector, uint16_t Tick, uint16_t Countdown, 
	uint16_t StartupCooldown, LocalAPICTimerCallback *Event, void *persistent, CommonMutex signal
){
	LocalAPICTimerEvent *TimerTable = ISRGetStackHeader(Vector);
	void *LocalAPIC = (void *)TimerTable - (sizeof(uint64_t) * 4);
	//	Walk to the End of the Table.
	//	Disable Interrupts.
	DisableLocalAPIC(LocalAPIC);
	while(TimerTable->Next && TimerTable->Callback){TimerTable = TimerTable->Next;}
	void *ptr = TimerTable->Next;
	if(!TimerTable->Next){ptr = mcalloc(1, sizeof(LocalAPICTimerEvent));}
	//	We query
	*(TimerTable->Callback? (LocalAPICTimerEvent *)TimerTable->Next: TimerTable) = (LocalAPICTimerEvent){
		.call = NULL, .persistent = persistent, .Next = ptr, .Callback = *Event, 
		.ErasureCountdown = Countdown, .StartupCooldown = StartupCooldown, 
		.signal = signal, .Tick = Tick
	};
	EnableLocalAPIC(LocalAPIC);
	return true;
}

ISRCallbackDefinition(APICTimerEventHandler){
	LocalAPICTimerEvent *ETable = (LocalAPICTimerEvent *)Frame->StackHeader, *Selected = (LocalAPICTimerEvent *)ETable->Next;
	void *LAPIC = GetLocalAPICBase(Frame->LocalAPIC, NULL);
	uint32_t Current = 0x00;
	ReadAPICRegister(LAPIC, LAR_TimerCurrentCount, Current);
	while(ETable->Next){
		if(Selected){
			if((Current % Selected->Tick) == 0){break;}else 
			if((Current % Selected->Tick) > (Current % ETable->Tick)){Selected = ETable;}else 
			//	Tick Error, we need to modify the Tick of Selected
			if((Current % Selected->Tick) == (Current % ETable->Tick)){Selected->Tick = ETable->Tick + 16;}
		}else{Selected = ETable;}
		ETable = (LocalAPICTimerEvent *)ETable->Next;
	}
	void *Temp = mcalloc(1, sizeof(LocalAPICTimerEvent));
	void *_call = (void *)Selected->Callback(Temp, Current, Selected->persistent, Selected->call);
	UnlockMutex(Selected->signal);
	memcpy(Selected, Temp, sizeof(LocalAPICTimerEvent));
	Selected->call = _call;
	LockMutex(Selected->signal);
	ISRCallbackReturn;
}

void InitializeAndCalibrateAPICTimer(void *acpibase, uint8_t Priviledge, uint8_t Frequency){
    void *Base = GetLocalAPICBase(acpibase, NULL);
    //	Stop the APIC timer first and set divider to 16
	uint8_t InterruptVector;
	if(!AllocateInterruptVector(&InterruptVector)){return;}
    WriteAPICRegister(Base, LAR_TimerInitialCount, 0);
    WriteAPICRegister(Base, LAR_TimerDivideConfiguration, 0x3); // Divide by 16

    //	Set APIC timer to Periodic mode with a dummy vector for calibration phase
    //	Bit 17 = Periodic mode (1), Masked (1) so it doesn't fire interrupts yet
    WriteAPICRegister(Base, LAR_LVTTimer, InterruptVector | (1 << 17) | (1 << 16));

    //	Load max count to start counting down
    WriteAPICRegister(Base, LAR_TimerInitialCount, 0xFFFFFFFF);

    //	Use PIT to wait for a precise duration (e.g., 10 milliseconds)
    //	Assuming you have a PIT wait function or can program the PIT directly:
    //	Here we use a standard 10ms calibration window
    //	(10ms = 11932 PIT ticks / 1193 = ~10ms if using 1.193182 MHz base)
    
    //	Quick inline PIT one-shot or wait routine for 10ms:
    SleepMS(10);
    
    //	Read current APIC count to see how many ticks elapsed in 10ms
    uint32_t ticks_in_10ms;
	ReadAPICRegister(Base, LAR_TimerCurrentCount, ticks_in_10ms);
	ticks_in_10ms = UINT32_MAX - ticks_in_10ms;

    //	Stop the timer
    WriteAPICRegister(Base, LAR_TimerInitialCount, 0);

	//	Initialise ISR
    uint8_t _IST;
    IDTEntrySegmentSelector64 sselector;
	LocalAPICTimerEvent *temp = mcalloc(1, sizeof(LocalAPICTimerEvent));
    if(!AllocateIST(&_IST, &sselector)){return;}
	GetTimerVector(true, InterruptVector);
	ISRSetCallback(acpibase, InterruptVector, Priviledge, _IST, (LocalAPICTimerEvent[]){{0}}, 
		sselector, true, (ISRCallback *)&APICTimerEventHandlerISR);
	
    //	Re-enable APIC timer in periodic mode with the actual vector and calculated count
	//	Keep divide by 16
    WriteAPICRegister(Base, LAR_TimerDivideConfiguration, 0x3);
	//	Periodic, unmasked
    WriteAPICRegister(Base, LAR_LVTTimer, InterruptVector | (1 << 17));
    WriteAPICRegister(Base, LAR_TimerInitialCount, (ticks_in_10ms * 100) / Frequency);
}