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

#include "fault_inject.h"
#include "settings.h"
#include "map.h"
#include "modules.h"

#include <iostream>
#include <string.h>
#include <stdarg.h>

static h_malloc real_malloc = NULL;
static h_realloc real_realloc = NULL;
static h_calloc real_calloc = NULL;
static h_fopen real_fopen = NULL;
static h_getline real_getline = NULL;
static h_fgets real_fgets = NULL;
static h_fread real_fread = NULL;
static h_fwrite real_fwrite = NULL;
static h_exit real_exit = NULL;
static h_exit real_exit_ = NULL;
static h_free real_free = NULL;

static unsigned int no_intercept = 0;

static FaultSettings settings;
static map_declare(faults);

static map_declare(types);

static map_declare(heap);
static map_declare(heap_location);

static void* current_fault = NULL;

static int init_done = 0;

//-----------------------------------------------------------------------------
void block() {
  no_intercept++;
}

//-----------------------------------------------------------------------------
void unblock() {
  if(no_intercept == 0) {
    printf("Something went wrong with locking!\n");
    return;
  }
  no_intercept--;
}

//-----------------------------------------------------------------------------
class NoIntercept {
  private:
  public:
    NoIntercept() {
      block();
    }
    ~NoIntercept() {
      unblock();
    }
};

//-----------------------------------------------------------------------------
static void _init(void) {
  block();

  // read non-intercepted function handles
  real_exit = (h_exit) dlsym(RTLD_NEXT, "exit");
  real_exit_ = (h_exit) dlsym(RTLD_NEXT, "_exit");
  real_free = (h_free) dlsym(RTLD_NEXT, "free");
  if(!real_exit || !real_exit_) {
    printf("Error getting 'exit'\n");
  }
  if(!real_free) {
    printf("Error getting 'free'\n");
  }

  // read settings from file
  FILE *f = fopen("settings", "rb");
  if(f) {
    fread(&settings, sizeof(FaultSettings), 1, f);
    fclose(f);
  }

  if(!heap)
    map_initialize(heap, MAP_GENERAL);
  if(!heap_location)
    map_initialize(heap_location, MAP_GENERAL);

  if(settings.mode == INJECT) {
    if(!faults)
      map_initialize(faults, MAP_GENERAL);
    if(!types)
      map_initialize(types, MAP_GENERAL);

    // read profile
    f = fopen("profile", "rb");
    if(f) {
      int entry = 0;
      while(!feof(f)) {
        ProfileEntry e;
        if(fread(&e, sizeof(ProfileEntry), 1, f) == 0)
          break;
        if(entry == settings.limit)
          e.count = 1;
        else
          e.count = 0;
        map(faults)->set((void*) e.address, (void*) e.count);
        map(types)->set((void*) e.address, (void*) e.type);
        entry++;
      }
    }
    fclose(f);
  }

  // install signal handler
  struct sigaction sig_handler;

  sig_handler.sa_handler = segfault_handler;
  sigemptyset(&sig_handler.sa_mask);
  sig_handler.sa_flags = 0;
  sigaction(SIGINT, &sig_handler, NULL);
  sigaction(SIGSEGV, &sig_handler, NULL);
  sigaction(SIGABRT, &sig_handler, NULL);

  unblock();
}

//-----------------------------------------------------------------------------
template<typename T>
void init(const char* name, T* function) {
  NoIntercept n;
  *function = (T) dlsym(RTLD_NEXT, name);
  if(!*function) {
    fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
    fprintf(stderr, "Cannot find function '%s'\n", name);
    return;
  }

  if(!init_done) {
    init_done = 1;
    _init();
  }
}

//-----------------------------------------------------------------------------
int module_active(const char* module) {
  int id = get_module_id(module);
  if(id == -1)
    return 0;
  return !!(settings.modules & (1 << id));
}

//-----------------------------------------------------------------------------
void print_backtrace() {
  int j, nptrs;
  void *buffer[100];
  char **strings;

  nptrs = backtrace(buffer, 100);
  strings = backtrace_symbols(buffer, nptrs);
  if(strings) {
    for(j = 0; j < nptrs; j++) {
      printf("[bt] %s\n", strings[j]);
    }
  }
  free(strings);
}

//-----------------------------------------------------------------------------
void* get_return_address(int index) {
  block();

  int j, nptrs;
  void *buffer[100];
  char **strings;
  void* addr = NULL;

  nptrs = backtrace(buffer, 100);
  strings = backtrace_symbols(buffer, nptrs);
  if(strings) {
    for(j = 0; j < nptrs; j++) {
      if(strncmp(strings[j], settings.filename, strlen(settings.filename)) == 0) {
        if(index) {
          //printf("skip\n");
          index--;
          continue;
        }
        addr = buffer[j];
        break;
      } else {
        //printf("skip: %s\n", strings[j]);
      }
    }
    free(strings);
  }
  unblock();
  return addr;
}

