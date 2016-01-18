/*
 * Shared library to let specific malloc/realloc/calloc/new calls fail
 * Used to test out-of-memory handling of programs.
 * 
 * 01/2016 by Michael Schwarz 
 *
 **/

#include "fault_inject.h"
#include "settings.h"
#include "map.h"

#include <iostream>
#include <string.h>
#include <stdarg.h>

static h_malloc real_malloc = NULL;
static h_abort real_abort = NULL;
static h_exit real_exit = NULL;
static h_exit real_exit1 = NULL;
static h_realloc real_realloc = NULL;
static h_calloc real_calloc = NULL;
static h_fopen real_fopen = NULL;

static unsigned int no_intercept = 0;

void segfault_handler(int sig);

static FaultSettings settings;
map_declare(faults);
map_declare(types);

void* current_malloc = NULL;

/*class NoIntercept {
  private:
    const char* from;
  public:
    NoIntercept(const char* where) {
      from = where;
    }
    void block() {
      __sync_add_and_fetch(&no_intercept, 1);
      printf("^ %d (%s)\n", no_intercept, from);
    }
    void unblock() {
      if(no_intercept > 0) __sync_sub_and_fetch(&no_intercept, 1);
      printf("v %d (%s)\n", no_intercept, from);
    }
};*/

void block() {
  no_intercept++;
}

void unblock() {
  if(no_intercept == 0) {
    printf("Something went wrong with locking!\n");
    return;
  }
  no_intercept--;
}

