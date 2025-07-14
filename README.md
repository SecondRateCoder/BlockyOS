# ![A modular OS featuring to include some interesting features.](./logo.png)
# <ins>BlockyOS</ins>
A modular OS featuring to include some interesting features, <br/> It will include the features in **[Features](#Features:)**.

## <ins>Table of Contents</ins>:
- [](#)
- [BlockyOS](#blockyos)
	- [Table of Contents:](#table-of-contents)
	- [Features:](#features)
		- [Features to be included:](#features-to-be-included)
- [Features explained:](#features-explained)
- [Concepts:](#concepts)

## <ins>Features:<ins>
### <ins>Features to be included:</ins>
* **[Dynamic Program loading and sanitation](#Dynamic-Program-Loading-and-Sanitation)**
* **[Static, Dynamic and Automatic Memory control and Allocation](#Memory-Manager)**,
* **[Memory Protection](#Memory-Protection)**
* **[Modular GUI object creation](#Modular-GUI)**,
* **[System calls](#Syscalls)**,
<br><br><br><br>

# Features explained:
* ## **kernel protection:**
	At set intervals a checksum is compiled from a Program's code,
	for Kernel Programs this checksum validates that the Program has not been overwritten, such as by [goonrepo](#goonrepo-priviledge-raisera-string-path-to-locaton-on-disk-int-how-many-tries).
* ## **"grippy" cli(Command Line Interface):**
	- ### Enviroment variables:
		These are short 30 char names that can be used to represent Executable Paths.
		The [Path](./src/Public/Publics.h) struct's Path contains the whole Path.
		A Path executable must contain the signature "_start(uint8_t *args, size_t arglen)" function which will contain args.
	- ### "goonrepo" Priviledge Raiser(a string Path to Locaton on Disk, int how many tries):
		Goonrepo is a **NOT IMPLEMENTED!** Program that will raise a Program's Admin level.
		- ### Arguments:
			- ### SphereOS:
				Oh my gosh, Spheres?!?!?!!!
				So blasphemous.
				Use the **sphos** command with goonrepo for it to clear out RAM,
				killing the OS,
				causing the OS to crash.
			- ### How does it work?
				**<ins>Option 1:</ins>**<br>
				The Program defines a Stream,
				The Buffer is zeroed,
				It takes the returned Pointer and decrements the size_t address, it does this till it meets the starting point of the Stream object **A string Path and a Buffer.**
				The Path is overidden to point to the OS's text(Code), the address is set to point back to the buffer,
				The buffer reads from the grippy cli Executable,
				goonrepo will override the grippy cli in RAM with the inputted Program's code.
				**The Program's size will be capped to how much of the grippy cli has been loaded to RAM.**
				**The cli may crash due to the execution suddenly changing, or being executed whilst being written to, goonrepo may repeat this instruction multiple times until the rewrite has function properly.**
				<br><br><br>
				**<ins>Option 2:</ins>**<br>
				The 


> [!NOTE]
> Remember this cli, It is **NOT IMPLEMENTED!!**

* ## **Memory Manager:**
	- ### Static allocation:
		This will automatically allocate Memory by a prediction of how much Memory a program or a function will use.
	- ### Dynamic allocation:
		This will allow Functions to allocate more data during Runtime, generally this should not be met as **[Static allocation](#static-allocation)** should already allocate enough Memory for the function's needs.
	- ### Automatic Memory control:
		This will run every few **[Render Pages](#Frames)** and free Memory Pages that have not been referenced frequently or **[Un-pointed](#Unpointed-Memory-Control)**.
	- ### Static Memory Control:
		Programs can **<ins>at Runtime</ins>** to Un-point memory via a **[System call](#Syscalls)**, freeing to for other programs to use it.
	- ### Unpointed Memory Control:
		Memory blocks use two properties to handle thier protection and how they are handled, An unsigned integer and a Boolean value.

* ## **Modular GUI:**
	- ### Modularity:
		Objects can be created and rendered, these objects are self-contained and closed Systems.<br>This will allow them to be Modular.

* ## **Syscalls:**
	- ### Exceptions:
		Programs can call pre-defined Exceptions to cause certain events to occur, these Exceptions can take Parameters or will peek into a function's Memory to retrieve expected info, if the expected info is not found an **ACTUAL** exception may be called. 
		* Explicit Execution requires the Psarameters to be inputted as arguments when the function is being called.
		* Implicit Execution does not require the Parameter to be inputted as arguments, the arguments can be loaded into a specific Memory Adress or stored in a variable with both the expected Type and a double dollar sign ($$)  before the name.
		> [!NOTE] 
		> See [Exception List](./src/Public/FunctionInterruptHashes.csv).

<br><br><br>

# <ins>Concepts:</ins>
* ## Kernel Threads:
	The OS comes with a pre-built, binary tree describing each function and it's dependencies.
	This tree can be loaded directly into memory without parsing as it will be constructed from the OSses binaries.
	This tree will define a config to use for isolating functions, this will be the core of spawning secure kernel threads, where the funcction itself will be isolated from the OS.
	The OS will use this to spawn isolated threads that can be considered priviledged however will not beexpected to last, these threads will not be protected, they will be de-spawned if nor used in 10 [Process frames](#frames), the fathering Process/thread is informed of this.

> [!NOTE]
> I use the phrase "spawning bytes" to loosely describe the process of bytes being loaded to memory.
	<br><br>
* ## Headers:
	Headers describe a data block and Context about the data stored there, this simplifies how data blocks are regarded and how they are accessed.
	Headers are not directly stored in RAM, instead the Data is stored in RAM, with the Header's starting address and it's Context stored in RAM.
	* ### Header metadata:
		A block of memory at the end of RAM stores the starting addresses(as size_t pointers) of data blocks in RAM.
		This allows for quic access to the data blocks, whenever neccessary rather than having to search through RAM.
* ## Frames:
	* ### Process Frames:
		Each program has it's performance graded in it's CPU Usage: CPU Frames, and GPU Usage: GPU Frames.
		* CPU Frames are measured in how much of it's time allocation is actually used.
		* GPU Frames are measured in how long it takes for each GPU request to be processed, with the longest times compared to the lowest times.
* ## Modularity:
	* ### OS Compilation:
		Binaries can be loaded into the OS's Program Files, the chenges actually come into effect on next start-up as the Program will be available to load it on Start-Up.
		More information is more available at [OS Programming](./Source/Public/OS-Unique%20syscalls.md).
		In order to maintain integrity **NO** Programs are allowed to edit the Program Files, aside from a specific Program.
* ## Public Data Buffers:
	Data that may be accesssed frequently is stored at publicly available adrresses that are updated on every [Interrupt](#interrupts).
* ## Interrupts:
	100 times per second an Interrupt occurs, allowing Low-Priority Instructions to be run etc.
* ## Dynamic Program loading and sanitation:
	Programs are loaded into RAM, they will have OS-dependent binaries swapped out for a supported alternative.
* ## Streams:
	Streams are read-only Memory buffers, being Kernel write-only Data blocks. They consist of a 

<br><br><br>

* # <ins>Changes:</ins>