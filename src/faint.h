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

#ifndef SRC_FAINT_H_
#define SRC_FAINT_H_

extern uint8_t fault_lib[] asm("_binary_fault_inject_so_start");
extern uint8_t fault_lib_end[] asm("_binary_fault_inject_so_end");
extern uint8_t fault_lib32[] asm("_binary_fault_inject32_so_start");
extern uint8_t fault_lib32_end[] asm("_binary_fault_inject32_so_end");


void usage(const char* binary);
void extract_shared_library(int arch);
int parse_profiling(size_t** addr, size_t** count, size_t** type, size_t* calls, cmap* types);
void summary(const char* binary, int crash_count, int injections, cmap* crashes, cmap* types);
void crash_details(const char* binary, const void* crash, const void* fault, cmap* types);
void print_fault_position(const char* binary, const void* fault, int type, int count);
int parse_commandline(int argc, char* argv[]);
void enable_default_modules();
void cleanup();
int get_crash_address(void** crash, void** fault_addr);
void clear_crash_report();
void list_modules();
void disable_module(const char* m);
void enable_module(const char* m);
const char* get_filename();
void set_filename(const char* fn);
void set_limit(int lim);
void set_mode(enum Mode m);
void write_settings();
void usage(const char* binary);

#endif /* SRC_FAINT_H_ */