//-----------------------------------------------------------------------------
void save_trace(const char* type) {
  if(!no_intercept) {
    printf("Error locking tracing! (%s)\n", type);
    return;
  }
  void* caller = get_return_address(0);

  Dl_info info;
  if(dladdr(caller, &info)) {
    if(info.dli_fname && strcmp(info.dli_fname, settings.filename) != 0) {
      // not our file
      return;
    }
  }

  if(!faults)
    map_initialize(faults, MAP_GENERAL);
  if(!types)
    map_initialize(types, MAP_GENERAL);

  if(map(faults)->has(caller)) {
    map(faults)->set(caller, (void*) ((size_t) (map(faults)->get(caller)) + 1));
  } else {
    map(faults)->set(caller, (void*) 1);
  }
  map(types)->set(caller, (void*) get_module_id(type));

  FILE* f = fopen("profile", "wb");
  cmap_iterator* it = map(faults)->iterator();
  while(!map_iterator(it)->end()) {
    void* k = map_iterator(it)->key();
    void* v = map_iterator(it)->value();
    ProfileEntry e;
    e.address = (uint64_t) k;
    e.count = (uint64_t) v;
    e.type = (uint64_t) map(types)->get(k);

    fwrite(&e, sizeof(ProfileEntry), 1, f);
    map_iterator(it)->next();
  }
  map_iterator(it)->destroy();
  fclose(f);
}

//-----------------------------------------------------------------------------
int is_valgrind() {
  block();

  int j, nptrs, val = 0;
  void *buffer[100];
  char **strings;

  nptrs = backtrace(buffer, 100);
  strings = backtrace_symbols(buffer, nptrs);
  if(strings) {
    for(j = 0; j < nptrs; j++) {
      if(strncmp(strings[j], settings.filename, strlen(settings.filename)) == 0) {
        break;
      }
      if(strncmp(strings[j], "/usr/bin/valgrind", strlen("/usr/bin/valgrind")) == 0) {
        val = 1;
        break;
      }
    }
    free(strings);
  }
  unblock();
  return val;
}

//-----------------------------------------------------------------------------
template<typename T>
int handle_inject(const char* name, T* function, const char* tracename) {
  if(*function == NULL) {
    NoIntercept n;
    init<T>(name, function);
  }

  if(!module_active(name) || no_intercept || is_valgrind()) {
    return REAL;
  }

  void* addr = get_return_address(0);
  current_fault = addr;

  if(settings.mode == PROFILE) {
    NoIntercept n;
    save_trace(tracename);
    return WRAP;
  } else if(settings.mode == INJECT) {
    if(!addr)
      return REAL;
    if(!map(faults)->has(addr)) {
      printf("strange, %p was not profiled\n", addr);
      return REAL;
    } else {
      if(map(faults)->get(addr)) {
        // let it fail
        return FAIL;
      } else {
        // real function
        return WRAP;
      }
    }
  }
  // don't know what to do
  return REAL;
}

//-----------------------------------------------------------------------------
template<typename T>
int handle_inject(const char* name, T* function) {
  return handle_inject(name, function, name);
}

//-----------------------------------------------------------------------------
void *malloc(size_t size) {
  int res;
  if((res = handle_inject<h_malloc>("malloc", &real_malloc)) == FAIL) {
    return NULL;
  } else {
    NoIntercept n;
    void* addr = real_malloc(size);
    if(res == WRAP && settings.trace_heap && get_return_address(0)) {
      map(heap)->set(addr, (void*) size);
      map(heap_location)->set(addr, get_return_address(0));
      save_heap();
    }
    return addr;
  }
}

//-----------------------------------------------------------------------------
void *realloc(void* mem, size_t size) {
  int res;
  if((res = handle_inject<h_realloc>("realloc", &real_realloc)) == FAIL) {
    return NULL;
  } else {
    NoIntercept n;
    void* addr = real_realloc(mem, size);
    if(res == WRAP && settings.trace_heap && get_return_address(0)) {
      map(heap)->unset(mem);
      map(heap_location)->unset(mem);
      map(heap)->set(addr, (void*) size);
      map(heap_location)->set(addr, get_return_address(0));
      save_heap();
    }
    return addr;
  }
}

