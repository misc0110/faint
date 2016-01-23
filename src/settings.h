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

#ifndef _FAINT_SETTINGS_H_
#define _FAINT_SETTINGS_H_

#include <stdint.h>

// ---------------------------------------------------------------------------
enum Mode {
  PROFILE, INJECT
};

// ---------------------------------------------------------------------------
typedef struct {
    int32_t limit;
    char filename[256];
    uint32_t modules;
    enum Mode mode;
    uint8_t trace_heap;
}__attribute__((packed)) FaultSettings;

// ---------------------------------------------------------------------------
typedef struct {
    uint64_t address;
    uint64_t count;
    uint64_t type;
}__attribute__((packed)) ProfileEntry;

// ---------------------------------------------------------------------------
typedef struct {
    uint64_t fault;
    uint64_t crash;
}__attribute__((packed)) CrashEntry;

// ---------------------------------------------------------------------------
typedef struct {
    uint64_t address;
    uint64_t size;
}__attribute__((packed)) HeapEntry;

#endif
