FAINT - FAult INjection Tester
=========

FAINT is a fully automated tool to dynamically check the out of memory handling in C and C++ programs. 
It injects errors amongst others in `malloc`, `calloc`, `realloc`, `new` and `new[]` and reports crashes caused by these faults. 

# Usage

`./faint [faint-options] [your-program] [your-program-options]`
 
FAINT is used with the binary file and does not need the source code. Its usage is similar to `valgrind`. For usage details see `man faint` or `faint`.

# Mode of operation

FAINT uses `LD_PRELOAD` to replace `malloc`, `calloc`, `realloc`, `new` and `new[]` with its own versions. 

First, the profiling phase starts. With the replaced functions, FAINT runs the binary and traces the calls to these functions. 
This information is then used for the fault injection phase. 

In the fault injection phase, the binary is executed multiple times. In every run of the binary, a different memory allocation is simulated to fail (returning `NULL` in C and throwing an exception in C++). 
A signal handler for segmentation faults (SIGSEGV) and aborts (SIGABRT) catches crashes and saves them to a crash log. In this log, the address of the faulty malloc and the address of the crashing instruction is saved. 

The addresses in the crash report are converted to function names and line numbers using `addr2line`. This only works if the examined program is compiled with the -g switch. 

After injecting all faults, the program presents a summary of all crashes and their details. 

To ensure that the addresses stay the same over multiple runs, ASLR is deactivated by the program. 

# Building

Needs `gcc-4.9-multilib` and `g++-4.9-multilib` for cross-compiling the 32bit library.

To build, simply `make && sudo make install`. The binary is called `faint`. 

# Example

