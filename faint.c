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
#include <stdint.h>
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

extern uint8_t malloc_lib[] asm("_binary_malloc_replace_so_start");
extern uint8_t malloc_lib_end[] asm("_binary_malloc_replace_so_end");

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
void enable_module(char* m) {
  int i;
  for(i = 0; i < MODULE_COUNT; i++) {
    if(!strcmp(m, modules[i])) {
      settings.modules |= (1 << i);
    }
  }
}

// ---------------------------------------------------------------------------
void disable_module(char* m) {
  int i;
  for(i = 0; i < MODULE_COUNT; i++) {
    if(!strcmp(m, modules[i])) {
      settings.modules &= ~(1 << i);
    }
  }
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
void list_modules() {
  int i;
  log("Available modules:\n");
  for(i = 0; i < MODULE_COUNT; i++) {
    log(" > %s\n", modules[i]);
  }
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
  remove("malloc_replace.so");
  log("\n");
  log("\n");
  log("finished successfully!\n");
}

// ---------------------------------------------------------------------------
void usage(char* binary) {
  printf("Usage: %s [--enable [module] --disable [module] --list-modules --no-memory] <binary to test> [arg1] [...]\n",
      binary);
  printf("\n");
  printf("--list-modules\t Lists all available modules which can be enabled/disabled\n\n");
  printf("--enable [module]\t Enables the module\n\n");
  printf("--disable [module]\t Disables the module\n\n");
  printf("--no-memory\t Disable all memory allocation modules");
}

// ---------------------------------------------------------------------------
void extract_shared_library() {
  size_t malloc_lib_size = (size_t) ((char*) malloc_lib_end - (char*) malloc_lib);

  FILE* so = fopen("./malloc_replace.so", "wb");
  if(!so) {
    log("Could not extract 'malloc_replace.so'. Aborting.\n");
    exit(1);
  }
  if(fwrite(malloc_lib, malloc_lib_size, 1, so) != 1) {
    log("Could not write to file 'malloc_replace.so'. Aborting.\n");
    fclose(so);
    exit(1);
  }
  fclose(so);
}

// ---------------------------------------------------------------------------
void enable_default_modules() {
  enable_module("malloc");
  enable_module("calloc");
  enable_module("realloc");
  enable_module("new");
}

// ---------------------------------------------------------------------------
int parse_commandline(int argc, char* argv[]) {
  int i, binary_pos = 0;
  for(i = 1; i < argc; i++) {
    if(strncmp(argv[i], "--", 2) == 0) {
      char* cmd = &argv[i][2];
      if(!strcmp(cmd, "list-modules")) {
        list_modules();
        exit(0);
      } else if(!strcmp(cmd, "enable") && i != argc - 1) {
        enable_module(argv[i + 1]);
        i++;
      } else if(!strcmp(cmd, "disable") && i != argc - 1) {
        disable_module(argv[i + 1]);
        i++;
      } else if(!strcmp(cmd, "no-memory")) {
        disable_module("malloc");
        disable_module("calloc");
        disable_module("realloc");
        disable_module("new");
      } else {
        log("Unknown command: %s\n", cmd);
        exit(1);
      }
    } else {
      binary_pos = i;
      break;
    }
  }
  write_settings();
  for(i = 0; i < MODULE_COUNT; i++) {
    if(settings.modules & (1 << i)) {
      log("Activate module '%s'\n", modules[i]);
    }
  }
  log("\n");
  return binary_pos;
}

// ---------------------------------------------------------------------------
void check_debug_symbols(char* binary) {
  char re_cmdline[256];
  sprintf(re_cmdline, "readelf --debug-dump=line \"%s\" | wc -l", binary);
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
}

// ---------------------------------------------------------------------------
void disable_aslr() {
  if(personality(ADDR_NO_RANDOMIZE) == -1) {
    log("Could not turn off ASLR: %s\n", strerror(errno));
  } else {
    log("ASLR turned off successfully\n");
  }
}

// ---------------------------------------------------------------------------
void summary(char* binary, int crash_count, int injections, cmap* crashes, cmap* types) {
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
      log("Crashed at %p, caused by %p [%s]\n", crash, m, modules[(size_t) map(types)->get(m)]);
      if(get_file_and_line(binary, crash, crash_file, &crash_line, crash_fnc)
          && get_file_and_line(binary, m, malloc_file, &malloc_line, malloc_fnc)) {
        log("   > crash: %s (%s) line %d\n", crash_fnc, crash_file, crash_line);
        log("   > %s: %s (%s) line %d\n", modules[(size_t) map(types)->get(m)], malloc_fnc, malloc_file, malloc_line);
      } else {
        log("   > N/A (maybe you forgot to compile with -g?)\n");
      }

      map_iterator(it)->next();
    }
    map_iterator(it)->destroy();
  } else {
    log("Everything ok, no crashes detected!\n");
  }
}

