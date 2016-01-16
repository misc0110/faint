#ifndef _MR_H_
#define _MR_H_

enum Mode {
  PROFILE, 
  INJECT  
};

typedef struct {
    enum Mode mode;
    int limit;
    char filename[256];
} __attribute__((packed)) MallocSettings; 

#endif
