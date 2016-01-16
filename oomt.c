/*
 * Out-of-memory testing program. Needs malloc_replace.so to be present in the 
 * working directory, as well as write privilege to the file "mallocs" in 
 * the working directory.
 * 
 * 01/2016 by Michael Schwarz 
 *
 **/ 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "settings.h"
#include "map.h"

MallocSettings settings;

void write_settings() {
  FILE* f = fopen("settings", "wb");
  if(f) 
  {
    fwrite(&settings, sizeof(MallocSettings), 1, f);
    fclose(f);
  }
}

void set_mode(enum Mode m) {
  settings.mode = m;
  write_settings();
}

void set_limit(int lim) {
   settings.limit = lim;
   write_settings();   
}

void set_filename(char* fn) {
   strncpy(settings.filename, fn, 255);
   settings.filename[255] = 0;
   write_settings();   
}

int log(const char *format, ...) {
    static int count = 0;
    time_t timer;
    char buffer[26];
    struct tm* tm_info;

    va_list args;
    va_start(args, format);
    printf("[ FAINT ] ");
    vprintf(format, args);
    va_end(args);
    
    va_start(args, format);
    FILE* f = fopen("log.txt", count == 0 ? "w" : "a");
    
    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 26, "%H:%M:%S", tm_info);
    fprintf(f, "[%s] ", buffer);
    
    vfprintf(f, format, args);
    fclose(f);
    va_end(args);
    count++;
}

void clear_crash_report() {
  FILE* f = fopen("crash", "wb");
  fclose(f);
}

int get_crash_address(void** crash, void** malloc_addr) {
  FILE* f = fopen("crash", "rb");
  if(!f) return 0;
  int s1 = fread(malloc_addr, sizeof(void*), 1, f);
  int s = fread(crash, sizeof(void*), 1, f);
  fclose(f);
  if(s == 0 || s1 == 0) return 0;
  return 1;
}

int get_file_and_line (char* binary, void* addr, char *file, int *line, char* function)
{
  static char buf[256];

  // prepare command to be executed
  // our program need to be passed after the -e parameter
  sprintf (buf, "/usr/bin/addr2line -C -e %s -s -f -i %lx", binary, (size_t)addr);
  FILE* f = popen (buf, "r");

  if (f == NULL)
  {
    perror (buf);
    return 0;
  }

  // get function name
  fgets (buf, 256, f);
  strcpy(function, buf);
  int i;
  for(i = 0; i < strlen(function); i++) {
    if(function[i] == '\n' || function[i] == '\r') {
      function[i] = 0;
      break;
    }
  }

  // get file and line
  fgets (buf, 256, f);

  if (buf[0] != '?')
  {
    char *p = buf;

    // file name is until ':'
    while (*p != ':')
    {
      p++;
    }

    *p++ = 0;
    // after file name follows line number
    strcpy (file , buf);
    sscanf (p,"%d", line);
  }
  else
  {
    strcpy (file,"unkown");
    *line = 0;
  }

  pclose(f);
}

/*
void* _malloc(size_t size) {
  return malloc(size);   
}
*/

