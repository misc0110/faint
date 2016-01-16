/*
 * Out-of-memory testing program. Needs malloc_replace.so to be present in the 
 * working directory.
 * 
 * 01/2016 by Michael Schwarz 
 *
 **/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/personality.h>
#include "settings.h"
#include "map.h"

#ifndef HAVE_PERSONALITY
#include <syscall.h>
#define personality(pers) ((long)syscall(SYS_personality, pers))
#endif

MallocSettings settings;

// ---------------------------------------------------------------------------
void write_settings() {
  FILE* f = fopen("settings", "wb");
  if(f) {
    fwrite(&settings, sizeof(MallocSettings), 1, f);
    fclose(f);
  }
}

// ---------------------------------------------------------------------------
void set_mode(enum Mode m) {
  settings.mode = m;
  write_settings();
}

// ---------------------------------------------------------------------------
void set_limit(int lim) {
  settings.limit = lim;
  write_settings();
}

// ---------------------------------------------------------------------------
void set_filename(char* fn) {
  strncpy(settings.filename, fn, 255);
  settings.filename[255] = 0;
  write_settings();
}

// ---------------------------------------------------------------------------
void log(const char *format, ...) {
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

// ---------------------------------------------------------------------------
void clear_crash_report() {
  FILE* f = fopen("crash", "wb");
  fclose(f);
}

// ---------------------------------------------------------------------------
int get_crash_address(void** crash, void** malloc_addr) {
  FILE* f = fopen("crash", "rb");
  if(!f)
    return 0;
  int s1 = fread(malloc_addr, sizeof(void*), 1, f);
  int s = fread(crash, sizeof(void*), 1, f);
  fclose(f);
  if(s == 0 || s1 == 0)
    return 0;
  return 1;
}

// ---------------------------------------------------------------------------
int get_file_and_line(char* binary, void* addr, char *file, int *line, char* function) {
  static char buf[256];

  // call addr2line
  sprintf(buf, "addr2line -C -e %s -s -f -i %lx", binary, (size_t) addr);
  FILE* f = popen(buf, "r");

  if(f == NULL) {
    log("Could not resolve address %p, do you have addr2line installed?\n", addr);
    return 0;
  }

  // get function name
  fgets(buf, 256, f);
  strcpy(function, buf);
  int i;
  for(i = 0; i < strlen(function); i++) {
    if(function[i] == '\n' || function[i] == '\r') {
      function[i] = 0;
      break;
    }
  }

  // get file and line
  fgets(buf, 256, f);

  if(buf[0] != '?') {
    char *p = buf;

    // file name is until ':'
    while(*p != ':') {
      p++;
      if((p - buf) >= 256) {
        pclose(f);
        return 0;
      }
    }

    *p++ = 0;
    // after file name follows line number
    strcpy(file, buf);
    sscanf(p, "%d", line);
  } else {
    strcpy(file, "unknown");
    *line = 0;
  }

  pclose(f);
  return 1;
}

// ---------------------------------------------------------------------------
void cleanup() {
  remove("settings");
  remove("profile");
  remove("crash");
}

// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
  if(argc <= 1) {
    printf("Usage: %s <binary to test> [arg1] [...]\n", argv[0]);
    return 0;
  }
  atexit(cleanup);

  log("Starting, Version 1.0\n");
  log("\n");

  FILE* so = fopen("malloc_replace.so", "rb");
  if(!so) {
    log("Could not find 'malloc_replace.so'. Aborting.\n");
    exit(1);
  }
  fclose(so);

  // preload malloc replacement
  char* const envs[] = { (char*) "LD_PRELOAD=./malloc_replace.so", NULL };
  char* args[argc];

  // inherit all arguments
  int i;
  log("Binary: %s\n", argv[0]);
  for(i = 0; i < argc - 1; i++) {
    if(i > 0) {
      log(" Param %2d: %s\n", i, argv[i + 1]);
    }
    args[i] = argv[i + 1];
  }
  args[i] = NULL;
  log("\n");

  set_filename(args[0]);

  // check if compiled with debug symbols
  char re_cmdline[256];
  sprintf(re_cmdline, "readelf --debug-dump=line \"%s\" | wc -l", args[0]);
  FILE* dbg = popen(re_cmdline, "r");
  if(dbg) {
    char debug_lines[32];
    fgets(debug_lines, 32, dbg);
    if(atoi(debug_lines) == 0) {
      log("Could not find debugging info! Did you compile with -g?\n");
      log("\n");
    }
    pclose(dbg);
  }

  // disable aslr to always get correct debug infos over multiple injection runs
  if(personality(ADDR_NO_RANDOMIZE) == -1) {
    log("Could not turn off ASLR: %s\n", strerror(errno));
  } else {
    log("ASLR turned off successfully\n");
  }

  // fork first to profile
  log("Profiling start\n");
  FILE* f = fopen("profile", "wb");
  if(!f) {
    log("Need write access to file 'profile'!\n");
    exit(1);
  }
  fclose(f);

  map_create(crashes, MAP_GENERAL);
  int crash_count = 0;
  int injections;

  pid_t p = fork();
  if(p) {
    int status;
    waitpid(p, &status, 0);
    if(!WIFEXITED(status)) {
      log("There was an error while profiling, aborting now\n");
      exit(1);
    }

    log("Profiling done\n");
    // profiling done, fork to inject
    int calls = 0, mallocs;

    FILE* f = fopen("profile", "rb");
    if(!f) {
      log("No trace generated, aborting now\n");
      exit(1);
    }
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

    log("Found %d different mallocs with %d call(s)\n", mallocs, calls);

    injections = mallocs;
    for(i = 0; i < mallocs; i++) {
      char m_file[256], m_function[256];
      int m_line;
      if(get_file_and_line(args[0], (void*) (m_addr[i]), m_file, &m_line, m_function)) {
        log(" >  %s in %s line %d: %d calls\n", m_function, m_file, m_line, m_count[i]);
      } else {
        log(" > N/A (maybe you forgot to compile with -g?)\n");
      }
    }
    log("\n");

    // let one specific malloc fail per loop iteration
    log("Injecting %d faults, one for every malloc\n", injections);
    log("\n");

    for(i = 0; i < mallocs; i++) {
      p = fork();
      if(p) {
        int status, killed = 0;
        waitpid(p, &status, 0);

        if(WIFEXITED(status)) {
          int ret = WEXITSTATUS(status);
          log("Exited, status: %d %s%s%s\n", ret, ret >= 128 ? "(" : "", ret >= 128 ? strsignal(ret - 128) : "",
              ret >= 128 ? ")" : "");
        } else if(WIFSIGNALED(status)) {
          log("Killed by signal %d (%s)\n", WTERMSIG(status), strsignal(WTERMSIG(status)));
          killed = 1;
        } else if(WIFSTOPPED(status)) {
          log("Stopped by signal %d (%s)\n", WSTOPSIG(status), strsignal(WTERMSIG(status)));
        }

        void *crash, *m;
        int has_addr = get_crash_address(&crash, &m);
        if(has_addr) {
          char crash_file[256], malloc_file[256], crash_fnc[256], malloc_fnc[256];
          int crash_line, malloc_line;
          log("Crashed at %p, caused by %p\n", crash, m);
          if(get_file_and_line(args[0], crash, crash_file, &crash_line, crash_fnc)
              && get_file_and_line(args[0], m, malloc_file, &malloc_line, malloc_fnc)) {
            log("Crash details: \n");
            log(" > Crash: %s (%s) @ %d\n", crash_fnc, crash_file, crash_line);
            log(" > Malloc: %s (%s) @ %d\n", malloc_fnc, malloc_file, malloc_line);
          } else {
            log("No crash details available (maybe you forgot to compile with -g?)\n");
          }
          map(crashes)->set(crash, m);
          crash_count++;
        } else {
          if(killed)
            crash_count++;
        }
        log("Injection #%d done\n", (i + 1));
      } else {
        log("\n");
        log("\n");
        log("Inject fault #%d\n", (i + 1));
        log("Fault position:\n");
        char m_file[256], m_function[256];
        int m_line;
        if(get_file_and_line(args[0], (void*) (m_addr[i]), m_file, &m_line, m_function)) {
          log(" >  %s in %s line %d\n", m_function, m_file, m_line);
        } else {
          log("Position not available (maybe you forgot to compile with -g?)\n");
        }
        log("\n");

        // -> inject
        clear_crash_report();
        set_mode(INJECT);
        set_limit(i);
        execve(args[0], args, envs);
        log("Could not execute %s\n", args[0]);
        exit(0);
      }
    }
    free(m_addr);
    free(m_count);
  } else {
    // -> profile
    set_mode(PROFILE);
    execve(args[0], args, envs);
    log("Could not execute %s\n", args[0]);
    exit(0);
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

  if(crash_count > 0) {
    log("Crash details:\n");

    it = map(crashes)->iterator();
    while(!map_iterator(it)->end()) {
      void* crash = map_iterator(it)->key();
      void* m = map_iterator(it)->value();
      char crash_file[256], malloc_file[256], crash_fnc[256], malloc_fnc[256];
      int crash_line, malloc_line;
      log("\n");
      log("Crashed at %p, caused by %p\n", crash, m);
      if(get_file_and_line(args[0], crash, crash_file, &crash_line, crash_fnc)
          && get_file_and_line(args[0], m, malloc_file, &malloc_line, malloc_fnc)) {
        log("   > Crash: %s (%s) line %d\n", crash_fnc, crash_file, crash_line);
        log("   > Malloc: %s (%s) line %d\n", malloc_fnc, malloc_file, malloc_line);
      } else {
        log("   > N/A (maybe you forgot to compile with -g?)\n");
      }

      map_iterator(it)->next();
    }
    map_iterator(it)->destroy();
  } else {
    log("Everything ok, no crashes detected!\n");
  }
  map(crashes)->destroy();
  log("\n");
  log("\n");
  log("finished successfully!\n");

  return 0;
}
