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

#include "usage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* str_replace(const char* orig, char* rep, char* with);

// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
  if(argc != 2) {
    printf("Usage: %s <binary name>\n", argv[0]);
    return 1;
  }
  char* binary = argv[1];
  char* binary_upper = strdup(binary);
  int i;
  char fn[256];

  for(i = 0; i < strlen(binary); i++) {
    binary_upper[i] = toupper(binary_upper[i]);
  }

  sprintf(fn, "docs/%s.1", binary);

  FILE* f = fopen(fn, "w");

  // binary name and short description
  fprintf(f, ".TH %s 1\n", binary_upper);
  fprintf(f, ".SH NAME\n");
  fprintf(f, "%s \\- fault injection testing for binaries\n", binary);
  // usage
  fprintf(f, ".SH SYNOPSIS\n");
  fprintf(f, ".B faint ");
  fprintf(f, "[\\fIfaint-options\\fR] ");
  fprintf(f, "[\\fByour-program\\fR] ");
  fprintf(f, "[\\fIyour-program-options\\fR]\n");
  // description
  fprintf(f, ".SH DESCRIPTION\n");
  fprintf(f, ".B %s\\fR ", binary);
  fprintf(f, "is an automated debugging tool to inject faults into Linux binaries.\n");
  // command line options
  fprintf(f, ".SH OPTIONS\n");

  Usage* u = generate_usage();
  for(i = 0; i < u->size; i++) {
    fprintf(f, ".TP\n");
    char* n = str_replace(u->entry[i].name, "-", "\\-");
    char* d = str_replace(u->entry[i].description, "-", "\\-");
    fprintf(f, ".BR %s\\fR", n);
    if(u->entry[i].has_param) {
      fprintf(f, "\\~");
      if(u->entry[i].parameter.optional)
        fprintf(f, "[");
      fprintf(f, "<\\fI%s\\fR>", u->entry[i].parameter.name);
      if(u->entry[i].parameter.optional)
        fprintf(f, "]");
    }
    fprintf(f, "\n");
    fprintf(f, "%s\n", d);
    free(n);
    free(d);
  }

  // example
  fprintf(f, ".SH EXAMPLES\n");
  fprintf(f, "Inspect a binary for (out of-)memory handling and display the result in colors\n");
  fprintf(f, ".PP\n.nf\n.RS\n");
  fprintf(f, "faint --colorlog my_binary my_arg1\n");
  fprintf(f, ".RE\n.fi\n.PP\n");

  // copyright
  fprintf(f, ".SH COPYRIGHT\n");
  fprintf(f, "Copyright 2016 Michael Schwarz <michael.schwarz91@gmail.com>\n\n");
  fprintf(f, "This is free software; see the  source  for  copying  conditions.\n");
  fprintf(f, "There  is  NO  warranty;  not  even  for\n");
  fprintf(f, "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");

  free(binary_upper);
  fclose(f);
  return 0;
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

