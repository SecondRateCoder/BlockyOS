#include "LocalAPIC.h"
#include "kernel/libcrt/hardware/GDT/GDT.h"
#include "kernel/libcrt/hardware/IDT/ISR.h"
#include "kernel/libcrt/mutex.h"
#include "kernel/libcrt/hardware/PIT.h"



//	@param timercount	This is the current APIC Timer.
//	@param persistent	This is data passed on Initialisation.
//	@param call			This is data passed by the previous call's return.
#define LocalAPICTimerCallbackDefinition(NAME)		\
	void * __sysvabi NAME(LocalAPICTimerEvent *this, uint32_t CurrentTick, void *persistent, void *call)
typedef void * __sysvabi(*LocalAPICTimerCallback)(struct LocalAPICTimerEvent *this, uint32_t CurrentTick, void *persistent, void *call);
typedef struct LocalAPICTimerEvent{
	uint16_t            		Tick;
	//	The Time in This.Tick till this Entry is no longer polled.
	uint16_t					ErasureCountdown;
	//	The Time in This.Tick that this Entry will not be polled.
	uint16_t					StartupCooldown;
	LocalAPICTimerCallback		Callback;
	void						*persistent, 
								*call;
	//	Write a Value to this Ptr in order to signal the Event Decay.
	CommonMutex					signal;
	struct LocalAPICTimerEvent 	*Next;
}__packed LocalAPICTimerEvent;

LibAPI uint32_t __noinline GetTimerVector(bool w, uint32_t vector);
LibAPI bool AddTimerEvent(uint32_t Vector, uint16_t Tick, uint16_t Countdown, 
	uint16_t StartupCooldown, LocalAPICTimerCallback *Event, void *persistent, CommonMutex signal);
LibAPI ISRCallbackDefinition(APICTimerEventHandler);
LibAPI void InitializeAndCalibrateAPICTimer(void *acpibase, uint8_t Priviledge, uint8_t Frequency);