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
#include <stdlib.h>
#include "map.h"

void* _obj_addr[MAX_MAP_NESTING];
int _obj_addr_pos = 0;

allocator map_allocate = malloc;

#define this ((cmap*)_obj_addr[_obj_addr_pos - 1])
#define thisit ((cmap_iterator*)_obj_addr[_obj_addr_pos - 1])
#define objreturn --_obj_addr_pos; return 

// ---------------------------------------------------------------------------
typedef struct _cmap_entry_ {
  void* key;
  void* value;
  struct _cmap_entry_* next;
} cmap_entry;

// ---------------------------------------------------------------------------
void cmap_entry_dump(cmap_entry* start) {
  printf("{%ld} ", (long) (start->value));
  start = start->next;
  while(start) {
    printf("(%p, %p) ", (start->key), (start->value));
    start = start->next;
  }
}

// ---------------------------------------------------------------------------
void cmap_entry_append(cmap_entry* start, const void* key, const void* value) {
  start->value = (void*) (((size_t) start->value) + 1);
  while(start->next) {
    start = start->next;
  }
  start->next = (cmap_entry*) map_allocate(sizeof(cmap_entry));
  start->next->next = NULL;
  start->next->key = (void*)key;
  start->next->value = (void*)value;
}

// ---------------------------------------------------------------------------
cmap_iterator* map_iterator(cmap_iterator* it) {
  if(_obj_addr_pos >= MAX_MAP_NESTING) {
    printf("ERROR! Max nesting reached!\n");
    return NULL;
  }
  _obj_addr[_obj_addr_pos++] = it;
  return it;
}

// ---------------------------------------------------------------------------
cmap* map(cmap* m) {
  if(_obj_addr_pos >= MAX_MAP_NESTING) {
    printf("ERROR! Max nesting reached!\n");
    return NULL;
  }
  _obj_addr[_obj_addr_pos++] = m;
  return m;
}

// ---------------------------------------------------------------------------
int map_hash(const void* key, int size) {
  return (long) key % size;
}

// ---------------------------------------------------------------------------
int map_key_compare(const void* key1, const void* key2) {
  return key1 == key2;
}

// ---------------------------------------------------------------------------
cmap_entry* _map_key_position(cmap* m, const void* key) {
  int position = m->hash(key, m->size);
  cmap_entry* start = (m->data[position]).next;
  while(start) {
    if(m->compare(start->key, key))
      return start;
    start = start->next;
  }
  return NULL;
}

// ---------------------------------------------------------------------------
void _map_resize(cmap* m, int new_size) {
  cmap_entry* new_data = (cmap_entry*) calloc(sizeof(cmap_entry), new_size);
  // iterate over all entries and insert into new structure
  int i;
  for(i = 0; i < m->size; i++) {
    cmap_entry* start = (m->data[i]).next;
    while(start) {
      // rehash key
      int position = m->hash(start->key, new_size);
      cmap_entry_append(&new_data[position], start->key, start->value);
      cmap_entry* last = start;
      start = start->next;
      free(last);
    }
  }
  free(m->data);
  m->data = new_data;
  m->size = new_size;
}

// ---------------------------------------------------------------------------
void* map_get(const void* key) {
  cmap_entry* entry = _map_key_position(this, key);
  objreturn
(  entry ? entry->value : NULL);
}

// ---------------------------------------------------------------------------
int map_exists(const void* key) {
  cmap_entry* entry = _map_key_position(this, key);
  objreturn
(  entry ? 1 : 0);
}

// ---------------------------------------------------------------------------
void map_set(const void* key, const void* value) {
  // check if already exists
  cmap_entry* exist = _map_key_position(this, key);
  if(exist) {
    exist->value = (void*)value;
    objreturn;
  }

  // otherwise, append
  int position = this->hash(key, this->size);
  cmap_entry_append(&(this->data[position]), key, value);
  this->entries++;
  if(this->entries > this->size / 2)
    _map_resize(this, this->size * 2);
  objreturn
;}

// ---------------------------------------------------------------------------
void map_unset(const void* key) {
  int position = this->hash(key, this->size);
  cmap_entry* last = &(this->data[position]);
  cmap_entry* start = last->next;
  while(start) {
    if(this->compare(start->key, key)) {
      cmap_entry* tofree = last->next;
      last->next = start->next;
      free(tofree);
      (this->data[position]).value = (void*) (((size_t) ((this->data[position]).value)) - 1);
      this->entries--;
      objreturn
;    }
    last = start;
    start = start->next;
  }
  objreturn
;}

