/*
 * Shared library to let new fail after a given number of successful new 
 * calls. Used to test out-of-memory handling of programs.
 * 
 * 01/2016 by Michael Schwarz 
 *
 **/ 

#ifndef _MALLOC_REPLACE_H_
#define _MALLOC_REPLACE_H_

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <typeinfo>
#include <cxxabi.h>

#ifndef __USE_GNU
 #define __USE_GNU
#endif

#include <dlfcn.h>


// structs to get name from an exception
struct __cxa_exception {
  std::type_info *inf;
};

struct __cxa_eh_globals {
    struct __cxa_exception *exc;
};

// function signatures of malloc and abort
typedef void* (*h_malloc)(size_t);
typedef void (*h_abort)(void);
typedef void (*h_exit)(int);


extern "C" __cxa_eh_globals* __cxa_get_globals();



#endif
