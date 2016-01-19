#ifndef _MR_H_
#define _MR_H_

#include <stdint.h>

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
  "fgets",
  "fread",
  "fwrite"
};
#define MODULE_COUNT (sizeof(modules) / sizeof(modules[0]))

typedef struct {
    int32_t limit;
    char filename[256];
    uint32_t modules;
    enum Mode mode;
}__attribute__((packed)) FaultSettings;

typedef struct {
    uint64_t address;
    uint64_t count;
    uint64_t type;
}__attribute__((packed)) ProfileEntry;

typedef struct {
    uint64_t fault;
    uint64_t crash;
}__attribute__((packed)) CrashEntry;

#endif
