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
#include "usage.h"

#ifndef HAVE_PERSONALITY
#include <syscall.h>
#define personality(pers) ((long)syscall(SYS_personality, pers))
#endif

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define ARCH_32   0
#define ARCH_64   1

static FaultSettings settings;

extern uint8_t fault_lib[] asm("_binary_fault_inject_so_start");
extern uint8_t fault_lib_end[] asm("_binary_fault_inject_so_end");
extern uint8_t fault_lib32[] asm("_binary_fault_inject32_so_start");
extern uint8_t fault_lib32_end[] asm("_binary_fault_inject32_so_end");

static int colorlog = 0;

#ifndef VERSION
#define VERSION "0.1-debug"
#endif

// ---------------------------------------------------------------------------
void write_settings() {
  FILE* f = fopen("settings", "wb");
  if(f) {
    fwrite(&settings, sizeof(FaultSettings), 1, f);
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
void set_filename(const char* fn) {
  strncpy(settings.filename, fn, 255);
  settings.filename[255] = 0;
  write_settings();
}

// ---------------------------------------------------------------------------
const char* get_filename() {
  return settings.filename;
}

// ---------------------------------------------------------------------------
void enable_module(const char* m) {
  int i;
  for(i = 0; i < MODULE_COUNT; i++) {
    if(!strcmp(m, modules[i])) {
      settings.modules |= (1 << i);
    }
  }
}

// ---------------------------------------------------------------------------
void disable_module(const char* m) {
  int i;
  for(i = 0; i < MODULE_COUNT; i++) {
    if(!strcmp(m, modules[i])) {
      settings.modules &= ~(1 << i);
    }
  }
}

// ---------------------------------------------------------------------------
char* str_replace(const char* orig, char* rep, char* with) {
  char *result, *tmp;
  int len_rep, len_with, len_front, count;

  if(!orig)
    return NULL;
  if(!rep)
    rep = "";
  len_rep = strlen(rep);
  if(!with)
    with = "";
  len_with = strlen(with);

  const char* ins = orig;
  for(count = 0; (tmp = strstr(ins, rep)); ++count) {
    ins = tmp + len_rep;
  }

  tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

  if(!result)
    return NULL;

  while(count--) {
    ins = strstr(orig, rep);
    len_front = ins - orig;
    tmp = strncpy(tmp, orig, len_front) + len_front;
    tmp = strcpy(tmp, with) + len_with;
    orig += len_front + len_rep;
  }
  strcpy(tmp, orig);
  return result;
}

// ---------------------------------------------------------------------------
void str_replace_inplace(char** orig, char* rep, char* with) {
  char* tmp = str_replace(*orig, rep, with);
  free(*orig);
  *orig = tmp;
}

// ---------------------------------------------------------------------------
void log(const char *format, ...) {
  static int count = 0;
  time_t timer;
  char buffer[26], time_buffer[28];
  struct tm* tm_info;

  // replace newlines with tag
#define LOG_TAG "[ FAINT ] "

  char* tag_format = str_replace(format, "\n", "\n" LOG_TAG);
  str_replace_inplace(&tag_format, "{red}", colorlog ? ANSI_COLOR_RED : "");
  str_replace_inplace(&tag_format, "{/red}", colorlog ? ANSI_COLOR_RESET : "");
  str_replace_inplace(&tag_format, "{green}", colorlog ? ANSI_COLOR_GREEN : "");
  str_replace_inplace(&tag_format, "{/green}", colorlog ? ANSI_COLOR_RESET : "");
  str_replace_inplace(&tag_format, "{blue}", colorlog ? ANSI_COLOR_BLUE : "");
  str_replace_inplace(&tag_format, "{/blue}", colorlog ? ANSI_COLOR_RESET : "");
  str_replace_inplace(&tag_format, "{yellow}", colorlog ? ANSI_COLOR_YELLOW : "");
  str_replace_inplace(&tag_format, "{/yellow}", colorlog ? ANSI_COLOR_RESET : "");
  str_replace_inplace(&tag_format, "{magenta}", colorlog ? ANSI_COLOR_MAGENTA : "");
  str_replace_inplace(&tag_format, "{/magenta}", colorlog ? ANSI_COLOR_RESET : "");
  str_replace_inplace(&tag_format, "{cyan}", colorlog ? ANSI_COLOR_CYAN : "");
  str_replace_inplace(&tag_format, "{/cyan}", colorlog ? ANSI_COLOR_RESET : "");

  // print to stderr
  va_list args;
  va_start(args, format);
  fprintf(stderr, "%s", LOG_TAG);
  vfprintf(stderr, tag_format, args);
  fprintf(stderr, "\n");
  va_end(args);

  // print to logfile
  va_start(args, format);
  FILE* f = fopen("log.txt", count == 0 ? "w" : "a");

  time(&timer);
  tm_info = localtime(&timer);
  strftime(buffer, 26, "%H:%M:%S", tm_info);
  sprintf(time_buffer, "\n[%s] ", buffer);

  char* file_format = str_replace(format, "\n", time_buffer);
  str_replace_inplace(&file_format, "{red}", "");
  str_replace_inplace(&file_format, "{/red}", "");
  str_replace_inplace(&file_format, "{green}", "");
  str_replace_inplace(&file_format, "{/green}", "");
  str_replace_inplace(&file_format, "{blue}", "");
  str_replace_inplace(&file_format, "{/blue}", "");
  str_replace_inplace(&file_format, "{yellow}", "");
  str_replace_inplace(&file_format, "{/yellow}", "");
  str_replace_inplace(&file_format, "{magenta}", "");
  str_replace_inplace(&file_format, "{/magenta}", "");
  str_replace_inplace(&file_format, "{cyan}", "");
  str_replace_inplace(&file_format, "{/cyan}", "");

  fprintf(f, "[%s] ", buffer);
  vfprintf(f, file_format, args);
  fprintf(f, "\n");
  fclose(f);
  va_end(args);
  count++;

  free(tag_format);
  free(file_format);
}

// ---------------------------------------------------------------------------
void list_modules() {
  int i;
  log("Available modules:\n");
  for(i = 1; i < MODULE_COUNT; i++) {
    log(" > %s\n", modules[i]);
  }
}

// ---------------------------------------------------------------------------
void clear_crash_report() {
  FILE* f = fopen("crash", "wb");
  fclose(f);
}

// ---------------------------------------------------------------------------
int get_crash_address(void** crash, void** fault_addr) {
  FILE* f = fopen("crash", "rb");
  if(!f)
    return 0;
  CrashEntry e;
  int s = fread(&e, sizeof(CrashEntry), 1, f);
  if(s == 0) {
    fclose(f);
    return 0;
  }
  *fault_addr = (void*) e.fault;
  *crash = (void*) e.crash;
  fclose(f);
  return 1;
}

// ---------------------------------------------------------------------------
int get_file_and_line(const char* binary, const void* addr, char *file, int *line, char* function) {
  static char buf[256];

  // call addr2line
  sprintf(buf, "addr2line -C -e %s -s -f -i %lx", binary, (size_t) addr);
  FILE* f = popen(buf, "r");

  if(f == NULL) {
    log("{red}Could not resolve address %p, do you have addr2line installed?{/red}\n", addr);
    return 0;
  }

  // get function name
  if(!fgets(buf, 256, f)) {
    pclose(f);
    return 0;
  }
  strcpy(function, buf);
  int i;
  for(i = 0; i < strlen(function); i++) {
    if(function[i] == '\n' || function[i] == '\r') {
      function[i] = 0;
      break;
    }
  }

  // get file and line
  if(!fgets(buf, 256, f)) {
    pclose(f);
    return 0;
  }

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
  remove("fault_inject.so");
  log("\n\nfinished successfully!");
}


// ---------------------------------------------------------------------------
void extract_shared_library(int arch) {
  size_t fault_lib_size;
  if(arch == ARCH_32) {
    fault_lib_size = (size_t) ((char*) fault_lib32_end - (char*) fault_lib32);
  } else {
    fault_lib_size = (size_t) ((char*) fault_lib_end - (char*) fault_lib);
  }

  FILE* so = fopen("./fault_inject.so", "rb");
  if(so) {
    // file already extracted, done
    fclose(so);
    return;
  }

  so = fopen("./fault_inject.so", "wb");
  if(!so) {
    log("{red}Could not extract 'fault_inject.so'. Aborting.{/red}");
    exit(1);
  }

  int write_ret;
  if(arch == ARCH_32) {
    write_ret = fwrite(fault_lib32, fault_lib_size, 1, so);
  } else {
    write_ret = fwrite(fault_lib, fault_lib_size, 1, so);
  }
  if(write_ret != 1) {
    log("{red}Could not write to file 'fault_inject.so'. Aborting.{/red}");
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
      } else if(!strcmp(cmd, "all")) {
        int j;
        for(j = 1; j < MODULE_COUNT; j++) {
          enable_module(modules[j]);
        }
      } else if(!strcmp(cmd, "none")) {
        int j;
        for(j = 1; j < MODULE_COUNT; j++) {
          disable_module(modules[j]);
        }
      } else if(!strcmp(cmd, "file-io")) {
        enable_module("fopen");
        enable_module("fread");
        enable_module("fwrite");
        enable_module("fgets");
      } else if(!strcmp(cmd, "colorlog")) {
        colorlog = 1;
      } else if(!strcmp(cmd, "version")) {
        printf("faint %s\n", VERSION);
        exit(0);
      } else {
        log("{red}Unknown command: %s{/red}", cmd);
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
      log("Activate module '{yellow}%s{/yellow}'", modules[i]);
    }
  }
  log("\n");
  return binary_pos;
}

// ---------------------------------------------------------------------------
void check_debug_symbols(const char* binary) {
  char re_cmdline[256];
  sprintf(re_cmdline, "readelf --debug-dump=line \"%s\" | wc -l", binary);
  FILE* dbg = popen(re_cmdline, "r");
  if(dbg) {
    char debug_lines[32];
    if(!fgets(debug_lines, 32, dbg)) {
      pclose(dbg);
      return;
    }
    if(atoi(debug_lines) == 0) {
      log("{red}Could not find debugging info! Did you compile with -g?{/red}\n");
    }
    pclose(dbg);
  }
}

// ---------------------------------------------------------------------------
int get_architecture(const char* binary) {
  int arch = ARCH_64;
  char obj_cmdline[256];
  sprintf(obj_cmdline, "objdump -f \"%s\" | grep elf", binary);
  FILE* dbg = popen(obj_cmdline, "r");
  if(dbg) {
    char debug_lines[256];
    if(!fgets(debug_lines, 256, dbg)) {
      pclose(dbg);
      return arch;
    }
    if(strstr(debug_lines, "elf32") != NULL)
      arch = ARCH_32;
    if(strstr(debug_lines, "elf64") != NULL)
      arch = ARCH_64;
    pclose(dbg);
  }
  return arch;
}

// ---------------------------------------------------------------------------
void disable_aslr() {
  if(personality(ADDR_NO_RANDOMIZE) == -1) {
    log("{red}Could not turn off ASLR: %s{/red}", strerror(errno));
  } else {
    log("ASLR turned off successfully");
  }
}

// ---------------------------------------------------------------------------
void print_fault_position(const char* binary, const void* fault, int type, int count) {
  char m_file[256], m_function[256];
  int m_line;
  if(get_file_and_line(binary, fault, m_file, &m_line, m_function)) {
    if(count != -1) {
      log(" >  [{yellow}%s{/yellow}] {cyan}%s{/cyan} in %s line {cyan}%d{/cyan}: %d calls", modules[type], m_function,
          m_file, m_line, count);
    } else {
      log(" >  [{yellow}%s{/yellow}] {cyan}%s{/cyan} in %s line {cyan}%d{/cyan}", modules[type], m_function, m_file,
          m_line);
    }
  } else {
    log(" > N/A ({red}maybe you forgot to compile with -g?{/red})");
  }
}

// ---------------------------------------------------------------------------
void crash_details(const char* binary, const void* crash, const void* fault, cmap* types) {
  char crash_file[256], fault_file[256], crash_fnc[256], fault_fnc[256];
  int crash_line, fault_line;

  log("{red}Crashed{/red} at %p, caused by %p [%s]", crash, fault, modules[(size_t) map(types)->get(fault)]);
  if(get_file_and_line(binary, crash, crash_file, &crash_line, crash_fnc)
      && get_file_and_line(binary, fault, fault_file, &fault_line, fault_fnc)) {
    log("  > {red}crash{/red}: {cyan}%s{/cyan} (%s) line {cyan}%d{/cyan}", crash_fnc, crash_file, crash_line);
    log("  > {yellow}%s{/yellow}: {cyan}%s{/cyan} (%s) line {cyan}%d{/cyan}", modules[(size_t) map(types)->get(fault)],
        fault_fnc, fault_file, fault_line);
  } else {
    log("No crash details available (maybe you forgot to compile with -g?)");
  }
}

// ---------------------------------------------------------------------------
void summary(const char* binary, int crash_count, int injections, cmap* crashes, cmap* types) {
  log("\n======= SUMMARY =======\n");
  log("Crashed at %d from %d injections", crash_count, injections);

  int unique = 0;
  cmap_iterator* it = map(crashes)->iterator();
  while(!map_iterator(it)->end()) {
    unique++;
    map_iterator(it)->next();
  }
  map_iterator(it)->destroy();

  log("Unique crashes: %d\n", unique);

  if(crash_count > 0) {
    log("Crash details:");

    it = map(crashes)->iterator();
    while(!map_iterator(it)->end()) {
      void* crash = map_iterator(it)->key();
      void* fault = map_iterator(it)->value();
      log("");
      crash_details(binary, crash, fault, types);
      map_iterator(it)->next();
    }
    map_iterator(it)->destroy();
  } else {
    log("{green}Everything ok, no crashes detected!{/green}");
  }
}

// ---------------------------------------------------------------------------
int parse_profiling(size_t** addr, size_t** count, size_t** type, size_t* calls, cmap* types) {

  FILE* f = fopen("profile", "rb");
  if(!f) {
    log("{red}No trace generated, aborting now{/red}\n");
    exit(1);
  }
  fseek(f, 0, SEEK_END);
  size_t fsize = ftell(f);
  size_t injections = fsize / sizeof(ProfileEntry);
  fseek(f, 0, SEEK_SET);

  *addr = malloc(sizeof(size_t) * injections);
  *count = malloc(sizeof(size_t) * injections);
  *type = malloc(sizeof(size_t) * injections);

  int i;
  *calls = 0;
  for(i = 0; i < injections; i++) {
    ProfileEntry e;
    if(!fread(&e, sizeof(ProfileEntry), 1, f)) {
      break;
    }

    (*addr)[i] = (size_t) e.address;
    (*count)[i] = (size_t) e.count;
    (*type)[i] = (size_t) e.type;
    map(types)->set((void*) ((*addr)[i]), (void*) ((*type)[i]));
    (*calls) += (*count)[i];
  }
  fclose(f);
  return injections;
}

// ---------------------------------------------------------------------------
void show_return_details(int status) {
  if(WIFEXITED(status)) {
    int ret = WEXITSTATUS(status);
    log("Exited, status: %d %s{red}%s{/red}%s", ret, ret >= 128 ? "(" : "", ret >= 128 ? strsignal(ret - 128) : "",
        ret >= 128 ? ")" : "");
  } else if(WIFSIGNALED(status)) {
    log("Killed by signal %d (%s)", WTERMSIG(status), strsignal(WTERMSIG(status)));
  } else if(WIFSTOPPED(status)) {
    log("Stopped by signal %d (%s)", WSTOPSIG(status), strsignal(WTERMSIG(status)));
  }
}

// ---------------------------------------------------------------------------
int wait_for_child(pid_t pid) {
  int status, killed = 0;
  waitpid(pid, &status, 0);

  show_return_details(status);
  if(WIFSIGNALED(status)) {
    killed = 1;
  }
  return killed;
}

// ---------------------------------------------------------------------------
void usage(const char* binary) {
  Usage* u = generate_usage();
  print_usage(binary, u);
  destroy_usage(u);
}

// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
  if(argc <= 1) {
    usage(argv[0]);
    return 0;
  }
  // modules enabled by default
  enable_default_modules();

  // parse commandline
  int binary_pos = parse_commandline(argc, argv);

  atexit(cleanup);
  log("Starting, Version %s\n", VERSION);

  // preload fault inject library
  char* const envs[] = { (char*) "LD_PRELOAD=./fault_inject.so", NULL };
  char* args[argc];

  // inherit all arguments
  int i, arch = ARCH_64;
  for(i = 0; i < argc - binary_pos; i++) {
    if(i > 0) {
      log(" Param %2d: %s", i, argv[i + binary_pos]);
    } else {
      // get architecture of binary
      set_filename(argv[i + binary_pos]);
      FILE* test = fopen(get_filename(), "rb");
      if(!test) {
        log("{red}Could not find file '%s'!{/red}", get_filename());
        return 1;
      }
      fclose(test);
      arch = get_architecture(get_filename());

      log("Binary: %s (%d bit)", get_filename(), arch == ARCH_32 ? 32 : 64);
    }
    args[i] = argv[i + binary_pos];
  }
  args[i] = NULL;
  log("");

  // check if compiled with debug symbols
  check_debug_symbols(get_filename());

  // extract fault inject library
  extract_shared_library(arch);

  // disable aslr to always get correct debug infos over multiple injection runs
  disable_aslr();

  // fork first to profile
  log("{green}Profiling start{/green}");
  FILE* f = fopen("profile", "wb");
  if(!f) {
    log("{red}Need write access to file 'profile'!{/red}");
    exit(1);
  }
  fclose(f);

  map_create(crashes, MAP_GENERAL);
  map_create(types, MAP_GENERAL);
  int crash_count = 0;
  int injections = 0;

  pid_t pid = fork();
  if(pid) {
    int status;
    waitpid(pid, &status, 0);
    if(!WIFEXITED(status)) {
      log("{red}There was an error while profiling, aborting now{/red}");
      show_return_details(status);
      exit(1);
    }

    log("{green}Profiling done{/green}");
    // profiling done, fork to inject
    size_t calls = 0;

    size_t *fault_addr, *fault_count, *fault_type;
    injections = parse_profiling(&fault_addr, &fault_count, &fault_type, &calls, types);

    log("Found %d different injection positions with %d call(s)", injections, calls);

    for(i = 0; i < injections; i++) {
      print_fault_position(get_filename(), (void*) (fault_addr[i]), fault_type[i], fault_count[i]);
    }
    log("");

    // let one specific function fail per loop iteration
    log("Injecting %d faults, one for every injection position", injections);

    for(i = 0; i < injections; i++) {
      pid = fork();
      if(pid) {
        int killed = wait_for_child(pid);

        void *crash, *fault;
        int has_addr = get_crash_address(&crash, &fault);
        if(has_addr) {
          crash_details(get_filename(), crash, fault, types);
          map(crashes)->set(crash, fault);
          crash_count++;
        } else {
          if(killed)
            crash_count++;
        }
        log("{green}Injection #%d done{/green}", (i + 1));
      } else {
        log("\n\n{green}Inject fault #%d{/green}", (i + 1));
        log("Fault position:");
        print_fault_position(get_filename(), (void*) (fault_addr[i]), fault_type[i], -1);
        log("");

        // -> inject
        clear_crash_report();
        set_mode(INJECT);
        set_limit(i);
        execve(get_filename(), args, envs);
        log("Could not execute %s", get_filename());
        exit(0);
      }
    }
    free(fault_addr);
    free(fault_count);
    free(fault_type);
  } else {
    // -> profile
    set_mode(PROFILE);
    execve(get_filename(), args, envs);
    log("{red}Could not execute %s{/red}", get_filename());
    exit(0);
  }

  summary(get_filename(), crash_count, injections, crashes, types);

  map(crashes)->destroy();
  map(types)->destroy();
  return 0;
}