//-----------------------------------------------------------------------------
void *calloc(size_t elem, size_t size) {
  int res;
  if((res = handle_inject<h_calloc>("calloc", &real_calloc)) == FAIL) {
    return NULL;
  } else {
    NoIntercept n;
    void* addr = real_calloc(elem, size);
    if(res == WRAP && settings.trace_heap && get_return_address(0)) {
      map(heap)->set(addr, (void*) size);
      map(heap_location)->set(addr, get_return_address(0));
      save_heap();
    }
    return addr;
  }
}

//-----------------------------------------------------------------------------
void* operator new(size_t size) {
  int res;
  if((res = handle_inject<h_malloc>("malloc", &real_malloc, "new")) == FAIL) {
    throw std::bad_alloc();
    return NULL;
  } else {
    NoIntercept n;
    void* addr = real_malloc(size);
    if(res == WRAP && settings.trace_heap && get_return_address(0)) {
      map(heap)->set(addr, (void*) size);
      map(heap_location)->set(addr, get_return_address(0));
      save_heap();
    }
    return addr;
  }
}

//-----------------------------------------------------------------------------
void free(void* addr) {
  if(!real_free)
    _init();

  if(no_intercept || is_valgrind())
    return real_free(addr);
  else {
    NoIntercept n;
    if(settings.trace_heap && get_return_address(0)) {
      map(heap)->unset(addr);
      map(heap_location)->unset(addr);
      save_heap();
    }
    return real_free(addr);
  }
}

//-----------------------------------------------------------------------------
void operator delete(void* addr) {
  if(!real_free)
    _init();

  if(no_intercept || is_valgrind())
    return real_free(addr);
  else {
    NoIntercept n;
    if(settings.trace_heap && get_return_address(0)) {
      map(heap)->unset(addr);
      map(heap_location)->unset(addr);
      save_heap();
    }
    return real_free(addr);
  }
}

//-----------------------------------------------------------------------------
FILE *fopen(const char* name, const char* mode) {
  if(!handle_inject<h_fopen>("fopen", &real_fopen)) {
    return NULL;
  } else {
    NoIntercept n;
    return real_fopen(name, mode);
  }
}

//-----------------------------------------------------------------------------
ssize_t getline(char** lineptr, size_t* len, FILE* stream) {
  if(!handle_inject<h_getline>("getline", &real_getline)) {
    return -1;
  } else {
    NoIntercept n;
    return real_getline(lineptr, len, stream);
  }
}

//-----------------------------------------------------------------------------
char* fgets(char* buffer, int size, FILE* f) {
  if(!handle_inject<h_fgets>("fgets", &real_fgets)) {
    return NULL;
  } else {
    NoIntercept n;
    return real_fgets(buffer, size, f);
  }
}

//-----------------------------------------------------------------------------
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  if(!handle_inject<h_fread>("fread", &real_fread)) {
    return 0;
  } else {
    NoIntercept n;
    return real_fread(ptr, size, nmemb, stream);
  }
}

//-----------------------------------------------------------------------------
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
  if(!handle_inject<h_fwrite>("fwrite", &real_fwrite)) {
    return 0;
  } else {
    NoIntercept n;
    return real_fwrite(ptr, size, nmemb, stream);
  }
}

//-----------------------------------------------------------------------------
void exit(int status) {
  if(!real_exit)
    _init();

  if(settings.trace_heap)
    save_heap();
  real_exit(status);
  while(1) {
    // to prevent gcc warning
  }
}

//-----------------------------------------------------------------------------
void _exit(int status) {
  if(!real_exit_)
    _init();

  if(settings.trace_heap)
    save_heap();
  real_exit_(status);
  while(1) {
    // to prevent gcc warning
  }
}

//-----------------------------------------------------------------------------
void save_heap() {
  NoIntercept n;

  cmap_iterator* it = map(heap)->iterator();
  FILE* f = fopen("heap", "wb");
  while(!map_iterator(it)->end()) {
    HeapEntry h;
    void* addr = map_iterator(it)->key();
    h.address = (uint64_t) map(heap_location)->get(addr);
    h.size = (uint64_t) map_iterator(it)->value();
    fwrite(&h, sizeof(HeapEntry), 1, f);
    map_iterator(it)->next();
  }
  map_iterator(it)->destroy();
  fclose(f);
}

//-----------------------------------------------------------------------------
void segfault_handler(int sig) {
  block();

  // write crash report
  FILE* f = fopen("crash", "wb");
  CrashEntry e;
  e.crash = (uint64_t) get_return_address(0);
  e.fault = (uint64_t) current_fault;

  fwrite(&e, sizeof(CrashEntry), 1, f);
  fclose(f);

  unblock();
  exit(sig + 128);
}

