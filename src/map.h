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
#define map_declare(m) cmap* m
#define map_initialize(m, type) do {_map_init(&m); if(type == MAP_STRING) {m->hash = map_hash_str; m->compare = map_compare_str;}} while(0)

// ---------------------------------------------------------------------------
typedef struct _cmap_iterator_ cmap_iterator;
typedef struct _cmap_entry_ cmap_entry;
typedef struct {
    cmap_entry* data;

    int size;
    int entries;

    void (*init)(int);
    void* (*get)(const void*);
    void (*set)(const void*, const void*);
    void (*dump)(void);
    int (*has)(const void*);
    void (*destroy)(void);
    void (*unset)(const void*);
    void (*clear)(void);
    int (*hash)(const void*, int);
    int (*compare)(const void*, const void*);
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

// ---------------------------------------------------------------------------
void _map_init(cmap** m);
cmap* map(cmap* m);
cmap_iterator* map_iterator(cmap_iterator* it);

int map_hash_str(const void *str, int size);
int map_compare_str(const void* key1, const void* key2);

#endif

