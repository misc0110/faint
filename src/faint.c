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
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "settings.h"
#include "map.h"
#include "usage.h"
#include "log.h"
#include "modules.h"
#include "utils.h"
#include "faint.h"

static FaultSettings settings;
static int valgrind = 0;
static int profile_only = 0;
static int inject_only = 0;

#ifndef VERSION
#define VERSION "0.1-debug"
#endif

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
  char* args[argc + 1];

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
    args[i + valgrind] = argv[i + binary_pos];
  }
  args[i + valgrind] = NULL;
  if(valgrind)
    args[0] = "/usr/bin/valgrind";
  log("");

  // check if compiled with debug symbols
  check_debug_symbols(get_filename());

  // extract fault inject library
  extract_shared_library(arch);

  // disable aslr to always get correct debug infos over multiple injection runs
  disable_aslr();

  // fork first to profile
  if(!inject_only) {
    log("{green}Profiling start{/green}");
    FILE* f = fopen("profile", "wb");
    if(!f) {
      log("{red}Need write access to file 'profile'!{/red}");
      exit(1);
    }
    fclose(f);
  } else {
    // inject only needs already a profile
    FILE* f = fopen("profile", "rb");
    if(!f) {
      log("{red}Need file 'profile'! Start with --profile-only first.{/red}");
      exit(1);
    }
    fclose(f);
  }

  map_create(crashes, MAP_GENERAL);
  map_create(types, MAP_GENERAL);
  int crash_count = 0;
  int injections = 0;
  size_t *fault_addr, *fault_count, *fault_type;
  size_t calls = 0;

  pid_t pid;
  // fork only if profiling is needed
  if(!inject_only) {
    pid = fork();
  } else {
    pid = 1;
  }
  if(pid) {
    if(!inject_only) {
      int status;
      waitpid(pid, &status, 0);
      if(!WIFEXITED(status)) {
        log("{red}There was an error while profiling, aborting now{/red}");
        show_return_details(status);
        exit(1);
      }

      log("{green}Profiling done{/green}");
      // profiling done, fork to inject
    }

    injections = parse_profiling(&fault_addr, &fault_count, &fault_type, &calls, types);
    if(settings.trace_heap)
      show_heap();

    log("Found %d different injection positions with %d call(s)", injections, calls);

    for(i = 0; i < injections; i++) {
      print_fault_position(get_filename(), (void*) (fault_addr[i]), fault_type[i], fault_count[i]);
    }
    log("");

    if(!profile_only) {
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

          if(settings.trace_heap)
            show_heap();
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
          execve(args[0], args, envs);
          log("Could not execute %s", get_filename());
          exit(0);
        }
      }
    }
    free(fault_addr);
    free(fault_count);
    free(fault_type);
  } else {
    // -> profile
    set_mode(PROFILE);

    execve(args[0], args, envs);
    log("{red}Could not execute %s{/red}", get_filename());
  }

  if(!profile_only)
    summary(get_filename(), crash_count, injections, crashes, types);

  map(crashes)->destroy();
  map(types)->destroy();
  return 0;
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
void usage(const char* binary) {
  Usage* u = generate_usage();
  print_usage(binary, u);
  destroy_usage(u);
}

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
  settings.modules |= (1 << get_module_id(m));
}

// ---------------------------------------------------------------------------
void disable_module(const char* m) {
  settings.modules &= ~(1 << get_module_id(m));
}

