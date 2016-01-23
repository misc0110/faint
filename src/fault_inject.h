///////////////////////////////////////////////////////////////////////////////
//
//    faint - a FAult INjection Tester
//    Copyright (C) 2016  Michael Schwarz
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//    E-Mail: michael.schwarz91@gmail.com
//
///////////////////////////////////////////////////////////////////////////////

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

#define FAIL 0
#define WRAP 1
#define REAL 2

// function signatures of fault injectable functions
typedef void* (*h_malloc)(size_t);
typedef void* (*h_realloc)(void*, size_t);
typedef void* (*h_calloc)(size_t, size_t);
typedef FILE* (*h_fopen)(const char*, const char*);
typedef ssize_t (*h_getline)(char**, size_t*, FILE*);
typedef char* (*h_fgets)(char*, int, FILE*);
typedef size_t (*h_fread)(void*, size_t, size_t, FILE*);
typedef size_t (*h_fwrite)(const void*, size_t, size_t, FILE*);
typedef void (*h_free)(void*);
typedef void (*h_exit)(int);

void segfault_handler(int sig);
void save_heap();

#endif
