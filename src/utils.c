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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/personality.h>
#include <sys/types.h>
#include <sys/wait.h>


#ifndef HAVE_PERSONALITY
#include <syscall.h>
#define personality(pers) ((long)syscall(SYS_personality, pers))
#endif

#include "utils.h"
#include "log.h"

// ---------------------------------------------------------------------------
char* str_replace(const char* orig, const char* rep, const char* with) {
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
  for(count = 0; (tmp = (char*)strstr(ins, rep)); ++count) {
    ins = tmp + len_rep;
  }

  tmp = result = (char*)malloc(strlen(orig) + (len_with - len_rep) * count + 1);

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
void str_replace_inplace(char** orig, const char* rep, const char* with) {
  char* tmp = str_replace(*orig, rep, with);
  free(*orig);
  *orig = tmp;
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
    pclose(f);
    return 0;
  }

  pclose(f);
  return 1;
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
size_t get_base_address() {
    size_t base = 0;
    char cmd[256];
    sprintf(cmd, "cat /proc/%d/maps | grep 'r-xp' | head -1 | sed 's/-/ /'", getpid());
    FILE* addr = popen(cmd, "r");
    if(addr) {
        char line[256];
        if(!fgets(line, 256, addr)) {
            pclose(addr);
            return 0;
        }
        if((base = strtoull(line, NULL, 16)) == 0) {
            log("{red}Could not find base address!{/red}\n");
        }
        pclose(addr);
    }
    return base;
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