// ---------------------------------------------------------------------------
void map_clear() {
  int i;
  for(i = 0; i < this->size; i++) {
    cmap_entry* start = (this->data[i]).next;
    while(start) {
      cmap_entry* last = start;
      start = start->next;
      free(last);
    }
    (this->data[i]).value = 0;
    (this->data[i]).next = NULL;
  }
  this->entries = 0;
  objreturn
;}

// ---------------------------------------------------------------------------
void map_destroy() {
  map(this)->clear();
  free(this->data);
  this->data = 0;
  this->size = 0;
  free(this);
  objreturn
;}

// ---------------------------------------------------------------------------
void map_iterator_next() {
  if(thisit->current && thisit->current->next) {
    thisit->current = thisit->current->next;
  } else {
    do {
      thisit->position++;
      if(thisit->position == thisit->obj->size)
        break;
      thisit->current = (thisit->obj->data[thisit->position]).next;
      if(thisit->position >= thisit->obj->size - 1)
        break;
    } while(!thisit->current);
  }
  objreturn
;}

// ---------------------------------------------------------------------------
void* map_iterator_key() {
  void* key = thisit->current->key;
  objreturn
key  ;
}

// ---------------------------------------------------------------------------
void* map_iterator_value() {
  void* val = thisit->current->value;
  objreturn
val  ;
}

// ---------------------------------------------------------------------------
int map_iterator_end() {
  if(thisit->current == NULL || thisit->position >= thisit->obj->size) {
    objreturn
1    ;
  }
  if(thisit->position >= thisit->obj->size && thisit->current->next == NULL) {
    objreturn 1;
  }
  else {
    objreturn 0;
  }
}

// ---------------------------------------------------------------------------
void map_iterator_destroy() {
  free(thisit);
  objreturn
;}

// ---------------------------------------------------------------------------
cmap_iterator* map_iter() {
  cmap_iterator* it = (cmap_iterator*) map_allocate(sizeof(cmap_iterator));
  it->position = 0;
  it->current = (this->data[0]).next;
  it->obj = this;

  it->end = map_iterator_end;
  it->next = map_iterator_next;
  it->key = map_iterator_key;
  it->value = map_iterator_value;
  it->destroy = map_iterator_destroy;

  if(!it->current)
    map_iterator(it)->next();
  objreturn
it  ;
}

// ---------------------------------------------------------------------------
void map_dump() {
  int i;
  printf("-----------------------------\n"), printf(" - size: %d\n", this->size);
  printf(" - entries: %d\n", this->entries);
  printf(" ------- data ---------\n");
  for(i = 0; i < this->size; i++) {
    printf("[%d] ", i);
    cmap_entry_dump(&(this->data[i]));
    printf("\n");
  }
  printf("-----------------------------\n"),

  objreturn
;}

// ---------------------------------------------------------------------------
void map_init(int size) {
  if(this->data) {
    map(this)->clear();
    free(this->data);
  }

  this->size = size;
  this->entries = 0;
  this->data = (cmap_entry*) calloc(sizeof(cmap_entry), this->size);
  objreturn
;}

// ---------------------------------------------------------------------------
void _map_init(cmap** m) {
  *m = (cmap*) map_allocate(sizeof(cmap));

  (*m)->init = map_init;
  (*m)->get = map_get;
  (*m)->set = map_set;
  (*m)->dump = map_dump;
  (*m)->has = map_exists;
  (*m)->destroy = map_destroy;
  (*m)->unset = map_unset;
  (*m)->clear = map_clear;
  (*m)->iterator = map_iter;
  (*m)->hash = map_hash;
  (*m)->compare = map_key_compare;

  (*m)->data = NULL;

  map(*m)->init(MAP_START_SIZE);
}

// ---------------------------------------------------------------------------
int map_hash_str(const void *str_, int size) {
  unsigned char* str = (unsigned char*) str_;
  int hash = 5381;
  int c;

  while((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  return hash % size;
}

// ---------------------------------------------------------------------------
int map_compare_str(const void* key1_, const void* key2_) {
  char* key1 = (char*) key1_;
  char* key2 = (char*) key2_;
  while(*key1 || *key2) {
    if(*key1 != *key2)
      return 0;
    key1++;
    key2++;
  }
  return 1;
}

