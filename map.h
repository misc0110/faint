#ifndef _MAP_H_
#define _MAP_H_

#define MAX_MAP_NESTING 32
#define MAP_START_SIZE 8

extern void* _obj_addr[MAX_MAP_NESTING];
extern int _obj_addr_pos;

typedef void* (*allocator)(size_t);
extern allocator map_allocate;

enum map_type {
  MAP_GENERAL, MAP_STRING
};

#define map_create(m, type) cmap* m; _map_init(&m); if(type == MAP_STRING) {m->hash = map_hash_str; m->compare = map_compare_str;}
#define map_declare(m) cmap* m;
#define map_initialize(m, type) _map_init(&m); if(type == MAP_STRING) {m->hash = map_hash_str; m->compare = map_compare_str;}

// ---------------------------------------------------------------------------
typedef struct _cmap_iterator_ cmap_iterator;
typedef struct _cmap_entry_ cmap_entry;
typedef struct {
  cmap_entry* data;
  
  int size;
  int entries;
  
  void (*init)(int);
  void* (*get)(void*);
  void (*set)(void*, void*);
  void (*dump)(void);
  int (*has)(void*);
  void (*destroy)(void);
  void (*unset)(void*);
  void (*clear)(void);
  int (*hash)(void*,int);
  int (*compare)(void*,void*);
  cmap_iterator* (*iterator)(void);
} cmap;


// ---------------------------------------------------------------------------
typedef struct _cmap_iterator_ {
  cmap* obj;
  int position;
  cmap_entry* current;
  
  void (*next)(void);
  void* (*key)(void);
  void* (*value)(void);
  int (*end)(void);
  void (*destroy)(void);
  
} cmap_iterator;


void _map_init(cmap** m);
cmap* map(cmap* m);
cmap_iterator* map_iterator(cmap_iterator* it);

int map_hash_str(void *str, int size);
int map_compare_str(void* key1, void* key2);

#endif

