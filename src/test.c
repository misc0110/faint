#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int *helper(int size) {
  printf("Helper\n");
  int* v = malloc(size * sizeof(int));
  if(!v)
    printf("Malloc (helper) failed\n");
  int* v2 = calloc(size, sizeof(int));
  v2[1] = 3;
  free(v2);
  return v;
}

int* do_mem(int size) {
  int i;
  for(i = 0; i < size; i++) {
    int* x = helper(size);
    x[0] = 3;
    x = realloc(x, (1024 + i) * sizeof(int));
    x[0] = 4;
    free(x);
  }
  int* y = malloc(size * sizeof(int));
  if(!y)
    printf("Malloc (do_mem) failed\n");
  return y;
}

int main() {
  char* foo = strdup("hallo");
  printf("%s\n", foo);

  int* a = do_mem(10);
  a[0] = 1;
  free(a);

  FILE* f = fopen("test.c", "r");
  if(!f) {
    printf("WUT?\n");
  }
  char* line = NULL;
  size_t len;
  int ret = getline(&line, &len, f);
  printf("read ok: %d, line: %p\n", ret, line);
  printf("%s\n", line);
  free(line);

  char buffer[256];
  printf("fgets: %s\n", fgets(buffer, 256, f));
  printf("%s\n", buffer);

  if(!fread(buffer, 256, 1, f)) {
    *(int*)(0) = 1;
  }

  fclose(f);

  return 0;
}