There are two samples included, one written in C and one in C++. Execute `make run` or `make runcpp` respectively to examine those samples using faint. The output of `make run` should look similar to the following log:

    [ FAINT ] Activate module 'malloc'
    [ FAINT ] Activate module 'realloc'
    [ FAINT ] Activate module 'calloc'
    [ FAINT ] Activate module 'new'
    [ FAINT ] 
    [ FAINT ] 
    [ FAINT ] Starting, Version 0.3
    [ FAINT ] 
    [ FAINT ] Binary: ./bin/test (64 bit)
    [ FAINT ] 
    [ FAINT ] ASLR turned off successfully
    [ FAINT ] Profiling start
    hallo
    Helper
    Helper
    Helper
    Helper
    Helper
    Helper
    Helper
    Helper
    Helper
    Helper
    WUT?
    [ FAINT ] Profiling done
    [ FAINT ] Found 5 different injection positions with 32 call(s)
    [ FAINT ]  >  [calloc] helper in test.c line 10: 10 calls
    [ FAINT ]  >  [realloc] do_mem in test.c line 21: 10 calls
    [ FAINT ]  >  [malloc] main in test.c line 32: 1 calls
    [ FAINT ]  >  [malloc] do_mem in test.c line 25: 1 calls
    [ FAINT ]  >  [malloc] helper in test.c line 7: 10 calls
    [ FAINT ] 
    [ FAINT ] Injecting 5 faults, one for every injection position
    [ FAINT ] 
    [ FAINT ] 
    [ FAINT ] Inject fault #1
    [ FAINT ] Fault position:
    [ FAINT ]  >  [calloc] helper in test.c line 10
    [ FAINT ] 
    hallo
    Helper
    [ FAINT ] Exited, status: 139 (Segmentation fault)
    [ FAINT ] Crashed at 0x40091f, caused by 0x400913 [calloc]
    [ FAINT ]   > crash: helper (test.c) line 11
    [ FAINT ]   > calloc: helper (test.c) line 10
    [ FAINT ] Injection #1 done
    [ FAINT ] 
    [ FAINT ] 
    [ FAINT ] Inject fault #2
    [ FAINT ] Fault position:
    [ FAINT ]  >  [realloc] do_mem in test.c line 21
    [ FAINT ] 
    hallo
    Helper
    [ FAINT ] Exited, status: 139 (Segmentation fault)
    [ FAINT ] Crashed at 0x40098c, caused by 0x400984 [realloc]
    [ FAINT ]   > crash: do_mem (test.c) line 22
    [ FAINT ]   > realloc: do_mem (test.c) line 21
    [ FAINT ] Injection #2 done
    [ FAINT ] 
    [ FAINT ] 
    [ FAINT ] Inject fault #3
    [ FAINT ] Fault position:
    [ FAINT ]  >  [malloc] main in test.c line 32
    [ FAINT ] 
    [ FAINT ] Exited, status: 139 (Segmentation fault)
    [ FAINT ] Crashed at 0x400a10, caused by 0x4009fa [malloc]
    [ FAINT ]   > crash: main (test.c) line 35
    [ FAINT ]   > malloc: main (test.c) line 32
    [ FAINT ] Injection #3 done
    [ FAINT ] 
    [ FAINT ] 
    [ FAINT ] Inject fault #4
    [ FAINT ] Fault position:
    [ FAINT ]  >  [malloc] do_mem in test.c line 25
    [ FAINT ] 
    hallo
    Helper
    Helper
    Helper
    Helper
    Helper
    Helper
    Helper
    Helper
    Helper
    Helper
    Malloc (do_mem) failed
    [ FAINT ] Exited, status: 139 (Segmentation fault)
    [ FAINT ] Crashed at 0x400a28, caused by 0x4009bb [malloc]
    [ FAINT ]   > crash: main (test.c) line 36
    [ FAINT ]   > malloc: do_mem (test.c) line 25
    [ FAINT ] Injection #4 done
    [ FAINT ] 
    [ FAINT ] 
    [ FAINT ] Inject fault #5
    [ FAINT ] Fault position:
    [ FAINT ]  >  [malloc] helper in test.c line 7
    [ FAINT ] 
    hallo
    Helper
    Malloc (helper) failed
    [ FAINT ] Exited, status: 139 (Segmentation fault)
    [ FAINT ] Crashed at 0x40095d, caused by 0x400913 [calloc]
    [ FAINT ]   > crash: do_mem (test.c) line 20
    [ FAINT ]   > calloc: helper (test.c) line 10
    [ FAINT ] Injection #5 done
    [ FAINT ] 
    [ FAINT ] ======= SUMMARY =======
    [ FAINT ] 
    [ FAINT ] Crashed at 5 from 5 injections
    [ FAINT ] Unique crashes: 5
    [ FAINT ] 
    [ FAINT ] Crash details:
    [ FAINT ] 
    [ FAINT ] Crashed at 0x400a10, caused by 0x4009fa [malloc]
    [ FAINT ]   > crash: main (test.c) line 35
    [ FAINT ]   > malloc: main (test.c) line 32
    [ FAINT ] 
    [ FAINT ] Crashed at 0x400a28, caused by 0x4009bb [malloc]
    [ FAINT ]   > crash: main (test.c) line 36
    [ FAINT ]   > malloc: do_mem (test.c) line 25
    [ FAINT ] 
    [ FAINT ] Crashed at 0x40098c, caused by 0x400984 [realloc]
    [ FAINT ]   > crash: do_mem (test.c) line 22
    [ FAINT ]   > realloc: do_mem (test.c) line 21
    [ FAINT ] 
    [ FAINT ] Crashed at 0x40095d, caused by 0x400913 [calloc]
    [ FAINT ]   > crash: do_mem (test.c) line 20
    [ FAINT ]   > calloc: helper (test.c) line 10
    [ FAINT ] 
    [ FAINT ] Crashed at 0x40091f, caused by 0x400913 [calloc]
    [ FAINT ]   > crash: helper (test.c) line 11
    [ FAINT ]   > calloc: helper (test.c) line 10
    [ FAINT ] 
    [ FAINT ] 
    [ FAINT ] finished successfully!

    