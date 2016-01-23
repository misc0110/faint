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
#include "usage.h"

// ---------------------------------------------------------------------------
int add_entry(Usage* u, const char* name, const char* desc, int optional) {
  UsageEntry* tmp = (UsageEntry*) realloc(u->entry, (u->size + 1) * sizeof(UsageEntry));
  if(!tmp) {
    return 0;
  }
  tmp[u->size].name = name;
  tmp[u->size].description = desc;
  tmp[u->size].optional = optional;
  tmp[u->size].has_param = 0;
  u->entry = tmp;
  u->size++;
  return 1;
}

// ---------------------------------------------------------------------------
int add_entry_param(Usage* u, const char* name, const char* desc, int optional, char* param, int param_optional) {
  if(!add_entry(u, name, desc, optional))
    return 0;
  u->entry[u->size - 1].has_param = 1;
  u->entry[u->size - 1].parameter.name = param;
  u->entry[u->size - 1].parameter.optional = param_optional;
  return 1;
}

// ---------------------------------------------------------------------------
Usage* generate_usage() {
  Usage* u = (Usage*) malloc(sizeof(Usage));
  if(!u)
    return NULL;

  u->size = 0;
  u->entry = NULL;

  add_entry(u, "--list-modules", "Lists all available modules which can be enabled/disabled", 1);
  add_entry(u, "--all", "Enable all modules", 1);
  add_entry(u, "--none", "Disable all modules", 1);
  add_entry_param(u, "--enable", "Enables the module", 1, "module", 0);
  add_entry_param(u, "--disable", "Disables the module", 1, "module", 0);
  add_entry(u, "--no-memory", "Disable all memory allocation modules", 1);
  add_entry(u, "--file-io", "Enable all file I/O modules", 1);
  add_entry(u, "--colorlog", "Enable log output with colors", 1);
  add_entry(u, "--silent", "Do not output anything", 1);
  add_entry_param(u, "--logfile", "Set name for logfile", 1, "filename", 0);
  add_entry(u, "--no-logfile", "Disable log file", 1);
  add_entry(u, "--valgrind", "Run profiled program under valgrind", 1);
  add_entry(u, "--profile-only", "Only to the profile step, no fault injection", 1);
  add_entry(u, "--inject-only", "Only to the injectino step, no profiling", 1);
  add_entry(u, "--trace-heap", "Trace heap allocations and memory leaks", 1);
  add_entry(u, "--version", "Show program version", 1);
  return u;
}

// ---------------------------------------------------------------------------
void print_usage(const char* binary, Usage* u) {
  if(!u)
    return;

  printf("Usage: %s ", binary);
  int i;
  for(i = 0; i < u->size; i++) {
    if(u->entry[i].optional)
      printf("[");
    printf("%s", u->entry[i].name);
    if(u->entry[i].has_param) {
      if(u->entry[i].parameter.optional)
        printf("[");
      printf("=%s", u->entry[i].parameter.name);
      if(u->entry[i].parameter.optional)
        printf("]");
    }
    if(u->entry[i].optional)
      printf("] ");
  }
  printf("\n\t\t\t <binary to test> [arg1] [...]\n\n");

  for(i = 0; i < u->size; i++) {
    printf("%s", u->entry[i].name);
    if(u->entry[i].has_param) {
      printf("=%s", u->entry[i].parameter.name);
    }
    printf("\n\t\t %s\n\n", u->entry[i].description);
  }
  printf("\nfaint is Copyright (C) 2016, and GNU GPL'd, by Michael Schwarz.\n\n");
}

// ---------------------------------------------------------------------------
void destroy_usage(Usage* u) {
  if(!u)
    return;
  free(u->entry);
  free(u);
}

