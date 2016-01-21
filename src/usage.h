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

#ifndef SRC_USAGE_H_
#define SRC_USAGE_H_

typedef struct {
    const char* name;
    int optional;
} UsageParam;

typedef struct {
    const char* name;
    const char* description;
    int optional;
    int has_param;
    UsageParam parameter;
} UsageEntry;

typedef struct {
  int size;
  UsageEntry* entry;
} Usage;

Usage* generate_usage();
void print_usage(const char* binary, Usage* u);
void destroy_usage(Usage* u);

#endif /* SRC_USAGE_H_ */
