# <ins>BlockyOS</ins>
A modular OS featuring to include some interesting features, <br/> It will include the features in **[Features](#Features:)**.

## <ins>Table of Contents</ins>:
* [Features](#Features:)
* [Features Explained](#features-explained)
* [Concepts](#concepts)

## <ins>Features:<ins>
### <ins>Features to be included:</ins>
* **[Dynamic Program loading and sanitation](#Dynamic-Program-Loading-and-Sanitation)**
* **[Static, Dynamic and Automatic Memory control and Allocation](#Memory-Manager)**,
* **[Memory Protection](#Memory-Protection)**
* **[Modular GUI object creation](#Modular-GUI)**,
* **[System calls](#Syscalls)**,
<br><br><br><br>

# Features explained:
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
        > See [Exception List](Source/OS/Metadata/ExcpetionHashTable.csv).

<br><br><br>

# <ins>Concepts:</ins>
* ## Frames:
    * ### Process Frames:
        Each program has it's performance graded in it's CPU Usage: CPU Frames, and GPU Usage: GPU Frames.
        * CPU Frames are measured in how much of it's time allocation is actually used.
        * GPU Frames are measured in how long it takes for each GPU request to be processed, with the longest times compared to the lowest times.
* ## Modularity:
    * ### OS Compilation:
        Binaries can be loaded into the OS's Program Files, the chenges actually come into effect on next start-up as the Program will be available to load it on Start-Up.
        More information is more available at [OS Programming](Metadata/OS%20Programming.md).
        In order to maintain integrity **NO** Programs are allowed to edit the Program Files, aside from a specific Program.
* ## Public Data Buffers:
    Data that may be accesssed frequently is stored at publicly available adrresses that are updated on every [Interrupt](#interrupts).
* ## Interrupts:
    100 times per second an Interrupt occurs, allowing Low-Priority Instructions to be run etc.