//-----------------------------------------------------------------------------
static void _init(void) {
  block();
  // read symbols from standard library
  real_malloc = (h_malloc) dlsym(RTLD_NEXT, "malloc");
  real_abort = (h_abort) dlsym(RTLD_NEXT, "abort");
  real_exit = (h_exit) dlsym(RTLD_NEXT, "exit");
  real_exit1 = (h_exit) dlsym(RTLD_NEXT, "_exit");
  real_realloc = (h_realloc) dlsym(RTLD_NEXT, "realloc");
  real_calloc = (h_calloc) dlsym(RTLD_NEXT, "calloc");
  real_fopen = (h_fopen)dlsym(RTLD_NEXT, "fopen");

  if(real_malloc == NULL || real_abort == NULL || real_exit == NULL || real_exit1 == NULL || real_realloc == NULL
      || real_calloc == NULL || real_fopen == NULL) {
    fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
    unblock();
    return;
  }

  //no_intercept = 1;
  // read settings from file
  FILE *f = real_fopen("settings", "rb");
  if(f) {
    fread(&settings, sizeof(FaultSettings), 1, f);
    fclose(f);
  }
  //printf("-- Mode: %d\n", settings.mode);
  if(settings.mode == INJECT) {
    if(!faults)
      map_initialize(faults, MAP_GENERAL);
    if(!types)
      map_initialize(types, MAP_GENERAL);

    // read profile
    f = real_fopen("profile", "rb");
    if(f) {
      int entry = 0;
      while(!feof(f)) {
        void* addr;
        void* cnt;
        void* type;
        fread(&addr, sizeof(void*), 1, f);
        fread(&cnt, sizeof(void*), 1, f);
        if(fread(&type, sizeof(void*), 1, f) == 0)
          break;
        //printf("%x -> %d\n", addr, cnt);
        if(entry == settings.limit)
          cnt = (void*) 1;
        else
          cnt = (void*) 0;
        map(faults)->set(addr, cnt);
        map(types)->set(addr, type);
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
size_t get_module_id(const char* module) {
  size_t i;
  for(i = 0; i < MODULE_COUNT; i++) {
    if(!strcmp(module, modules[i])) {
      return i;
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
int module_active(const char* module) {
  int id = get_module_id(module);
  if(id == -1)
    return 0;
  return !!(settings.modules & (1 << id));
}

//-----------------------------------------------------------------------------
void* _malloc(size_t size) {
  return real_malloc(size);
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
  //NoIntercept n;
  // get callee address
  //no_intercept = 1;
  if(!no_intercept) {
    printf("WTF? Can't intercept tracing! (%s)\n", type);
    return;
  }
  void* p = get_return_address(0); //__builtin_return_address(1);

  //printf("0x%x\n", p);

  Dl_info info;
  if(dladdr(p, &info)) {
    //printf("%s\n", info.dli_fname);
    if(info.dli_fname && strcmp(info.dli_fname, settings.filename) != 0) {
      // not our file
      //no_intercept = 0;
      return;
    }
  }

  if(!faults)
    map_initialize(faults, MAP_GENERAL);
  if(!types)
    map_initialize(types, MAP_GENERAL);

  if(map(faults)->has(p)) {
    map(faults)->set(p, (void*) ((size_t) (map(faults)->get(p)) + 1));
  } else {
    map(faults)->set(p, (void*) 1);
  }
  map(types)->set(p, (void*) get_module_id(type));

  FILE* f = real_fopen("profile", "wb");
  cmap_iterator* it = map(faults)->iterator();
  while(!map_iterator(it)->end()) {
    void* k = map_iterator(it)->key();
    void* v = map_iterator(it)->value();
    fwrite(&k, sizeof(void*), 1, f);
    fwrite(&v, sizeof(void*), 1, f);
    void* type = map(types)->get(k);
    fwrite(&type, sizeof(void*), 1, f);
    map_iterator(it)->next();
  }
  map_iterator(it)->destroy();
  fclose(f);
  //no_intercept = 0;
}


//-----------------------------------------------------------------------------
template <typename T>
int handle_inject(const char* name, T* function) {
  if(*function == NULL) {
    block();
    _init();
    unblock();
  }

  if(!module_active(name) || no_intercept) {
    return 0;
  }

  void* addr = get_return_address(0);
  current_malloc = addr;

  if(settings.mode == PROFILE) {
    block();
    save_trace(name);
    unblock();
    return 0;
  } else if(settings.mode == INJECT) {
    if(!map(faults)->has(addr)) {
      printf("strange, %p was not profiled\n", addr);
      return 0;
    } else {
      if(map(faults)->get(addr)) {
        // let it fail
        return 1;
      } else {
        // real function
        return 0;
      }
    }
  }
  // don't know what to do
  return 0;
}

//-----------------------------------------------------------------------------
void *malloc(size_t size) {
  if(handle_inject<h_malloc>("malloc", &real_malloc)) {
    return NULL;
  } else {
    block();
    void* addr = real_malloc(size);
    unblock();
    return addr;
  }
}

//-----------------------------------------------------------------------------
void *realloc(void* mem, size_t size) {
  if(handle_inject<h_realloc>("realloc", &real_realloc)) {
    return NULL;
  } else {
    block();
    void* addr = real_realloc(mem, size);
    unblock();
    return addr;
  }
}

//-----------------------------------------------------------------------------
void *calloc(size_t elem, size_t size) {
  if(handle_inject<h_calloc>("calloc", &real_calloc)) {
    return NULL;
  } else {
    block();
    void* addr = real_calloc(elem, size);
    unblock();
    return addr;
  }
}

//-----------------------------------------------------------------------------
void* operator new(size_t size) {
  if(handle_inject<h_malloc>("new", &real_malloc)) {
    throw std::bad_alloc();
    return NULL;
  } else {
    block();
    void* addr = real_malloc(size);
    unblock();
    return addr;
  }
}

//-----------------------------------------------------------------------------
FILE *fopen(const char* name, const char* mode) {
  if(handle_inject<h_fopen>("fopen", &real_fopen)) {
    return NULL;
  } else {
    block();
    FILE* f = real_fopen(name, mode);
    unblock();
    return f;
  }
}

//-----------------------------------------------------------------------------
void exit(int val) {
  if(real_exit == NULL)
    _init();
  real_exit(val);
  while(1) {
    // to suppress gcc warning
  }
}

//-----------------------------------------------------------------------------
void _exit(int val) {
  if(real_exit1 == NULL)
    _init();
  real_exit1(val);
}

//-----------------------------------------------------------------------------
void segfault_handler(int sig) {
  block();

  void* crash_addr = NULL;

  // find crash address in backtrace
  crash_addr = get_return_address(0);

  // write crash report
  FILE* f = real_fopen("crash", "wb");
  fwrite(&current_malloc, sizeof(void*), 1, f);
  fwrite(&crash_addr, sizeof(void*), 1, f);
  fclose(f);
  unblock();
  real_exit1(sig + 128);
}

