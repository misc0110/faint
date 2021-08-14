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
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include "utils.h"
#include "log.h"

#define LOG_TAG "[ FAINT ] "

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

static int colorlog = 0;
static int logfile = 1;
static int console_log = 1;
static const char* logfile_name = "log.txt";

// ---------------------------------------------------------------------------
void enable_logfile(int en) {
  logfile = en;
}

// ---------------------------------------------------------------------------
void enable_log(int en) {
  console_log = en;
}

// ---------------------------------------------------------------------------
void enable_colorlog(int col) {
  colorlog = col;
}

// ---------------------------------------------------------------------------
void set_log_name(const char* name) {
  logfile_name = name;
}

// ---------------------------------------------------------------------------
void log(const char *format, ...) {
  static int count = 0;
  time_t timer;
  char buffer[26], time_buffer[32];
  struct tm* tm_info;
  va_list args;

  if(console_log) {
    // replace newlines with tag
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
    va_start(args, format);
    fprintf(stderr, "%s", LOG_TAG);
    vfprintf(stderr, tag_format, args);
    fprintf(stderr, "\n");
    va_end(args);
    free(tag_format);
  }

  if(!logfile) {
    return;
  }
  // print to logfile
  va_start(args, format);
  FILE* f = fopen(logfile_name, count == 0 ? "w" : "a");

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

  free(file_format);
}