int parse_profiling(size_t** addr, size_t** count, size_t** type, size_t* calls, cmap* types) {

  FILE* f = fopen("profile", "rb");
  if(!f) {
    log("No trace generated, aborting now\n");
    exit(1);
  }
  fseek(f, 0, SEEK_END);
  size_t fsize = ftell(f);
  size_t injections = fsize / (3 * sizeof(size_t));
  fseek(f, 0, SEEK_SET);

  *addr = malloc(sizeof(size_t) * injections);
  *count = malloc(sizeof(size_t) * injections);
  *type = malloc(sizeof(size_t) * injections);

  int i;
  *calls = 0;
  for(i = 0; i < injections; i++) {
    fread(&((*addr)[i]), sizeof(size_t), 1, f);
    fread(&((*count)[i]), sizeof(size_t), 1, f);
    fread(&((*type)[i]), sizeof(size_t), 1, f);
    map(types)->set((void*) addr[i], (void*) type[i]);
    (*calls) += (*count)[i];
  }
  fclose(f);
  return injections;
}

// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
  if(argc <= 1) {
    usage(argv[0]);
    return 0;
  }
  atexit(cleanup);

  log("Starting, Version 1.0\n");
  log("\n");

  // extract malloc replace library
  extract_shared_library();

  // modules enabled by default
  enable_default_modules();

  // parse commandline
  int binary_pos = parse_commandline(argc, argv);

  // preload malloc replacement
  char* const envs[] = { (char*) "LD_PRELOAD=./malloc_replace.so", NULL };
  char* args[argc];

  // inherit all arguments
  int i;
  for(i = 0; i < argc - binary_pos; i++) {
    if(i > 0) {
      log(" Param %2d: %s\n", i, argv[i + binary_pos]);
    } else {
      log("Binary: %s\n", argv[i + binary_pos]);
    }
    args[i] = argv[i + binary_pos];
  }
  args[i] = NULL;
  log("\n");

  set_filename(args[0]);

  // check if compiled with debug symbols
  check_debug_symbols(args[0]);

  // disable aslr to always get correct debug infos over multiple injection runs
  disable_aslr();

  // fork first to profile
  log("Profiling start\n");
  FILE* f = fopen("profile", "wb");
  if(!f) {
    log("Need write access to file 'profile'!\n");
    exit(1);
  }
  fclose(f);

  map_create(crashes, MAP_GENERAL);
  map_create(types, MAP_GENERAL);
  int crash_count = 0;
  int injections = 0;

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
    size_t calls = 0;

    size_t *m_addr, *m_count, *m_type;
    injections = parse_profiling(&m_addr, &m_count, &m_type, &calls, types);

    log("Found %d different injection positions with %d call(s)\n", injections, calls);

    for(i = 0; i < injections; i++) {
      char m_file[256], m_function[256];
      int m_line;
      if(get_file_and_line(args[0], (void*) (m_addr[i]), m_file, &m_line, m_function)) {
        log(" >  [%s] %s in %s line %d: %d calls \n", modules[m_type[i]], m_function, m_file, m_line, m_count[i]);
      } else {
        log(" > N/A (maybe you forgot to compile with -g?)\n");
      }
    }
    log("\n");

    // let one specific malloc fail per loop iteration
    log("Injecting %d faults, one for every injection position\n", injections);
    log("\n");

    for(i = 0; i < injections; i++) {
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
          log("Crashed at %p, caused by %p [%s]\n", crash, m, modules[(size_t) map(types)->get(m)]);
          if(get_file_and_line(args[0], crash, crash_file, &crash_line, crash_fnc)
              && get_file_and_line(args[0], m, malloc_file, &malloc_line, malloc_fnc)) {
            log("Crash details: \n");
            log(" > crash: %s (%s) @ %d\n", crash_fnc, crash_file, crash_line);
            log(" > %s: %s (%s) @ %d\n", modules[(size_t) map(types)->get(m)], malloc_fnc, malloc_file, malloc_line);
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
          log(" >  [%s] %s in %s line %d\n", modules[m_type[i]], m_function, m_file, m_line);
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
    free(m_type);
  } else {
    // -> profile
    set_mode(PROFILE);
    execve(args[0], args, envs);
    log("Could not execute %s\n", args[0]);
    exit(0);
  }

  summary(args[0], crash_count, injections, crashes, types);

  map(crashes)->destroy();
  return 0;
}
