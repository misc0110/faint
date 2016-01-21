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

#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#define ARCH_32   0
#define ARCH_64   1


char* str_replace(const char* orig, const char* rep, const char* with);
void str_replace_inplace(char** orig, const char* rep, const char* with);
int get_file_and_line(const char* binary, const void* addr, char *file, int *line, char* function);
void check_debug_symbols(const char* binary);
int get_architecture(const char* binary);
void disable_aslr();
void show_return_details(int status);
int wait_for_child(pid_t pid);

#endif /* SRC_UTILS_H_ */
