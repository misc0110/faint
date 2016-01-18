#ifndef _MR_H_
#define _MR_H_

enum Mode {
  PROFILE, INJECT
};

const char* modules[] = {
  "(unknown)",
  "malloc",
  "realloc",
  "calloc",
  "new",
  "fopen",
  "getline",
  "fgets"
};
#define MODULE_COUNT (sizeof(modules) / sizeof(modules[0]))

typedef struct {
    enum Mode mode;
    int limit;
    char filename[256];
    unsigned int modules;
}__attribute__((packed)) FaultSettings;

#endif
