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

#include <string.h>
#include "modules.h"

// ---------------------------------------------------------------------------
const char* modules[] = {
  "(unknown)",
  "malloc",
  "realloc",
  "calloc",
  "new",
  "fopen",
  "getline",
  "fgets",
  "fread",
  "fwrite"
};

// ---------------------------------------------------------------------------
const size_t MODULE_COUNT = (sizeof(modules) / sizeof(modules[0]));

// ---------------------------------------------------------------------------
size_t get_module_count() {
  return MODULE_COUNT;
}

// ---------------------------------------------------------------------------
const char* get_module(int i) {
  return modules[i];
}

//-----------------------------------------------------------------------------
size_t get_module_id(const char* module) {
  size_t i;
  for(i = 0; i < get_module_count(); i++) {
    if(!strcmp(module, modules[i])) {
      return i;
    }
  }
  return -1;
}

