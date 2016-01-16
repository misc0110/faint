/*
 * Shared library to let new fail after a given number of successful new 
 * calls. Used to test out-of-memory handling of programs.
 * 
 * 06/2013 by Michael Schwarz 
 *
 **/ 
#include "malloc_replace.h"
#include "settings.h"
#include "map.h"

static h_malloc real_malloc = NULL;
static h_abort real_abort = NULL;
static h_exit real_exit = NULL;
static h_exit real_exit1 = NULL;

static unsigned int is_backtrace = 0;

static MallocSettings settings;
map_declare(mallocs);

#define get_malloc_address() __builtin_return_address(1)

//-----------------------------------------------------------------------------
static void _init(void)
{
    // read symbols from standard library
    real_malloc = (h_malloc)dlsym(RTLD_NEXT, "malloc");
    real_abort = (h_abort)dlsym(RTLD_NEXT, "abort");
    real_exit = (h_exit)dlsym(RTLD_NEXT, "exit");
    real_exit1 = (h_exit)dlsym(RTLD_NEXT, "_exit");
    
    if (real_malloc == NULL || real_abort == NULL || real_exit == NULL || real_exit1 == NULL) 
    {
      fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
      return;
    }
        
    is_backtrace = 1;
    // read number of allowed mallocs from file
    FILE *f = fopen("settings", "rb");
    if(f) 
    {
      fread(&settings, sizeof(MallocSettings), 1, f);
      fclose(f);
    }  
    printf("-- Mode: %d\n", settings.mode);
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
              printf("%x -> %d\n", addr, cnt);
              if(entry == settings.limit) cnt = (void*)1; else cnt = (void*)0;
              map(mallocs)->set(addr, cnt);
              entry++;
            }
        }
        fclose(f);
        map(mallocs)->dump();
    }
    is_backtrace = 0;
}

void* _malloc(size_t size) {
  return real_malloc(size);   
}


void show_backtrace()
{
        /*
        // get current address
        void* p = __builtin_return_address(0);
        printf("0x%x\n", p);
        */
        // get callee address
        void* p = __builtin_return_address(2);
        is_backtrace = 1;

        printf("0x%x\n", p);
        
        Dl_info info;
        dladdr(p, &info);
        printf("%s\n", info.dli_fname);
        
        if(!mallocs) map_initialize(mallocs, MAP_GENERAL);
        
        if(map(mallocs)->has(p)) {
          map(mallocs)->set(p, map(mallocs)->get(p) + 1);   
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

  void* addr = __builtin_return_address(1);
  
  if(settings.mode == PROFILE ) {
    show_backtrace();
    return real_malloc(size);
  } else if(settings.mode == INJECT) {
    if(!map(mallocs)->has(addr)) {
      printf("strange, %x was not profiled\n", addr);   
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


void exit(int val) {
   printf("Done\n");
   real_exit(val);   
}

void _exit(int val) {
   printf("Done\n");
   real_exit1(val);   
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