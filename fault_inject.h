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

// function signatures of fault injectable functions
typedef void* (*h_malloc)(size_t);
typedef void* (*h_realloc)(void*, size_t);
typedef void* (*h_calloc)(size_t, size_t);
typedef void (*h_abort)(void);
typedef void (*h_exit)(int);
typedef FILE* (*h_fopen)(const char*, const char*);
typedef ssize_t (*h_getline)(char**, size_t*, FILE*);
typedef char* (*h_fgets)(char*, int, FILE*);
typedef size_t (*h_fread)(void*, size_t, size_t, FILE*);
typedef size_t (*h_fwrite)(const void*, size_t, size_t, FILE*);

#endif