// ---------------------------------------------------------------------------
void list_modules() {
  int i;
  log("Available modules:\n");
  for(i = 1; i < get_module_count(); i++) {
    log(" > %s\n", get_module(i));
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
void cleanup() {
  remove("settings");
  if(!profile_only)
    remove("profile");
  remove("heap");
  remove("crash");
  remove("fault_inject.so");
  log("\n\nfinished successfully!");
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
        for(j = 1; j < get_module_count(); j++) {
          enable_module(get_module(j));
        }
      } else if(!strcmp(cmd, "none")) {
        int j;
        for(j = 1; j < get_module_count(); j++) {
          disable_module(get_module(j));
        }
      } else if(!strcmp(cmd, "file-io")) {
        enable_module("fopen");
        enable_module("fread");
        enable_module("fwrite");
        enable_module("fgets");
      } else if(!strcmp(cmd, "colorlog")) {
        enable_colorlog(1);
      } else if(!strcmp(cmd, "no-logfile")) {
        enable_logfile(0);
      } else if(!strcmp(cmd, "valgrind")) {
        valgrind = 1;
      } else if(!strcmp(cmd, "profile-only")) {
        if(inject_only) {
          log("{red}--profile-only and --inject-only are mutually exclusive!{/red}");
          exit(1);
        }
        profile_only = 1;
      } else if(!strcmp(cmd, "inject-only")) {
        if(profile_only) {
          log("{red}--profile-only and --inject-only are mutually exclusive!{/red}");
          exit(1);
        }
        inject_only = 1;
      } else if(!strcmp(cmd, "trace-heap")) {
        settings.trace_heap = 1;
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
  for(i = 0; i < get_module_count(); i++) {
    if(settings.modules & (1 << i)) {
      log("Activate module '{yellow}%s{/yellow}'", get_module(i));
    }
  }
  log("\n");
  return binary_pos;
}

// ---------------------------------------------------------------------------
void print_fault_position(const char* binary, const void* fault, int type, int count) {
  char m_file[256], m_function[256];
  int m_line;
  if(get_file_and_line(binary, fault, m_file, &m_line, m_function)) {
    if(count != -1) {
      log(" >  [{yellow}%s{/yellow}] {cyan}%s{/cyan} in %s line {cyan}%d{/cyan}: %d calls", get_module(type),
          m_function, m_file, m_line, count);
    } else {
      log(" >  [{yellow}%s{/yellow}] {cyan}%s{/cyan} in %s line {cyan}%d{/cyan}", get_module(type), m_function, m_file,
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

  log("{red}Crashed{/red} at %p, caused by %p [%s]", crash, fault, get_module((size_t) map(types)->get(fault)));
  if(get_file_and_line(binary, crash, crash_file, &crash_line, crash_fnc)
      && get_file_and_line(binary, fault, fault_file, &fault_line, fault_fnc)) {
    log("  > {red}crash{/red}: {cyan}%s{/cyan} (%s) line {cyan}%d{/cyan}", crash_fnc, crash_file, crash_line);
    log("  > {yellow}%s{/yellow}: {cyan}%s{/cyan} (%s) line {cyan}%d{/cyan}",
        get_module((size_t) map(types)->get(fault)), fault_fnc, fault_file, fault_line);
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
int parse_heap(size_t** addr, size_t** size, size_t* blocks, size_t* total_size) {
  FILE* f = fopen("heap", "rb");
  if(!f) {
    log("{red}No heap profile generated!{/red}\n");
    return 0;
  }
  fseek(f, 0, SEEK_END);
  size_t fsize = ftell(f);
  size_t heap_blocks = fsize / sizeof(HeapEntry);
  fseek(f, 0, SEEK_SET);

  *addr = malloc(sizeof(size_t) * heap_blocks);
  *size = malloc(sizeof(size_t) * heap_blocks);

  int i;
  *total_size = 0;
  for(i = 0; i < heap_blocks; i++) {
    HeapEntry e;
    if(!fread(&e, sizeof(HeapEntry), 1, f)) {
      break;
    }

    (*addr)[i] = (size_t) e.address;
    (*size)[i] = (size_t) e.size;
    *total_size += e.size;
  }

  fclose(f);
  *blocks = heap_blocks;
  return 1;
}

// ---------------------------------------------------------------------------
void show_heap() {
  size_t *addr, *size, blocks, total_size;
  if(!parse_heap(&addr, &size, &blocks, &total_size)) {
    return;
  }
  log("\n");
  if(blocks == 0) {
    log("{green}All heap blocks are freed, no memory leak found{/green}");
    return;
  }
  int i;
  for(i = 0; i < blocks; i++) {
    char file[256], fnc[256];
    int line;
    if(get_file_and_line(get_filename(), (void*)(addr[i]), file, &line, fnc)) {
      log("Lost %d bytes at {cyan}%s{/cyan} (%s) line {cyan}%d{/cyan}", size[i], fnc, file, line);
    } else {
      log("Lost %d bytes at %p", size[i], addr[i]);
    }
  }
  log("\n{red}Heap summary: lost %d bytes in %d blocks{/red}\n", total_size, blocks);
}
