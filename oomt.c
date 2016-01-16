/*
 * Out-of-memory testing program. Needs malloc_replace.so to be present in the 
 * working directory, as well as write privilege to the file "mallocs" in 
 * the working directory.
 * 
 * 06/2013 by Michael Schwarz 
 *
 **/ 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "settings.h"

MallocSettings settings;

void write_settings() {
  FILE* f = fopen("settings", "wb");
  if(f) 
  {
    fwrite(&settings, sizeof(MallocSettings), 1, f);
    fclose(f);
  }
}

void set_mode(enum Mode m) {
  settings.mode = m;
  write_settings();
}

void set_limit(int lim) {
   settings.limit = lim;
   write_settings();   
}

int main(int argc, char* argv[]) 
{
  if(argc <= 1) 
  {
    printf("Usage: %s <binary to test> [arg1] [...]\n", argv[0]);
    return 0;
  }

  // preload malloc replacement
  char* const envs[] = {(char*)"LD_PRELOAD=./malloc_replace.so", NULL};
  char* args[argc];

  // inherit all arguments (except allowed mallocs limit)
  int i;
  for(i = 0; i < argc - 1; i++) 
  {
    args[i] = argv[i + 1]; 
  }  
  args[i] = NULL;
  
  // fork first to profile
  pid_t p = fork();
  if(p) {
    waitpid(p, NULL, 0);
    // profiling done, fork to inject
    FILE* f = fopen("profile", "rb");
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fclose(f);
    
    for(i = 0; i < (fsize / (2 * sizeof(size_t))); i++) {
        p = fork();
        if(p) {
            waitpid(p, NULL, 0);
            printf("done #%d\n", (i + 1));
        } else {
            // -> inject
            set_mode(INJECT);
            set_limit(i);
            execve(args[0], args, envs);
        }    
    }
  } else {
    // -> profile
    set_mode(PROFILE);
    execve(args[0], args, envs);
  }
  // won't reach this, just to be sure
  return 0;
}
