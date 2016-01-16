/*
 * Shared library to let specific malloc calls fail
 * Used to test out-of-memory handling of programs.
 * 
 * 01/2016 by Michael Schwarz 
 *
 **/ 

#include <string.h>
#include "malloc_replace.h"
#include "settings.h"
#include "map.h"

static h_malloc real_malloc = NULL;
static h_abort real_abort = NULL;
static h_exit real_exit = NULL;
static h_exit real_exit1 = NULL;
static h_realloc real_realloc = NULL;
static h_calloc real_calloc = NULL;

static unsigned int is_backtrace = 0;

void segfault_handler(int sig);

static MallocSettings settings;
map_declare(mallocs);

void* current_malloc = NULL;

//-----------------------------------------------------------------------------
static void _init(void)
{
    // read symbols from standard library
    real_malloc = (h_malloc)dlsym(RTLD_NEXT, "malloc");
    real_abort = (h_abort)dlsym(RTLD_NEXT, "abort");
    real_exit = (h_exit)dlsym(RTLD_NEXT, "exit");
    real_exit1 = (h_exit)dlsym(RTLD_NEXT, "_exit");
    real_realloc = (h_realloc)dlsym(RTLD_NEXT, "realloc");
    real_calloc = (h_calloc)dlsym(RTLD_NEXT, "calloc");
    
    if (real_malloc == NULL || real_abort == NULL || real_exit == NULL || real_exit1 == NULL || real_realloc == NULL || real_calloc == NULL) 
    {
      fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
      return;
    }
        
    is_backtrace = 1;
    // read settings from file
    FILE *f = fopen("settings", "rb");
    if(f) 
    {
      fread(&settings, sizeof(MallocSettings), 1, f);
      fclose(f);
    }  
    //printf("-- Mode: %d\n", settings.mode);
    if(settings.mode == INJECT) {
        if(!mallocs) map_initialize(mallocs, MAP_GENERAL);

        // read profile
        f = fopen("profile", "rb");
        if(f) {
            int entry = 0;
            while(!feof(f)) {
              void* addr;
              void* cnt;
              fread(&addr, sizeof(void*), 1, f);
              if(fread(&cnt, sizeof(void*), 1, f) == 0) break;
              //printf("%x -> %d\n", addr, cnt);
              if(entry == settings.limit) cnt = (void*)1; else cnt = (void*)0;
              map(mallocs)->set(addr, cnt);
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

    is_backtrace = 0;
}

//-----------------------------------------------------------------------------
void* _malloc(size_t size) {
  return real_malloc(size);   
}


//-----------------------------------------------------------------------------
void save_trace()
{
        // get callee address
        void* p = __builtin_return_address(1);
        is_backtrace = 1;

        //printf("0x%x\n", p);
        
        Dl_info info;
        if(dladdr(p, &info)) {
            //printf("%s\n", info.dli_fname);
            if(info.dli_fname && strcmp(info.dli_fname, settings.filename) != 0) {
                // not our file
                return;
            }
        }
        
        if(!mallocs) map_initialize(mallocs, MAP_GENERAL);
        
        if(map(mallocs)->has(p)) {
          map(mallocs)->set(p, (void*)((size_t)(map(mallocs)->get(p)) + 1));   
        } else {
          map(mallocs)->set(p, (void*)1);
        }
        
        FILE* f = fopen("profile", "wb");
        cmap_iterator* it = map(mallocs)->iterator();
        while(!map_iterator(it)->end()) {
            void* k = map_iterator(it)->key();
            void* v = map_iterator(it)->value();
            fwrite(&k, sizeof(void*), 1, f);
            fwrite(&v, sizeof(void*), 1, f);
            map_iterator(it)->next();
        }
        map_iterator(it)->destroy();
        fclose(f);
        is_backtrace = 0;
}
//-----------------------------------------------------------------------------
void *malloc(size_t size)
{
  if(real_malloc == NULL) _init();
  
  if(is_backtrace) {
      return real_malloc(size);
  }

  void* addr = __builtin_return_address(0);
  current_malloc = addr;
  
  if(settings.mode == PROFILE ) {
    save_trace();
    return real_malloc(size);
  } else if(settings.mode == INJECT) {
    if(!map(mallocs)->has(addr)) {
      printf("strange, %p was not profiled\n", addr);   
      return real_malloc(size);
    } else {
      if(map(mallocs)->get(addr)) {
        // let it fail   
        return NULL;
      } else {
        // real malloc   
        return real_malloc(size);
      }
    }
  }
  // don't know what to do
  return NULL;
}

//-----------------------------------------------------------------------------
void *realloc(void* mem, size_t size)
{
  if(real_realloc == NULL) _init();
  
  if(is_backtrace) {
      return real_realloc(mem, size);
  }

  void* addr = __builtin_return_address(0);
  current_malloc = addr;
  
  if(settings.mode == PROFILE ) {
    save_trace();
    return real_realloc(mem, size);
  } else if(settings.mode == INJECT) {
    if(!map(mallocs)->has(addr)) {
      printf("strange, %p was not profiled\n", addr);   
      return real_realloc(mem, size);
    } else {
      if(map(mallocs)->get(addr)) {
        // let it fail   
        return NULL;
      } else {
        // real realloc   
        return real_realloc(mem, size);
      }
    }
  }
  // don't know what to do
  return NULL;
}

//-----------------------------------------------------------------------------
void *calloc(size_t elem, size_t size)
{
  if(real_calloc == NULL) _init();
  
  if(is_backtrace) {
      return real_calloc(elem, size);
  }

  void* addr = __builtin_return_address(0);
  current_malloc = addr;
  
  if(settings.mode == PROFILE ) {
    save_trace();
    return real_calloc(elem, size);
  } else if(settings.mode == INJECT) {
    if(!map(mallocs)->has(addr)) {
      printf("strange, %p was not profiled\n", addr);   
      return real_calloc(elem, size);
    } else {
      if(map(mallocs)->get(addr)) {
        // let it fail   
        return NULL;
      } else {
        // real calloc   
        return real_calloc(elem, size);
      }
    }
  }
  // don't know what to do
  return NULL;
}


//-----------------------------------------------------------------------------
void exit(int val) {
   if(real_exit == NULL) _init();
   real_exit(val);   
   while(1); // to suppress gcc warning
}

//-----------------------------------------------------------------------------
void _exit(int val) {
   if(real_exit1 == NULL) _init();
   real_exit1(val);   
}

//-----------------------------------------------------------------------------
void segfault_handler(int sig) {
    is_backtrace = 1;
    void* crash_addr = NULL;
    
    // find crash address in backtrace
    int j, nptrs;
    unsigned int pos, i;
    void *buffer[100];
    char addr_buffer[32];
    char **strings;

    nptrs = backtrace(buffer, 100);
    strings = backtrace_symbols(buffer, nptrs);
    if(strings) {
        for (j = 0; j < nptrs; j++) {
            if(strncmp(strings[j], settings.filename, strlen(settings.filename)) == 0) {
                pos = 0;
                while(strings[j][pos] != '[' && pos < strlen(strings[j])) pos++;
                if(pos >= strlen(strings[j])) {
                  // not found
                  break;   
                }
                for(i = pos + 1; i < strlen(strings[j]); i++) {
                  if(strings[j][i] == ']') break;
                  addr_buffer[i - pos - 1] = strings[j][i];   
                }
                addr_buffer[i] = 0;
                crash_addr = (void*)strtol(addr_buffer, NULL, 16);
                break;
            }
        }

        free(strings);
    }
    
   // write crash report
   FILE* f = fopen("crash", "wb");
   fwrite(&current_malloc, sizeof(void*), 1, f);
   fwrite(&crash_addr, sizeof(void*), 1, f);
   fclose(f);
   is_backtrace = 0;
   real_exit1(sig + 128);
}

/*
//-----------------------------------------------------------------------------
void abort(void) {
  if(real_abort == NULL) _init();

  // abort was called -> default terminate handler for uncaught exceptions
  char* realname = (char*)"Unknown";
  __cxa_eh_globals* eh = __cxa_get_globals(); 
  if (eh && eh->exc && eh->exc->inf) {
    int status;
    // get name of thrown exception and demangle it
    realname = abi::__cxa_demangle(eh->exc->inf->name(), 0, 0, &status);
  }
  
  // display uncaught exception and cause segfault to make it easier to debug
  fprintf(stderr, "\n[[Uncaught Exception <%s> - Aborting]]\n", realname);
  
  // segfault!
  int *a = NULL;
  *a = 42;
  
  // will never reach this, 
  // just to be safe (otherwise abort would be an endless loop)
  exit(-91);
}
*/