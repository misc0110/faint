/*
 * 
 * 01/2016 by Michael Schwarz 
 *
 **/

#ifndef _MALLOC_REPLACE_H_
#define _FAULT_INJECT_H_

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
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
typedef void* (*h_realloc)(void*, size_t);
typedef void* (*h_calloc)(size_t, size_t);
typedef void (*h_abort)(void);
typedef void (*h_exit)(int);
typedef FILE* (*h_fopen)(const char*, const char*);

extern "C" __cxa_eh_globals* __cxa_get_globals();
#endif
