FAINT - FAult INjection Tester
=========

FAINT is a fully automated tool to dynamically check the out of memory handling in C and C++ programs. 
It injects errors in `malloc`, `calloc`, `realloc`, `new` and `new[]` and reports crashes caused by these faults. 

# Usage

`./faint <binary> [additional parameters]`
 
FAINT is used with the binary file and does not need the source code. Its usage is similar to `valgrind`.

# Mode of operation

FAINT uses `LD_PRELOAD` to replace `malloc`, `calloc`, `realloc`, `new` and `new[]` with its own versions. 

First, the profiling phase starts. With the replaced functions, FAINT runs the binary and traces the calls to these functions. 
This information is then used for the fault injection phase. 

In the fault injection phase, the binary is executed multiple times. In every run of the binary, a different memory allocation is simulated to fail (returning `NULL` in C and throwing an exception in C++). 
A signal handler for segmentation faults (SIGSEGV) and aborts (SIGABRT) catches crashes and saves them to a crash log. In this log, the address of the faulty malloc and the address of the crashing instruction is saved. 

The addresses in the crash report are converted to function names and line numbers using `addr2line`. This only works if the examined program is compiled with the -g switch. 

After injecting all faults, the program presents a summary of all crashes and their details. 

To ensure that the addresses stay the same over multiple runs, ASLR is deactivated by the program. 

# Example

There are two samples included, one written in C and one in C++. Execute `make run` or `make runcpp` respectively to examine those samples using faint. The output of `make run` should look similar to the following log:

    [ FAINT ] Starting, Version 1.0
    [ FAINT ] 
    [ FAINT ] Binary: ./faint
    [ FAINT ] 
    [ FAINT ] ASLR turned off successfully
    [ FAINT ] Profiling start
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
    [ FAINT ] Profiling done
    [ FAINT ] Found 4 different mallocs with 31 call(s)
    [ FAINT ]  >  helper in test.c line 9: 10 calls
    [ FAINT ]  >  do_mem in test.c line 24: 1 calls
    [ FAINT ]  >  helper in test.c line 6: 10 calls
    [ FAINT ]  >  do_mem in test.c line 20: 10 calls
    [ FAINT ] 
    [ FAINT ] Injecting 4 faults, one for every malloc
    [ FAINT ] 
    [ FAINT ] 
    [ FAINT ] 
    [ FAINT ] Inject fault #1
    [ FAINT ] Fault position:
    [ FAINT ]  >  helper in test.c line 9
    [ FAINT ] 
    Helper
    [ FAINT ] Exited, status: 139 (Segmentation fault)
    [ FAINT ] Crashed at 0x4006af, caused by 0x4006a3
    [ FAINT ] Crash details: 
    [ FAINT ]  > Crash: helper (test.c) @ 10
    [ FAINT ]  > Malloc: helper (test.c) @ 9
    [ FAINT ] Injection #1 done
    [ FAINT ] 
    [ FAINT ] 
    [ FAINT ] Inject fault #2
    [ FAINT ] Fault position:
    [ FAINT ]  >  do_mem in test.c line 24
    [ FAINT ] 
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
    [ FAINT ] Crashed at 0x400780, caused by 0x40074b
    [ FAINT ] Crash details: 
    [ FAINT ]  > Crash: main (test.c) @ 32
    [ FAINT ]  > Malloc: do_mem (test.c) @ 24
    [ FAINT ] Injection #2 done
    [ FAINT ] 
    [ FAINT ] 
    [ FAINT ] Inject fault #3
    [ FAINT ] Fault position:
    [ FAINT ]  >  helper in test.c line 6
    [ FAINT ] 
    Helper
    Malloc (helper) failed
    [ FAINT ] Exited, status: 139 (Segmentation fault)
    [ FAINT ] Crashed at 0x4006ed, caused by 0x4006a3
    [ FAINT ] Crash details: 
    [ FAINT ]  > Crash: do_mem (test.c) @ 19
    [ FAINT ]  > Malloc: helper (test.c) @ 9
    [ FAINT ] Injection #3 done
    [ FAINT ] 
    [ FAINT ] 
    [ FAINT ] Inject fault #4
    [ FAINT ] Fault position:
    [ FAINT ]  >  do_mem in test.c line 20
    [ FAINT ] 
    Helper
    [ FAINT ] Exited, status: 139 (Segmentation fault)
    [ FAINT ] Crashed at 0x40071c, caused by 0x400714
    [ FAINT ] Crash details: 
    [ FAINT ]  > Crash: do_mem (test.c) @ 21
    [ FAINT ]  > Malloc: do_mem (test.c) @ 20
    [ FAINT ] Injection #4 done
    [ FAINT ] 
    [ FAINT ] ======= SUMMARY =======
    [ FAINT ] 
    [ FAINT ] Crashed at 4 from 4 injections
    [ FAINT ] Unique crashes: 4
    [ FAINT ] 
    [ FAINT ] Crash details:
    [ FAINT ] 
    [ FAINT ] Crashed at 0x400780, caused by 0x40074b
    [ FAINT ]    > Crash: main (test.c) line 32
    [ FAINT ]    > Malloc: do_mem (test.c) line 24
    [ FAINT ] 
    [ FAINT ] Crashed at 0x40071c, caused by 0x400714
    [ FAINT ]    > Crash: do_mem (test.c) line 21
    [ FAINT ]    > Malloc: do_mem (test.c) line 20
    [ FAINT ] 
    [ FAINT ] Crashed at 0x4006ed, caused by 0x4006a3
    [ FAINT ]    > Crash: do_mem (test.c) line 19
    [ FAINT ]    > Malloc: helper (test.c) line 9
    [ FAINT ] 
    [ FAINT ] Crashed at 0x4006af, caused by 0x4006a3
    [ FAINT ]    > Crash: helper (test.c) line 10
    [ FAINT ]    > Malloc: helper (test.c) line 9
    [ FAINT ] 
    [ FAINT ] 
    [ FAINT ] finished successfully!

    