int main(int argc, char* argv[]) 
{
  if(argc <= 1) 
  {
    printf("Usage: %s <binary to test> [arg1] [...]\n", argv[0]);
    return 0;
  }

  // preload malloc replacement
  char* const envs[] = {(char*)"LD_PRELOAD=./malloc_replace.so", NULL};
  char* args[argc];

  // inherit all arguments
  int i;
  for(i = 0; i < argc - 1; i++) 
  {
    args[i] = argv[i + 1]; 
  }  
  args[i] = NULL;
  
  set_filename(args[0]);
  
  // fork first to profile
  log("Profiling start\n");
 
  map_create(crashes, MAP_GENERAL);
  int crash_count = 0;
  int injections;

  pid_t p = fork();
  if(p) {
    waitpid(p, NULL, 0);
    log("Profiling done\n");
    // profiling done, fork to inject
    int calls = 0, mallocs;
    
    FILE* f = fopen("profile", "rb");
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    mallocs = fsize / (2 * sizeof(size_t));
    fseek(f, 0, SEEK_SET);
    
    size_t* m_addr = malloc(sizeof(size_t) * mallocs);
    size_t* m_count = malloc(sizeof(size_t) * mallocs);
    
    for(i = 0; i < mallocs; i++) {
       fread(&m_addr[i], sizeof(size_t), 1, f);
       fread(&m_count[i], sizeof(size_t), 1, f);
       calls += m_count[i];
    }
    fclose(f);
    
    
    log("Found %d different mallocs with %d call(s) ***\n", mallocs, calls);
    
    injections = mallocs;
    for(i = 0; i < mallocs; i++) {
      char m_file[256], m_function[256];
      int m_line;
      get_file_and_line(args[0], (void*)(m_addr[i]), m_file, &m_line, m_function);
      log(" >  %s in %s line %d: %d calls\n", m_function, m_file, m_line, m_count[i]);
    }
    log("\n");
    
    free(m_addr);
    free(m_count);
    // let one specific malloc fail per loop iteration
    log("Injecting %d faults, one for every malloc\n", injections);
    log("\n");
    
    for(i = 0; i < mallocs; i++) {
        p = fork();
        if(p) {
            int status, killed = 0;
            waitpid(p, &status, 0);
            
            if (WIFEXITED(status)) {
                log("Exited, status: %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                log("Killed by signal %d (%s)\n", WTERMSIG(status), strsignal(WTERMSIG(status)));
                killed = 1;
            } else if (WIFSTOPPED(status)) {
                log("Stopped by signal %d (%s)\n", WSTOPSIG(status), strsignal(WTERMSIG(status)));
            }
            
            void *crash, *m;
            int has_addr = get_crash_address(&crash, &m);
            if(has_addr) {
                char crash_file[256], malloc_file[256], crash_fnc[256], malloc_fnc[256];
                int crash_line, malloc_line;
                log("Crashed at %p, caused by %p\n", crash, m);
                get_file_and_line(args[0], crash, crash_file, &crash_line, crash_fnc);
                get_file_and_line(args[0], m, malloc_file, &malloc_line, malloc_fnc);
                log("Crash details: \n");
                log(" > Crash: %s (%s) @ %d\n", crash_fnc, crash_file, crash_line);
                log(" > Malloc: %s (%s) @ %d\n", malloc_fnc, malloc_file, malloc_line);
                map(crashes)->set(crash, m);
                crash_count++;
            } else {
              if(killed) crash_count++;   
            }
            log("Injection #%d done\n", (i + 1));
        } else {
            log("\n");
            log("Inject fault #%d\n", (i + 1));
            // -> inject
            clear_crash_report();
            set_mode(INJECT);
            set_limit(i);
            execve(args[0], args, envs);
        }    
    }
  } else {
    // -> profile
    set_mode(PROFILE);
    execve(args[0], args, envs);
  }
  log("\n");
  log("======= SUMMARY =======\n");
  log("\n");
  log("Crashed at %d from %d injections\n", crash_count, injections);
  
  int unique = 0;
  cmap_iterator* it = map(crashes)->iterator();
  while(!map_iterator(it)->end()) {
    unique++;
    map_iterator(it)->next();
  }
  map_iterator(it)->destroy();
 
  log("Unique crashes: %d\n", unique);
  log("\n");
  log("Crash details:\n");
  
  it = map(crashes)->iterator();
  while(!map_iterator(it)->end()) {
    void* crash = map_iterator(it)->key();
    void* m = map_iterator(it)->value();
    char crash_file[256], malloc_file[256], crash_fnc[256], malloc_fnc[256];
    int crash_line, malloc_line;
    log("\n");
    log("Crashed at %p, caused by %p\n", crash, m);
    get_file_and_line(args[0], crash, crash_file, &crash_line, crash_fnc);
    get_file_and_line(args[0], m, malloc_file, &malloc_line, malloc_fnc);
    log("   > Crash: %s (%s) line %d\n", crash_fnc, crash_file, crash_line);
    log("   > Malloc: %s (%s) line %d\n", malloc_fnc, malloc_file, malloc_line);
    map_iterator(it)->next();
  }
  map_iterator(it)->destroy();
 
  return 0;
}
